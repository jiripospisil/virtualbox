/*
 * Copyright © 2021 Intel Corporation
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

#include "vk_alloc.h"
#include "vk_command_buffer.h"
#include "vk_common_entrypoints.h"
#include "vk_device.h"
#include "vk_queue.h"
#include "vk_util.h"
#include "../wsi/wsi_common.h"

VKAPI_ATTR void VKAPI_CALL
vk_common_CmdWriteTimestamp(
   VkCommandBuffer                             commandBuffer,
   VkPipelineStageFlagBits                     pipelineStage,
   VkQueryPool                                 queryPool,
   uint32_t                                    query)
{
   VK_FROM_HANDLE(vk_command_buffer, cmd_buffer, commandBuffer);
   struct vk_device *device = cmd_buffer->base.device;

   device->dispatch_table.CmdWriteTimestamp2KHR(commandBuffer,
                                                (VkPipelineStageFlags2KHR) pipelineStage,
                                                queryPool,
                                                query);
}

static VkMemoryBarrier2KHR
upgrade_memory_barrier(const VkMemoryBarrier *barrier,
                       VkPipelineStageFlags2KHR src_stage_mask2,
                       VkPipelineStageFlags2KHR dst_stage_mask2)
{
   return (VkMemoryBarrier2KHR) {
      .sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2_KHR,
      .srcStageMask  = src_stage_mask2,
      .srcAccessMask = (VkAccessFlags2KHR) barrier->srcAccessMask,
      .dstStageMask  = dst_stage_mask2,
      .dstAccessMask = (VkAccessFlags2KHR) barrier->dstAccessMask,
   };
}

static VkBufferMemoryBarrier2KHR
upgrade_buffer_memory_barrier(const VkBufferMemoryBarrier *barrier,
                              VkPipelineStageFlags2KHR src_stage_mask2,
                              VkPipelineStageFlags2KHR dst_stage_mask2)
{
   return (VkBufferMemoryBarrier2KHR) {
      .sType                = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR,
      .srcStageMask         = src_stage_mask2,
      .srcAccessMask        = (VkAccessFlags2KHR) barrier->srcAccessMask,
      .dstStageMask         = dst_stage_mask2,
      .dstAccessMask        = (VkAccessFlags2KHR) barrier->dstAccessMask,
      .srcQueueFamilyIndex  = barrier->srcQueueFamilyIndex,
      .dstQueueFamilyIndex  = barrier->dstQueueFamilyIndex,
      .buffer               = barrier->buffer,
      .offset               = barrier->offset,
      .size                 = barrier->size,
   };
}

static VkImageMemoryBarrier2KHR
upgrade_image_memory_barrier(const VkImageMemoryBarrier *barrier,
                             VkPipelineStageFlags2KHR src_stage_mask2,
                             VkPipelineStageFlags2KHR dst_stage_mask2)
{
   return (VkImageMemoryBarrier2KHR) {
      .sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR,
      .srcStageMask         = src_stage_mask2,
      .srcAccessMask        = (VkAccessFlags2KHR) barrier->srcAccessMask,
      .dstStageMask         = dst_stage_mask2,
      .dstAccessMask        = (VkAccessFlags2KHR) barrier->dstAccessMask,
      .oldLayout            = barrier->oldLayout,
      .newLayout            = barrier->newLayout,
      .srcQueueFamilyIndex  = barrier->srcQueueFamilyIndex,
      .dstQueueFamilyIndex  = barrier->dstQueueFamilyIndex,
      .image                = barrier->image,
      .subresourceRange     = barrier->subresourceRange,
   };
}

VKAPI_ATTR void VKAPI_CALL
vk_common_CmdPipelineBarrier(
    VkCommandBuffer                             commandBuffer,
    VkPipelineStageFlags                        srcStageMask,
    VkPipelineStageFlags                        dstStageMask,
    VkDependencyFlags                           dependencyFlags,
    uint32_t                                    memoryBarrierCount,
    const VkMemoryBarrier*                      pMemoryBarriers,
    uint32_t                                    bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier*                pBufferMemoryBarriers,
    uint32_t                                    imageMemoryBarrierCount,
    const VkImageMemoryBarrier*                 pImageMemoryBarriers)
{
   VK_FROM_HANDLE(vk_command_buffer, cmd_buffer, commandBuffer);
   struct vk_device *device = cmd_buffer->base.device;

   STACK_ARRAY(VkMemoryBarrier2KHR, memory_barriers, memoryBarrierCount);
   STACK_ARRAY(VkBufferMemoryBarrier2KHR, buffer_barriers, bufferMemoryBarrierCount);
   STACK_ARRAY(VkImageMemoryBarrier2KHR, image_barriers, imageMemoryBarrierCount);

   VkPipelineStageFlags2KHR src_stage_mask2 = (VkPipelineStageFlags2KHR) srcStageMask;
   VkPipelineStageFlags2KHR dst_stage_mask2 = (VkPipelineStageFlags2KHR) dstStageMask;

   for (uint32_t i = 0; i < memoryBarrierCount; i++) {
      memory_barriers[i] = upgrade_memory_barrier(&pMemoryBarriers[i],
                                                  src_stage_mask2,
                                                  dst_stage_mask2);
   }
   for (uint32_t i = 0; i < bufferMemoryBarrierCount; i++) {
      buffer_barriers[i] = upgrade_buffer_memory_barrier(&pBufferMemoryBarriers[i],
                                                         src_stage_mask2,
                                                         dst_stage_mask2);
   }
   for (uint32_t i = 0; i < imageMemoryBarrierCount; i++) {
      image_barriers[i] = upgrade_image_memory_barrier(&pImageMemoryBarriers[i],
                                                       src_stage_mask2,
                                                       dst_stage_mask2);
   }

   VkDependencyInfoKHR dep_info = {
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
      .memoryBarrierCount = memoryBarrierCount,
      .pMemoryBarriers = memory_barriers,
      .bufferMemoryBarrierCount = bufferMemoryBarrierCount,
      .pBufferMemoryBarriers = buffer_barriers,
      .imageMemoryBarrierCount = imageMemoryBarrierCount,
      .pImageMemoryBarriers = image_barriers,
   };

   device->dispatch_table.CmdPipelineBarrier2KHR(commandBuffer, &dep_info);

   STACK_ARRAY_FINISH(memory_barriers);
   STACK_ARRAY_FINISH(buffer_barriers);
   STACK_ARRAY_FINISH(image_barriers);
}

VKAPI_ATTR void VKAPI_CALL
vk_common_CmdSetEvent(
    VkCommandBuffer                             commandBuffer,
    VkEvent                                     event,
    VkPipelineStageFlags                        stageMask)
{
   VK_FROM_HANDLE(vk_command_buffer, cmd_buffer, commandBuffer);
   struct vk_device *device = cmd_buffer->base.device;

   VkMemoryBarrier2KHR mem_barrier = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2_KHR,
      .srcStageMask = (VkPipelineStageFlags2KHR) stageMask,
      .dstStageMask = (VkPipelineStageFlags2KHR) stageMask,
   };
   VkDependencyInfoKHR dep_info = {
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
      .memoryBarrierCount = 1,
      .pMemoryBarriers = &mem_barrier,
   };

   device->dispatch_table.CmdSetEvent2KHR(commandBuffer, event, &dep_info);
}

VKAPI_ATTR void VKAPI_CALL
vk_common_CmdResetEvent(
    VkCommandBuffer                             commandBuffer,
    VkEvent                                     event,
    VkPipelineStageFlags                        stageMask)
{
   VK_FROM_HANDLE(vk_command_buffer, cmd_buffer, commandBuffer);
   struct vk_device *device = cmd_buffer->base.device;

   device->dispatch_table.CmdResetEvent2KHR(commandBuffer,
                                            event,
                                            (VkPipelineStageFlags2KHR) stageMask);
}

VKAPI_ATTR void VKAPI_CALL
vk_common_CmdWaitEvents(
    VkCommandBuffer                             commandBuffer,
    uint32_t                                    eventCount,
    const VkEvent*                              pEvents,
    VkPipelineStageFlags                        srcStageMask,
    VkPipelineStageFlags                        destStageMask,
    uint32_t                                    memoryBarrierCount,
    const VkMemoryBarrier*                      pMemoryBarriers,
    uint32_t                                    bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier*                pBufferMemoryBarriers,
    uint32_t                                    imageMemoryBarrierCount,
    const VkImageMemoryBarrier*                 pImageMemoryBarriers)
{
   VK_FROM_HANDLE(vk_command_buffer, cmd_buffer, commandBuffer);
   struct vk_device *device = cmd_buffer->base.device;

   STACK_ARRAY(VkDependencyInfoKHR, deps, eventCount);

   /* Note that dstStageMask and srcStageMask in the CmdWaitEvent2() call
    * are the same.  This is to match the CmdSetEvent2() call from
    * vk_common_CmdSetEvent().  The actual src->dst stage barrier will
    * happen as part of the CmdPipelineBarrier() call below.
    */
   VkMemoryBarrier2KHR stage_barrier = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2_KHR,
      .srcStageMask = srcStageMask,
      .dstStageMask = srcStageMask,
   };

   for (uint32_t i = 0; i < eventCount; i++) {
      deps[i] = (VkDependencyInfoKHR) {
         .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
         .memoryBarrierCount = 1,
         .pMemoryBarriers = &stage_barrier,
      };
   }
   device->dispatch_table.CmdWaitEvents2KHR(commandBuffer, eventCount, pEvents, deps);

   STACK_ARRAY_FINISH(deps);

   /* Setting dependency to 0 because :
    *
    *    - For BY_REGION_BIT and VIEW_LOCAL_BIT, events are not allowed inside a
    *      render pass so these don't apply.
    *
    *    - For DEVICE_GROUP_BIT, we have the following bit of spec text:
    *
    *        "Semaphore and event dependencies are device-local and only
    *         execute on the one physical device that performs the
    *         dependency."
    */
   const VkDependencyFlags dep_flags = 0;

   device->dispatch_table.CmdPipelineBarrier(commandBuffer,
                                             srcStageMask, destStageMask,
                                             dep_flags,
                                             memoryBarrierCount, pMemoryBarriers,
                                             bufferMemoryBarrierCount, pBufferMemoryBarriers,
                                             imageMemoryBarrierCount, pImageMemoryBarriers);
}

VKAPI_ATTR void VKAPI_CALL
vk_common_CmdWriteBufferMarkerAMD(
    VkCommandBuffer                             commandBuffer,
    VkPipelineStageFlagBits                     pipelineStage,
    VkBuffer                                    dstBuffer,
    VkDeviceSize                                dstOffset,
    uint32_t                                    marker)
{
   VK_FROM_HANDLE(vk_command_buffer, cmd_buffer, commandBuffer);
   struct vk_device *device = cmd_buffer->base.device;

   device->dispatch_table.CmdWriteBufferMarker2AMD(commandBuffer,
                                                   (VkPipelineStageFlags2KHR) pipelineStage,
                                                   dstBuffer,
                                                   dstOffset,
                                                   marker);
}

VKAPI_ATTR void VKAPI_CALL
vk_common_GetQueueCheckpointDataNV(
    VkQueue                                     queue,
    uint32_t*                                   pCheckpointDataCount,
    VkCheckpointDataNV*                         pCheckpointData)
{
   unreachable("Entrypoint not implemented");
}

VKAPI_ATTR VkResult VKAPI_CALL
vk_common_QueueSubmit(
    VkQueue                                     _queue,
    uint32_t                                    submitCount,
    const VkSubmitInfo*                         pSubmits,
    VkFence                                     fence)
{
   VK_FROM_HANDLE(vk_queue, queue, _queue);
   struct vk_device *device = queue->base.device;

   STACK_ARRAY(VkSubmitInfo2KHR, submit_info_2, submitCount);
   STACK_ARRAY(VkPerformanceQuerySubmitInfoKHR, perf_query_submit_info, submitCount);
   STACK_ARRAY(struct wsi_memory_signal_submit_info, wsi_mem_submit_info, submitCount);

   uint32_t n_wait_semaphores = 0;
   uint32_t n_command_buffers = 0;
   uint32_t n_signal_semaphores = 0;
   for (uint32_t s = 0; s < submitCount; s++) {
      n_wait_semaphores += pSubmits[s].waitSemaphoreCount;
      n_command_buffers += pSubmits[s].commandBufferCount;
      n_signal_semaphores += pSubmits[s].signalSemaphoreCount;
   }

   STACK_ARRAY(VkSemaphoreSubmitInfoKHR, wait_semaphores, n_wait_semaphores);
   STACK_ARRAY(VkCommandBufferSubmitInfoKHR, command_buffers, n_command_buffers);
   STACK_ARRAY(VkSemaphoreSubmitInfoKHR, signal_semaphores, n_signal_semaphores);

   n_wait_semaphores = 0;
   n_command_buffers = 0;
   n_signal_semaphores = 0;

   for (uint32_t s = 0; s < submitCount; s++) {
      const VkTimelineSemaphoreSubmitInfoKHR *timeline_info =
         vk_find_struct_const(pSubmits[s].pNext,
                              TIMELINE_SEMAPHORE_SUBMIT_INFO_KHR);
      const uint64_t *wait_values =
         timeline_info && timeline_info->waitSemaphoreValueCount ?
         timeline_info->pWaitSemaphoreValues : NULL;
      const uint64_t *signal_values =
         timeline_info && timeline_info->signalSemaphoreValueCount ?
         timeline_info->pSignalSemaphoreValues : NULL;

      const VkDeviceGroupSubmitInfo *group_info =
         vk_find_struct_const(pSubmits[s].pNext, DEVICE_GROUP_SUBMIT_INFO);

      for (uint32_t i = 0; i < pSubmits[s].waitSemaphoreCount; i++) {
         wait_semaphores[n_wait_semaphores + i] = (VkSemaphoreSubmitInfoKHR) {
            .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR,
            .semaphore   = pSubmits[s].pWaitSemaphores[i],
            .value       = wait_values ? wait_values[i] : 0,
            .stageMask   = pSubmits[s].pWaitDstStageMask[i],
            .deviceIndex = group_info ? group_info->pWaitSemaphoreDeviceIndices[i] : 0,
         };
      }
      for (uint32_t i = 0; i < pSubmits[s].commandBufferCount; i++) {
         command_buffers[n_command_buffers + i] = (VkCommandBufferSubmitInfoKHR) {
            .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR,
            .commandBuffer = pSubmits[s].pCommandBuffers[i],
            .deviceMask    = group_info ? group_info->pCommandBufferDeviceMasks[i] : 0,
         };
      }
      for (uint32_t i = 0; i < pSubmits[s].signalSemaphoreCount; i++) {
         signal_semaphores[n_signal_semaphores + i] = (VkSemaphoreSubmitInfoKHR) {
            .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR,
            .semaphore = pSubmits[s].pSignalSemaphores[i],
            .value     = signal_values ? signal_values[i] : 0,
            .stageMask = 0,
            .deviceIndex = group_info ? group_info->pSignalSemaphoreDeviceIndices[i] : 0,
         };
      }

      const VkProtectedSubmitInfo *protected_info =
         vk_find_struct_const(pSubmits[s].pNext, PROTECTED_SUBMIT_INFO);

      submit_info_2[s] = (VkSubmitInfo2KHR) {
         .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR,
         .flags                    = ((protected_info && protected_info->protectedSubmit) ?
                                      VK_SUBMIT_PROTECTED_BIT_KHR : 0),
         .waitSemaphoreInfoCount   = pSubmits[s].waitSemaphoreCount,
         .pWaitSemaphoreInfos      = &wait_semaphores[n_wait_semaphores],
         .commandBufferInfoCount   = pSubmits[s].commandBufferCount,
         .pCommandBufferInfos      = &command_buffers[n_command_buffers],
         .signalSemaphoreInfoCount = pSubmits[s].signalSemaphoreCount,
         .pSignalSemaphoreInfos    = &signal_semaphores[n_signal_semaphores],
      };

      const VkPerformanceQuerySubmitInfoKHR *query_info =
         vk_find_struct_const(pSubmits[s].pNext,
                              PERFORMANCE_QUERY_SUBMIT_INFO_KHR);
      if (query_info) {
         perf_query_submit_info[s] = *query_info;
         perf_query_submit_info[s].pNext = NULL;
         __vk_append_struct(&submit_info_2[s], &perf_query_submit_info[s]);
      }

      const struct wsi_memory_signal_submit_info *mem_signal_info =
         vk_find_struct_const(pSubmits[s].pNext,
                              WSI_MEMORY_SIGNAL_SUBMIT_INFO_MESA);
      if (mem_signal_info) {
         wsi_mem_submit_info[s] = *mem_signal_info;
         wsi_mem_submit_info[s].pNext = NULL;
         __vk_append_struct(&submit_info_2[s], &wsi_mem_submit_info[s]);
      }

      n_wait_semaphores += pSubmits[s].waitSemaphoreCount;
      n_command_buffers += pSubmits[s].commandBufferCount;
      n_signal_semaphores += pSubmits[s].signalSemaphoreCount;
   }

   VkResult result = device->dispatch_table.QueueSubmit2KHR(_queue,
                                                            submitCount,
                                                            submit_info_2,
                                                            fence);

   STACK_ARRAY_FINISH(wait_semaphores);
   STACK_ARRAY_FINISH(command_buffers);
   STACK_ARRAY_FINISH(signal_semaphores);
   STACK_ARRAY_FINISH(submit_info_2);
   STACK_ARRAY_FINISH(perf_query_submit_info);
   STACK_ARRAY_FINISH(wsi_mem_submit_info);

   return result;
}
