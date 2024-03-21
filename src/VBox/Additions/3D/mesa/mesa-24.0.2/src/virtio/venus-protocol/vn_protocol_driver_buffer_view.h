/* This file is generated by venus-protocol.  See vn_protocol_driver.h. */

/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

#ifndef VN_PROTOCOL_DRIVER_BUFFER_VIEW_H
#define VN_PROTOCOL_DRIVER_BUFFER_VIEW_H

#include "vn_ring.h"
#include "vn_protocol_driver_structs.h"

/* struct VkBufferViewCreateInfo chain */

static inline size_t
vn_sizeof_VkBufferViewCreateInfo_pnext(const void *val)
{
    /* no known/supported struct */
    return vn_sizeof_simple_pointer(NULL);
}

static inline size_t
vn_sizeof_VkBufferViewCreateInfo_self(const VkBufferViewCreateInfo *val)
{
    size_t size = 0;
    /* skip val->{sType,pNext} */
    size += vn_sizeof_VkFlags(&val->flags);
    size += vn_sizeof_VkBuffer(&val->buffer);
    size += vn_sizeof_VkFormat(&val->format);
    size += vn_sizeof_VkDeviceSize(&val->offset);
    size += vn_sizeof_VkDeviceSize(&val->range);
    return size;
}

static inline size_t
vn_sizeof_VkBufferViewCreateInfo(const VkBufferViewCreateInfo *val)
{
    size_t size = 0;

    size += vn_sizeof_VkStructureType(&val->sType);
    size += vn_sizeof_VkBufferViewCreateInfo_pnext(val->pNext);
    size += vn_sizeof_VkBufferViewCreateInfo_self(val);

    return size;
}

static inline void
vn_encode_VkBufferViewCreateInfo_pnext(struct vn_cs_encoder *enc, const void *val)
{
    /* no known/supported struct */
    vn_encode_simple_pointer(enc, NULL);
}

static inline void
vn_encode_VkBufferViewCreateInfo_self(struct vn_cs_encoder *enc, const VkBufferViewCreateInfo *val)
{
    /* skip val->{sType,pNext} */
    vn_encode_VkFlags(enc, &val->flags);
    vn_encode_VkBuffer(enc, &val->buffer);
    vn_encode_VkFormat(enc, &val->format);
    vn_encode_VkDeviceSize(enc, &val->offset);
    vn_encode_VkDeviceSize(enc, &val->range);
}

static inline void
vn_encode_VkBufferViewCreateInfo(struct vn_cs_encoder *enc, const VkBufferViewCreateInfo *val)
{
    assert(val->sType == VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO);
    vn_encode_VkStructureType(enc, &(VkStructureType){ VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO });
    vn_encode_VkBufferViewCreateInfo_pnext(enc, val->pNext);
    vn_encode_VkBufferViewCreateInfo_self(enc, val);
}

static inline size_t vn_sizeof_vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBufferView* pView)
{
    const VkCommandTypeEXT cmd_type = VK_COMMAND_TYPE_vkCreateBufferView_EXT;
    const VkFlags cmd_flags = 0;
    size_t cmd_size = vn_sizeof_VkCommandTypeEXT(&cmd_type) + vn_sizeof_VkFlags(&cmd_flags);

    cmd_size += vn_sizeof_VkDevice(&device);
    cmd_size += vn_sizeof_simple_pointer(pCreateInfo);
    if (pCreateInfo)
        cmd_size += vn_sizeof_VkBufferViewCreateInfo(pCreateInfo);
    cmd_size += vn_sizeof_simple_pointer(pAllocator);
    if (pAllocator)
        assert(false);
    cmd_size += vn_sizeof_simple_pointer(pView);
    if (pView)
        cmd_size += vn_sizeof_VkBufferView(pView);

    return cmd_size;
}

static inline void vn_encode_vkCreateBufferView(struct vn_cs_encoder *enc, VkCommandFlagsEXT cmd_flags, VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBufferView* pView)
{
    const VkCommandTypeEXT cmd_type = VK_COMMAND_TYPE_vkCreateBufferView_EXT;

    vn_encode_VkCommandTypeEXT(enc, &cmd_type);
    vn_encode_VkFlags(enc, &cmd_flags);

    vn_encode_VkDevice(enc, &device);
    if (vn_encode_simple_pointer(enc, pCreateInfo))
        vn_encode_VkBufferViewCreateInfo(enc, pCreateInfo);
    if (vn_encode_simple_pointer(enc, pAllocator))
        assert(false);
    if (vn_encode_simple_pointer(enc, pView))
        vn_encode_VkBufferView(enc, pView);
}

static inline size_t vn_sizeof_vkCreateBufferView_reply(VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBufferView* pView)
{
    const VkCommandTypeEXT cmd_type = VK_COMMAND_TYPE_vkCreateBufferView_EXT;
    size_t cmd_size = vn_sizeof_VkCommandTypeEXT(&cmd_type);

    VkResult ret;
    cmd_size += vn_sizeof_VkResult(&ret);
    /* skip device */
    /* skip pCreateInfo */
    /* skip pAllocator */
    cmd_size += vn_sizeof_simple_pointer(pView);
    if (pView)
        cmd_size += vn_sizeof_VkBufferView(pView);

    return cmd_size;
}

static inline VkResult vn_decode_vkCreateBufferView_reply(struct vn_cs_decoder *dec, VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBufferView* pView)
{
    VkCommandTypeEXT command_type;
    vn_decode_VkCommandTypeEXT(dec, &command_type);
    assert(command_type == VK_COMMAND_TYPE_vkCreateBufferView_EXT);

    VkResult ret;
    vn_decode_VkResult(dec, &ret);
    /* skip device */
    /* skip pCreateInfo */
    /* skip pAllocator */
    if (vn_decode_simple_pointer(dec)) {
        vn_decode_VkBufferView(dec, pView);
    } else {
        pView = NULL;
    }

    return ret;
}

static inline size_t vn_sizeof_vkDestroyBufferView(VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks* pAllocator)
{
    const VkCommandTypeEXT cmd_type = VK_COMMAND_TYPE_vkDestroyBufferView_EXT;
    const VkFlags cmd_flags = 0;
    size_t cmd_size = vn_sizeof_VkCommandTypeEXT(&cmd_type) + vn_sizeof_VkFlags(&cmd_flags);

    cmd_size += vn_sizeof_VkDevice(&device);
    cmd_size += vn_sizeof_VkBufferView(&bufferView);
    cmd_size += vn_sizeof_simple_pointer(pAllocator);
    if (pAllocator)
        assert(false);

    return cmd_size;
}

static inline void vn_encode_vkDestroyBufferView(struct vn_cs_encoder *enc, VkCommandFlagsEXT cmd_flags, VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks* pAllocator)
{
    const VkCommandTypeEXT cmd_type = VK_COMMAND_TYPE_vkDestroyBufferView_EXT;

    vn_encode_VkCommandTypeEXT(enc, &cmd_type);
    vn_encode_VkFlags(enc, &cmd_flags);

    vn_encode_VkDevice(enc, &device);
    vn_encode_VkBufferView(enc, &bufferView);
    if (vn_encode_simple_pointer(enc, pAllocator))
        assert(false);
}

static inline size_t vn_sizeof_vkDestroyBufferView_reply(VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks* pAllocator)
{
    const VkCommandTypeEXT cmd_type = VK_COMMAND_TYPE_vkDestroyBufferView_EXT;
    size_t cmd_size = vn_sizeof_VkCommandTypeEXT(&cmd_type);

    /* skip device */
    /* skip bufferView */
    /* skip pAllocator */

    return cmd_size;
}

static inline void vn_decode_vkDestroyBufferView_reply(struct vn_cs_decoder *dec, VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks* pAllocator)
{
    VkCommandTypeEXT command_type;
    vn_decode_VkCommandTypeEXT(dec, &command_type);
    assert(command_type == VK_COMMAND_TYPE_vkDestroyBufferView_EXT);

    /* skip device */
    /* skip bufferView */
    /* skip pAllocator */
}

static inline void vn_submit_vkCreateBufferView(struct vn_ring *vn_ring, VkCommandFlagsEXT cmd_flags, VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBufferView* pView, struct vn_ring_submit_command *submit)
{
    uint8_t local_cmd_data[VN_SUBMIT_LOCAL_CMD_SIZE];
    void *cmd_data = local_cmd_data;
    size_t cmd_size = vn_sizeof_vkCreateBufferView(device, pCreateInfo, pAllocator, pView);
    if (cmd_size > sizeof(local_cmd_data)) {
        cmd_data = malloc(cmd_size);
        if (!cmd_data)
            cmd_size = 0;
    }
    const size_t reply_size = cmd_flags & VK_COMMAND_GENERATE_REPLY_BIT_EXT ? vn_sizeof_vkCreateBufferView_reply(device, pCreateInfo, pAllocator, pView) : 0;

    struct vn_cs_encoder *enc = vn_ring_submit_command_init(vn_ring, submit, cmd_data, cmd_size, reply_size);
    if (cmd_size) {
        vn_encode_vkCreateBufferView(enc, cmd_flags, device, pCreateInfo, pAllocator, pView);
        vn_ring_submit_command(vn_ring, submit);
        if (cmd_data != local_cmd_data)
            free(cmd_data);
    }
}

static inline void vn_submit_vkDestroyBufferView(struct vn_ring *vn_ring, VkCommandFlagsEXT cmd_flags, VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks* pAllocator, struct vn_ring_submit_command *submit)
{
    uint8_t local_cmd_data[VN_SUBMIT_LOCAL_CMD_SIZE];
    void *cmd_data = local_cmd_data;
    size_t cmd_size = vn_sizeof_vkDestroyBufferView(device, bufferView, pAllocator);
    if (cmd_size > sizeof(local_cmd_data)) {
        cmd_data = malloc(cmd_size);
        if (!cmd_data)
            cmd_size = 0;
    }
    const size_t reply_size = cmd_flags & VK_COMMAND_GENERATE_REPLY_BIT_EXT ? vn_sizeof_vkDestroyBufferView_reply(device, bufferView, pAllocator) : 0;

    struct vn_cs_encoder *enc = vn_ring_submit_command_init(vn_ring, submit, cmd_data, cmd_size, reply_size);
    if (cmd_size) {
        vn_encode_vkDestroyBufferView(enc, cmd_flags, device, bufferView, pAllocator);
        vn_ring_submit_command(vn_ring, submit);
        if (cmd_data != local_cmd_data)
            free(cmd_data);
    }
}

static inline VkResult vn_call_vkCreateBufferView(struct vn_ring *vn_ring, VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBufferView* pView)
{
    VN_TRACE_FUNC();

    struct vn_ring_submit_command submit;
    vn_submit_vkCreateBufferView(vn_ring, VK_COMMAND_GENERATE_REPLY_BIT_EXT, device, pCreateInfo, pAllocator, pView, &submit);
    struct vn_cs_decoder *dec = vn_ring_get_command_reply(vn_ring, &submit);
    if (dec) {
        const VkResult ret = vn_decode_vkCreateBufferView_reply(dec, device, pCreateInfo, pAllocator, pView);
        vn_ring_free_command_reply(vn_ring, &submit);
        return ret;
    } else {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
}

static inline void vn_async_vkCreateBufferView(struct vn_ring *vn_ring, VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBufferView* pView)
{
    struct vn_ring_submit_command submit;
    vn_submit_vkCreateBufferView(vn_ring, 0, device, pCreateInfo, pAllocator, pView, &submit);
}

static inline void vn_call_vkDestroyBufferView(struct vn_ring *vn_ring, VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks* pAllocator)
{
    VN_TRACE_FUNC();

    struct vn_ring_submit_command submit;
    vn_submit_vkDestroyBufferView(vn_ring, VK_COMMAND_GENERATE_REPLY_BIT_EXT, device, bufferView, pAllocator, &submit);
    struct vn_cs_decoder *dec = vn_ring_get_command_reply(vn_ring, &submit);
    if (dec) {
        vn_decode_vkDestroyBufferView_reply(dec, device, bufferView, pAllocator);
        vn_ring_free_command_reply(vn_ring, &submit);
    }
}

static inline void vn_async_vkDestroyBufferView(struct vn_ring *vn_ring, VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks* pAllocator)
{
    struct vn_ring_submit_command submit;
    vn_submit_vkDestroyBufferView(vn_ring, 0, device, bufferView, pAllocator, &submit);
}

#endif /* VN_PROTOCOL_DRIVER_BUFFER_VIEW_H */
