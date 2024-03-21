/*******************************************************************************
    Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.

*******************************************************************************/

#include "nvtypes.h"

#ifndef _cl85b5_h_
#define _cl85b5_h_

#ifdef __cplusplus
extern "C" {
#endif

#define GT212_DMA_COPY                                                            (0x000085B5)

#define NV85B5_NOP                                                              (0x00000100)
#define NV85B5_NOP_PARAMETER                                                    31:0
#define NV85B5_PM_TRIGGER                                                       (0x00000140)
#define NV85B5_PM_TRIGGER_V                                                     31:0
#define NV85B5_SET_CTX_DMA(b)                                                   (0x00000180 + (b)*0x00000004)
#define NV85B5_SET_CTX_DMA_HANDLE                                               31:0
#define NV85B5_SET_APPLICATION_ID                                               (0x00000200)
#define NV85B5_SET_APPLICATION_ID_ID                                            31:0
#define NV85B5_SET_APPLICATION_ID_ID_NORMAL                                     (0x00000001)
#define NV85B5_SET_WATCHDOG_TIMER                                               (0x00000204)
#define NV85B5_SET_WATCHDOG_TIMER_TIMER                                         31:0
#define NV85B5_SET_SEMAPHORE_A                                                  (0x00000240)
#define NV85B5_SET_SEMAPHORE_A_UPPER                                            7:0
#define NV85B5_SET_SEMAPHORE_A_CTX_DMA                                          31:28
#define NV85B5_SET_SEMAPHORE_B                                                  (0x00000244)
#define NV85B5_SET_SEMAPHORE_B_LOWER                                            31:0
#define NV85B5_SET_SEMAPHORE_PAYLOAD                                            (0x00000248)
#define NV85B5_SET_SEMAPHORE_PAYLOAD_PAYLOAD                                    31:0
#define NV85B5_LAUNCH_DMA                                                       (0x00000300)
#define NV85B5_LAUNCH_DMA_DATA_TRANSFER_TYPE                                    1:0
#define NV85B5_LAUNCH_DMA_DATA_TRANSFER_TYPE_NONE                               (0x00000000)
#define NV85B5_LAUNCH_DMA_DATA_TRANSFER_TYPE_PIPELINED                          (0x00000001)
#define NV85B5_LAUNCH_DMA_DATA_TRANSFER_TYPE_NON_PIPELINED                      (0x00000002)
#define NV85B5_LAUNCH_DMA_FLUSH_ENABLE                                          2:2
#define NV85B5_LAUNCH_DMA_FLUSH_ENABLE_FALSE                                    (0x00000000)
#define NV85B5_LAUNCH_DMA_FLUSH_ENABLE_TRUE                                     (0x00000001)
#define NV85B5_LAUNCH_DMA_SEMAPHORE_TYPE                                        4:3
#define NV85B5_LAUNCH_DMA_SEMAPHORE_TYPE_NONE                                   (0x00000000)
#define NV85B5_LAUNCH_DMA_SEMAPHORE_TYPE_RELEASE_ONE_WORD_SEMAPHORE             (0x00000001)
#define NV85B5_LAUNCH_DMA_SEMAPHORE_TYPE_RELEASE_FOUR_WORD_SEMAPHORE            (0x00000002)
#define NV85B5_LAUNCH_DMA_INTERRUPT_TYPE                                        6:5
#define NV85B5_LAUNCH_DMA_INTERRUPT_TYPE_NONE                                   (0x00000000)
#define NV85B5_LAUNCH_DMA_INTERRUPT_TYPE_BLOCKING                               (0x00000001)
#define NV85B5_LAUNCH_DMA_INTERRUPT_TYPE_NON_BLOCKING                           (0x00000002)
#define NV85B5_LAUNCH_DMA_SRC_MEMORY_LAYOUT                                     7:7
#define NV85B5_LAUNCH_DMA_SRC_MEMORY_LAYOUT_BLOCKLINEAR                         (0x00000000)
#define NV85B5_LAUNCH_DMA_SRC_MEMORY_LAYOUT_PITCH                               (0x00000001)
#define NV85B5_LAUNCH_DMA_DST_MEMORY_LAYOUT                                     8:8
#define NV85B5_LAUNCH_DMA_DST_MEMORY_LAYOUT_BLOCKLINEAR                         (0x00000000)
#define NV85B5_LAUNCH_DMA_DST_MEMORY_LAYOUT_PITCH                               (0x00000001)
#define NV85B5_LAUNCH_DMA_MULTI_LINE_ENABLE                                     9:9
#define NV85B5_LAUNCH_DMA_MULTI_LINE_ENABLE_FALSE                               (0x00000000)
#define NV85B5_LAUNCH_DMA_MULTI_LINE_ENABLE_TRUE                                (0x00000001)
#define NV85B5_LAUNCH_DMA_REMAP_ENABLE                                          10:10
#define NV85B5_LAUNCH_DMA_REMAP_ENABLE_FALSE                                    (0x00000000)
#define NV85B5_LAUNCH_DMA_REMAP_ENABLE_TRUE                                     (0x00000001)
#define NV85B5_OFFSET_IN_UPPER                                                  (0x00000400)
#define NV85B5_OFFSET_IN_UPPER_UPPER                                            7:0
#define NV85B5_OFFSET_IN_UPPER_CTX_DMA                                          31:28
#define NV85B5_OFFSET_IN_LOWER                                                  (0x00000404)
#define NV85B5_OFFSET_IN_LOWER_VALUE                                            31:0
#define NV85B5_OFFSET_OUT_UPPER                                                 (0x00000408)
#define NV85B5_OFFSET_OUT_UPPER_UPPER                                           7:0
#define NV85B5_OFFSET_OUT_UPPER_CTX_DMA                                         31:28
#define NV85B5_OFFSET_OUT_LOWER                                                 (0x0000040C)
#define NV85B5_OFFSET_OUT_LOWER_VALUE                                           31:0
#define NV85B5_PITCH_IN                                                         (0x00000410)
#define NV85B5_PITCH_IN_VALUE                                                   31:0
#define NV85B5_PITCH_OUT                                                        (0x00000414)
#define NV85B5_PITCH_OUT_VALUE                                                  31:0
#define NV85B5_LINE_LENGTH_IN                                                   (0x00000418)
#define NV85B5_LINE_LENGTH_IN_VALUE                                             31:0
#define NV85B5_LINE_COUNT                                                       (0x0000041C)
#define NV85B5_LINE_COUNT_VALUE                                                 31:0
#define NV85B5_SET_REMAP_CONST_A                                                (0x00000700)
#define NV85B5_SET_REMAP_CONST_A_V                                              31:0
#define NV85B5_SET_REMAP_CONST_B                                                (0x00000704)
#define NV85B5_SET_REMAP_CONST_B_V                                              31:0
#define NV85B5_SET_REMAP_COMPONENTS                                             (0x00000708)
#define NV85B5_SET_REMAP_COMPONENTS_DST_X                                       2:0
#define NV85B5_SET_REMAP_COMPONENTS_DST_X_SRC_X                                 (0x00000000)
#define NV85B5_SET_REMAP_COMPONENTS_DST_X_SRC_Y                                 (0x00000001)
#define NV85B5_SET_REMAP_COMPONENTS_DST_X_SRC_Z                                 (0x00000002)
#define NV85B5_SET_REMAP_COMPONENTS_DST_X_SRC_W                                 (0x00000003)
#define NV85B5_SET_REMAP_COMPONENTS_DST_X_CONST_A                               (0x00000004)
#define NV85B5_SET_REMAP_COMPONENTS_DST_X_CONST_B                               (0x00000005)
#define NV85B5_SET_REMAP_COMPONENTS_DST_X_NO_WRITE                              (0x00000006)
#define NV85B5_SET_REMAP_COMPONENTS_DST_Y                                       6:4
#define NV85B5_SET_REMAP_COMPONENTS_DST_Y_SRC_X                                 (0x00000000)
#define NV85B5_SET_REMAP_COMPONENTS_DST_Y_SRC_Y                                 (0x00000001)
#define NV85B5_SET_REMAP_COMPONENTS_DST_Y_SRC_Z                                 (0x00000002)
#define NV85B5_SET_REMAP_COMPONENTS_DST_Y_SRC_W                                 (0x00000003)
#define NV85B5_SET_REMAP_COMPONENTS_DST_Y_CONST_A                               (0x00000004)
#define NV85B5_SET_REMAP_COMPONENTS_DST_Y_CONST_B                               (0x00000005)
#define NV85B5_SET_REMAP_COMPONENTS_DST_Y_NO_WRITE                              (0x00000006)
#define NV85B5_SET_REMAP_COMPONENTS_DST_Z                                       10:8
#define NV85B5_SET_REMAP_COMPONENTS_DST_Z_SRC_X                                 (0x00000000)
#define NV85B5_SET_REMAP_COMPONENTS_DST_Z_SRC_Y                                 (0x00000001)
#define NV85B5_SET_REMAP_COMPONENTS_DST_Z_SRC_Z                                 (0x00000002)
#define NV85B5_SET_REMAP_COMPONENTS_DST_Z_SRC_W                                 (0x00000003)
#define NV85B5_SET_REMAP_COMPONENTS_DST_Z_CONST_A                               (0x00000004)
#define NV85B5_SET_REMAP_COMPONENTS_DST_Z_CONST_B                               (0x00000005)
#define NV85B5_SET_REMAP_COMPONENTS_DST_Z_NO_WRITE                              (0x00000006)
#define NV85B5_SET_REMAP_COMPONENTS_DST_W                                       14:12
#define NV85B5_SET_REMAP_COMPONENTS_DST_W_SRC_X                                 (0x00000000)
#define NV85B5_SET_REMAP_COMPONENTS_DST_W_SRC_Y                                 (0x00000001)
#define NV85B5_SET_REMAP_COMPONENTS_DST_W_SRC_Z                                 (0x00000002)
#define NV85B5_SET_REMAP_COMPONENTS_DST_W_SRC_W                                 (0x00000003)
#define NV85B5_SET_REMAP_COMPONENTS_DST_W_CONST_A                               (0x00000004)
#define NV85B5_SET_REMAP_COMPONENTS_DST_W_CONST_B                               (0x00000005)
#define NV85B5_SET_REMAP_COMPONENTS_DST_W_NO_WRITE                              (0x00000006)
#define NV85B5_SET_REMAP_COMPONENTS_COMPONENT_SIZE                              17:16
#define NV85B5_SET_REMAP_COMPONENTS_COMPONENT_SIZE_ONE                          (0x00000000)
#define NV85B5_SET_REMAP_COMPONENTS_COMPONENT_SIZE_TWO                          (0x00000001)
#define NV85B5_SET_REMAP_COMPONENTS_COMPONENT_SIZE_THREE                        (0x00000002)
#define NV85B5_SET_REMAP_COMPONENTS_COMPONENT_SIZE_FOUR                         (0x00000003)
#define NV85B5_SET_REMAP_COMPONENTS_NUM_SRC_COMPONENTS                          21:20
#define NV85B5_SET_REMAP_COMPONENTS_NUM_SRC_COMPONENTS_ONE                      (0x00000000)
#define NV85B5_SET_REMAP_COMPONENTS_NUM_SRC_COMPONENTS_TWO                      (0x00000001)
#define NV85B5_SET_REMAP_COMPONENTS_NUM_SRC_COMPONENTS_THREE                    (0x00000002)
#define NV85B5_SET_REMAP_COMPONENTS_NUM_SRC_COMPONENTS_FOUR                     (0x00000003)
#define NV85B5_SET_REMAP_COMPONENTS_NUM_DST_COMPONENTS                          25:24
#define NV85B5_SET_REMAP_COMPONENTS_NUM_DST_COMPONENTS_ONE                      (0x00000000)
#define NV85B5_SET_REMAP_COMPONENTS_NUM_DST_COMPONENTS_TWO                      (0x00000001)
#define NV85B5_SET_REMAP_COMPONENTS_NUM_DST_COMPONENTS_THREE                    (0x00000002)
#define NV85B5_SET_REMAP_COMPONENTS_NUM_DST_COMPONENTS_FOUR                     (0x00000003)
#define NV85B5_SET_DST_BLOCK_SIZE                                               (0x0000070C)
#define NV85B5_SET_DST_BLOCK_SIZE_WIDTH                                         3:0
#define NV85B5_SET_DST_BLOCK_SIZE_WIDTH_QUARTER_GOB                             (0x0000000E)
#define NV85B5_SET_DST_BLOCK_SIZE_WIDTH_ONE_GOB                                 (0x00000000)
#define NV85B5_SET_DST_BLOCK_SIZE_HEIGHT                                        7:4
#define NV85B5_SET_DST_BLOCK_SIZE_HEIGHT_ONE_GOB                                (0x00000000)
#define NV85B5_SET_DST_BLOCK_SIZE_HEIGHT_TWO_GOBS                               (0x00000001)
#define NV85B5_SET_DST_BLOCK_SIZE_HEIGHT_FOUR_GOBS                              (0x00000002)
#define NV85B5_SET_DST_BLOCK_SIZE_HEIGHT_EIGHT_GOBS                             (0x00000003)
#define NV85B5_SET_DST_BLOCK_SIZE_HEIGHT_SIXTEEN_GOBS                           (0x00000004)
#define NV85B5_SET_DST_BLOCK_SIZE_HEIGHT_THIRTYTWO_GOBS                         (0x00000005)
#define NV85B5_SET_DST_BLOCK_SIZE_DEPTH                                         11:8
#define NV85B5_SET_DST_BLOCK_SIZE_DEPTH_ONE_GOB                                 (0x00000000)
#define NV85B5_SET_DST_BLOCK_SIZE_DEPTH_TWO_GOBS                                (0x00000001)
#define NV85B5_SET_DST_BLOCK_SIZE_DEPTH_FOUR_GOBS                               (0x00000002)
#define NV85B5_SET_DST_BLOCK_SIZE_DEPTH_EIGHT_GOBS                              (0x00000003)
#define NV85B5_SET_DST_BLOCK_SIZE_DEPTH_SIXTEEN_GOBS                            (0x00000004)
#define NV85B5_SET_DST_BLOCK_SIZE_DEPTH_THIRTYTWO_GOBS                          (0x00000005)
#define NV85B5_SET_DST_BLOCK_SIZE_GOB_HEIGHT                                    15:12
#define NV85B5_SET_DST_BLOCK_SIZE_GOB_HEIGHT_GOB_HEIGHT_TESLA_4                 (0x00000000)
#define NV85B5_SET_DST_BLOCK_SIZE_GOB_HEIGHT_GOB_HEIGHT_FERMI_8                 (0x00000001)
#define NV85B5_SET_DST_WIDTH                                                    (0x00000710)
#define NV85B5_SET_DST_WIDTH_V                                                  31:0
#define NV85B5_SET_DST_HEIGHT                                                   (0x00000714)
#define NV85B5_SET_DST_HEIGHT_V                                                 31:0
#define NV85B5_SET_DST_DEPTH                                                    (0x00000718)
#define NV85B5_SET_DST_DEPTH_V                                                  31:0
#define NV85B5_SET_DST_LAYER                                                    (0x0000071C)
#define NV85B5_SET_DST_LAYER_V                                                  31:0
#define NV85B5_SET_DST_ORIGIN                                                   (0x00000720)
#define NV85B5_SET_DST_ORIGIN_X                                                 15:0
#define NV85B5_SET_DST_ORIGIN_Y                                                 31:16
#define NV85B5_SET_SRC_BLOCK_SIZE                                               (0x00000728)
#define NV85B5_SET_SRC_BLOCK_SIZE_WIDTH                                         3:0
#define NV85B5_SET_SRC_BLOCK_SIZE_WIDTH_QUARTER_GOB                             (0x0000000E)
#define NV85B5_SET_SRC_BLOCK_SIZE_WIDTH_ONE_GOB                                 (0x00000000)
#define NV85B5_SET_SRC_BLOCK_SIZE_HEIGHT                                        7:4
#define NV85B5_SET_SRC_BLOCK_SIZE_HEIGHT_ONE_GOB                                (0x00000000)
#define NV85B5_SET_SRC_BLOCK_SIZE_HEIGHT_TWO_GOBS                               (0x00000001)
#define NV85B5_SET_SRC_BLOCK_SIZE_HEIGHT_FOUR_GOBS                              (0x00000002)
#define NV85B5_SET_SRC_BLOCK_SIZE_HEIGHT_EIGHT_GOBS                             (0x00000003)
#define NV85B5_SET_SRC_BLOCK_SIZE_HEIGHT_SIXTEEN_GOBS                           (0x00000004)
#define NV85B5_SET_SRC_BLOCK_SIZE_HEIGHT_THIRTYTWO_GOBS                         (0x00000005)
#define NV85B5_SET_SRC_BLOCK_SIZE_DEPTH                                         11:8
#define NV85B5_SET_SRC_BLOCK_SIZE_DEPTH_ONE_GOB                                 (0x00000000)
#define NV85B5_SET_SRC_BLOCK_SIZE_DEPTH_TWO_GOBS                                (0x00000001)
#define NV85B5_SET_SRC_BLOCK_SIZE_DEPTH_FOUR_GOBS                               (0x00000002)
#define NV85B5_SET_SRC_BLOCK_SIZE_DEPTH_EIGHT_GOBS                              (0x00000003)
#define NV85B5_SET_SRC_BLOCK_SIZE_DEPTH_SIXTEEN_GOBS                            (0x00000004)
#define NV85B5_SET_SRC_BLOCK_SIZE_DEPTH_THIRTYTWO_GOBS                          (0x00000005)
#define NV85B5_SET_SRC_BLOCK_SIZE_GOB_HEIGHT                                    15:12
#define NV85B5_SET_SRC_BLOCK_SIZE_GOB_HEIGHT_GOB_HEIGHT_TESLA_4                 (0x00000000)
#define NV85B5_SET_SRC_BLOCK_SIZE_GOB_HEIGHT_GOB_HEIGHT_FERMI_8                 (0x00000001)
#define NV85B5_SET_SRC_WIDTH                                                    (0x0000072C)
#define NV85B5_SET_SRC_WIDTH_V                                                  31:0
#define NV85B5_SET_SRC_HEIGHT                                                   (0x00000730)
#define NV85B5_SET_SRC_HEIGHT_V                                                 31:0
#define NV85B5_SET_SRC_DEPTH                                                    (0x00000734)
#define NV85B5_SET_SRC_DEPTH_V                                                  31:0
#define NV85B5_SET_SRC_LAYER                                                    (0x00000738)
#define NV85B5_SET_SRC_LAYER_V                                                  31:0
#define NV85B5_SET_SRC_ORIGIN                                                   (0x0000073C)
#define NV85B5_SET_SRC_ORIGIN_X                                                 15:0
#define NV85B5_SET_SRC_ORIGIN_Y                                                 31:16
#define NV85B5_PM_TRIGGER_END                                                   (0x00001114)
#define NV85B5_PM_TRIGGER_END_V                                                 31:0

#ifdef __cplusplus
};     /* extern "C" */
#endif
#endif // _cl85b5_h

