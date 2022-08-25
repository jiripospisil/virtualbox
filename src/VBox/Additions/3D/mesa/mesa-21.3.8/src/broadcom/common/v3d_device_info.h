/*
 * Copyright © 2016 Broadcom
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

#ifndef V3D_CHIP_H
#define V3D_CHIP_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Struct for tracking features of the V3D chip across driver and compiler.
 */
struct v3d_device_info {
        /** Simple V3D version: major * 10 + minor */
        uint8_t ver;

        /** Size of the VPM, in bytes. */
        int vpm_size;

        /* NSLC * QUPS from the core's IDENT registers. */
        int qpu_count;
};

typedef int (*v3d_ioctl_fun)(int fd, unsigned long request, void *arg);

bool
v3d_get_device_info(int fd, struct v3d_device_info* devinfo, v3d_ioctl_fun fun);

#endif
