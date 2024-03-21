/*
 * Copyright (C) 2022 Collabora Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "bi_builder.h"
#include "va_compiler.h"
#include "valhall_enums.h"

/*
 * Merge NOPs with flow control with nearby instructions to eliminate the NOPs,
 * according to the following rules:
 *
 * 1. Waits may be combined by waiting on a union of the slots.
 * 2. Waits may be moved up as far as the first (last) asynchronous instruction
 *    writing to a slot waited on, at a performance cost.
 * 3. Discard may be moved down, at a performance cost.
 * 4. Reconverge must be on the last instruction of the block.
 * 5. End must be on the last instruction of the block.
 *
 * For simplicity, this pass only merges within a single basic block. This
 * should be sufficient. The algorithm works as follows.
 *
 * Because reconverge/end must be on the last instruction, there may be only
 * one of these in a block. First check for a NOP at the end, and if it has
 * reconverge/end flow control, merge with the penultimate instruction. Now we
 * only have to worry about waits and discard.
 *
 * The cost of moving a wait up is assumed to be greater than the cost of moving
 * a discard down, so we next move waits while we have more freedom. For each
 * wait, merge with an instruction above that either is free or has another
 * wait, and merge the flow control, aborting if we hit a message signaling the
 * relevant slot. By maintaining the candidate instruction at all steps, this is
 * implemented in a linear time walk.
 *
 * Finally, all that's left are discards. Move discards down to merge with a
 * free instruction with a similar linear time walk.
 *
 * Since discarding helper threads is an optimization (not required for
 * correctness), we may remove discards if we believe them harmful to the
 * performance of the shader. Keeping with the local structure of the pass, we
 * remove discards only if they are at the end of the last block and cannot be
 * merged with any other instruction.  Such a condition means every following
 * instruction has incompatible flow control (wait and end). In practice, this
 * means BLEND and ATEST run for helper threads in certain shaders to save a
 * NOP, but BLEND is a no-op for helper threads anyway.
 *
 * One case we don't handle is merging "foo, bar, wait, reconverge" to
 * "foo.wait, bar.reconverge". This sequence is rarely generated by the dataflow
 * analysis, so we don't care for the complexity.
 */

/*
 * If the last instruction has reconverge or end, try to merge with the
 * second to last instruction.
 *
 * Precondition: block has at least 2 instructions.
 */
static void
merge_end_reconverge(bi_block *block)
{
   bi_instr *last = list_last_entry(&block->instructions, bi_instr, link);
   bi_instr *penult = bi_prev_op(last);

   if (last->op != BI_OPCODE_NOP)
      return;
   if (last->flow != VA_FLOW_RECONVERGE && last->flow != VA_FLOW_END)
      return;

   /* End implies all other flow control except for waiting on barriers (slot
    * #7, with VA_FLOW_WAIT), so remove blocking flow control.
    */
   if (last->flow == VA_FLOW_END) {
      while (penult->op == BI_OPCODE_NOP && penult->flow != VA_FLOW_WAIT) {
         bi_remove_instruction(penult);

         /* There may be nothing left */
         if (list_is_singular(&block->instructions))
            return;

         penult = bi_prev_op(last);
      }
   }

   /* If there is blocking flow control, we can't merge */
   if (penult->flow != VA_FLOW_NONE)
      return;

   /* Else, merge */
   penult->flow = last->flow;
   bi_remove_instruction(last);
}

/*
 * Calculate the union of two waits. We may wait on any combination of slots #0,
 * #1, #2 or the entirety of 0126 and 01267. If we wait on the entirety, the
 * union is trivial. If we do not wait on slot #6 (by extension slot #7), we
 * wait only on slots #0, #1, and #2, for which the waits are encoded as a
 * bitset and the union is just a bitwise OR.
 */
static enum va_flow
union_waits(enum va_flow x, enum va_flow y)
{
   assert(va_flow_is_wait_or_none(x) && va_flow_is_wait_or_none(y));

   if ((x == VA_FLOW_WAIT) || (y == VA_FLOW_WAIT))
      return VA_FLOW_WAIT;
   else if ((x == VA_FLOW_WAIT0126) || (y == VA_FLOW_WAIT0126))
      return VA_FLOW_WAIT0126;
   else
      return x | y;
}

static void
merge_waits(bi_block *block)
{
   /* Most recent instruction with which we can merge, or NULL if none */
   bi_instr *last_free = NULL;

   bi_foreach_instr_in_block_safe(block, I) {
      if (last_free != NULL && I->op == BI_OPCODE_NOP &&
          va_flow_is_wait_or_none(I->flow)) {

         /* Merge waits with compatible instructions */
         last_free->flow = union_waits(last_free->flow, I->flow);
         bi_remove_instruction(I);
         continue;
      }

      /* Don't move waits past async instructions, since they might be what
       * we're waiting for. If we wanted to optimize this case, we could check
       * the signaled slots.
       */
      if (bi_opcode_props[I->op].message)
         last_free = NULL;

      /* We can only merge with instructions whose flow control is a wait.
       * This includes such an instruction after merging in a wait. It also
       * includes async instructions.
       */
      if (va_flow_is_wait_or_none(I->flow))
         last_free = I;
   }
}

static bool
bi_is_first_instr(bi_block *block, bi_instr *I)
{
   return block->instructions.next == &I->link;
}

static void
merge_discard(bi_block *block)
{
   bi_instr *last_free = NULL;

   bi_foreach_instr_in_block_safe_rev(block, I) {
      if ((I->op == BI_OPCODE_NOP) && (I->flow == VA_FLOW_DISCARD)) {
         /* Try to merge with the instruction *preceding* discard, because
          * because flow control happens at the end of an instruction and
          * discard is a NOP.
          */
         if (!bi_is_first_instr(block, I)) {
            bi_instr *prev = bi_prev_op(I);

            if (prev->flow == VA_FLOW_NONE) {
               prev->flow = VA_FLOW_DISCARD;
               bi_remove_instruction(I);
               continue;
            }
         }

         /* Or try to merge with the next instruction with no flow control */
         if (last_free != NULL) {
            last_free->flow = VA_FLOW_DISCARD;
            bi_remove_instruction(I);
            continue;
         }

         /* If there's nowhere to merge and this is the end of the shader, just
          * remove the discard.
          */
         if (bi_num_successors(block) == 0) {
            bi_remove_instruction(I);
            continue;
         }
      }

      /* Allow merging into free instructions */
      if (I->flow == VA_FLOW_NONE)
         last_free = I;
   }
}

void
va_merge_flow(bi_context *ctx)
{
   bi_foreach_block(ctx, block) {
      /* If there are less than 2 instructions, there's nothing to merge */
      if (list_is_empty(&block->instructions))
         continue;
      if (list_is_singular(&block->instructions))
         continue;

      merge_end_reconverge(block);
      merge_waits(block);

      if (ctx->stage == MESA_SHADER_FRAGMENT && !ctx->inputs->is_blend)
         merge_discard(block);
   }
}
