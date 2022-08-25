/*
 * Copyright © 2017 Intel Corporation
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

#ifndef VK_SHADER_MODULE_H
#define VK_SHADER_MODULE_H

#include <vulkan/vulkan.h>
#include "vk_object.h"

#ifdef __cplusplus
extern "C" {
#endif

struct nir_shader;

struct vk_shader_module {
   struct vk_object_base base;
   struct nir_shader *nir;
   unsigned char sha1[20];
   uint32_t size;
   char data[0];
};

VK_DEFINE_NONDISP_HANDLE_CASTS(vk_shader_module, base, VkShaderModule,
                               VK_OBJECT_TYPE_SHADER_MODULE)

/* this should only be used for stack-allocated, temporary objects */
#define vk_shader_module_handle_from_nir(_nir) \
   ((VkShaderModule)(uintptr_t)&(struct vk_shader_module) { \
      .base.type = VK_OBJECT_TYPE_SHADER_MODULE, \
      .nir = _nir, \
   })
#define vk_shader_module_from_nir(_nir) \
   (struct vk_shader_module) { \
      .base.type = VK_OBJECT_TYPE_SHADER_MODULE, \
      .nir = _nir, \
   }

#ifdef __cplusplus
}
#endif

#endif /* VK_SHADER_MODULE_H */
