/* This file is generated by venus-protocol.  See vn_protocol_driver.h. */

/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

#ifndef VN_PROTOCOL_DRIVER_DEFINES_H
#define VN_PROTOCOL_DRIVER_DEFINES_H

#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

#define VN_SUBMIT_LOCAL_CMD_SIZE 256

/* VkStructureType */
#define VK_STRUCTURE_TYPE_RING_CREATE_INFO_MESA ((VkStructureType)1000384000)
#define VK_STRUCTURE_TYPE_MEMORY_RESOURCE_PROPERTIES_MESA ((VkStructureType)1000384001)
#define VK_STRUCTURE_TYPE_IMPORT_MEMORY_RESOURCE_INFO_MESA ((VkStructureType)1000384002)
#define VK_STRUCTURE_TYPE_MEMORY_RESOURCE_ALLOCATION_SIZE_PROPERTIES_MESA ((VkStructureType)1000384003)
#define VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_RESOURCE_INFO_MESA ((VkStructureType)1000384004)
#define VK_STRUCTURE_TYPE_DEVICE_QUEUE_TIMELINE_INFO_MESA ((VkStructureType)1000384005)
#define VK_STRUCTURE_TYPE_RING_MONITOR_INFO_MESA ((VkStructureType)1000384006)

typedef enum VkCommandTypeEXT {
    VK_COMMAND_TYPE_vkCreateInstance_EXT = 0,
    VK_COMMAND_TYPE_vkDestroyInstance_EXT = 1,
    VK_COMMAND_TYPE_vkEnumeratePhysicalDevices_EXT = 2,
    VK_COMMAND_TYPE_vkGetPhysicalDeviceFeatures_EXT = 3,
    VK_COMMAND_TYPE_vkGetPhysicalDeviceFormatProperties_EXT = 4,
    VK_COMMAND_TYPE_vkGetPhysicalDeviceImageFormatProperties_EXT = 5,
    VK_COMMAND_TYPE_vkGetPhysicalDeviceProperties_EXT = 6,
    VK_COMMAND_TYPE_vkGetPhysicalDeviceQueueFamilyProperties_EXT = 7,
    VK_COMMAND_TYPE_vkGetPhysicalDeviceMemoryProperties_EXT = 8,
    VK_COMMAND_TYPE_vkGetInstanceProcAddr_EXT = 9,
    VK_COMMAND_TYPE_vkGetDeviceProcAddr_EXT = 10,
    VK_COMMAND_TYPE_vkCreateDevice_EXT = 11,
    VK_COMMAND_TYPE_vkDestroyDevice_EXT = 12,
    VK_COMMAND_TYPE_vkEnumerateInstanceExtensionProperties_EXT = 13,
    VK_COMMAND_TYPE_vkEnumerateDeviceExtensionProperties_EXT = 14,
    VK_COMMAND_TYPE_vkEnumerateInstanceLayerProperties_EXT = 15,
    VK_COMMAND_TYPE_vkEnumerateDeviceLayerProperties_EXT = 16,
    VK_COMMAND_TYPE_vkGetDeviceQueue_EXT = 17,
    VK_COMMAND_TYPE_vkQueueSubmit_EXT = 18,
    VK_COMMAND_TYPE_vkQueueWaitIdle_EXT = 19,
    VK_COMMAND_TYPE_vkDeviceWaitIdle_EXT = 20,
    VK_COMMAND_TYPE_vkAllocateMemory_EXT = 21,
    VK_COMMAND_TYPE_vkFreeMemory_EXT = 22,
    VK_COMMAND_TYPE_vkMapMemory_EXT = 23,
    VK_COMMAND_TYPE_vkUnmapMemory_EXT = 24,
    VK_COMMAND_TYPE_vkFlushMappedMemoryRanges_EXT = 25,
    VK_COMMAND_TYPE_vkInvalidateMappedMemoryRanges_EXT = 26,
    VK_COMMAND_TYPE_vkGetDeviceMemoryCommitment_EXT = 27,
    VK_COMMAND_TYPE_vkBindBufferMemory_EXT = 28,
    VK_COMMAND_TYPE_vkBindImageMemory_EXT = 29,
    VK_COMMAND_TYPE_vkGetBufferMemoryRequirements_EXT = 30,
    VK_COMMAND_TYPE_vkGetImageMemoryRequirements_EXT = 31,
    VK_COMMAND_TYPE_vkGetImageSparseMemoryRequirements_EXT = 32,
    VK_COMMAND_TYPE_vkGetPhysicalDeviceSparseImageFormatProperties_EXT = 33,
    VK_COMMAND_TYPE_vkQueueBindSparse_EXT = 34,
    VK_COMMAND_TYPE_vkCreateFence_EXT = 35,
    VK_COMMAND_TYPE_vkDestroyFence_EXT = 36,
    VK_COMMAND_TYPE_vkResetFences_EXT = 37,
    VK_COMMAND_TYPE_vkGetFenceStatus_EXT = 38,
    VK_COMMAND_TYPE_vkWaitForFences_EXT = 39,
    VK_COMMAND_TYPE_vkCreateSemaphore_EXT = 40,
    VK_COMMAND_TYPE_vkDestroySemaphore_EXT = 41,
    VK_COMMAND_TYPE_vkCreateEvent_EXT = 42,
    VK_COMMAND_TYPE_vkDestroyEvent_EXT = 43,
    VK_COMMAND_TYPE_vkGetEventStatus_EXT = 44,
    VK_COMMAND_TYPE_vkSetEvent_EXT = 45,
    VK_COMMAND_TYPE_vkResetEvent_EXT = 46,
    VK_COMMAND_TYPE_vkCreateQueryPool_EXT = 47,
    VK_COMMAND_TYPE_vkDestroyQueryPool_EXT = 48,
    VK_COMMAND_TYPE_vkGetQueryPoolResults_EXT = 49,
    VK_COMMAND_TYPE_vkCreateBuffer_EXT = 50,
    VK_COMMAND_TYPE_vkDestroyBuffer_EXT = 51,
    VK_COMMAND_TYPE_vkCreateBufferView_EXT = 52,
    VK_COMMAND_TYPE_vkDestroyBufferView_EXT = 53,
    VK_COMMAND_TYPE_vkCreateImage_EXT = 54,
    VK_COMMAND_TYPE_vkDestroyImage_EXT = 55,
    VK_COMMAND_TYPE_vkGetImageSubresourceLayout_EXT = 56,
    VK_COMMAND_TYPE_vkCreateImageView_EXT = 57,
    VK_COMMAND_TYPE_vkDestroyImageView_EXT = 58,
    VK_COMMAND_TYPE_vkCreateShaderModule_EXT = 59,
    VK_COMMAND_TYPE_vkDestroyShaderModule_EXT = 60,
    VK_COMMAND_TYPE_vkCreatePipelineCache_EXT = 61,
    VK_COMMAND_TYPE_vkDestroyPipelineCache_EXT = 62,
    VK_COMMAND_TYPE_vkGetPipelineCacheData_EXT = 63,
    VK_COMMAND_TYPE_vkMergePipelineCaches_EXT = 64,
    VK_COMMAND_TYPE_vkCreateGraphicsPipelines_EXT = 65,
    VK_COMMAND_TYPE_vkCreateComputePipelines_EXT = 66,
    VK_COMMAND_TYPE_vkDestroyPipeline_EXT = 67,
    VK_COMMAND_TYPE_vkCreatePipelineLayout_EXT = 68,
    VK_COMMAND_TYPE_vkDestroyPipelineLayout_EXT = 69,
    VK_COMMAND_TYPE_vkCreateSampler_EXT = 70,
    VK_COMMAND_TYPE_vkDestroySampler_EXT = 71,
    VK_COMMAND_TYPE_vkCreateDescriptorSetLayout_EXT = 72,
    VK_COMMAND_TYPE_vkDestroyDescriptorSetLayout_EXT = 73,
    VK_COMMAND_TYPE_vkCreateDescriptorPool_EXT = 74,
    VK_COMMAND_TYPE_vkDestroyDescriptorPool_EXT = 75,
    VK_COMMAND_TYPE_vkResetDescriptorPool_EXT = 76,
    VK_COMMAND_TYPE_vkAllocateDescriptorSets_EXT = 77,
    VK_COMMAND_TYPE_vkFreeDescriptorSets_EXT = 78,
    VK_COMMAND_TYPE_vkUpdateDescriptorSets_EXT = 79,
    VK_COMMAND_TYPE_vkCreateFramebuffer_EXT = 80,
    VK_COMMAND_TYPE_vkDestroyFramebuffer_EXT = 81,
    VK_COMMAND_TYPE_vkCreateRenderPass_EXT = 82,
    VK_COMMAND_TYPE_vkDestroyRenderPass_EXT = 83,
    VK_COMMAND_TYPE_vkGetRenderAreaGranularity_EXT = 84,
    VK_COMMAND_TYPE_vkCreateCommandPool_EXT = 85,
    VK_COMMAND_TYPE_vkDestroyCommandPool_EXT = 86,
    VK_COMMAND_TYPE_vkResetCommandPool_EXT = 87,
    VK_COMMAND_TYPE_vkAllocateCommandBuffers_EXT = 88,
    VK_COMMAND_TYPE_vkFreeCommandBuffers_EXT = 89,
    VK_COMMAND_TYPE_vkBeginCommandBuffer_EXT = 90,
    VK_COMMAND_TYPE_vkEndCommandBuffer_EXT = 91,
    VK_COMMAND_TYPE_vkResetCommandBuffer_EXT = 92,
    VK_COMMAND_TYPE_vkCmdBindPipeline_EXT = 93,
    VK_COMMAND_TYPE_vkCmdSetViewport_EXT = 94,
    VK_COMMAND_TYPE_vkCmdSetScissor_EXT = 95,
    VK_COMMAND_TYPE_vkCmdSetLineWidth_EXT = 96,
    VK_COMMAND_TYPE_vkCmdSetDepthBias_EXT = 97,
    VK_COMMAND_TYPE_vkCmdSetBlendConstants_EXT = 98,
    VK_COMMAND_TYPE_vkCmdSetDepthBounds_EXT = 99,
    VK_COMMAND_TYPE_vkCmdSetStencilCompareMask_EXT = 100,
    VK_COMMAND_TYPE_vkCmdSetStencilWriteMask_EXT = 101,
    VK_COMMAND_TYPE_vkCmdSetStencilReference_EXT = 102,
    VK_COMMAND_TYPE_vkCmdBindDescriptorSets_EXT = 103,
    VK_COMMAND_TYPE_vkCmdBindIndexBuffer_EXT = 104,
    VK_COMMAND_TYPE_vkCmdBindVertexBuffers_EXT = 105,
    VK_COMMAND_TYPE_vkCmdDraw_EXT = 106,
    VK_COMMAND_TYPE_vkCmdDrawIndexed_EXT = 107,
    VK_COMMAND_TYPE_vkCmdDrawIndirect_EXT = 108,
    VK_COMMAND_TYPE_vkCmdDrawIndexedIndirect_EXT = 109,
    VK_COMMAND_TYPE_vkCmdDispatch_EXT = 110,
    VK_COMMAND_TYPE_vkCmdDispatchIndirect_EXT = 111,
    VK_COMMAND_TYPE_vkCmdCopyBuffer_EXT = 112,
    VK_COMMAND_TYPE_vkCmdCopyImage_EXT = 113,
    VK_COMMAND_TYPE_vkCmdBlitImage_EXT = 114,
    VK_COMMAND_TYPE_vkCmdCopyBufferToImage_EXT = 115,
    VK_COMMAND_TYPE_vkCmdCopyImageToBuffer_EXT = 116,
    VK_COMMAND_TYPE_vkCmdUpdateBuffer_EXT = 117,
    VK_COMMAND_TYPE_vkCmdFillBuffer_EXT = 118,
    VK_COMMAND_TYPE_vkCmdClearColorImage_EXT = 119,
    VK_COMMAND_TYPE_vkCmdClearDepthStencilImage_EXT = 120,
    VK_COMMAND_TYPE_vkCmdClearAttachments_EXT = 121,
    VK_COMMAND_TYPE_vkCmdResolveImage_EXT = 122,
    VK_COMMAND_TYPE_vkCmdSetEvent_EXT = 123,
    VK_COMMAND_TYPE_vkCmdResetEvent_EXT = 124,
    VK_COMMAND_TYPE_vkCmdWaitEvents_EXT = 125,
    VK_COMMAND_TYPE_vkCmdPipelineBarrier_EXT = 126,
    VK_COMMAND_TYPE_vkCmdBeginQuery_EXT = 127,
    VK_COMMAND_TYPE_vkCmdEndQuery_EXT = 128,
    VK_COMMAND_TYPE_vkCmdResetQueryPool_EXT = 129,
    VK_COMMAND_TYPE_vkCmdWriteTimestamp_EXT = 130,
    VK_COMMAND_TYPE_vkCmdCopyQueryPoolResults_EXT = 131,
    VK_COMMAND_TYPE_vkCmdPushConstants_EXT = 132,
    VK_COMMAND_TYPE_vkCmdBeginRenderPass_EXT = 133,
    VK_COMMAND_TYPE_vkCmdNextSubpass_EXT = 134,
    VK_COMMAND_TYPE_vkCmdEndRenderPass_EXT = 135,
    VK_COMMAND_TYPE_vkCmdExecuteCommands_EXT = 136,
    VK_COMMAND_TYPE_vkEnumerateInstanceVersion_EXT = 137,
    VK_COMMAND_TYPE_vkBindBufferMemory2_EXT = 138,
    VK_COMMAND_TYPE_vkBindBufferMemory2KHR_EXT = 138,
    VK_COMMAND_TYPE_vkBindImageMemory2_EXT = 139,
    VK_COMMAND_TYPE_vkBindImageMemory2KHR_EXT = 139,
    VK_COMMAND_TYPE_vkGetDeviceGroupPeerMemoryFeatures_EXT = 140,
    VK_COMMAND_TYPE_vkGetDeviceGroupPeerMemoryFeaturesKHR_EXT = 140,
    VK_COMMAND_TYPE_vkCmdSetDeviceMask_EXT = 141,
    VK_COMMAND_TYPE_vkCmdSetDeviceMaskKHR_EXT = 141,
    VK_COMMAND_TYPE_vkCmdDispatchBase_EXT = 142,
    VK_COMMAND_TYPE_vkCmdDispatchBaseKHR_EXT = 142,
    VK_COMMAND_TYPE_vkEnumeratePhysicalDeviceGroups_EXT = 143,
    VK_COMMAND_TYPE_vkEnumeratePhysicalDeviceGroupsKHR_EXT = 143,
    VK_COMMAND_TYPE_vkGetImageMemoryRequirements2_EXT = 144,
    VK_COMMAND_TYPE_vkGetImageMemoryRequirements2KHR_EXT = 144,
    VK_COMMAND_TYPE_vkGetBufferMemoryRequirements2_EXT = 145,
    VK_COMMAND_TYPE_vkGetBufferMemoryRequirements2KHR_EXT = 145,
    VK_COMMAND_TYPE_vkGetImageSparseMemoryRequirements2_EXT = 146,
    VK_COMMAND_TYPE_vkGetImageSparseMemoryRequirements2KHR_EXT = 146,
    VK_COMMAND_TYPE_vkGetPhysicalDeviceFeatures2_EXT = 147,
    VK_COMMAND_TYPE_vkGetPhysicalDeviceFeatures2KHR_EXT = 147,
    VK_COMMAND_TYPE_vkGetPhysicalDeviceProperties2_EXT = 148,
    VK_COMMAND_TYPE_vkGetPhysicalDeviceProperties2KHR_EXT = 148,
    VK_COMMAND_TYPE_vkGetPhysicalDeviceFormatProperties2_EXT = 149,
    VK_COMMAND_TYPE_vkGetPhysicalDeviceFormatProperties2KHR_EXT = 149,
    VK_COMMAND_TYPE_vkGetPhysicalDeviceImageFormatProperties2_EXT = 150,
    VK_COMMAND_TYPE_vkGetPhysicalDeviceImageFormatProperties2KHR_EXT = 150,
    VK_COMMAND_TYPE_vkGetPhysicalDeviceQueueFamilyProperties2_EXT = 151,
    VK_COMMAND_TYPE_vkGetPhysicalDeviceQueueFamilyProperties2KHR_EXT = 151,
    VK_COMMAND_TYPE_vkGetPhysicalDeviceMemoryProperties2_EXT = 152,
    VK_COMMAND_TYPE_vkGetPhysicalDeviceMemoryProperties2KHR_EXT = 152,
    VK_COMMAND_TYPE_vkGetPhysicalDeviceSparseImageFormatProperties2_EXT = 153,
    VK_COMMAND_TYPE_vkGetPhysicalDeviceSparseImageFormatProperties2KHR_EXT = 153,
    VK_COMMAND_TYPE_vkTrimCommandPool_EXT = 154,
    VK_COMMAND_TYPE_vkTrimCommandPoolKHR_EXT = 154,
    VK_COMMAND_TYPE_vkGetDeviceQueue2_EXT = 155,
    VK_COMMAND_TYPE_vkCreateSamplerYcbcrConversion_EXT = 156,
    VK_COMMAND_TYPE_vkCreateSamplerYcbcrConversionKHR_EXT = 156,
    VK_COMMAND_TYPE_vkDestroySamplerYcbcrConversion_EXT = 157,
    VK_COMMAND_TYPE_vkDestroySamplerYcbcrConversionKHR_EXT = 157,
    VK_COMMAND_TYPE_vkCreateDescriptorUpdateTemplate_EXT = 158,
    VK_COMMAND_TYPE_vkCreateDescriptorUpdateTemplateKHR_EXT = 158,
    VK_COMMAND_TYPE_vkDestroyDescriptorUpdateTemplate_EXT = 159,
    VK_COMMAND_TYPE_vkDestroyDescriptorUpdateTemplateKHR_EXT = 159,
    VK_COMMAND_TYPE_vkUpdateDescriptorSetWithTemplate_EXT = 160,
    VK_COMMAND_TYPE_vkUpdateDescriptorSetWithTemplateKHR_EXT = 160,
    VK_COMMAND_TYPE_vkGetPhysicalDeviceExternalBufferProperties_EXT = 161,
    VK_COMMAND_TYPE_vkGetPhysicalDeviceExternalBufferPropertiesKHR_EXT = 161,
    VK_COMMAND_TYPE_vkGetPhysicalDeviceExternalFenceProperties_EXT = 162,
    VK_COMMAND_TYPE_vkGetPhysicalDeviceExternalFencePropertiesKHR_EXT = 162,
    VK_COMMAND_TYPE_vkGetPhysicalDeviceExternalSemaphoreProperties_EXT = 163,
    VK_COMMAND_TYPE_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR_EXT = 163,
    VK_COMMAND_TYPE_vkGetDescriptorSetLayoutSupport_EXT = 164,
    VK_COMMAND_TYPE_vkGetDescriptorSetLayoutSupportKHR_EXT = 164,
    VK_COMMAND_TYPE_vkCmdDrawIndirectCount_EXT = 165,
    VK_COMMAND_TYPE_vkCmdDrawIndirectCountKHR_EXT = 165,
    VK_COMMAND_TYPE_vkCmdDrawIndirectCountAMD_EXT = 165,
    VK_COMMAND_TYPE_vkCmdDrawIndexedIndirectCount_EXT = 166,
    VK_COMMAND_TYPE_vkCmdDrawIndexedIndirectCountKHR_EXT = 166,
    VK_COMMAND_TYPE_vkCmdDrawIndexedIndirectCountAMD_EXT = 166,
    VK_COMMAND_TYPE_vkCreateRenderPass2_EXT = 167,
    VK_COMMAND_TYPE_vkCreateRenderPass2KHR_EXT = 167,
    VK_COMMAND_TYPE_vkCmdBeginRenderPass2_EXT = 168,
    VK_COMMAND_TYPE_vkCmdBeginRenderPass2KHR_EXT = 168,
    VK_COMMAND_TYPE_vkCmdNextSubpass2_EXT = 169,
    VK_COMMAND_TYPE_vkCmdNextSubpass2KHR_EXT = 169,
    VK_COMMAND_TYPE_vkCmdEndRenderPass2_EXT = 170,
    VK_COMMAND_TYPE_vkCmdEndRenderPass2KHR_EXT = 170,
    VK_COMMAND_TYPE_vkResetQueryPool_EXT = 171,
    VK_COMMAND_TYPE_vkResetQueryPoolEXT_EXT = 171,
    VK_COMMAND_TYPE_vkGetSemaphoreCounterValue_EXT = 172,
    VK_COMMAND_TYPE_vkGetSemaphoreCounterValueKHR_EXT = 172,
    VK_COMMAND_TYPE_vkWaitSemaphores_EXT = 173,
    VK_COMMAND_TYPE_vkWaitSemaphoresKHR_EXT = 173,
    VK_COMMAND_TYPE_vkSignalSemaphore_EXT = 174,
    VK_COMMAND_TYPE_vkSignalSemaphoreKHR_EXT = 174,
    VK_COMMAND_TYPE_vkGetBufferDeviceAddress_EXT = 175,
    VK_COMMAND_TYPE_vkGetBufferDeviceAddressKHR_EXT = 175,
    VK_COMMAND_TYPE_vkGetBufferDeviceAddressEXT_EXT = 175,
    VK_COMMAND_TYPE_vkGetBufferOpaqueCaptureAddress_EXT = 176,
    VK_COMMAND_TYPE_vkGetBufferOpaqueCaptureAddressKHR_EXT = 176,
    VK_COMMAND_TYPE_vkGetDeviceMemoryOpaqueCaptureAddress_EXT = 177,
    VK_COMMAND_TYPE_vkGetDeviceMemoryOpaqueCaptureAddressKHR_EXT = 177,
    VK_COMMAND_TYPE_vkGetPhysicalDeviceToolProperties_EXT = 196,
    VK_COMMAND_TYPE_vkGetPhysicalDeviceToolPropertiesEXT_EXT = 196,
    VK_COMMAND_TYPE_vkCreatePrivateDataSlot_EXT = 197,
    VK_COMMAND_TYPE_vkCreatePrivateDataSlotEXT_EXT = 197,
    VK_COMMAND_TYPE_vkDestroyPrivateDataSlot_EXT = 198,
    VK_COMMAND_TYPE_vkDestroyPrivateDataSlotEXT_EXT = 198,
    VK_COMMAND_TYPE_vkSetPrivateData_EXT = 199,
    VK_COMMAND_TYPE_vkSetPrivateDataEXT_EXT = 199,
    VK_COMMAND_TYPE_vkGetPrivateData_EXT = 200,
    VK_COMMAND_TYPE_vkGetPrivateDataEXT_EXT = 200,
    VK_COMMAND_TYPE_vkCmdSetEvent2_EXT = 201,
    VK_COMMAND_TYPE_vkCmdSetEvent2KHR_EXT = 201,
    VK_COMMAND_TYPE_vkCmdResetEvent2_EXT = 202,
    VK_COMMAND_TYPE_vkCmdResetEvent2KHR_EXT = 202,
    VK_COMMAND_TYPE_vkCmdWaitEvents2_EXT = 203,
    VK_COMMAND_TYPE_vkCmdWaitEvents2KHR_EXT = 203,
    VK_COMMAND_TYPE_vkCmdPipelineBarrier2_EXT = 204,
    VK_COMMAND_TYPE_vkCmdPipelineBarrier2KHR_EXT = 204,
    VK_COMMAND_TYPE_vkCmdWriteTimestamp2_EXT = 205,
    VK_COMMAND_TYPE_vkCmdWriteTimestamp2KHR_EXT = 205,
    VK_COMMAND_TYPE_vkQueueSubmit2_EXT = 206,
    VK_COMMAND_TYPE_vkQueueSubmit2KHR_EXT = 206,
    VK_COMMAND_TYPE_vkCmdCopyBuffer2_EXT = 207,
    VK_COMMAND_TYPE_vkCmdCopyBuffer2KHR_EXT = 207,
    VK_COMMAND_TYPE_vkCmdCopyImage2_EXT = 208,
    VK_COMMAND_TYPE_vkCmdCopyImage2KHR_EXT = 208,
    VK_COMMAND_TYPE_vkCmdCopyBufferToImage2_EXT = 209,
    VK_COMMAND_TYPE_vkCmdCopyBufferToImage2KHR_EXT = 209,
    VK_COMMAND_TYPE_vkCmdCopyImageToBuffer2_EXT = 210,
    VK_COMMAND_TYPE_vkCmdCopyImageToBuffer2KHR_EXT = 210,
    VK_COMMAND_TYPE_vkCmdBlitImage2_EXT = 211,
    VK_COMMAND_TYPE_vkCmdBlitImage2KHR_EXT = 211,
    VK_COMMAND_TYPE_vkCmdResolveImage2_EXT = 212,
    VK_COMMAND_TYPE_vkCmdResolveImage2KHR_EXT = 212,
    VK_COMMAND_TYPE_vkCmdBeginRendering_EXT = 213,
    VK_COMMAND_TYPE_vkCmdBeginRenderingKHR_EXT = 213,
    VK_COMMAND_TYPE_vkCmdEndRendering_EXT = 214,
    VK_COMMAND_TYPE_vkCmdEndRenderingKHR_EXT = 214,
    VK_COMMAND_TYPE_vkCmdSetCullMode_EXT = 215,
    VK_COMMAND_TYPE_vkCmdSetCullModeEXT_EXT = 215,
    VK_COMMAND_TYPE_vkCmdSetFrontFace_EXT = 216,
    VK_COMMAND_TYPE_vkCmdSetFrontFaceEXT_EXT = 216,
    VK_COMMAND_TYPE_vkCmdSetPrimitiveTopology_EXT = 217,
    VK_COMMAND_TYPE_vkCmdSetPrimitiveTopologyEXT_EXT = 217,
    VK_COMMAND_TYPE_vkCmdSetViewportWithCount_EXT = 218,
    VK_COMMAND_TYPE_vkCmdSetViewportWithCountEXT_EXT = 218,
    VK_COMMAND_TYPE_vkCmdSetScissorWithCount_EXT = 219,
    VK_COMMAND_TYPE_vkCmdSetScissorWithCountEXT_EXT = 219,
    VK_COMMAND_TYPE_vkCmdBindVertexBuffers2_EXT = 220,
    VK_COMMAND_TYPE_vkCmdBindVertexBuffers2EXT_EXT = 220,
    VK_COMMAND_TYPE_vkCmdSetDepthTestEnable_EXT = 221,
    VK_COMMAND_TYPE_vkCmdSetDepthTestEnableEXT_EXT = 221,
    VK_COMMAND_TYPE_vkCmdSetDepthWriteEnable_EXT = 222,
    VK_COMMAND_TYPE_vkCmdSetDepthWriteEnableEXT_EXT = 222,
    VK_COMMAND_TYPE_vkCmdSetDepthCompareOp_EXT = 223,
    VK_COMMAND_TYPE_vkCmdSetDepthCompareOpEXT_EXT = 223,
    VK_COMMAND_TYPE_vkCmdSetDepthBoundsTestEnable_EXT = 224,
    VK_COMMAND_TYPE_vkCmdSetDepthBoundsTestEnableEXT_EXT = 224,
    VK_COMMAND_TYPE_vkCmdSetStencilTestEnable_EXT = 225,
    VK_COMMAND_TYPE_vkCmdSetStencilTestEnableEXT_EXT = 225,
    VK_COMMAND_TYPE_vkCmdSetStencilOp_EXT = 226,
    VK_COMMAND_TYPE_vkCmdSetStencilOpEXT_EXT = 226,
    VK_COMMAND_TYPE_vkCmdSetRasterizerDiscardEnable_EXT = 227,
    VK_COMMAND_TYPE_vkCmdSetRasterizerDiscardEnableEXT_EXT = 227,
    VK_COMMAND_TYPE_vkCmdSetDepthBiasEnable_EXT = 228,
    VK_COMMAND_TYPE_vkCmdSetDepthBiasEnableEXT_EXT = 228,
    VK_COMMAND_TYPE_vkCmdSetPrimitiveRestartEnable_EXT = 229,
    VK_COMMAND_TYPE_vkCmdSetPrimitiveRestartEnableEXT_EXT = 229,
    VK_COMMAND_TYPE_vkGetDeviceBufferMemoryRequirements_EXT = 230,
    VK_COMMAND_TYPE_vkGetDeviceBufferMemoryRequirementsKHR_EXT = 230,
    VK_COMMAND_TYPE_vkGetDeviceImageMemoryRequirements_EXT = 231,
    VK_COMMAND_TYPE_vkGetDeviceImageMemoryRequirementsKHR_EXT = 231,
    VK_COMMAND_TYPE_vkGetDeviceImageSparseMemoryRequirements_EXT = 232,
    VK_COMMAND_TYPE_vkGetDeviceImageSparseMemoryRequirementsKHR_EXT = 232,
    VK_COMMAND_TYPE_vkCmdBindTransformFeedbackBuffersEXT_EXT = 181,
    VK_COMMAND_TYPE_vkCmdBeginTransformFeedbackEXT_EXT = 182,
    VK_COMMAND_TYPE_vkCmdEndTransformFeedbackEXT_EXT = 183,
    VK_COMMAND_TYPE_vkCmdBeginQueryIndexedEXT_EXT = 184,
    VK_COMMAND_TYPE_vkCmdEndQueryIndexedEXT_EXT = 185,
    VK_COMMAND_TYPE_vkCmdDrawIndirectByteCountEXT_EXT = 186,
    VK_COMMAND_TYPE_vkGetMemoryFdKHR_EXT = 193,
    VK_COMMAND_TYPE_vkGetMemoryFdPropertiesKHR_EXT = 194,
    VK_COMMAND_TYPE_vkImportSemaphoreFdKHR_EXT = 242,
    VK_COMMAND_TYPE_vkGetSemaphoreFdKHR_EXT = 243,
    VK_COMMAND_TYPE_vkCmdPushDescriptorSetKHR_EXT = 249,
    VK_COMMAND_TYPE_vkCmdPushDescriptorSetWithTemplateKHR_EXT = 250,
    VK_COMMAND_TYPE_vkCmdBeginConditionalRenderingEXT_EXT = 240,
    VK_COMMAND_TYPE_vkCmdEndConditionalRenderingEXT_EXT = 241,
    VK_COMMAND_TYPE_vkImportFenceFdKHR_EXT = 238,
    VK_COMMAND_TYPE_vkGetFenceFdKHR_EXT = 239,
    VK_COMMAND_TYPE_vkGetImageDrmFormatModifierPropertiesEXT_EXT = 187,
    VK_COMMAND_TYPE_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT_EXT = 235,
    VK_COMMAND_TYPE_vkGetCalibratedTimestampsEXT_EXT = 236,
    VK_COMMAND_TYPE_vkCmdSetLineStippleEXT_EXT = 237,
    VK_COMMAND_TYPE_vkCmdSetVertexInputEXT_EXT = 255,
    VK_COMMAND_TYPE_vkCmdSetPatchControlPointsEXT_EXT = 233,
    VK_COMMAND_TYPE_vkCmdSetLogicOpEXT_EXT = 234,
    VK_COMMAND_TYPE_vkCmdSetColorWriteEnableEXT_EXT = 254,
    VK_COMMAND_TYPE_vkCmdDrawMultiEXT_EXT = 247,
    VK_COMMAND_TYPE_vkCmdDrawMultiIndexedEXT_EXT = 248,
    VK_COMMAND_TYPE_vkCmdSetTessellationDomainOriginEXT_EXT = 256,
    VK_COMMAND_TYPE_vkCmdSetDepthClampEnableEXT_EXT = 257,
    VK_COMMAND_TYPE_vkCmdSetPolygonModeEXT_EXT = 258,
    VK_COMMAND_TYPE_vkCmdSetRasterizationSamplesEXT_EXT = 259,
    VK_COMMAND_TYPE_vkCmdSetSampleMaskEXT_EXT = 260,
    VK_COMMAND_TYPE_vkCmdSetAlphaToCoverageEnableEXT_EXT = 261,
    VK_COMMAND_TYPE_vkCmdSetAlphaToOneEnableEXT_EXT = 262,
    VK_COMMAND_TYPE_vkCmdSetLogicOpEnableEXT_EXT = 263,
    VK_COMMAND_TYPE_vkCmdSetColorBlendEnableEXT_EXT = 264,
    VK_COMMAND_TYPE_vkCmdSetColorBlendEquationEXT_EXT = 265,
    VK_COMMAND_TYPE_vkCmdSetColorWriteMaskEXT_EXT = 266,
    VK_COMMAND_TYPE_vkCmdSetRasterizationStreamEXT_EXT = 267,
    VK_COMMAND_TYPE_vkCmdSetConservativeRasterizationModeEXT_EXT = 268,
    VK_COMMAND_TYPE_vkCmdSetExtraPrimitiveOverestimationSizeEXT_EXT = 269,
    VK_COMMAND_TYPE_vkCmdSetDepthClipEnableEXT_EXT = 270,
    VK_COMMAND_TYPE_vkCmdSetSampleLocationsEnableEXT_EXT = 271,
    VK_COMMAND_TYPE_vkCmdSetColorBlendAdvancedEXT_EXT = 272,
    VK_COMMAND_TYPE_vkCmdSetProvokingVertexModeEXT_EXT = 273,
    VK_COMMAND_TYPE_vkCmdSetLineRasterizationModeEXT_EXT = 274,
    VK_COMMAND_TYPE_vkCmdSetLineStippleEnableEXT_EXT = 275,
    VK_COMMAND_TYPE_vkCmdSetDepthClipNegativeOneToOneEXT_EXT = 276,
    VK_COMMAND_TYPE_vkSetReplyCommandStreamMESA_EXT = 178,
    VK_COMMAND_TYPE_vkSeekReplyCommandStreamMESA_EXT = 179,
    VK_COMMAND_TYPE_vkExecuteCommandStreamsMESA_EXT = 180,
    VK_COMMAND_TYPE_vkCreateRingMESA_EXT = 188,
    VK_COMMAND_TYPE_vkDestroyRingMESA_EXT = 189,
    VK_COMMAND_TYPE_vkNotifyRingMESA_EXT = 190,
    VK_COMMAND_TYPE_vkWriteRingExtraMESA_EXT = 191,
    VK_COMMAND_TYPE_vkGetMemoryResourcePropertiesMESA_EXT = 192,
    VK_COMMAND_TYPE_vkResetFenceResourceMESA_EXT = 244,
    VK_COMMAND_TYPE_vkWaitSemaphoreResourceMESA_EXT = 245,
    VK_COMMAND_TYPE_vkImportSemaphoreResourceMESA_EXT = 246,
    VK_COMMAND_TYPE_vkSubmitVirtqueueSeqnoMESA_EXT = 251,
    VK_COMMAND_TYPE_vkWaitVirtqueueSeqnoMESA_EXT = 252,
    VK_COMMAND_TYPE_vkWaitRingSeqnoMESA_EXT = 253,
} VkCommandTypeEXT;

typedef enum VkCommandFlagBitsEXT {
    VK_COMMAND_GENERATE_REPLY_BIT_EXT = 0x00000001,
} VkCommandFlagBitsEXT;

typedef enum VkRingStatusFlagBitsMESA {
    VK_RING_STATUS_NONE_MESA = 0,
    VK_RING_STATUS_IDLE_BIT_MESA = 0x00000001,
    VK_RING_STATUS_FATAL_BIT_MESA = 0x00000002,
    VK_RING_STATUS_ALIVE_BIT_MESA = 0x00000004,
} VkRingStatusFlagBitsMESA;

typedef VkFlags VkCommandFlagsEXT;

typedef VkFlags VkCommandStreamExecutionFlagsMESA;

typedef VkFlags VkRingCreateFlagsMESA;

typedef VkFlags VkRingNotifyFlagsMESA;

typedef VkFlags VkRingStatusFlagsMESA;

typedef struct VkCommandStreamDescriptionMESA {
    uint32_t resourceId;
    size_t offset;
    size_t size;
} VkCommandStreamDescriptionMESA;

typedef struct VkCommandStreamDependencyMESA {
    uint32_t srcCommandStream;
    uint32_t dstCommandStream;
} VkCommandStreamDependencyMESA;

typedef struct VkRingCreateInfoMESA {
    VkStructureType sType;
    const void* pNext;
    VkRingCreateFlagsMESA flags;
    uint32_t resourceId;
    size_t offset;
    size_t size;
    uint64_t idleTimeout;
    size_t headOffset;
    size_t tailOffset;
    size_t statusOffset;
    size_t bufferOffset;
    size_t bufferSize;
    size_t extraOffset;
    size_t extraSize;
} VkRingCreateInfoMESA;

typedef struct VkRingMonitorInfoMESA {
    VkStructureType sType;
    const void* pNext;
    uint32_t maxReportingPeriodMicroseconds;
} VkRingMonitorInfoMESA;

typedef struct VkMemoryResourcePropertiesMESA {
    VkStructureType sType;
    void* pNext;
    uint32_t memoryTypeBits;
} VkMemoryResourcePropertiesMESA;

typedef struct VkImportMemoryResourceInfoMESA {
    VkStructureType sType;
    const void* pNext;
    uint32_t resourceId;
} VkImportMemoryResourceInfoMESA;

typedef struct VkMemoryResourceAllocationSizePropertiesMESA {
    VkStructureType sType;
    void* pNext;
    uint64_t allocationSize;
} VkMemoryResourceAllocationSizePropertiesMESA;

typedef struct VkImportSemaphoreResourceInfoMESA {
    VkStructureType sType;
    const void* pNext;
    VkSemaphore semaphore;
    uint32_t resourceId;
} VkImportSemaphoreResourceInfoMESA;

typedef struct VkDeviceQueueTimelineInfoMESA {
    VkStructureType sType;
    const void* pNext;
    uint32_t ringIdx;
} VkDeviceQueueTimelineInfoMESA;

#endif /* VN_PROTOCOL_DRIVER_DEFINES_H */
