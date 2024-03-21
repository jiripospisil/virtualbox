/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdbool.h>
#include "util/glheader.h"
#include "draw_validate.h"
#include "arrayobj.h"
#include "bufferobj.h"
#include "context.h"

#include "mtypes.h"
#include "pipelineobj.h"
#include "enums.h"
#include "state.h"
#include "transformfeedback.h"
#include "uniforms.h"
#include "program/prog_print.h"


/**
 * Compute the bitmask of allowed primitive types (ValidPrimMask) depending
 * on shaders and current states. This is used by draw validation.
 *
 * If some combinations of shaders and states are invalid, ValidPrimMask is
 * set to 0, which will always set GL_INVALID_OPERATION in draw calls
 * except for invalid enums, which will set GL_INVALID_ENUM, minimizing
 * the number of gl_context variables that have to be read by draw calls.
 */
void
_mesa_update_valid_to_render_state(struct gl_context *ctx)
{
   struct gl_pipeline_object *shader = ctx->_Shader;
   unsigned mask = ctx->SupportedPrimMask;
   bool drawpix_valid = true;

   if (_mesa_is_no_error_enabled(ctx)) {
      ctx->ValidPrimMask = mask;
      ctx->ValidPrimMaskIndexed = mask;
      ctx->DrawPixValid = drawpix_valid;
      return;
   }

   /* Start with an empty mask and set this to the trimmed mask at the end. */
   ctx->ValidPrimMask = 0;
   ctx->ValidPrimMaskIndexed = 0;
   ctx->DrawPixValid = false;

   /* The default error is GL_INVALID_OPERATION if mode is a valid enum.
    * It can be overriden by following code if we should return a different
    * error.
    */
   ctx->DrawGLError = GL_INVALID_OPERATION;

   if (!ctx->DrawBuffer ||
       ctx->DrawBuffer->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      ctx->DrawGLError = GL_INVALID_FRAMEBUFFER_OPERATION;
      return;
   }

   /* A pipeline object is bound */
   if (shader->Name && !shader->Validated &&
       !_mesa_validate_program_pipeline(ctx, shader))
      return;

   /* If a program is active and SSO not in use, check if validation of
    * samplers succeeded for the active program. */
   if (shader->ActiveProgram && shader != ctx->Pipeline.Current &&
       !_mesa_sampler_uniforms_are_valid(shader->ActiveProgram, NULL, 0))
      return;

   /* The ARB_blend_func_extended spec's ERRORS section says:
    *
    *    "The error INVALID_OPERATION is generated by Begin or any procedure
    *     that implicitly calls Begin if any draw buffer has a blend function
    *     requiring the second color input (SRC1_COLOR, ONE_MINUS_SRC1_COLOR,
    *     SRC1_ALPHA or ONE_MINUS_SRC1_ALPHA), and a framebuffer is bound that
    *     has more than the value of MAX_DUAL_SOURCE_DRAW_BUFFERS-1 active
    *     color attachements."
    */
   unsigned max_dual_source_buffers = ctx->Const.MaxDualSourceDrawBuffers;
   unsigned num_color_buffers = ctx->DrawBuffer->_NumColorDrawBuffers;

   if (num_color_buffers > max_dual_source_buffers &&
       ctx->Color._BlendUsesDualSrc &
       BITFIELD_RANGE(max_dual_source_buffers,
                      num_color_buffers - max_dual_source_buffers))
      return;

   if (ctx->Color.BlendEnabled &&
       ctx->Color._AdvancedBlendMode != BLEND_NONE) {
      /* The KHR_blend_equation_advanced spec says:
       *
       *    "If any non-NONE draw buffer uses a blend equation found in table
       *     X.1 or X.2, the error INVALID_OPERATION is generated by Begin or
       *     any operation that implicitly calls Begin (such as DrawElements)
       *     if:
       *
       *       * the draw buffer for color output zero selects multiple color
       *         buffers (e.g., FRONT_AND_BACK in the default framebuffer); or
       *
       *       * the draw buffer for any other color output is not NONE."
       */
      if (ctx->DrawBuffer->ColorDrawBuffer[0] == GL_FRONT_AND_BACK)
         return;

      for (unsigned i = 1; i < num_color_buffers; i++) {
         if (ctx->DrawBuffer->ColorDrawBuffer[i] != GL_NONE)
            return;
      }

      /* The KHR_blend_equation_advanced spec says:
       *
       *    "Advanced blending equations require the use of a fragment shader
       *     with a matching "blend_support" layout qualifier.  If the current
       *     blend equation is found in table X.1 or X.2, and the active
       *     fragment shader does not include the layout qualifier matching
       *     the blend equation or "blend_support_all_equations", the error
       *     INVALID_OPERATION is generated [...]"
       */
      const struct gl_program *prog =
         ctx->_Shader->CurrentProgram[MESA_SHADER_FRAGMENT];
      const GLbitfield blend_support = !prog ? 0 : prog->info.fs.advanced_blend_modes;

      if ((blend_support & BITFIELD_BIT(ctx->Color._AdvancedBlendMode)) == 0)
         return;
   }

   if (_mesa_is_desktop_gl_compat(ctx)) {
      if (!shader->CurrentProgram[MESA_SHADER_FRAGMENT]) {
         if (ctx->FragmentProgram.Enabled &&
             !_mesa_arb_fragment_program_enabled(ctx))
            return;

         /* If drawing to integer-valued color buffers, there must be an
          * active fragment shader (GL_EXT_texture_integer).
          */
         if (ctx->DrawBuffer->_IntegerBuffers)
            return;
      }
   }

   /* DrawPixels/CopyPixels/Bitmap is valid after this point. */
   ctx->DrawPixValid = true;

   /* Section 11.2 (Tessellation) of the ES 3.2 spec says:
    *
    * "An INVALID_OPERATION error is generated by any command that
    *  transfers vertices to the GL if the current program state has
    *  one but not both of a tessellation control shader and tessellation
    *  evaluation shader."
    *
    * The OpenGL spec argues that this is allowed because a tess ctrl shader
    * without a tess eval shader can be used with transform feedback.
    * However, glBeginTransformFeedback doesn't allow GL_PATCHES and
    * therefore doesn't allow tessellation.
    *
    * Further investigation showed that this is indeed a spec bug and
    * a tess ctrl shader without a tess eval shader shouldn't have been
    * allowed, because there is no API in GL 4.0 that can make use this
    * to produce something useful.
    *
    * Also, all vendors except one don't support a tess ctrl shader without
    * a tess eval shader anyway.
    */
   if (shader->CurrentProgram[MESA_SHADER_TESS_CTRL] &&
       !shader->CurrentProgram[MESA_SHADER_TESS_EVAL])
      return;

   switch (ctx->API) {
   case API_OPENGLES2:
      /* Section 11.2 (Tessellation) of the ES 3.2 spec says:
       *
       * "An INVALID_OPERATION error is generated by any command that
       *  transfers vertices to the GL if the current program state has
       *  one but not both of a tessellation control shader and tessellation
       *  evaluation shader."
       */
      if (_mesa_is_gles3(ctx) &&
          shader->CurrentProgram[MESA_SHADER_TESS_EVAL] &&
          !shader->CurrentProgram[MESA_SHADER_TESS_CTRL])
         return;

      /* From GL_EXT_color_buffer_float:
       *
       *     "Blending applies only if the color buffer has a fixed-point or
       *     or floating-point format. If the color buffer has an integer
       *     format, proceed to the next operation.  Furthermore, an
       *     INVALID_OPERATION error is generated by DrawArrays and the other
       *     drawing commands defined in section 2.8.3 (10.5 in ES 3.1) if
       *     blending is enabled (see below) and any draw buffer has 32-bit
       *     floating-point format components."
       *
       * However GL_EXT_float_blend removes this text.
       */
      if (!ctx->Extensions.EXT_float_blend &&
          (ctx->DrawBuffer->_FP32Buffers & ctx->Color.BlendEnabled))
         return;
      break;

   case API_OPENGL_CORE:
      /* Section 10.4 (Drawing Commands Using Vertex Arrays) of the OpenGL 4.5
       * Core Profile spec says:
       *
       *     "An INVALID_OPERATION error is generated if no vertex array
       *     object is bound (see section 10.3.1)."
       */
      if (ctx->Array.VAO == ctx->Array.DefaultVAO)
         return;
      break;

   case API_OPENGLES:
      break;

   case API_OPENGL_COMPAT:
      /* Check invalid ARB vertex programs. */
      if (!shader->CurrentProgram[MESA_SHADER_VERTEX] &&
          ctx->VertexProgram.Enabled &&
          !_mesa_arb_vertex_program_enabled(ctx))
         return;
      break;

   default:
      unreachable("Invalid API value in _mesa_update_valid_to_render_state");
   }

   /* From the GL_NV_fill_rectangle spec:
    *
    * "An INVALID_OPERATION error is generated by Begin or any Draw command if
    *  only one of the front and back polygon mode is FILL_RECTANGLE_NV."
    */
   if ((ctx->Polygon.FrontMode == GL_FILL_RECTANGLE_NV) !=
       (ctx->Polygon.BackMode == GL_FILL_RECTANGLE_NV))
      return;

   /* From GL_INTEL_conservative_rasterization spec:
    *
    * The conservative rasterization option applies only to polygons with
    * PolygonMode state set to FILL. Draw requests for polygons with different
    * PolygonMode setting or for other primitive types (points/lines) generate
    * INVALID_OPERATION error.
    */
   if (ctx->IntelConservativeRasterization) {
      if (ctx->Polygon.FrontMode != GL_FILL ||
          ctx->Polygon.BackMode != GL_FILL) {
         return;
      } else {
         mask &= (1 << GL_TRIANGLES) |
                 (1 << GL_TRIANGLE_STRIP) |
                 (1 << GL_TRIANGLE_FAN) |
                 (1 << GL_QUADS) |
                 (1 << GL_QUAD_STRIP) |
                 (1 << GL_POLYGON) |
                 (1 << GL_TRIANGLES_ADJACENCY) |
                 (1 << GL_TRIANGLE_STRIP_ADJACENCY);
      }
   }

   /* From the GL_EXT_transform_feedback spec:
    *
    *     "The error INVALID_OPERATION is generated if Begin, or any command
    *      that performs an explicit Begin, is called when:
    *
    *      * a geometry shader is not active and <mode> does not match the
    *        allowed begin modes for the current transform feedback state as
    *        given by table X.1.
    *
    *      * a geometry shader is active and the output primitive type of the
    *        geometry shader does not match the allowed begin modes for the
    *        current transform feedback state as given by table X.1.
    *
    */
   if (_mesa_is_xfb_active_and_unpaused(ctx)) {
      if(shader->CurrentProgram[MESA_SHADER_GEOMETRY]) {
         switch (shader->CurrentProgram[MESA_SHADER_GEOMETRY]->
                    info.gs.output_primitive) {
         case GL_POINTS:
            if (ctx->TransformFeedback.Mode != GL_POINTS)
               mask = 0;
            break;
         case GL_LINE_STRIP:
            if (ctx->TransformFeedback.Mode != GL_LINES)
               mask = 0;
            break;
         case GL_TRIANGLE_STRIP:
            if (ctx->TransformFeedback.Mode != GL_TRIANGLES)
               mask = 0;
            break;
         default:
            mask = 0;
         }
      }
      else if (shader->CurrentProgram[MESA_SHADER_TESS_EVAL]) {
         struct gl_program *tes =
            shader->CurrentProgram[MESA_SHADER_TESS_EVAL];
         if (tes->info.tess.point_mode) {
            if (ctx->TransformFeedback.Mode != GL_POINTS)
               mask = 0;
         } else if (tes->info.tess._primitive_mode == TESS_PRIMITIVE_ISOLINES) {
            if (ctx->TransformFeedback.Mode != GL_LINES)
               mask = 0;
         } else {
            if (ctx->TransformFeedback.Mode != GL_TRIANGLES)
               mask = 0;
         }
      }
      else {
         switch (ctx->TransformFeedback.Mode) {
         case GL_POINTS:
            mask &= 1 << GL_POINTS;
            break;
         case GL_LINES:
            mask &= (1 << GL_LINES) |
                    (1 << GL_LINE_LOOP) |
                    (1 << GL_LINE_STRIP);
            break;
         case GL_TRIANGLES:
            /* TODO: This doesn't look right, but it matches the original code. */
            mask &= ~((1 << GL_POINTS) |
                      (1 << GL_LINES) |
                      (1 << GL_LINE_LOOP) |
                      (1 << GL_LINE_STRIP));
            break;
         }
      }

      if (!mask)
         return;
   }

   /* From the OpenGL 4.5 specification, section 11.3.1:
    *
    * The error INVALID_OPERATION is generated if Begin, or any command that
    * implicitly calls Begin, is called when a geometry shader is active and:
    *
    * * the input primitive type of the current geometry shader is
    *   POINTS and <mode> is not POINTS,
    *
    * * the input primitive type of the current geometry shader is
    *   LINES and <mode> is not LINES, LINE_STRIP, or LINE_LOOP,
    *
    * * the input primitive type of the current geometry shader is
    *   TRIANGLES and <mode> is not TRIANGLES, TRIANGLE_STRIP or
    *   TRIANGLE_FAN,
    *
    * * the input primitive type of the current geometry shader is
    *   LINES_ADJACENCY_ARB and <mode> is not LINES_ADJACENCY_ARB or
    *   LINE_STRIP_ADJACENCY_ARB, or
    *
    * * the input primitive type of the current geometry shader is
    *   TRIANGLES_ADJACENCY_ARB and <mode> is not
    *   TRIANGLES_ADJACENCY_ARB or TRIANGLE_STRIP_ADJACENCY_ARB.
    *
    * The GL spec doesn't mention any interaction with tessellation, which
    * is clearly a spec bug. The same rule should apply, but instead of
    * the draw primitive mode, the tessellation evaluation shader primitive
    * mode should be used for the checking.
   */
   if (shader->CurrentProgram[MESA_SHADER_GEOMETRY]) {
      const GLenum geom_mode =
         shader->CurrentProgram[MESA_SHADER_GEOMETRY]->
            info.gs.input_primitive;
      struct gl_program *tes =
         shader->CurrentProgram[MESA_SHADER_TESS_EVAL];

      if (tes) {
         bool valid;

         if (tes->info.tess.point_mode)
            valid = geom_mode == GL_POINTS;
         else if (tes->info.tess._primitive_mode == TESS_PRIMITIVE_ISOLINES)
            valid = geom_mode == GL_LINES;
         else
            /* the GL_QUADS mode generates triangles too */
            valid = geom_mode == GL_TRIANGLES;

         /* TES and GS use incompatible primitive types. Discard all draws. */
         if (!valid)
            return;
      } else {
         switch (geom_mode) {
         case GL_POINTS:
            mask &= 1 << GL_POINTS;
            break;
         case GL_LINES:
            mask &= (1 << GL_LINES) |
                    (1 << GL_LINE_LOOP) |
                    (1 << GL_LINE_STRIP);
            break;
         case GL_TRIANGLES:
            mask &= (1 << GL_TRIANGLES) |
                    (1 << GL_TRIANGLE_STRIP) |
                    (1 << GL_TRIANGLE_FAN);
            break;
         case GL_LINES_ADJACENCY:
            mask &= (1 << GL_LINES_ADJACENCY) |
                    (1 << GL_LINE_STRIP_ADJACENCY);
            break;
         case GL_TRIANGLES_ADJACENCY:
            mask &= (1 << GL_TRIANGLES_ADJACENCY) |
                    (1 << GL_TRIANGLE_STRIP_ADJACENCY);
            break;
         }
      }
   }

   /* From the OpenGL 4.0 (Core Profile) spec (section 2.12):
    *
    *     "Tessellation operates only on patch primitives. If tessellation is
    *      active, any command that transfers vertices to the GL will
    *      generate an INVALID_OPERATION error if the primitive mode is not
    *      PATCHES.
    *      Patch primitives are not supported by pipeline stages below the
    *      tessellation evaluation shader. If there is no active program
    *      object or the active program object does not contain a tessellation
    *      evaluation shader, the error INVALID_OPERATION is generated by any
    *      command that transfers vertices to the GL if the primitive mode is
    *      PATCHES."
    *
    */
   if (shader->CurrentProgram[MESA_SHADER_TESS_EVAL] ||
       shader->CurrentProgram[MESA_SHADER_TESS_CTRL]) {
      mask &= 1 << GL_PATCHES;
   }
   else {
      mask &= ~(1 << GL_PATCHES);
   }

#ifdef DEBUG
   if (shader->Flags & GLSL_LOG) {
      struct gl_program **prog = shader->CurrentProgram;

      for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
	 if (prog[i] == NULL || prog[i]->_Used)
	    continue;

	 /* This is the first time this shader is being used.
	  * Append shader's constants/uniforms to log file.
	  *
	  * Only log data for the program target that matches the shader
	  * target.  It's possible to have a program bound to the vertex
	  * shader target that also supplied a fragment shader.  If that
	  * program isn't also bound to the fragment shader target we don't
	  * want to log its fragment data.
	  */
	 _mesa_append_uniforms_to_file(prog[i]);
      }

      for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
	 if (prog[i] != NULL)
	    prog[i]->_Used = GL_TRUE;
      }
   }
#endif

   /* Non-indexed draws are valid after this point. */
   ctx->ValidPrimMask = mask;

   /* Section 2.14.2 (Transform Feedback Primitive Capture) of the OpenGL ES
    * 3.1 spec says:
    *
    *   The error INVALID_OPERATION is also generated by DrawElements,
    *   DrawElementsInstanced, and DrawRangeElements while transform feedback
    *   is active and not paused, regardless of mode.
    *
    * The OES_geometry_shader_spec says:
    *
    *    Issues:
    *
    *    ...
    *
    *    (13) Does this extension change how transform feedback operates
    *    compared to unextended OpenGL ES 3.0 or 3.1?
    *
    *    RESOLVED: Yes... Since we no longer require being able to predict how
    *    much geometry will be generated, we also lift the restriction that
    *    only DrawArray* commands are supported and also support the
    *    DrawElements* commands for transform feedback.
    *
    * This should also be reflected in the body of the spec, but that appears
    * to have been overlooked.  The body of the spec only explicitly allows
    * the indirect versions.
    */
   if (_mesa_is_gles3(ctx) &&
       !_mesa_has_OES_geometry_shader(ctx) &&
       _mesa_is_xfb_active_and_unpaused(ctx))
      return;

   ctx->ValidPrimMaskIndexed = mask;
}
