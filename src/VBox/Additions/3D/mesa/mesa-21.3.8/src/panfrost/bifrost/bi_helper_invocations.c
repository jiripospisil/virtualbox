/*
 * Copyright (C) 2019-2021 Collabora, Ltd.
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
 *
 * Authors (Collabora):
 *    Alyssa Rosenzweig <alyssa.rosenzweig@collabora.com>
 */

#include "compiler.h"

/* Bifrost texture operations have a `skip` bit, instructinh helper invocations
 * to skip execution. Each clause has a `terminate_discarded_threads` bit,
 * which will terminate helper invocations.
 *
 * The terminate bit should be set on the last clause requiring helper
 * invocations. Without control flow, that's the last source-order instruction;
 * with control flow, there may be multiple such instructions (with ifs) or no
 * such instruction (with loops).
 *
 * The skip bit should be set unless the value of this instruction is required
 * by a future instruction requiring helper invocations. Consider:
 *
 *      0 = texture ...
 *      1 = fmul 0, #10
 *      2 = dfdx 1
 *      store 2
 *
 * Since the derivative calculation 2 requires helper invocations, the value 1
 * must be calculated by helper invocations, and since it depends on 0, 0 must
 * be calculated by helpers. Hence the texture op does NOT have the skip bit
 * set, and the clause containing the derivative has the terminate bit set.
 *
 * Calculating the terminate bit occurs by forward dataflow analysis to
 * determine which blocks require helper invocations. A block requires
 * invocations in if any of its instructions use helper invocations, or if it
 * depends on a block that requires invocation. With that analysis, the
 * terminate bit is set on the last instruction using invocations within any
 * block that does *not* require invocations out.
 *
 * Likewise, calculating the execute bit requires backward dataflow analysis
 * with union as the join operation and the generating set being the union of
 * sources of instructions writing executed values. The skip bit is the inverse
 * of the execute bit.
 */

static bool
bi_has_skip_bit(enum bi_opcode op)
{
        switch (op) {
        case BI_OPCODE_TEXC:
        case BI_OPCODE_TEXS_2D_F16:
        case BI_OPCODE_TEXS_2D_F32:
        case BI_OPCODE_TEXS_CUBE_F16:
        case BI_OPCODE_TEXS_CUBE_F32:
        case BI_OPCODE_VAR_TEX_F16:
        case BI_OPCODE_VAR_TEX_F32:
                return true;
        default:
                return false;
        }
}

/* Does a given instruction require helper threads to be active (because it
 * reads from other subgroup lanes)? This only applies to fragment shaders.
 * Other shader stages do not have a notion of helper threads. */

static bool
bi_instr_uses_helpers(bi_instr *I)
{
        switch (I->op) {
        case BI_OPCODE_TEXC:
        case BI_OPCODE_TEXS_2D_F16:
        case BI_OPCODE_TEXS_2D_F32:
        case BI_OPCODE_TEXS_CUBE_F16:
        case BI_OPCODE_TEXS_CUBE_F32:
        case BI_OPCODE_VAR_TEX_F16:
        case BI_OPCODE_VAR_TEX_F32:
                return !I->lod_mode; /* set for zero, clear for computed */
        case BI_OPCODE_CLPER_I32:
        case BI_OPCODE_CLPER_V6_I32:
                /* Fragment shaders require helpers to implement derivatives.
                 * Other shader stages don't have helpers at all */
                return true;
        default:
                return false;
        }
}

/* Does a block use helpers directly */
static bool
bi_block_uses_helpers(bi_block *block)
{
        bi_foreach_instr_in_block(block, I) {
                if (bi_instr_uses_helpers(I))
                        return true;
        }

        return false;
}

static bool
bi_block_terminates_helpers(bi_block *block)
{
        /* Can't terminate if a successor needs helpers */
        bi_foreach_successor(block, succ) {
                if (succ->pass_flags & 1)
                        return false;
        }

        /* Otherwise we terminate */
        return true;
}

void
bi_analyze_helper_terminate(bi_context *ctx)
{
        /* Other shader stages do not have a notion of helper threads, so we
         * can skip the analysis. Don't run for blend shaders, either, since
         * they run in the context of another shader that we don't see. */
        if (ctx->stage != MESA_SHADER_FRAGMENT || ctx->inputs->is_blend)
                return;

        /* Set blocks as directly requiring helpers, and if they do add them to
         * the worklist to propagate to their predecessors */

        struct set *worklist = _mesa_set_create(NULL,
                        _mesa_hash_pointer,
                        _mesa_key_pointer_equal);

        struct set *visited = _mesa_set_create(NULL,
                        _mesa_hash_pointer,
                        _mesa_key_pointer_equal);

        bi_foreach_block(ctx, block) {
                block->pass_flags = bi_block_uses_helpers(block) ? 1 : 0;

                if (block->pass_flags & 1)
                        _mesa_set_add(worklist, block);
        }

        /* Next, propagate back. Since there are a finite number of blocks, the
         * worklist (a subset of all the blocks) is finite. Since a block can
         * only be added to the worklist if it is not on the visited list and
         * the visited list - also a subset of the blocks - grows every
         * iteration, the algorithm must terminate. */

        struct set_entry *cur;

        while((cur = _mesa_set_next_entry(worklist, NULL)) != NULL) {
                /* Pop off a block requiring helpers */
                bi_block *blk = (struct bi_block *) cur->key;
                _mesa_set_remove(worklist, cur);

                /* Its predecessors also require helpers */
                bi_foreach_predecessor(blk, pred) {
                        if (!_mesa_set_search(visited, pred)) {
                                pred->pass_flags |= 1;
                                _mesa_set_add(worklist, pred);
                        }
                }
 
                _mesa_set_add(visited, blk);
        }

        _mesa_set_destroy(visited, NULL);
        _mesa_set_destroy(worklist, NULL);

        /* Finally, mark clauses requiring helpers */
        bi_foreach_block(ctx, block) {
                /* At the end, there are helpers iff we don't terminate */
                bool helpers = !bi_block_terminates_helpers(block);

                bi_foreach_clause_in_block_rev(block, clause) {
                        bi_foreach_instr_in_clause_rev(block, clause, I) {
                                helpers |= bi_instr_uses_helpers(I);
                        }

                        clause->td = !helpers;
                }
        }
}

static bool
bi_helper_block_update(BITSET_WORD *deps, bi_block *block)
{
        bool progress = false;

        bi_foreach_instr_in_block_rev(block, I) {
                /* If our destination is required by helper invocation... */
                if (I->dest[0].type != BI_INDEX_NORMAL)
                        continue;

                if (!BITSET_TEST(deps, bi_get_node(I->dest[0])))
                        continue;

                /* ...so are our sources */
                bi_foreach_src(I, s) {
                        if (I->src[s].type == BI_INDEX_NORMAL) {
                                unsigned node = bi_get_node(I->src[s]);
                                progress |= !BITSET_TEST(deps, node);
                                BITSET_SET(deps, node);
                        }
                }
        }

        return progress;
}

void
bi_analyze_helper_requirements(bi_context *ctx)
{
        unsigned temp_count = bi_max_temp(ctx);
        BITSET_WORD *deps = calloc(sizeof(BITSET_WORD), BITSET_WORDS(temp_count));

        /* Initialize with the sources of instructions consuming
         * derivatives */

        bi_foreach_instr_global(ctx, I) {
                if (I->dest[0].type != BI_INDEX_NORMAL) continue;
                if (!bi_instr_uses_helpers(I)) continue;

                bi_foreach_src(I, s) {
                        if (I->src[s].type == BI_INDEX_NORMAL)
                                BITSET_SET(deps, bi_get_node(I->src[s]));
                }
        }

        /* Propagate that up */

        struct set *work_list = _mesa_set_create(NULL,
                        _mesa_hash_pointer,
                        _mesa_key_pointer_equal);

        struct set *visited = _mesa_set_create(NULL,
                        _mesa_hash_pointer,
                        _mesa_key_pointer_equal);

        struct set_entry *cur = _mesa_set_add(work_list, pan_exit_block(&ctx->blocks));

        do {
                bi_block *blk = (struct bi_block *) cur->key;
                _mesa_set_remove(work_list, cur);

                bool progress = bi_helper_block_update(deps, blk);

                if (progress || !_mesa_set_search(visited, blk)) {
                        bi_foreach_predecessor(blk, pred)
                                _mesa_set_add(work_list, pred);
                }

                _mesa_set_add(visited, blk);
        } while((cur = _mesa_set_next_entry(work_list, NULL)) != NULL);

        _mesa_set_destroy(visited, NULL);
        _mesa_set_destroy(work_list, NULL);

        /* Set the execute bits */

        bi_foreach_instr_global(ctx, I) {
                if (!bi_has_skip_bit(I->op)) continue;
                if (I->dest[0].type != BI_INDEX_NORMAL) continue;

                I->skip = !BITSET_TEST(deps, bi_get_node(I->dest[0]));
        }

        free(deps);
}
