/*
 * Copyright © 2015 Intel Corporation
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "nir/nir_builder.h"
#include "radv_debug.h"
#include "radv_meta.h"
#include "radv_private.h"

#include "util/format_rgb9e5.h"
#include "vk_format.h"

enum { DEPTH_CLEAR_SLOW, DEPTH_CLEAR_FAST };

static void
build_color_shaders(struct nir_shader **out_vs, struct nir_shader **out_fs, uint32_t frag_output)
{
   nir_builder vs_b =
      nir_builder_init_simple_shader(MESA_SHADER_VERTEX, NULL, "meta_clear_color_vs");
   nir_builder fs_b =
      nir_builder_init_simple_shader(MESA_SHADER_FRAGMENT, NULL, "meta_clear_color_fs");

   const struct glsl_type *position_type = glsl_vec4_type();
   const struct glsl_type *color_type = glsl_vec4_type();

   nir_variable *vs_out_pos =
      nir_variable_create(vs_b.shader, nir_var_shader_out, position_type, "gl_Position");
   vs_out_pos->data.location = VARYING_SLOT_POS;

   nir_ssa_def *in_color_load =
      nir_load_push_constant(&fs_b, 4, 32, nir_imm_int(&fs_b, 0), .range = 16);

   nir_variable *fs_out_color =
      nir_variable_create(fs_b.shader, nir_var_shader_out, color_type, "f_color");
   fs_out_color->data.location = FRAG_RESULT_DATA0 + frag_output;

   nir_store_var(&fs_b, fs_out_color, in_color_load, 0xf);

   nir_ssa_def *outvec = radv_meta_gen_rect_vertices(&vs_b);
   nir_store_var(&vs_b, vs_out_pos, outvec, 0xf);

   const struct glsl_type *layer_type = glsl_int_type();
   nir_variable *vs_out_layer =
      nir_variable_create(vs_b.shader, nir_var_shader_out, layer_type, "v_layer");
   vs_out_layer->data.location = VARYING_SLOT_LAYER;
   vs_out_layer->data.interpolation = INTERP_MODE_FLAT;
   nir_ssa_def *inst_id = nir_load_instance_id(&vs_b);
   nir_ssa_def *base_instance = nir_load_base_instance(&vs_b);

   nir_ssa_def *layer_id = nir_iadd(&vs_b, inst_id, base_instance);
   nir_store_var(&vs_b, vs_out_layer, layer_id, 0x1);

   *out_vs = vs_b.shader;
   *out_fs = fs_b.shader;
}

static VkResult
create_pipeline(struct radv_device *device, struct radv_render_pass *render_pass, uint32_t samples,
                struct nir_shader *vs_nir, struct nir_shader *fs_nir,
                const VkPipelineVertexInputStateCreateInfo *vi_state,
                const VkPipelineDepthStencilStateCreateInfo *ds_state,
                const VkPipelineColorBlendStateCreateInfo *cb_state, const VkPipelineLayout layout,
                const struct radv_graphics_pipeline_create_info *extra,
                const VkAllocationCallbacks *alloc, VkPipeline *pipeline)
{
   VkDevice device_h = radv_device_to_handle(device);
   VkResult result;

   result = radv_graphics_pipeline_create(
      device_h, radv_pipeline_cache_to_handle(&device->meta_state.cache),
      &(VkGraphicsPipelineCreateInfo){
         .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
         .stageCount = fs_nir ? 2 : 1,
         .pStages =
            (VkPipelineShaderStageCreateInfo[]){
               {
                  .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                  .stage = VK_SHADER_STAGE_VERTEX_BIT,
                  .module = vk_shader_module_handle_from_nir(vs_nir),
                  .pName = "main",
               },
               {
                  .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                  .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                  .module = vk_shader_module_handle_from_nir(fs_nir),
                  .pName = "main",
               },
            },
         .pVertexInputState = vi_state,
         .pInputAssemblyState =
            &(VkPipelineInputAssemblyStateCreateInfo){
               .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
               .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
               .primitiveRestartEnable = false,
            },
         .pViewportState =
            &(VkPipelineViewportStateCreateInfo){
               .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
               .viewportCount = 1,
               .scissorCount = 1,
            },
         .pRasterizationState =
            &(VkPipelineRasterizationStateCreateInfo){
               .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
               .rasterizerDiscardEnable = false,
               .polygonMode = VK_POLYGON_MODE_FILL,
               .cullMode = VK_CULL_MODE_NONE,
               .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
               .depthBiasEnable = false,
            },
         .pMultisampleState =
            &(VkPipelineMultisampleStateCreateInfo){
               .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
               .rasterizationSamples = samples,
               .sampleShadingEnable = false,
               .pSampleMask = NULL,
               .alphaToCoverageEnable = false,
               .alphaToOneEnable = false,
            },
         .pDepthStencilState = ds_state,
         .pColorBlendState = cb_state,
         .pDynamicState =
            &(VkPipelineDynamicStateCreateInfo){
               /* The meta clear pipeline declares all state as dynamic.
                * As a consequence, vkCmdBindPipeline writes no dynamic state
                * to the cmd buffer. Therefore, at the end of the meta clear,
                * we need only restore dynamic state was vkCmdSet.
                */
               .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
               .dynamicStateCount = 8,
               .pDynamicStates =
                  (VkDynamicState[]){
                     /* Everything except stencil write mask */
                     VK_DYNAMIC_STATE_VIEWPORT,
                     VK_DYNAMIC_STATE_SCISSOR,
                     VK_DYNAMIC_STATE_LINE_WIDTH,
                     VK_DYNAMIC_STATE_DEPTH_BIAS,
                     VK_DYNAMIC_STATE_BLEND_CONSTANTS,
                     VK_DYNAMIC_STATE_DEPTH_BOUNDS,
                     VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK,
                     VK_DYNAMIC_STATE_STENCIL_REFERENCE,
                  },
            },
         .layout = layout,
         .flags = 0,
         .renderPass = radv_render_pass_to_handle(render_pass),
         .subpass = 0,
      },
      extra, alloc, pipeline);

   ralloc_free(vs_nir);
   ralloc_free(fs_nir);

   return result;
}

static VkResult
create_color_renderpass(struct radv_device *device, VkFormat vk_format, uint32_t samples,
                        VkRenderPass *pass)
{
   mtx_lock(&device->meta_state.mtx);
   if (*pass) {
      mtx_unlock(&device->meta_state.mtx);
      return VK_SUCCESS;
   }

   VkResult result = radv_CreateRenderPass2(
      radv_device_to_handle(device),
      &(VkRenderPassCreateInfo2){
         .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
         .attachmentCount = 1,
         .pAttachments =
            &(VkAttachmentDescription2){
               .sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
               .format = vk_format,
               .samples = samples,
               .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
               .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
               .initialLayout = VK_IMAGE_LAYOUT_GENERAL,
               .finalLayout = VK_IMAGE_LAYOUT_GENERAL,
            },
         .subpassCount = 1,
         .pSubpasses =
            &(VkSubpassDescription2){
               .sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
               .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
               .inputAttachmentCount = 0,
               .colorAttachmentCount = 1,
               .pColorAttachments =
                  &(VkAttachmentReference2){
                     .sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
                     .attachment = 0,
                     .layout = VK_IMAGE_LAYOUT_GENERAL,
                  },
               .pResolveAttachments = NULL,
               .pDepthStencilAttachment =
                  &(VkAttachmentReference2){
                     .sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
                     .attachment = VK_ATTACHMENT_UNUSED,
                     .layout = VK_IMAGE_LAYOUT_GENERAL,
                  },
               .preserveAttachmentCount = 0,
               .pPreserveAttachments = NULL,
            },
         .dependencyCount = 2,
         .pDependencies =
            (VkSubpassDependency2[]){{.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
                                      .srcSubpass = VK_SUBPASS_EXTERNAL,
                                      .dstSubpass = 0,
                                      .srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                      .dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                      .srcAccessMask = 0,
                                      .dstAccessMask = 0,
                                      .dependencyFlags = 0},
                                     {.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
                                      .srcSubpass = 0,
                                      .dstSubpass = VK_SUBPASS_EXTERNAL,
                                      .srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                      .dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                      .srcAccessMask = 0,
                                      .dstAccessMask = 0,
                                      .dependencyFlags = 0}},
      },
      &device->meta_state.alloc, pass);
   mtx_unlock(&device->meta_state.mtx);
   return result;
}

static VkResult
create_color_pipeline(struct radv_device *device, uint32_t samples, uint32_t frag_output,
                      VkPipeline *pipeline, VkRenderPass pass)
{
   struct nir_shader *vs_nir;
   struct nir_shader *fs_nir;
   VkResult result;

   mtx_lock(&device->meta_state.mtx);
   if (*pipeline) {
      mtx_unlock(&device->meta_state.mtx);
      return VK_SUCCESS;
   }

   build_color_shaders(&vs_nir, &fs_nir, frag_output);

   const VkPipelineVertexInputStateCreateInfo vi_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 0,
      .vertexAttributeDescriptionCount = 0,
   };

   const VkPipelineDepthStencilStateCreateInfo ds_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = false,
      .depthWriteEnable = false,
      .depthBoundsTestEnable = false,
      .stencilTestEnable = false,
   };

   VkPipelineColorBlendAttachmentState blend_attachment_state[MAX_RTS] = {0};
   blend_attachment_state[frag_output] = (VkPipelineColorBlendAttachmentState){
      .blendEnable = false,
      .colorWriteMask = VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT |
                        VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT,
   };

   const VkPipelineColorBlendStateCreateInfo cb_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOpEnable = false,
      .attachmentCount = MAX_RTS,
      .pAttachments = blend_attachment_state};

   struct radv_graphics_pipeline_create_info extra = {
      .use_rectlist = true,
   };
   result =
      create_pipeline(device, radv_render_pass_from_handle(pass), samples, vs_nir, fs_nir,
                      &vi_state, &ds_state, &cb_state, device->meta_state.clear_color_p_layout,
                      &extra, &device->meta_state.alloc, pipeline);

   mtx_unlock(&device->meta_state.mtx);
   return result;
}

static void
finish_meta_clear_htile_mask_state(struct radv_device *device)
{
   struct radv_meta_state *state = &device->meta_state;

   radv_DestroyPipeline(radv_device_to_handle(device), state->clear_htile_mask_pipeline,
                        &state->alloc);
   radv_DestroyPipelineLayout(radv_device_to_handle(device), state->clear_htile_mask_p_layout,
                              &state->alloc);
   radv_DestroyDescriptorSetLayout(radv_device_to_handle(device), state->clear_htile_mask_ds_layout,
                                   &state->alloc);
}

static void
finish_meta_clear_dcc_comp_to_single_state(struct radv_device *device)
{
   struct radv_meta_state *state = &device->meta_state;

   for (uint32_t i = 0; i < 2; i++) {
      radv_DestroyPipeline(radv_device_to_handle(device),
                           state->clear_dcc_comp_to_single_pipeline[i], &state->alloc);
   }
   radv_DestroyPipelineLayout(radv_device_to_handle(device), state->clear_dcc_comp_to_single_p_layout,
                              &state->alloc);
   radv_DestroyDescriptorSetLayout(radv_device_to_handle(device), state->clear_dcc_comp_to_single_ds_layout,
                                   &state->alloc);
}

void
radv_device_finish_meta_clear_state(struct radv_device *device)
{
   struct radv_meta_state *state = &device->meta_state;

   for (uint32_t i = 0; i < ARRAY_SIZE(state->clear); ++i) {
      for (uint32_t j = 0; j < ARRAY_SIZE(state->clear[i].color_pipelines); ++j) {
         radv_DestroyPipeline(radv_device_to_handle(device), state->clear[i].color_pipelines[j],
                              &state->alloc);
         radv_DestroyRenderPass(radv_device_to_handle(device), state->clear[i].render_pass[j],
                                &state->alloc);
      }

      for (uint32_t j = 0; j < NUM_DEPTH_CLEAR_PIPELINES; j++) {
         radv_DestroyPipeline(radv_device_to_handle(device), state->clear[i].depth_only_pipeline[j],
                              &state->alloc);
         radv_DestroyPipeline(radv_device_to_handle(device),
                              state->clear[i].stencil_only_pipeline[j], &state->alloc);
         radv_DestroyPipeline(radv_device_to_handle(device),
                              state->clear[i].depthstencil_pipeline[j], &state->alloc);

         radv_DestroyPipeline(radv_device_to_handle(device),
                              state->clear[i].depth_only_unrestricted_pipeline[j], &state->alloc);
         radv_DestroyPipeline(radv_device_to_handle(device),
                              state->clear[i].stencil_only_unrestricted_pipeline[j], &state->alloc);
         radv_DestroyPipeline(radv_device_to_handle(device),
                              state->clear[i].depthstencil_unrestricted_pipeline[j], &state->alloc);
      }
      radv_DestroyRenderPass(radv_device_to_handle(device), state->clear[i].depthstencil_rp,
                             &state->alloc);
   }
   radv_DestroyPipelineLayout(radv_device_to_handle(device), state->clear_color_p_layout,
                              &state->alloc);
   radv_DestroyPipelineLayout(radv_device_to_handle(device), state->clear_depth_p_layout,
                              &state->alloc);
   radv_DestroyPipelineLayout(radv_device_to_handle(device),
                              state->clear_depth_unrestricted_p_layout, &state->alloc);

   finish_meta_clear_htile_mask_state(device);
   finish_meta_clear_dcc_comp_to_single_state(device);
}

static void
emit_color_clear(struct radv_cmd_buffer *cmd_buffer, const VkClearAttachment *clear_att,
                 const VkClearRect *clear_rect, uint32_t view_mask)
{
   struct radv_device *device = cmd_buffer->device;
   const struct radv_subpass *subpass = cmd_buffer->state.subpass;
   const uint32_t subpass_att = clear_att->colorAttachment;
   const uint32_t pass_att = subpass->color_attachments[subpass_att].attachment;
   const struct radv_image_view *iview =
      cmd_buffer->state.attachments ? cmd_buffer->state.attachments[pass_att].iview : NULL;
   uint32_t samples, samples_log2;
   VkFormat format;
   unsigned fs_key;
   VkClearColorValue clear_value = clear_att->clearValue.color;
   VkCommandBuffer cmd_buffer_h = radv_cmd_buffer_to_handle(cmd_buffer);
   VkPipeline pipeline;

   /* When a framebuffer is bound to the current command buffer, get the
    * number of samples from it. Otherwise, get the number of samples from
    * the render pass because it's likely a secondary command buffer.
    */
   if (iview) {
      samples = iview->image->info.samples;
      format = iview->vk_format;
   } else {
      samples = cmd_buffer->state.pass->attachments[pass_att].samples;
      format = cmd_buffer->state.pass->attachments[pass_att].format;
   }

   samples_log2 = ffs(samples) - 1;
   fs_key = radv_format_meta_fs_key(device, format);
   assert(fs_key != -1);

   if (device->meta_state.clear[samples_log2].render_pass[fs_key] == VK_NULL_HANDLE) {
      VkResult ret =
         create_color_renderpass(device, radv_fs_key_format_exemplars[fs_key], samples,
                                 &device->meta_state.clear[samples_log2].render_pass[fs_key]);
      if (ret != VK_SUCCESS) {
         cmd_buffer->record_result = ret;
         return;
      }
   }

   if (device->meta_state.clear[samples_log2].color_pipelines[fs_key] == VK_NULL_HANDLE) {
      VkResult ret = create_color_pipeline(
         device, samples, 0, &device->meta_state.clear[samples_log2].color_pipelines[fs_key],
         device->meta_state.clear[samples_log2].render_pass[fs_key]);
      if (ret != VK_SUCCESS) {
         cmd_buffer->record_result = ret;
         return;
      }
   }

   pipeline = device->meta_state.clear[samples_log2].color_pipelines[fs_key];

   assert(samples_log2 < ARRAY_SIZE(device->meta_state.clear));
   assert(pipeline);
   assert(clear_att->aspectMask == VK_IMAGE_ASPECT_COLOR_BIT);
   assert(clear_att->colorAttachment < subpass->color_count);

   radv_CmdPushConstants(radv_cmd_buffer_to_handle(cmd_buffer),
                         device->meta_state.clear_color_p_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                         16, &clear_value);

   struct radv_subpass clear_subpass = {
      .color_count = 1,
      .color_attachments =
         (struct radv_subpass_attachment[]){subpass->color_attachments[clear_att->colorAttachment]},
      .depth_stencil_attachment = NULL,
   };

   radv_cmd_buffer_set_subpass(cmd_buffer, &clear_subpass);

   radv_CmdBindPipeline(cmd_buffer_h, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

   radv_CmdSetViewport(radv_cmd_buffer_to_handle(cmd_buffer), 0, 1,
                       &(VkViewport){.x = clear_rect->rect.offset.x,
                                     .y = clear_rect->rect.offset.y,
                                     .width = clear_rect->rect.extent.width,
                                     .height = clear_rect->rect.extent.height,
                                     .minDepth = 0.0f,
                                     .maxDepth = 1.0f});

   radv_CmdSetScissor(radv_cmd_buffer_to_handle(cmd_buffer), 0, 1, &clear_rect->rect);

   if (view_mask) {
      u_foreach_bit(i, view_mask) radv_CmdDraw(cmd_buffer_h, 3, 1, 0, i);
   } else {
      radv_CmdDraw(cmd_buffer_h, 3, clear_rect->layerCount, 0, clear_rect->baseArrayLayer);
   }

   radv_cmd_buffer_restore_subpass(cmd_buffer, subpass);
}

static void
build_depthstencil_shader(struct nir_shader **out_vs, struct nir_shader **out_fs, bool unrestricted)
{
   nir_builder vs_b = nir_builder_init_simple_shader(
      MESA_SHADER_VERTEX, NULL,
      unrestricted ? "meta_clear_depthstencil_unrestricted_vs" : "meta_clear_depthstencil_vs");
   nir_builder fs_b = nir_builder_init_simple_shader(
      MESA_SHADER_FRAGMENT, NULL,
      unrestricted ? "meta_clear_depthstencil_unrestricted_fs" : "meta_clear_depthstencil_fs");

   const struct glsl_type *position_out_type = glsl_vec4_type();

   nir_variable *vs_out_pos =
      nir_variable_create(vs_b.shader, nir_var_shader_out, position_out_type, "gl_Position");
   vs_out_pos->data.location = VARYING_SLOT_POS;

   nir_ssa_def *z;
   if (unrestricted) {
      nir_ssa_def *in_color_load =
         nir_load_push_constant(&fs_b, 1, 32, nir_imm_int(&fs_b, 0), .range = 4);

      nir_variable *fs_out_depth =
         nir_variable_create(fs_b.shader, nir_var_shader_out, glsl_int_type(), "f_depth");
      fs_out_depth->data.location = FRAG_RESULT_DEPTH;
      nir_store_var(&fs_b, fs_out_depth, in_color_load, 0x1);

      z = nir_imm_float(&vs_b, 0.0);
   } else {
      z = nir_load_push_constant(&vs_b, 1, 32, nir_imm_int(&vs_b, 0), .range = 4);
   }

   nir_ssa_def *outvec = radv_meta_gen_rect_vertices_comp2(&vs_b, z);
   nir_store_var(&vs_b, vs_out_pos, outvec, 0xf);

   const struct glsl_type *layer_type = glsl_int_type();
   nir_variable *vs_out_layer =
      nir_variable_create(vs_b.shader, nir_var_shader_out, layer_type, "v_layer");
   vs_out_layer->data.location = VARYING_SLOT_LAYER;
   vs_out_layer->data.interpolation = INTERP_MODE_FLAT;
   nir_ssa_def *inst_id = nir_load_instance_id(&vs_b);
   nir_ssa_def *base_instance = nir_load_base_instance(&vs_b);

   nir_ssa_def *layer_id = nir_iadd(&vs_b, inst_id, base_instance);
   nir_store_var(&vs_b, vs_out_layer, layer_id, 0x1);

   *out_vs = vs_b.shader;
   *out_fs = fs_b.shader;
}

static VkResult
create_depthstencil_renderpass(struct radv_device *device, uint32_t samples,
                               VkRenderPass *render_pass)
{
   mtx_lock(&device->meta_state.mtx);
   if (*render_pass) {
      mtx_unlock(&device->meta_state.mtx);
      return VK_SUCCESS;
   }

   VkResult result = radv_CreateRenderPass2(
      radv_device_to_handle(device),
      &(VkRenderPassCreateInfo2){
         .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
         .attachmentCount = 1,
         .pAttachments =
            &(VkAttachmentDescription2){
               .sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
               .format = VK_FORMAT_D32_SFLOAT_S8_UINT,
               .samples = samples,
               .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
               .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
               .initialLayout = VK_IMAGE_LAYOUT_GENERAL,
               .finalLayout = VK_IMAGE_LAYOUT_GENERAL,
            },
         .subpassCount = 1,
         .pSubpasses =
            &(VkSubpassDescription2){
               .sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
               .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
               .inputAttachmentCount = 0,
               .colorAttachmentCount = 0,
               .pColorAttachments = NULL,
               .pResolveAttachments = NULL,
               .pDepthStencilAttachment =
                  &(VkAttachmentReference2){
                     .sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
                     .attachment = 0,
                     .layout = VK_IMAGE_LAYOUT_GENERAL,
                  },
               .preserveAttachmentCount = 0,
               .pPreserveAttachments = NULL,
            },
         .dependencyCount = 2,
         .pDependencies =
            (VkSubpassDependency2[]){{.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
                                      .srcSubpass = VK_SUBPASS_EXTERNAL,
                                      .dstSubpass = 0,
                                      .srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                      .dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                      .srcAccessMask = 0,
                                      .dstAccessMask = 0,
                                      .dependencyFlags = 0},
                                     {.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
                                      .srcSubpass = 0,
                                      .dstSubpass = VK_SUBPASS_EXTERNAL,
                                      .srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                      .dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                      .srcAccessMask = 0,
                                      .dstAccessMask = 0,
                                      .dependencyFlags = 0}}},
      &device->meta_state.alloc, render_pass);
   mtx_unlock(&device->meta_state.mtx);
   return result;
}

static VkResult
create_depthstencil_pipeline(struct radv_device *device, VkImageAspectFlags aspects,
                             uint32_t samples, int index, bool unrestricted, VkPipeline *pipeline,
                             VkRenderPass render_pass)
{
   struct nir_shader *vs_nir, *fs_nir;
   VkResult result;

   mtx_lock(&device->meta_state.mtx);
   if (*pipeline) {
      mtx_unlock(&device->meta_state.mtx);
      return VK_SUCCESS;
   }

   build_depthstencil_shader(&vs_nir, &fs_nir, unrestricted);

   const VkPipelineVertexInputStateCreateInfo vi_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 0,
      .vertexAttributeDescriptionCount = 0,
   };

   const VkPipelineDepthStencilStateCreateInfo ds_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = !!(aspects & VK_IMAGE_ASPECT_DEPTH_BIT),
      .depthCompareOp = VK_COMPARE_OP_ALWAYS,
      .depthWriteEnable = !!(aspects & VK_IMAGE_ASPECT_DEPTH_BIT),
      .depthBoundsTestEnable = false,
      .stencilTestEnable = !!(aspects & VK_IMAGE_ASPECT_STENCIL_BIT),
      .front =
         {
            .passOp = VK_STENCIL_OP_REPLACE,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .writeMask = UINT32_MAX,
            .reference = 0, /* dynamic */
         },
      .back = {0 /* dont care */},
   };

   const VkPipelineColorBlendStateCreateInfo cb_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOpEnable = false,
      .attachmentCount = 0,
      .pAttachments = NULL,
   };

   struct radv_graphics_pipeline_create_info extra = {
      .use_rectlist = true,
   };

   if (aspects & VK_IMAGE_ASPECT_DEPTH_BIT) {
      extra.db_depth_clear = index == DEPTH_CLEAR_SLOW ? false : true;
   }
   if (aspects & VK_IMAGE_ASPECT_STENCIL_BIT) {
      extra.db_stencil_clear = index == DEPTH_CLEAR_SLOW ? false : true;
   }
   result =
      create_pipeline(device, radv_render_pass_from_handle(render_pass), samples, vs_nir, fs_nir,
                      &vi_state, &ds_state, &cb_state, device->meta_state.clear_depth_p_layout,
                      &extra, &device->meta_state.alloc, pipeline);

   mtx_unlock(&device->meta_state.mtx);
   return result;
}

static bool
depth_view_can_fast_clear(struct radv_cmd_buffer *cmd_buffer, const struct radv_image_view *iview,
                          VkImageAspectFlags aspects, VkImageLayout layout, bool in_render_loop,
                          const VkClearRect *clear_rect, VkClearDepthStencilValue clear_value)
{
   if (!iview)
      return false;

   uint32_t queue_mask = radv_image_queue_family_mask(iview->image, cmd_buffer->queue_family_index,
                                                      cmd_buffer->queue_family_index);
   if (clear_rect->rect.offset.x || clear_rect->rect.offset.y ||
       clear_rect->rect.extent.width != iview->extent.width ||
       clear_rect->rect.extent.height != iview->extent.height)
      return false;
   if (radv_image_is_tc_compat_htile(iview->image) &&
       (((aspects & VK_IMAGE_ASPECT_DEPTH_BIT) && clear_value.depth != 0.0 &&
         clear_value.depth != 1.0) ||
        ((aspects & VK_IMAGE_ASPECT_STENCIL_BIT) && clear_value.stencil != 0)))
      return false;
   if (radv_htile_enabled(iview->image, iview->base_mip) && iview->base_mip == 0 &&
       iview->base_layer == 0 && iview->layer_count == iview->image->info.array_size &&
       radv_layout_is_htile_compressed(cmd_buffer->device, iview->image, layout, in_render_loop,
                                       queue_mask) &&
       radv_image_extent_compare(iview->image, &iview->extent))
      return true;
   return false;
}

static VkPipeline
pick_depthstencil_pipeline(struct radv_cmd_buffer *cmd_buffer, struct radv_meta_state *meta_state,
                           const struct radv_image_view *iview, int samples_log2,
                           VkImageAspectFlags aspects, VkImageLayout layout, bool in_render_loop,
                           const VkClearRect *clear_rect, VkClearDepthStencilValue clear_value)
{
   bool fast = depth_view_can_fast_clear(cmd_buffer, iview, aspects, layout, in_render_loop,
                                         clear_rect, clear_value);
   bool unrestricted = cmd_buffer->device->vk.enabled_extensions.EXT_depth_range_unrestricted;
   int index = fast ? DEPTH_CLEAR_FAST : DEPTH_CLEAR_SLOW;
   VkPipeline *pipeline;

   switch (aspects) {
   case VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT:
      pipeline = unrestricted
                    ? &meta_state->clear[samples_log2].depthstencil_unrestricted_pipeline[index]
                    : &meta_state->clear[samples_log2].depthstencil_pipeline[index];
      break;
   case VK_IMAGE_ASPECT_DEPTH_BIT:
      pipeline = unrestricted
                    ? &meta_state->clear[samples_log2].depth_only_unrestricted_pipeline[index]
                    : &meta_state->clear[samples_log2].depth_only_pipeline[index];
      break;
   case VK_IMAGE_ASPECT_STENCIL_BIT:
      pipeline = unrestricted
                    ? &meta_state->clear[samples_log2].stencil_only_unrestricted_pipeline[index]
                    : &meta_state->clear[samples_log2].stencil_only_pipeline[index];
      break;
   default:
      unreachable("expected depth or stencil aspect");
   }

   if (cmd_buffer->device->meta_state.clear[samples_log2].depthstencil_rp == VK_NULL_HANDLE) {
      VkResult ret = create_depthstencil_renderpass(
         cmd_buffer->device, 1u << samples_log2,
         &cmd_buffer->device->meta_state.clear[samples_log2].depthstencil_rp);
      if (ret != VK_SUCCESS) {
         cmd_buffer->record_result = ret;
         return VK_NULL_HANDLE;
      }
   }

   if (*pipeline == VK_NULL_HANDLE) {
      VkResult ret = create_depthstencil_pipeline(
         cmd_buffer->device, aspects, 1u << samples_log2, index, unrestricted, pipeline,
         cmd_buffer->device->meta_state.clear[samples_log2].depthstencil_rp);
      if (ret != VK_SUCCESS) {
         cmd_buffer->record_result = ret;
         return VK_NULL_HANDLE;
      }
   }
   return *pipeline;
}

static void
emit_depthstencil_clear(struct radv_cmd_buffer *cmd_buffer, const VkClearAttachment *clear_att,
                        const VkClearRect *clear_rect, struct radv_subpass_attachment *ds_att,
                        uint32_t view_mask)
{
   struct radv_device *device = cmd_buffer->device;
   struct radv_meta_state *meta_state = &device->meta_state;
   const struct radv_subpass *subpass = cmd_buffer->state.subpass;
   const uint32_t pass_att = ds_att->attachment;
   VkClearDepthStencilValue clear_value = clear_att->clearValue.depthStencil;
   VkImageAspectFlags aspects = clear_att->aspectMask;
   const struct radv_image_view *iview =
      cmd_buffer->state.attachments ? cmd_buffer->state.attachments[pass_att].iview : NULL;
   uint32_t samples, samples_log2;
   VkCommandBuffer cmd_buffer_h = radv_cmd_buffer_to_handle(cmd_buffer);

   /* When a framebuffer is bound to the current command buffer, get the
    * number of samples from it. Otherwise, get the number of samples from
    * the render pass because it's likely a secondary command buffer.
    */
   if (iview) {
      samples = iview->image->info.samples;
   } else {
      samples = cmd_buffer->state.pass->attachments[pass_att].samples;
   }

   samples_log2 = ffs(samples) - 1;

   assert(pass_att != VK_ATTACHMENT_UNUSED);

   if (!(aspects & VK_IMAGE_ASPECT_DEPTH_BIT))
      clear_value.depth = 1.0f;

   if (cmd_buffer->device->vk.enabled_extensions.EXT_depth_range_unrestricted) {
      radv_CmdPushConstants(radv_cmd_buffer_to_handle(cmd_buffer),
                            device->meta_state.clear_depth_unrestricted_p_layout,
                            VK_SHADER_STAGE_FRAGMENT_BIT, 0, 4, &clear_value.depth);
   } else {
      radv_CmdPushConstants(radv_cmd_buffer_to_handle(cmd_buffer),
                            device->meta_state.clear_depth_p_layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                            4, &clear_value.depth);
   }

   uint32_t prev_reference = cmd_buffer->state.dynamic.stencil_reference.front;
   if (aspects & VK_IMAGE_ASPECT_STENCIL_BIT) {
      radv_CmdSetStencilReference(cmd_buffer_h, VK_STENCIL_FACE_FRONT_BIT, clear_value.stencil);
   }

   VkPipeline pipeline =
      pick_depthstencil_pipeline(cmd_buffer, meta_state, iview, samples_log2, aspects,
                                 ds_att->layout, ds_att->in_render_loop, clear_rect, clear_value);
   if (!pipeline)
      return;

   struct radv_subpass clear_subpass = {
      .color_count = 0,
      .color_attachments = NULL,
      .depth_stencil_attachment = ds_att,
   };

   radv_cmd_buffer_set_subpass(cmd_buffer, &clear_subpass);

   radv_CmdBindPipeline(cmd_buffer_h, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

   if (depth_view_can_fast_clear(cmd_buffer, iview, aspects, ds_att->layout, ds_att->in_render_loop,
                                 clear_rect, clear_value))
      radv_update_ds_clear_metadata(cmd_buffer, iview, clear_value, aspects);

   radv_CmdSetViewport(radv_cmd_buffer_to_handle(cmd_buffer), 0, 1,
                       &(VkViewport){.x = clear_rect->rect.offset.x,
                                     .y = clear_rect->rect.offset.y,
                                     .width = clear_rect->rect.extent.width,
                                     .height = clear_rect->rect.extent.height,
                                     .minDepth = 0.0f,
                                     .maxDepth = 1.0f});

   radv_CmdSetScissor(radv_cmd_buffer_to_handle(cmd_buffer), 0, 1, &clear_rect->rect);

   if (view_mask) {
      u_foreach_bit(i, view_mask) radv_CmdDraw(cmd_buffer_h, 3, 1, 0, i);
   } else {
      radv_CmdDraw(cmd_buffer_h, 3, clear_rect->layerCount, 0, clear_rect->baseArrayLayer);
   }

   if (aspects & VK_IMAGE_ASPECT_STENCIL_BIT) {
      radv_CmdSetStencilReference(cmd_buffer_h, VK_STENCIL_FACE_FRONT_BIT, prev_reference);
   }

   radv_cmd_buffer_restore_subpass(cmd_buffer, subpass);
}

static uint32_t
clear_htile_mask(struct radv_cmd_buffer *cmd_buffer, const struct radv_image *image,
                 struct radeon_winsys_bo *bo, uint64_t offset, uint64_t size, uint32_t htile_value,
                 uint32_t htile_mask)
{
   struct radv_device *device = cmd_buffer->device;
   struct radv_meta_state *state = &device->meta_state;
   uint64_t block_count = round_up_u64(size, 1024);
   struct radv_meta_saved_state saved_state;
   struct radv_buffer dst_buffer;

   radv_meta_save(
      &saved_state, cmd_buffer,
      RADV_META_SAVE_COMPUTE_PIPELINE | RADV_META_SAVE_CONSTANTS | RADV_META_SAVE_DESCRIPTORS);

   radv_buffer_init(&dst_buffer, device, bo, size, offset);

   radv_CmdBindPipeline(radv_cmd_buffer_to_handle(cmd_buffer), VK_PIPELINE_BIND_POINT_COMPUTE,
                        state->clear_htile_mask_pipeline);

   radv_meta_push_descriptor_set(
      cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, state->clear_htile_mask_p_layout, 0, /* set */
      1, /* descriptorWriteCount */
      (VkWriteDescriptorSet[]){
         {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstBinding = 0,
          .dstArrayElement = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .pBufferInfo = &(VkDescriptorBufferInfo){.buffer = radv_buffer_to_handle(&dst_buffer),
                                                   .offset = 0,
                                                   .range = size}}});

   const unsigned constants[2] = {
      htile_value & htile_mask,
      ~htile_mask,
   };

   radv_CmdPushConstants(radv_cmd_buffer_to_handle(cmd_buffer), state->clear_htile_mask_p_layout,
                         VK_SHADER_STAGE_COMPUTE_BIT, 0, 8, constants);

   radv_CmdDispatch(radv_cmd_buffer_to_handle(cmd_buffer), block_count, 1, 1);

   radv_buffer_finish(&dst_buffer);

   radv_meta_restore(&saved_state, cmd_buffer);

   return RADV_CMD_FLAG_CS_PARTIAL_FLUSH |
          radv_src_access_flush(cmd_buffer, VK_ACCESS_SHADER_WRITE_BIT, image);
}

static uint32_t
radv_get_htile_fast_clear_value(const struct radv_device *device, const struct radv_image *image,
                                VkClearDepthStencilValue value)
{
   uint32_t max_zval = 0x3fff; /* maximum 14-bit value. */
   uint32_t zmask = 0, smem = 0;
   uint32_t htile_value;
   uint32_t zmin, zmax;

   /* Convert the depth value to 14-bit zmin/zmax values. */
   zmin = lroundf(value.depth * max_zval);
   zmax = zmin;

   if (radv_image_tile_stencil_disabled(device, image)) {
      /* Z only (no stencil):
       *
       * |31     18|17      4|3     0|
       * +---------+---------+-------+
       * |  Max Z  |  Min Z  | ZMask |
       */
      htile_value = (((zmax  & 0x3fff) << 18) |
                     ((zmin  & 0x3fff) <<  4) |
                     ((zmask &    0xf) <<  0));
   } else {

      /* Z and stencil:
       *
       * |31       12|11 10|9    8|7   6|5   4|3     0|
       * +-----------+-----+------+-----+-----+-------+
       * |  Z Range  |     | SMem | SR1 | SR0 | ZMask |
       *
       * Z, stencil, 4 bit VRS encoding:
       * |31       12| 11      10 |9    8|7         6 |5   4|3     0|
       * +-----------+------------+------+------------+-----+-------+
       * |  Z Range  | VRS Y-rate | SMem | VRS X-rate | SR0 | ZMask |
       */
      uint32_t delta = 0;
      uint32_t zrange = ((zmax << 6) | delta);
      uint32_t sresults = 0xf; /* SR0/SR1 both as 0x3. */

      if (radv_image_has_vrs_htile(device, image))
         sresults = 0x3;

      htile_value = (((zrange   & 0xfffff) << 12) |
                     ((smem     & 0x3)     <<  8) |
                     ((sresults & 0xf)     <<  4) |
                     ((zmask    & 0xf)     <<  0));
   }

   return htile_value;
}

static uint32_t
radv_get_htile_mask(const struct radv_device *device, const struct radv_image *image,
                    VkImageAspectFlags aspects)
{
   uint32_t mask = 0;

   if (radv_image_tile_stencil_disabled(device, image)) {
      /* All the HTILE buffer is used when there is no stencil. */
      mask = UINT32_MAX;
   } else {
      if (aspects & VK_IMAGE_ASPECT_DEPTH_BIT)
         mask |= 0xfffffc0f;
      if (aspects & VK_IMAGE_ASPECT_STENCIL_BIT)
         mask |= 0x000003f0;
   }

   return mask;
}

static bool
radv_is_fast_clear_depth_allowed(VkClearDepthStencilValue value)
{
   return value.depth == 1.0f || value.depth == 0.0f;
}

static bool
radv_is_fast_clear_stencil_allowed(VkClearDepthStencilValue value)
{
   return value.stencil == 0;
}

static bool
radv_can_fast_clear_depth(struct radv_cmd_buffer *cmd_buffer, const struct radv_image_view *iview,
                          VkImageLayout image_layout, bool in_render_loop,
                          VkImageAspectFlags aspects, const VkClearRect *clear_rect,
                          const VkClearDepthStencilValue clear_value, uint32_t view_mask)
{
   if (!iview || !iview->support_fast_clear)
      return false;

   if (!radv_layout_is_htile_compressed(
          cmd_buffer->device, iview->image, image_layout, in_render_loop,
          radv_image_queue_family_mask(iview->image, cmd_buffer->queue_family_index,
                                       cmd_buffer->queue_family_index)))
      return false;

   if (clear_rect->rect.offset.x || clear_rect->rect.offset.y ||
       clear_rect->rect.extent.width != iview->image->info.width ||
       clear_rect->rect.extent.height != iview->image->info.height)
      return false;

   if (view_mask && (iview->image->info.array_size >= 32 ||
                     (1u << iview->image->info.array_size) - 1u != view_mask))
      return false;
   if (!view_mask && clear_rect->baseArrayLayer != 0)
      return false;
   if (!view_mask && clear_rect->layerCount != iview->image->info.array_size)
      return false;

   if (cmd_buffer->device->vk.enabled_extensions.EXT_depth_range_unrestricted &&
       (aspects & VK_IMAGE_ASPECT_DEPTH_BIT) &&
       (clear_value.depth < 0.0 || clear_value.depth > 1.0))
      return false;

   if (radv_image_is_tc_compat_htile(iview->image) &&
       (((aspects & VK_IMAGE_ASPECT_DEPTH_BIT) && !radv_is_fast_clear_depth_allowed(clear_value)) ||
        ((aspects & VK_IMAGE_ASPECT_STENCIL_BIT) &&
         !radv_is_fast_clear_stencil_allowed(clear_value))))
      return false;

   if (iview->image->info.levels > 1) {
      uint32_t last_level = iview->base_mip + iview->level_count - 1;
      if (last_level >= iview->image->planes[0].surface.num_meta_levels) {
         /* Do not fast clears if one level can't be fast cleared. */
         return false;
      }
   }

   return true;
}

static void
radv_fast_clear_depth(struct radv_cmd_buffer *cmd_buffer, const struct radv_image_view *iview,
                      const VkClearAttachment *clear_att, enum radv_cmd_flush_bits *pre_flush,
                      enum radv_cmd_flush_bits *post_flush)
{
   VkClearDepthStencilValue clear_value = clear_att->clearValue.depthStencil;
   VkImageAspectFlags aspects = clear_att->aspectMask;
   uint32_t clear_word, flush_bits;

   clear_word = radv_get_htile_fast_clear_value(cmd_buffer->device, iview->image, clear_value);

   if (pre_flush) {
      enum radv_cmd_flush_bits bits =
         radv_src_access_flush(cmd_buffer, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                               iview->image) |
         radv_dst_access_flush(cmd_buffer, VK_ACCESS_SHADER_WRITE_BIT |
                                           VK_ACCESS_SHADER_READ_BIT, iview->image);
      cmd_buffer->state.flush_bits |= bits & ~*pre_flush;
      *pre_flush |= cmd_buffer->state.flush_bits;
   }

   VkImageSubresourceRange range = {
      .aspectMask = aspects,
      .baseMipLevel = iview->base_mip,
      .levelCount = iview->level_count,
      .baseArrayLayer = iview->base_layer,
      .layerCount = iview->layer_count,
   };

   flush_bits = radv_clear_htile(cmd_buffer, iview->image, &range, clear_word);

   if (iview->image->planes[0].surface.has_stencil &&
       !(aspects == (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT))) {
      /* Synchronize after performing a depth-only or a stencil-only
       * fast clear because the driver uses an optimized path which
       * performs a read-modify-write operation, and the two separate
       * aspects might use the same HTILE memory.
       */
      cmd_buffer->state.flush_bits |= flush_bits;
   }

   radv_update_ds_clear_metadata(cmd_buffer, iview, clear_value, aspects);
   if (post_flush) {
      *post_flush |= flush_bits;
   }
}

static nir_shader *
build_clear_htile_mask_shader()
{
   nir_builder b =
      nir_builder_init_simple_shader(MESA_SHADER_COMPUTE, NULL, "meta_clear_htile_mask");
   b.shader->info.workgroup_size[0] = 64;
   b.shader->info.workgroup_size[1] = 1;
   b.shader->info.workgroup_size[2] = 1;

   nir_ssa_def *global_id = get_global_ids(&b, 1);

   nir_ssa_def *offset = nir_imul(&b, global_id, nir_imm_int(&b, 16));
   offset = nir_channel(&b, offset, 0);

   nir_ssa_def *buf = radv_meta_load_descriptor(&b, 0, 0);

   nir_ssa_def *constants = nir_load_push_constant(&b, 2, 32, nir_imm_int(&b, 0), .range = 8);

   nir_ssa_def *load = nir_load_ssbo(&b, 4, 32, buf, offset, .align_mul = 16);

   /* data = (data & ~htile_mask) | (htile_value & htile_mask) */
   nir_ssa_def *data = nir_iand(&b, load, nir_channel(&b, constants, 1));
   data = nir_ior(&b, data, nir_channel(&b, constants, 0));

   nir_store_ssbo(&b, data, buf, offset, .write_mask = 0xf, .access = ACCESS_NON_READABLE,
                  .align_mul = 16);

   return b.shader;
}

static VkResult
init_meta_clear_htile_mask_state(struct radv_device *device)
{
   struct radv_meta_state *state = &device->meta_state;
   VkResult result;
   nir_shader *cs = build_clear_htile_mask_shader();

   VkDescriptorSetLayoutCreateInfo ds_layout_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR,
      .bindingCount = 1,
      .pBindings = (VkDescriptorSetLayoutBinding[]){
         {.binding = 0,
          .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .descriptorCount = 1,
          .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
          .pImmutableSamplers = NULL},
      }};

   result = radv_CreateDescriptorSetLayout(radv_device_to_handle(device), &ds_layout_info,
                                           &state->alloc, &state->clear_htile_mask_ds_layout);
   if (result != VK_SUCCESS)
      goto fail;

   VkPipelineLayoutCreateInfo p_layout_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &state->clear_htile_mask_ds_layout,
      .pushConstantRangeCount = 1,
      .pPushConstantRanges =
         &(VkPushConstantRange){
            VK_SHADER_STAGE_COMPUTE_BIT,
            0,
            8,
         },
   };

   result = radv_CreatePipelineLayout(radv_device_to_handle(device), &p_layout_info, &state->alloc,
                                      &state->clear_htile_mask_p_layout);
   if (result != VK_SUCCESS)
      goto fail;

   VkPipelineShaderStageCreateInfo shader_stage = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_COMPUTE_BIT,
      .module = vk_shader_module_handle_from_nir(cs),
      .pName = "main",
      .pSpecializationInfo = NULL,
   };

   VkComputePipelineCreateInfo pipeline_info = {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .stage = shader_stage,
      .flags = 0,
      .layout = state->clear_htile_mask_p_layout,
   };

   result = radv_CreateComputePipelines(radv_device_to_handle(device),
                                        radv_pipeline_cache_to_handle(&state->cache), 1,
                                        &pipeline_info, NULL, &state->clear_htile_mask_pipeline);

   ralloc_free(cs);
   return result;
fail:
   ralloc_free(cs);
   return result;
}

/* Clear DCC using comp-to-single by storing the clear value at the beginning of every 256B block.
 * For MSAA images, clearing the first sample should be enough as long as CMASK is also cleared.
 */
static nir_shader *
build_clear_dcc_comp_to_single_shader(bool is_msaa)
{
   enum glsl_sampler_dim dim = is_msaa ? GLSL_SAMPLER_DIM_MS : GLSL_SAMPLER_DIM_2D;
   const struct glsl_type *img_type = glsl_image_type(dim, true, GLSL_TYPE_FLOAT);

   nir_builder b =
      nir_builder_init_simple_shader(MESA_SHADER_COMPUTE, NULL, "meta_clear_dcc_comp_to_single-%s",
                                     is_msaa ? "multisampled" : "singlesampled");
   b.shader->info.workgroup_size[0] = 8;
   b.shader->info.workgroup_size[1] = 8;
   b.shader->info.workgroup_size[2] = 1;

   nir_ssa_def *global_id = get_global_ids(&b, 3);

   /* Load the dimensions in pixels of a block that gets compressed to one DCC byte. */
   nir_ssa_def *dcc_block_size = nir_load_push_constant(&b, 2, 32, nir_imm_int(&b, 0), .range = 8);

   /* Compute the coordinates. */
   nir_ssa_def *coord = nir_channels(&b, global_id, 0x3);
   coord = nir_imul(&b, coord, dcc_block_size);
   coord = nir_vec4(&b, nir_channel(&b, coord, 0),
                        nir_channel(&b, coord, 1),
                        nir_channel(&b, global_id, 2),
                        nir_ssa_undef(&b, 1, 32));

   nir_variable *output_img = nir_variable_create(b.shader, nir_var_uniform, img_type, "out_img");
   output_img->data.descriptor_set = 0;
   output_img->data.binding = 0;

   /* Load the clear color values. */
   nir_ssa_def *clear_values = nir_load_push_constant(&b, 2, 32, nir_imm_int(&b, 8), .range = 8);

   nir_ssa_def *data = nir_vec4(&b, nir_channel(&b, clear_values, 0),
                                    nir_channel(&b, clear_values, 1),
                                    nir_channel(&b, clear_values, 1),
                                    nir_channel(&b, clear_values, 1));

   /* Store the clear color values. */
   nir_ssa_def *sample_id = is_msaa ? nir_imm_int(&b, 0) : nir_ssa_undef(&b, 1, 32);
   nir_image_deref_store(&b, &nir_build_deref_var(&b, output_img)->dest.ssa, coord,
                         sample_id, data, nir_imm_int(&b, 0),
                         .image_dim = dim, .image_array = true);

   return b.shader;
}

static VkResult
create_dcc_comp_to_single_pipeline(struct radv_device *device, bool is_msaa, VkPipeline *pipeline)
{
   struct radv_meta_state *state = &device->meta_state;
   VkResult result;
   nir_shader *cs = build_clear_dcc_comp_to_single_shader(is_msaa);

   VkPipelineShaderStageCreateInfo shader_stage = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_COMPUTE_BIT,
      .module = vk_shader_module_handle_from_nir(cs),
      .pName = "main",
      .pSpecializationInfo = NULL,
   };

   VkComputePipelineCreateInfo pipeline_info = {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .stage = shader_stage,
      .flags = 0,
      .layout = state->clear_dcc_comp_to_single_p_layout,
   };

   result = radv_CreateComputePipelines(radv_device_to_handle(device),
                                        radv_pipeline_cache_to_handle(&state->cache), 1,
                                        &pipeline_info, NULL, pipeline);

   ralloc_free(cs);
   return result;
}

static VkResult
init_meta_clear_dcc_comp_to_single_state(struct radv_device *device)
{
   struct radv_meta_state *state = &device->meta_state;
   VkResult result;

   VkDescriptorSetLayoutCreateInfo ds_layout_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR,
      .bindingCount = 1,
      .pBindings = (VkDescriptorSetLayoutBinding[]){
         {.binding = 0,
          .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
          .descriptorCount = 1,
          .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
          .pImmutableSamplers = NULL},
      }};

   result = radv_CreateDescriptorSetLayout(radv_device_to_handle(device), &ds_layout_info,
                                           &state->alloc, &state->clear_dcc_comp_to_single_ds_layout);
   if (result != VK_SUCCESS)
      goto fail;

   VkPipelineLayoutCreateInfo p_layout_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &state->clear_dcc_comp_to_single_ds_layout,
      .pushConstantRangeCount = 1,
      .pPushConstantRanges =
         &(VkPushConstantRange){
            VK_SHADER_STAGE_COMPUTE_BIT,
            0,
            16,
         },
   };

   result = radv_CreatePipelineLayout(radv_device_to_handle(device), &p_layout_info, &state->alloc,
                                      &state->clear_dcc_comp_to_single_p_layout);
   if (result != VK_SUCCESS)
      goto fail;

   for (uint32_t i = 0; i < 2; i++) {
      result = create_dcc_comp_to_single_pipeline(device, !!i,
                                                  &state->clear_dcc_comp_to_single_pipeline[i]);
      if (result != VK_SUCCESS)
         goto fail;
   }

fail:
   return result;
}

VkResult
radv_device_init_meta_clear_state(struct radv_device *device, bool on_demand)
{
   VkResult res;
   struct radv_meta_state *state = &device->meta_state;

   VkPipelineLayoutCreateInfo pl_color_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 0,
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = &(VkPushConstantRange){VK_SHADER_STAGE_FRAGMENT_BIT, 0, 16},
   };

   res = radv_CreatePipelineLayout(radv_device_to_handle(device), &pl_color_create_info,
                                   &device->meta_state.alloc,
                                   &device->meta_state.clear_color_p_layout);
   if (res != VK_SUCCESS)
      goto fail;

   VkPipelineLayoutCreateInfo pl_depth_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 0,
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = &(VkPushConstantRange){VK_SHADER_STAGE_VERTEX_BIT, 0, 4},
   };

   res = radv_CreatePipelineLayout(radv_device_to_handle(device), &pl_depth_create_info,
                                   &device->meta_state.alloc,
                                   &device->meta_state.clear_depth_p_layout);
   if (res != VK_SUCCESS)
      goto fail;

   VkPipelineLayoutCreateInfo pl_depth_unrestricted_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 0,
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = &(VkPushConstantRange){VK_SHADER_STAGE_FRAGMENT_BIT, 0, 4},
   };

   res = radv_CreatePipelineLayout(radv_device_to_handle(device),
                                   &pl_depth_unrestricted_create_info, &device->meta_state.alloc,
                                   &device->meta_state.clear_depth_unrestricted_p_layout);
   if (res != VK_SUCCESS)
      goto fail;

   res = init_meta_clear_htile_mask_state(device);
   if (res != VK_SUCCESS)
      goto fail;

   res = init_meta_clear_dcc_comp_to_single_state(device);
   if (res != VK_SUCCESS)
      goto fail;

   if (on_demand)
      return VK_SUCCESS;

   for (uint32_t i = 0; i < ARRAY_SIZE(state->clear); ++i) {
      uint32_t samples = 1 << i;
      for (uint32_t j = 0; j < NUM_META_FS_KEYS; ++j) {
         VkFormat format = radv_fs_key_format_exemplars[j];
         unsigned fs_key = radv_format_meta_fs_key(device, format);
         assert(!state->clear[i].color_pipelines[fs_key]);

         res =
            create_color_renderpass(device, format, samples, &state->clear[i].render_pass[fs_key]);
         if (res != VK_SUCCESS)
            goto fail;

         res = create_color_pipeline(device, samples, 0, &state->clear[i].color_pipelines[fs_key],
                                     state->clear[i].render_pass[fs_key]);
         if (res != VK_SUCCESS)
            goto fail;
      }

      res = create_depthstencil_renderpass(device, samples, &state->clear[i].depthstencil_rp);
      if (res != VK_SUCCESS)
         goto fail;

      for (uint32_t j = 0; j < NUM_DEPTH_CLEAR_PIPELINES; j++) {
         res = create_depthstencil_pipeline(device, VK_IMAGE_ASPECT_DEPTH_BIT, samples, j, false,
                                            &state->clear[i].depth_only_pipeline[j],
                                            state->clear[i].depthstencil_rp);
         if (res != VK_SUCCESS)
            goto fail;

         res = create_depthstencil_pipeline(device, VK_IMAGE_ASPECT_STENCIL_BIT, samples, j, false,
                                            &state->clear[i].stencil_only_pipeline[j],
                                            state->clear[i].depthstencil_rp);
         if (res != VK_SUCCESS)
            goto fail;

         res = create_depthstencil_pipeline(
            device, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, samples, j, false,
            &state->clear[i].depthstencil_pipeline[j], state->clear[i].depthstencil_rp);
         if (res != VK_SUCCESS)
            goto fail;

         res = create_depthstencil_pipeline(device, VK_IMAGE_ASPECT_DEPTH_BIT, samples, j, true,
                                            &state->clear[i].depth_only_unrestricted_pipeline[j],
                                            state->clear[i].depthstencil_rp);
         if (res != VK_SUCCESS)
            goto fail;

         res = create_depthstencil_pipeline(device, VK_IMAGE_ASPECT_STENCIL_BIT, samples, j, true,
                                            &state->clear[i].stencil_only_unrestricted_pipeline[j],
                                            state->clear[i].depthstencil_rp);
         if (res != VK_SUCCESS)
            goto fail;

         res = create_depthstencil_pipeline(
            device, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, samples, j, true,
            &state->clear[i].depthstencil_unrestricted_pipeline[j],
            state->clear[i].depthstencil_rp);
         if (res != VK_SUCCESS)
            goto fail;
      }
   }
   return VK_SUCCESS;

fail:
   radv_device_finish_meta_clear_state(device);
   return res;
}

static uint32_t
radv_get_cmask_fast_clear_value(const struct radv_image *image)
{
   uint32_t value = 0; /* Default value when no DCC. */

   /* The fast-clear value is different for images that have both DCC and
    * CMASK metadata.
    */
   if (radv_image_has_dcc(image)) {
      /* DCC fast clear with MSAA should clear CMASK to 0xC. */
      return image->info.samples > 1 ? 0xcccccccc : 0xffffffff;
   }

   return value;
}

uint32_t
radv_clear_cmask(struct radv_cmd_buffer *cmd_buffer, struct radv_image *image,
                 const VkImageSubresourceRange *range, uint32_t value)
{
   uint64_t offset = image->offset + image->planes[0].surface.cmask_offset;
   uint64_t size;

   if (cmd_buffer->device->physical_device->rad_info.chip_class == GFX9) {
      /* TODO: clear layers. */
      size = image->planes[0].surface.cmask_size;
   } else {
      unsigned slice_size = image->planes[0].surface.cmask_slice_size;

      offset += slice_size * range->baseArrayLayer;
      size = slice_size * radv_get_layerCount(image, range);
   }

   return radv_fill_buffer(cmd_buffer, image, image->bo, offset, size, value);
}

uint32_t
radv_clear_fmask(struct radv_cmd_buffer *cmd_buffer, struct radv_image *image,
                 const VkImageSubresourceRange *range, uint32_t value)
{
   uint64_t offset = image->offset + image->planes[0].surface.fmask_offset;
   unsigned slice_size = image->planes[0].surface.fmask_slice_size;
   uint64_t size;

   /* MSAA images do not support mipmap levels. */
   assert(range->baseMipLevel == 0 && radv_get_levelCount(image, range) == 1);

   offset += slice_size * range->baseArrayLayer;
   size = slice_size * radv_get_layerCount(image, range);

   return radv_fill_buffer(cmd_buffer, image, image->bo, offset, size, value);
}

uint32_t
radv_clear_dcc(struct radv_cmd_buffer *cmd_buffer, struct radv_image *image,
               const VkImageSubresourceRange *range, uint32_t value)
{
   uint32_t level_count = radv_get_levelCount(image, range);
   uint32_t layer_count = radv_get_layerCount(image, range);
   uint32_t flush_bits = 0;

   /* Mark the image as being compressed. */
   radv_update_dcc_metadata(cmd_buffer, image, range, true);

   for (uint32_t l = 0; l < level_count; l++) {
      uint64_t offset = image->offset + image->planes[0].surface.meta_offset;
      uint32_t level = range->baseMipLevel + l;
      uint64_t size;

      if (cmd_buffer->device->physical_device->rad_info.chip_class >= GFX10) {
         /* DCC for mipmaps+layers is currently disabled. */
         offset += image->planes[0].surface.meta_slice_size * range->baseArrayLayer +
                   image->planes[0].surface.u.gfx9.meta_levels[level].offset;
         size = image->planes[0].surface.u.gfx9.meta_levels[level].size * layer_count;
      } else if (cmd_buffer->device->physical_device->rad_info.chip_class == GFX9) {
         /* Mipmap levels and layers aren't implemented. */
         assert(level == 0);
         size = image->planes[0].surface.meta_size;
      } else {
         const struct legacy_surf_dcc_level *dcc_level =
            &image->planes[0].surface.u.legacy.color.dcc_level[level];

         /* If dcc_fast_clear_size is 0 (which might happens for
          * mipmaps) the fill buffer operation below is a no-op.
          * This can only happen during initialization as the
          * fast clear path fallbacks to slow clears if one
          * level can't be fast cleared.
          */
         offset +=
            dcc_level->dcc_offset + dcc_level->dcc_slice_fast_clear_size * range->baseArrayLayer;
         size = dcc_level->dcc_slice_fast_clear_size * radv_get_layerCount(image, range);
      }

      /* Do not clear this level if it can't be compressed. */
      if (!size)
         continue;

      flush_bits |= radv_fill_buffer(cmd_buffer, image, image->bo, offset, size, value);
   }

   return flush_bits;
}

static uint32_t
radv_clear_dcc_comp_to_single(struct radv_cmd_buffer *cmd_buffer,
                              struct radv_image *image,
                              const VkImageSubresourceRange *range,
                              uint32_t color_values[2])
{
   struct radv_device *device = cmd_buffer->device;
   unsigned bytes_per_pixel = vk_format_get_blocksize(image->vk_format);
   unsigned layer_count = radv_get_layerCount(image, range);
   struct radv_meta_saved_state saved_state;
   bool is_msaa = image->info.samples > 1;
   struct radv_image_view iview;
   VkFormat format;

   switch (bytes_per_pixel) {
   case 1:
      format = VK_FORMAT_R8_UINT;
      break;
   case 2:
      format = VK_FORMAT_R16_UINT;
      break;
   case 4:
      format = VK_FORMAT_R32_UINT;
      break;
   case 8:
      format = VK_FORMAT_R32G32_UINT;
      break;
   case 16:
      format = VK_FORMAT_R32G32B32A32_UINT;
      break;
   default:
      unreachable("Unsupported number of bytes per pixel");
   }

   radv_meta_save(
      &saved_state, cmd_buffer,
      RADV_META_SAVE_DESCRIPTORS | RADV_META_SAVE_COMPUTE_PIPELINE | RADV_META_SAVE_CONSTANTS);

   VkPipeline pipeline = device->meta_state.clear_dcc_comp_to_single_pipeline[is_msaa];

   radv_CmdBindPipeline(radv_cmd_buffer_to_handle(cmd_buffer), VK_PIPELINE_BIND_POINT_COMPUTE,
                        pipeline);

   for (uint32_t l = 0; l < radv_get_levelCount(image, range); l++) {
      uint32_t width, height;

      /* Do not write the clear color value for levels without DCC. */
      if (!radv_dcc_enabled(image, range->baseMipLevel + l))
         continue;

      width = radv_minify(image->info.width, range->baseMipLevel + l);
      height = radv_minify(image->info.height, range->baseMipLevel + l);

      radv_image_view_init(
         &iview, cmd_buffer->device,
         &(VkImageViewCreateInfo){
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = radv_image_to_handle(image),
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format,
            .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                 .baseMipLevel = range->baseMipLevel + l,
                                 .levelCount = 1,
                                 .baseArrayLayer = range->baseArrayLayer,
                                 .layerCount = layer_count},
         },
         &(struct radv_image_view_extra_create_info){.disable_compression = true});

      radv_meta_push_descriptor_set(
         cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
         device->meta_state.clear_dcc_comp_to_single_p_layout, 0,
         1,
         (VkWriteDescriptorSet[]){{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                   .dstBinding = 0,
                                   .dstArrayElement = 0,
                                   .descriptorCount = 1,
                                   .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                                   .pImageInfo =
                                      (VkDescriptorImageInfo[]){
                                         {
                                            .sampler = VK_NULL_HANDLE,
                                            .imageView = radv_image_view_to_handle(&iview),
                                            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
                                         },
                                      }}});

      unsigned dcc_width =
         DIV_ROUND_UP(width, image->planes[0].surface.u.gfx9.color.dcc_block_width);
      unsigned dcc_height =
         DIV_ROUND_UP(height, image->planes[0].surface.u.gfx9.color.dcc_block_height);

      const unsigned constants[4] = {
         image->planes[0].surface.u.gfx9.color.dcc_block_width,
         image->planes[0].surface.u.gfx9.color.dcc_block_height,
         color_values[0],
         color_values[1],
      };

      radv_CmdPushConstants(radv_cmd_buffer_to_handle(cmd_buffer),
                            device->meta_state.clear_dcc_comp_to_single_p_layout,
                            VK_SHADER_STAGE_COMPUTE_BIT, 0, 16, constants);

      radv_unaligned_dispatch(cmd_buffer, dcc_width, dcc_height, layer_count);

      radv_image_view_finish(&iview);
   }

   radv_meta_restore(&saved_state, cmd_buffer);

   return RADV_CMD_FLAG_CS_PARTIAL_FLUSH |
          radv_src_access_flush(cmd_buffer, VK_ACCESS_SHADER_WRITE_BIT, image);
}

uint32_t
radv_clear_htile(struct radv_cmd_buffer *cmd_buffer, const struct radv_image *image,
                 const VkImageSubresourceRange *range, uint32_t value)
{
   uint32_t level_count = radv_get_levelCount(image, range);
   uint32_t flush_bits = 0;
   uint32_t htile_mask;

   htile_mask = radv_get_htile_mask(cmd_buffer->device, image, range->aspectMask);

   if (level_count != image->info.levels) {
      assert(cmd_buffer->device->physical_device->rad_info.chip_class >= GFX10);

      /* Clear individuals levels separately. */
      for (uint32_t l = 0; l < level_count; l++) {
         uint32_t level = range->baseMipLevel + l;
         uint64_t offset = image->offset + image->planes[0].surface.meta_offset +
                           image->planes[0].surface.u.gfx9.meta_levels[level].offset;
         uint32_t size = image->planes[0].surface.u.gfx9.meta_levels[level].size;

         /* Do not clear this level if it can be compressed. */
         if (!size)
            continue;

         if (htile_mask == UINT_MAX) {
            /* Clear the whole HTILE buffer. */
            flush_bits |= radv_fill_buffer(cmd_buffer, image, image->bo, offset, size, value);
         } else {
            /* Only clear depth or stencil bytes in the HTILE buffer. */
            flush_bits |=
               clear_htile_mask(cmd_buffer, image, image->bo, offset, size, value, htile_mask);
         }
      }
   } else {
      unsigned layer_count = radv_get_layerCount(image, range);
      uint64_t size = image->planes[0].surface.meta_slice_size * layer_count;
      uint64_t offset = image->offset + image->planes[0].surface.meta_offset +
                        image->planes[0].surface.meta_slice_size * range->baseArrayLayer;

      if (htile_mask == UINT_MAX) {
         /* Clear the whole HTILE buffer. */
         flush_bits = radv_fill_buffer(cmd_buffer, image, image->bo, offset, size, value);
      } else {
         /* Only clear depth or stencil bytes in the HTILE buffer. */
         flush_bits =
            clear_htile_mask(cmd_buffer, image, image->bo, offset, size, value, htile_mask);
      }
   }

   return flush_bits;
}

enum {
   RADV_DCC_CLEAR_0000 = 0x00000000U,
   RADV_DCC_CLEAR_0001 = 0x40404040U,
   RADV_DCC_CLEAR_1110 = 0x80808080U,
   RADV_DCC_CLEAR_1111 = 0xC0C0C0C0U,
   RADV_DCC_CLEAR_REG = 0x20202020U,
   RADV_DCC_CLEAR_SINGLE = 0x10101010U,
};

static void
vi_get_fast_clear_parameters(struct radv_device *device, const struct radv_image_view *iview,
                             const VkClearColorValue *clear_value,
                             uint32_t *reset_value, bool *can_avoid_fast_clear_elim)
{
   bool values[4] = {0};
   int extra_channel;
   bool main_value = false;
   bool extra_value = false;
   bool has_color = false;
   bool has_alpha = false;

   /* comp-to-single allows to perform DCC fast clears without requiring a FCE. */
   if (iview->image->support_comp_to_single) {
      *reset_value = RADV_DCC_CLEAR_SINGLE;
      *can_avoid_fast_clear_elim = true;
   } else {
      *reset_value = RADV_DCC_CLEAR_REG;
      *can_avoid_fast_clear_elim = false;
   }

   const struct util_format_description *desc = vk_format_description(iview->vk_format);
   if (iview->vk_format == VK_FORMAT_B10G11R11_UFLOAT_PACK32 ||
       iview->vk_format == VK_FORMAT_R5G6B5_UNORM_PACK16 || iview->vk_format == VK_FORMAT_B5G6R5_UNORM_PACK16)
      extra_channel = -1;
   else if (desc->layout == UTIL_FORMAT_LAYOUT_PLAIN) {
      if (vi_alpha_is_on_msb(device, iview->vk_format))
         extra_channel = desc->nr_channels - 1;
      else
         extra_channel = 0;
   } else
      return;

   for (int i = 0; i < 4; i++) {
      int index = desc->swizzle[i] - PIPE_SWIZZLE_X;
      if (desc->swizzle[i] < PIPE_SWIZZLE_X || desc->swizzle[i] > PIPE_SWIZZLE_W)
         continue;

      if (desc->channel[i].pure_integer && desc->channel[i].type == UTIL_FORMAT_TYPE_SIGNED) {
         /* Use the maximum value for clamping the clear color. */
         int max = u_bit_consecutive(0, desc->channel[i].size - 1);

         values[i] = clear_value->int32[i] != 0;
         if (clear_value->int32[i] != 0 && MIN2(clear_value->int32[i], max) != max)
            return;
      } else if (desc->channel[i].pure_integer &&
                 desc->channel[i].type == UTIL_FORMAT_TYPE_UNSIGNED) {
         /* Use the maximum value for clamping the clear color. */
         unsigned max = u_bit_consecutive(0, desc->channel[i].size);

         values[i] = clear_value->uint32[i] != 0U;
         if (clear_value->uint32[i] != 0U && MIN2(clear_value->uint32[i], max) != max)
            return;
      } else {
         values[i] = clear_value->float32[i] != 0.0F;
         if (clear_value->float32[i] != 0.0F && clear_value->float32[i] != 1.0F)
            return;
      }

      if (index == extra_channel) {
         extra_value = values[i];
         has_alpha = true;
      } else {
         main_value = values[i];
         has_color = true;
      }
   }

   /* If alpha isn't present, make it the same as color, and vice versa. */
   if (!has_alpha)
      extra_value = main_value;
   else if (!has_color)
      main_value = extra_value;

   for (int i = 0; i < 4; ++i)
      if (values[i] != main_value && desc->swizzle[i] - PIPE_SWIZZLE_X != extra_channel &&
          desc->swizzle[i] >= PIPE_SWIZZLE_X && desc->swizzle[i] <= PIPE_SWIZZLE_W)
         return;

   /* Only DCC clear code 0000 is allowed for signed<->unsigned formats. */
   if ((main_value || extra_value) && iview->image->dcc_sign_reinterpret)
      return;

   *can_avoid_fast_clear_elim = true;

   if (main_value) {
      if (extra_value)
         *reset_value = RADV_DCC_CLEAR_1111;
      else
         *reset_value = RADV_DCC_CLEAR_1110;
   } else {
      if (extra_value)
         *reset_value = RADV_DCC_CLEAR_0001;
      else
         *reset_value = RADV_DCC_CLEAR_0000;
   }
}

static bool
radv_can_fast_clear_color(struct radv_cmd_buffer *cmd_buffer, const struct radv_image_view *iview,
                          VkImageLayout image_layout, bool in_render_loop,
                          const VkClearRect *clear_rect, VkClearColorValue clear_value,
                          uint32_t view_mask)
{
   uint32_t clear_color[2];

   if (!iview || !iview->support_fast_clear)
      return false;

   if (!radv_layout_can_fast_clear(
          cmd_buffer->device, iview->image, iview->base_mip, image_layout, in_render_loop,
          radv_image_queue_family_mask(iview->image, cmd_buffer->queue_family_index,
                                       cmd_buffer->queue_family_index)))
      return false;

   if (clear_rect->rect.offset.x || clear_rect->rect.offset.y ||
       clear_rect->rect.extent.width != iview->image->info.width ||
       clear_rect->rect.extent.height != iview->image->info.height)
      return false;

   if (view_mask && (iview->image->info.array_size >= 32 ||
                     (1u << iview->image->info.array_size) - 1u != view_mask))
      return false;
   if (!view_mask && clear_rect->baseArrayLayer != 0)
      return false;
   if (!view_mask && clear_rect->layerCount != iview->image->info.array_size)
      return false;

   /* DCC */
   if (!radv_format_pack_clear_color(iview->vk_format, clear_color, &clear_value))
      return false;

   if (!radv_image_has_clear_value(iview->image) && (clear_color[0] != 0 || clear_color[1] != 0))
      return false;

   if (radv_dcc_enabled(iview->image, iview->base_mip)) {
      bool can_avoid_fast_clear_elim;
      uint32_t reset_value;

      vi_get_fast_clear_parameters(cmd_buffer->device, iview, &clear_value, &reset_value,
                                   &can_avoid_fast_clear_elim);

      if (iview->image->info.levels > 1) {
         if (cmd_buffer->device->physical_device->rad_info.chip_class >= GFX9) {
            uint32_t last_level = iview->base_mip + iview->level_count - 1;
            if (last_level >= iview->image->planes[0].surface.num_meta_levels) {
               /* Do not fast clears if one level can't be fast cleard. */
               return false;
            }
         } else {
            for (uint32_t l = 0; l < iview->level_count; l++) {
               uint32_t level = iview->base_mip + l;
               struct legacy_surf_dcc_level *dcc_level =
                  &iview->image->planes[0].surface.u.legacy.color.dcc_level[level];

               /* Do not fast clears if one level can't be
                * fast cleared.
                */
               if (!dcc_level->dcc_fast_clear_size)
                  return false;
            }
         }
      }
   }

   return true;
}

static void
radv_fast_clear_color(struct radv_cmd_buffer *cmd_buffer, const struct radv_image_view *iview,
                      const VkClearAttachment *clear_att, uint32_t subpass_att,
                      enum radv_cmd_flush_bits *pre_flush, enum radv_cmd_flush_bits *post_flush)
{
   VkClearColorValue clear_value = clear_att->clearValue.color;
   uint32_t clear_color[2], flush_bits = 0;
   uint32_t cmask_clear_value;
   VkImageSubresourceRange range = {
      .aspectMask = iview->aspect_mask,
      .baseMipLevel = iview->base_mip,
      .levelCount = iview->level_count,
      .baseArrayLayer = iview->base_layer,
      .layerCount = iview->layer_count,
   };

   if (pre_flush) {
      enum radv_cmd_flush_bits bits =
         radv_src_access_flush(cmd_buffer, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, iview->image) |
         radv_dst_access_flush(cmd_buffer, VK_ACCESS_SHADER_WRITE_BIT, iview->image);
      cmd_buffer->state.flush_bits |= bits & ~*pre_flush;
      *pre_flush |= cmd_buffer->state.flush_bits;
   }

   /* DCC */
   radv_format_pack_clear_color(iview->vk_format, clear_color, &clear_value);

   cmask_clear_value = radv_get_cmask_fast_clear_value(iview->image);

   /* clear cmask buffer */
   bool need_decompress_pass = false;
   if (radv_dcc_enabled(iview->image, iview->base_mip)) {
      uint32_t reset_value;
      bool can_avoid_fast_clear_elim;

      vi_get_fast_clear_parameters(cmd_buffer->device, iview, &clear_value, &reset_value,
                                   &can_avoid_fast_clear_elim);

      if (radv_image_has_cmask(iview->image)) {
         flush_bits = radv_clear_cmask(cmd_buffer, iview->image, &range, cmask_clear_value);
      }

      if (!can_avoid_fast_clear_elim)
         need_decompress_pass = true;

      flush_bits |= radv_clear_dcc(cmd_buffer, iview->image, &range, reset_value);

      if (reset_value == RADV_DCC_CLEAR_SINGLE) {
         /* Write the clear color to the first byte of each 256B block when the image supports DCC
          * fast clears with comp-to-single.
          */
         flush_bits |= radv_clear_dcc_comp_to_single(cmd_buffer, iview->image, &range, clear_color);
      }
   } else {
      flush_bits = radv_clear_cmask(cmd_buffer, iview->image, &range, cmask_clear_value);

      /* Fast clearing with CMASK should always be eliminated. */
      need_decompress_pass = true;
   }

   if (post_flush) {
      *post_flush |= flush_bits;
   }

   /* Update the FCE predicate to perform a fast-clear eliminate. */
   radv_update_fce_metadata(cmd_buffer, iview->image, &range, need_decompress_pass);

   radv_update_color_clear_metadata(cmd_buffer, iview, subpass_att, clear_color);
}

/**
 * The parameters mean that same as those in vkCmdClearAttachments.
 */
static void
emit_clear(struct radv_cmd_buffer *cmd_buffer, const VkClearAttachment *clear_att,
           const VkClearRect *clear_rect, enum radv_cmd_flush_bits *pre_flush,
           enum radv_cmd_flush_bits *post_flush, uint32_t view_mask, bool ds_resolve_clear)
{
   const struct radv_framebuffer *fb = cmd_buffer->state.framebuffer;
   const struct radv_subpass *subpass = cmd_buffer->state.subpass;
   VkImageAspectFlags aspects = clear_att->aspectMask;

   if (aspects & VK_IMAGE_ASPECT_COLOR_BIT) {
      const uint32_t subpass_att = clear_att->colorAttachment;
      assert(subpass_att < subpass->color_count);
      const uint32_t pass_att = subpass->color_attachments[subpass_att].attachment;
      if (pass_att == VK_ATTACHMENT_UNUSED)
         return;

      VkImageLayout image_layout = subpass->color_attachments[subpass_att].layout;
      bool in_render_loop = subpass->color_attachments[subpass_att].in_render_loop;
      const struct radv_image_view *iview =
         fb ? cmd_buffer->state.attachments[pass_att].iview : NULL;
      VkClearColorValue clear_value = clear_att->clearValue.color;

      if (radv_can_fast_clear_color(cmd_buffer, iview, image_layout, in_render_loop, clear_rect,
                                    clear_value, view_mask)) {
         radv_fast_clear_color(cmd_buffer, iview, clear_att, subpass_att, pre_flush, post_flush);
      } else {
         emit_color_clear(cmd_buffer, clear_att, clear_rect, view_mask);
      }
   } else {
      struct radv_subpass_attachment *ds_att = subpass->depth_stencil_attachment;

      if (ds_resolve_clear)
         ds_att = subpass->ds_resolve_attachment;

      if (!ds_att || ds_att->attachment == VK_ATTACHMENT_UNUSED)
         return;

      VkImageLayout image_layout = ds_att->layout;
      bool in_render_loop = ds_att->in_render_loop;
      const struct radv_image_view *iview =
         fb ? cmd_buffer->state.attachments[ds_att->attachment].iview : NULL;
      VkClearDepthStencilValue clear_value = clear_att->clearValue.depthStencil;

      assert(aspects & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT));

      if (radv_can_fast_clear_depth(cmd_buffer, iview, image_layout, in_render_loop, aspects,
                                    clear_rect, clear_value, view_mask)) {
         radv_fast_clear_depth(cmd_buffer, iview, clear_att, pre_flush, post_flush);
      } else {
         emit_depthstencil_clear(cmd_buffer, clear_att, clear_rect, ds_att, view_mask);
      }
   }
}

static inline bool
radv_attachment_needs_clear(struct radv_cmd_state *cmd_state, uint32_t a)
{
   uint32_t view_mask = cmd_state->subpass->view_mask;
   return (a != VK_ATTACHMENT_UNUSED && cmd_state->attachments[a].pending_clear_aspects &&
           (!view_mask || (view_mask & ~cmd_state->attachments[a].cleared_views)));
}

static bool
radv_subpass_needs_clear(struct radv_cmd_buffer *cmd_buffer)
{
   struct radv_cmd_state *cmd_state = &cmd_buffer->state;
   uint32_t a;

   if (!cmd_state->subpass)
      return false;

   for (uint32_t i = 0; i < cmd_state->subpass->color_count; ++i) {
      a = cmd_state->subpass->color_attachments[i].attachment;
      if (radv_attachment_needs_clear(cmd_state, a))
         return true;
   }

   if (cmd_state->subpass->depth_stencil_attachment) {
      a = cmd_state->subpass->depth_stencil_attachment->attachment;
      if (radv_attachment_needs_clear(cmd_state, a))
         return true;
   }

   if (!cmd_state->subpass->ds_resolve_attachment)
      return false;

   a = cmd_state->subpass->ds_resolve_attachment->attachment;
   return radv_attachment_needs_clear(cmd_state, a);
}

static void
radv_subpass_clear_attachment(struct radv_cmd_buffer *cmd_buffer,
                              struct radv_attachment_state *attachment,
                              const VkClearAttachment *clear_att,
                              enum radv_cmd_flush_bits *pre_flush,
                              enum radv_cmd_flush_bits *post_flush, bool ds_resolve_clear)
{
   struct radv_cmd_state *cmd_state = &cmd_buffer->state;
   uint32_t view_mask = cmd_state->subpass->view_mask;

   VkClearRect clear_rect = {
      .rect = cmd_state->render_area,
      .baseArrayLayer = 0,
      .layerCount = cmd_state->framebuffer->layers,
   };

   radv_describe_begin_render_pass_clear(cmd_buffer, clear_att->aspectMask);

   emit_clear(cmd_buffer, clear_att, &clear_rect, pre_flush, post_flush,
              view_mask & ~attachment->cleared_views, ds_resolve_clear);
   if (view_mask)
      attachment->cleared_views |= view_mask;
   else
      attachment->pending_clear_aspects = 0;

   radv_describe_end_render_pass_clear(cmd_buffer);
}

/**
 * Emit any pending attachment clears for the current subpass.
 *
 * @see radv_attachment_state::pending_clear_aspects
 */
void
radv_cmd_buffer_clear_subpass(struct radv_cmd_buffer *cmd_buffer)
{
   struct radv_cmd_state *cmd_state = &cmd_buffer->state;
   struct radv_meta_saved_state saved_state;
   enum radv_cmd_flush_bits pre_flush = 0;
   enum radv_cmd_flush_bits post_flush = 0;

   if (!radv_subpass_needs_clear(cmd_buffer))
      return;

   radv_meta_save(&saved_state, cmd_buffer,
                  RADV_META_SAVE_GRAPHICS_PIPELINE | RADV_META_SAVE_CONSTANTS);

   for (uint32_t i = 0; i < cmd_state->subpass->color_count; ++i) {
      uint32_t a = cmd_state->subpass->color_attachments[i].attachment;

      if (!radv_attachment_needs_clear(cmd_state, a))
         continue;

      assert(cmd_state->attachments[a].pending_clear_aspects == VK_IMAGE_ASPECT_COLOR_BIT);

      VkClearAttachment clear_att = {
         .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
         .colorAttachment = i, /* Use attachment index relative to subpass */
         .clearValue = cmd_state->attachments[a].clear_value,
      };

      radv_subpass_clear_attachment(cmd_buffer, &cmd_state->attachments[a], &clear_att, &pre_flush,
                                    &post_flush, false);
   }

   if (cmd_state->subpass->depth_stencil_attachment) {
      uint32_t ds = cmd_state->subpass->depth_stencil_attachment->attachment;
      if (radv_attachment_needs_clear(cmd_state, ds)) {
         VkClearAttachment clear_att = {
            .aspectMask = cmd_state->attachments[ds].pending_clear_aspects,
            .clearValue = cmd_state->attachments[ds].clear_value,
         };

         radv_subpass_clear_attachment(cmd_buffer, &cmd_state->attachments[ds], &clear_att,
                                       &pre_flush, &post_flush, false);
      }
   }

   if (cmd_state->subpass->ds_resolve_attachment) {
      uint32_t ds_resolve = cmd_state->subpass->ds_resolve_attachment->attachment;
      if (radv_attachment_needs_clear(cmd_state, ds_resolve)) {
         VkClearAttachment clear_att = {
            .aspectMask = cmd_state->attachments[ds_resolve].pending_clear_aspects,
            .clearValue = cmd_state->attachments[ds_resolve].clear_value,
         };

         radv_subpass_clear_attachment(cmd_buffer, &cmd_state->attachments[ds_resolve], &clear_att,
                                       &pre_flush, &post_flush, true);
      }
   }

   radv_meta_restore(&saved_state, cmd_buffer);
   cmd_buffer->state.flush_bits |= post_flush;
}

static void
radv_clear_image_layer(struct radv_cmd_buffer *cmd_buffer, struct radv_image *image,
                       VkImageLayout image_layout, const VkImageSubresourceRange *range,
                       VkFormat format, int level, unsigned layer_count,
                       const VkClearValue *clear_val)
{
   VkDevice device_h = radv_device_to_handle(cmd_buffer->device);
   struct radv_image_view iview;
   uint32_t width = radv_minify(image->info.width, range->baseMipLevel + level);
   uint32_t height = radv_minify(image->info.height, range->baseMipLevel + level);

   radv_image_view_init(&iview, cmd_buffer->device,
                        &(VkImageViewCreateInfo){
                           .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                           .image = radv_image_to_handle(image),
                           .viewType = radv_meta_get_view_type(image),
                           .format = format,
                           .subresourceRange = {.aspectMask = range->aspectMask,
                                                .baseMipLevel = range->baseMipLevel + level,
                                                .levelCount = 1,
                                                .baseArrayLayer = range->baseArrayLayer,
                                                .layerCount = layer_count},
                        },
                        NULL);

   VkFramebuffer fb;
   radv_CreateFramebuffer(
      device_h,
      &(VkFramebufferCreateInfo){.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                                 .attachmentCount = 1,
                                 .pAttachments =
                                    (VkImageView[]){
                                       radv_image_view_to_handle(&iview),
                                    },
                                 .width = width,
                                 .height = height,
                                 .layers = layer_count},
      &cmd_buffer->pool->alloc, &fb);

   VkAttachmentDescription2 att_desc = {
      .sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
      .format = iview.vk_format,
      .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
      .initialLayout = image_layout,
      .finalLayout = image_layout,
   };

   VkSubpassDescription2 subpass_desc = {
      .sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .inputAttachmentCount = 0,
      .colorAttachmentCount = 0,
      .pColorAttachments = NULL,
      .pResolveAttachments = NULL,
      .pDepthStencilAttachment = NULL,
      .preserveAttachmentCount = 0,
      .pPreserveAttachments = NULL,
   };

   const VkAttachmentReference2 att_ref = {
      .sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
      .attachment = 0,
      .layout = image_layout,
   };

   if (range->aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) {
      subpass_desc.colorAttachmentCount = 1;
      subpass_desc.pColorAttachments = &att_ref;
   } else {
      subpass_desc.pDepthStencilAttachment = &att_ref;
   }

   VkRenderPass pass;
   radv_CreateRenderPass2(
      device_h,
      &(VkRenderPassCreateInfo2){
         .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
         .attachmentCount = 1,
         .pAttachments = &att_desc,
         .subpassCount = 1,
         .pSubpasses = &subpass_desc,
         .dependencyCount = 2,
         .pDependencies =
            (VkSubpassDependency2[]){{.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
                                      .srcSubpass = VK_SUBPASS_EXTERNAL,
                                      .dstSubpass = 0,
                                      .srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                      .dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                      .srcAccessMask = 0,
                                      .dstAccessMask = 0,
                                      .dependencyFlags = 0},
                                     {.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
                                      .srcSubpass = 0,
                                      .dstSubpass = VK_SUBPASS_EXTERNAL,
                                      .srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                      .dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                      .srcAccessMask = 0,
                                      .dstAccessMask = 0,
                                      .dependencyFlags = 0}}},
      &cmd_buffer->pool->alloc, &pass);

   radv_cmd_buffer_begin_render_pass(cmd_buffer,
                                     &(VkRenderPassBeginInfo){
                                        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                                        .renderArea =
                                           {
                                              .offset =
                                                 {
                                                    0,
                                                    0,
                                                 },
                                              .extent =
                                                 {
                                                    .width = width,
                                                    .height = height,
                                                 },
                                           },
                                        .renderPass = pass,
                                        .framebuffer = fb,
                                        .clearValueCount = 0,
                                        .pClearValues = NULL,
                                     },
                                     NULL);

   radv_cmd_buffer_set_subpass(cmd_buffer, &cmd_buffer->state.pass->subpasses[0]);

   VkClearAttachment clear_att = {
      .aspectMask = range->aspectMask,
      .colorAttachment = 0,
      .clearValue = *clear_val,
   };

   VkClearRect clear_rect = {
      .rect =
         {
            .offset = {0, 0},
            .extent = {width, height},
         },
      .baseArrayLayer = 0,
      .layerCount = layer_count,
   };

   emit_clear(cmd_buffer, &clear_att, &clear_rect, NULL, NULL, 0, false);

   radv_image_view_finish(&iview);
   radv_cmd_buffer_end_render_pass(cmd_buffer);
   radv_DestroyRenderPass(device_h, pass, &cmd_buffer->pool->alloc);
   radv_DestroyFramebuffer(device_h, fb, &cmd_buffer->pool->alloc);
}

/**
 * Return TRUE if a fast color or depth clear has been performed.
 */
static bool
radv_fast_clear_range(struct radv_cmd_buffer *cmd_buffer, struct radv_image *image, VkFormat format,
                      VkImageLayout image_layout, bool in_render_loop,
                      const VkImageSubresourceRange *range, const VkClearValue *clear_val)
{
   struct radv_image_view iview;
   bool fast_cleared = false;

   radv_image_view_init(&iview, cmd_buffer->device,
                        &(VkImageViewCreateInfo){
                           .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                           .image = radv_image_to_handle(image),
                           .viewType = radv_meta_get_view_type(image),
                           .format = image->vk_format,
                           .subresourceRange =
                              {
                                 .aspectMask = range->aspectMask,
                                 .baseMipLevel = range->baseMipLevel,
                                 .levelCount = range->levelCount,
                                 .baseArrayLayer = range->baseArrayLayer,
                                 .layerCount = range->layerCount,
                              },
                        },
                        NULL);

   VkClearRect clear_rect = {
      .rect =
         {
            .offset = {0, 0},
            .extent =
               {
                  radv_minify(image->info.width, range->baseMipLevel),
                  radv_minify(image->info.height, range->baseMipLevel),
               },
         },
      .baseArrayLayer = range->baseArrayLayer,
      .layerCount = range->layerCount,
   };

   VkClearAttachment clear_att = {
      .aspectMask = range->aspectMask,
      .colorAttachment = 0,
      .clearValue = *clear_val,
   };

   if (vk_format_is_color(format)) {
      if (radv_can_fast_clear_color(cmd_buffer, &iview, image_layout, in_render_loop, &clear_rect,
                                    clear_att.clearValue.color, 0)) {
         radv_fast_clear_color(cmd_buffer, &iview, &clear_att, clear_att.colorAttachment, NULL,
                               NULL);
         fast_cleared = true;
      }
   } else {
      if (radv_can_fast_clear_depth(cmd_buffer, &iview, image_layout, in_render_loop,
                                    range->aspectMask, &clear_rect,
                                    clear_att.clearValue.depthStencil, 0)) {
         radv_fast_clear_depth(cmd_buffer, &iview, &clear_att, NULL, NULL);
         fast_cleared = true;
      }
   }

   radv_image_view_finish(&iview);
   return fast_cleared;
}

static void
radv_cmd_clear_image(struct radv_cmd_buffer *cmd_buffer, struct radv_image *image,
                     VkImageLayout image_layout, const VkClearValue *clear_value,
                     uint32_t range_count, const VkImageSubresourceRange *ranges, bool cs)
{
   VkFormat format = image->vk_format;
   VkClearValue internal_clear_value;

   if (ranges->aspectMask & VK_IMAGE_ASPECT_COLOR_BIT)
      internal_clear_value.color = clear_value->color;
   else
      internal_clear_value.depthStencil = clear_value->depthStencil;

   bool disable_compression = false;

   if (format == VK_FORMAT_E5B9G9R9_UFLOAT_PACK32) {
      bool blendable;
      if (cs ? !radv_is_storage_image_format_supported(cmd_buffer->device->physical_device, format)
             : !radv_is_colorbuffer_format_supported(cmd_buffer->device->physical_device, format,
                                                     &blendable)) {
         format = VK_FORMAT_R32_UINT;
         internal_clear_value.color.uint32[0] = float3_to_rgb9e5(clear_value->color.float32);

         uint32_t queue_mask = radv_image_queue_family_mask(image, cmd_buffer->queue_family_index,
                                                            cmd_buffer->queue_family_index);

         for (uint32_t r = 0; r < range_count; r++) {
            const VkImageSubresourceRange *range = &ranges[r];

            /* Don't use compressed image stores because they will use an incompatible format. */
            if (radv_layout_dcc_compressed(cmd_buffer->device, image, range->baseMipLevel,
                                           image_layout, false, queue_mask)) {
               disable_compression = cs;
               break;
            }
         }
      }
   }

   if (format == VK_FORMAT_R4G4_UNORM_PACK8) {
      uint8_t r, g;
      format = VK_FORMAT_R8_UINT;
      r = float_to_ubyte(clear_value->color.float32[0]) >> 4;
      g = float_to_ubyte(clear_value->color.float32[1]) >> 4;
      internal_clear_value.color.uint32[0] = (r << 4) | (g & 0xf);
   }

   for (uint32_t r = 0; r < range_count; r++) {
      const VkImageSubresourceRange *range = &ranges[r];

      /* Try to perform a fast clear first, otherwise fallback to
       * the legacy path.
       */
      if (!cs && radv_fast_clear_range(cmd_buffer, image, format, image_layout, false, range,
                                       &internal_clear_value)) {
         continue;
      }

      for (uint32_t l = 0; l < radv_get_levelCount(image, range); ++l) {
         const uint32_t layer_count = image->type == VK_IMAGE_TYPE_3D
                                         ? radv_minify(image->info.depth, range->baseMipLevel + l)
                                         : radv_get_layerCount(image, range);

         if (cs) {
            for (uint32_t s = 0; s < layer_count; ++s) {
               struct radv_meta_blit2d_surf surf;
               surf.format = format;
               surf.image = image;
               surf.level = range->baseMipLevel + l;
               surf.layer = range->baseArrayLayer + s;
               surf.aspect_mask = range->aspectMask;
               surf.disable_compression = disable_compression;
               radv_meta_clear_image_cs(cmd_buffer, &surf, &internal_clear_value.color);
            }
         } else {
            assert(!disable_compression);
            radv_clear_image_layer(cmd_buffer, image, image_layout, range, format, l, layer_count,
                                   &internal_clear_value);
         }
      }
   }

   if (disable_compression) {
      enum radv_cmd_flush_bits flush_bits = 0;
      for (unsigned i = 0; i < range_count; i++) {
         if (radv_dcc_enabled(image, ranges[i].baseMipLevel))
            flush_bits |= radv_clear_dcc(cmd_buffer, image, &ranges[i], 0xffffffffu);
      }
      cmd_buffer->state.flush_bits |= flush_bits;
   }
}

void
radv_CmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image_h, VkImageLayout imageLayout,
                        const VkClearColorValue *pColor, uint32_t rangeCount,
                        const VkImageSubresourceRange *pRanges)
{
   RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
   RADV_FROM_HANDLE(radv_image, image, image_h);
   struct radv_meta_saved_state saved_state;
   bool cs;

   cs = cmd_buffer->queue_family_index == RADV_QUEUE_COMPUTE ||
        !radv_image_is_renderable(cmd_buffer->device, image);

   if (cs) {
      radv_meta_save(
         &saved_state, cmd_buffer,
         RADV_META_SAVE_COMPUTE_PIPELINE | RADV_META_SAVE_CONSTANTS | RADV_META_SAVE_DESCRIPTORS);
   } else {
      radv_meta_save(&saved_state, cmd_buffer,
                     RADV_META_SAVE_GRAPHICS_PIPELINE | RADV_META_SAVE_CONSTANTS);
   }

   radv_cmd_clear_image(cmd_buffer, image, imageLayout, (const VkClearValue *)pColor, rangeCount,
                        pRanges, cs);

   radv_meta_restore(&saved_state, cmd_buffer);
}

void
radv_CmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage image_h,
                               VkImageLayout imageLayout,
                               const VkClearDepthStencilValue *pDepthStencil, uint32_t rangeCount,
                               const VkImageSubresourceRange *pRanges)
{
   RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
   RADV_FROM_HANDLE(radv_image, image, image_h);
   struct radv_meta_saved_state saved_state;

   radv_meta_save(&saved_state, cmd_buffer,
                  RADV_META_SAVE_GRAPHICS_PIPELINE | RADV_META_SAVE_CONSTANTS);

   radv_cmd_clear_image(cmd_buffer, image, imageLayout, (const VkClearValue *)pDepthStencil,
                        rangeCount, pRanges, false);

   radv_meta_restore(&saved_state, cmd_buffer);
}

void
radv_CmdClearAttachments(VkCommandBuffer commandBuffer, uint32_t attachmentCount,
                         const VkClearAttachment *pAttachments, uint32_t rectCount,
                         const VkClearRect *pRects)
{
   RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
   struct radv_meta_saved_state saved_state;
   enum radv_cmd_flush_bits pre_flush = 0;
   enum radv_cmd_flush_bits post_flush = 0;

   if (!cmd_buffer->state.subpass)
      return;

   radv_meta_save(&saved_state, cmd_buffer,
                  RADV_META_SAVE_GRAPHICS_PIPELINE | RADV_META_SAVE_CONSTANTS);

   /* FINISHME: We can do better than this dumb loop. It thrashes too much
    * state.
    */
   for (uint32_t a = 0; a < attachmentCount; ++a) {
      for (uint32_t r = 0; r < rectCount; ++r) {
         emit_clear(cmd_buffer, &pAttachments[a], &pRects[r], &pre_flush, &post_flush,
                    cmd_buffer->state.subpass->view_mask, false);
      }
   }

   radv_meta_restore(&saved_state, cmd_buffer);
   cmd_buffer->state.flush_bits |= post_flush;
}
