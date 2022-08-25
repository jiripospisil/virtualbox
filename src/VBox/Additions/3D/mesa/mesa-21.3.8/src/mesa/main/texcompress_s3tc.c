/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
 * Copyright (c) 2008 VMware, Inc.
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


/**
 * \file texcompress_s3tc.c
 * GL_EXT_texture_compression_s3tc support.
 */

#include "glheader.h"

#include "image.h"
#include "macros.h"
#include "mtypes.h"
#include "texcompress.h"
#include "texcompress_s3tc.h"
#include "texcompress_s3tc_tmp.h"
#include "texstore.h"
#include "format_unpack.h"
#include "util/format_srgb.h"


/**
 * Store user's image in rgb_dxt1 format.
 */
GLboolean
_mesa_texstore_rgb_dxt1(TEXSTORE_PARAMS)
{
   const GLubyte *pixels;
   GLubyte *dst;
   const GLubyte *tempImage = NULL;

   assert(dstFormat == MESA_FORMAT_RGB_DXT1 ||
          dstFormat == MESA_FORMAT_SRGB_DXT1);

   if (srcFormat != GL_RGB ||
       srcType != GL_UNSIGNED_BYTE ||
       ctx->_ImageTransferState ||
       ALIGN(srcPacking->RowLength, srcPacking->Alignment) != srcWidth ||
       srcPacking->SwapBytes) {
      /* convert image to RGB/GLubyte */
      GLubyte *tempImageSlices[1];
      int rgbRowStride = 3 * srcWidth * sizeof(GLubyte);
      tempImage = malloc(srcWidth * srcHeight * 3 * sizeof(GLubyte));
      if (!tempImage)
         return GL_FALSE; /* out of memory */
      tempImageSlices[0] = (GLubyte *) tempImage;
      _mesa_texstore(ctx, dims,
                     baseInternalFormat,
                     MESA_FORMAT_RGB_UNORM8,
                     rgbRowStride, tempImageSlices,
                     srcWidth, srcHeight, srcDepth,
                     srcFormat, srcType, srcAddr,
                     srcPacking);
      pixels = tempImage;
      srcFormat = GL_RGB;
   }
   else {
      pixels = _mesa_image_address2d(srcPacking, srcAddr, srcWidth, srcHeight,
                                     srcFormat, srcType, 0, 0);
   }

   dst = dstSlices[0];

   tx_compress_dxtn(3, srcWidth, srcHeight, pixels,
                    GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
                    dst, dstRowStride);

   free((void *) tempImage);

   return GL_TRUE;
}


/**
 * Store user's image in rgba_dxt1 format.
 */
GLboolean
_mesa_texstore_rgba_dxt1(TEXSTORE_PARAMS)
{
   const GLubyte *pixels;
   GLubyte *dst;
   const GLubyte *tempImage = NULL;

   assert(dstFormat == MESA_FORMAT_RGBA_DXT1 ||
          dstFormat == MESA_FORMAT_SRGBA_DXT1);

   if (srcFormat != GL_RGBA ||
       srcType != GL_UNSIGNED_BYTE ||
       ctx->_ImageTransferState ||
       ALIGN(srcPacking->RowLength, srcPacking->Alignment) != srcWidth ||
       srcPacking->SwapBytes) {
      /* convert image to RGBA/GLubyte */
      GLubyte *tempImageSlices[1];
      int rgbaRowStride = 4 * srcWidth * sizeof(GLubyte);
      tempImage = malloc(srcWidth * srcHeight * 4 * sizeof(GLubyte));
      if (!tempImage)
         return GL_FALSE; /* out of memory */
      tempImageSlices[0] = (GLubyte *) tempImage;
      _mesa_texstore(ctx, dims,
                     baseInternalFormat,
#if UTIL_ARCH_LITTLE_ENDIAN
                     MESA_FORMAT_R8G8B8A8_UNORM,
#else
                     MESA_FORMAT_A8B8G8R8_UNORM,
#endif
                     rgbaRowStride, tempImageSlices,
                     srcWidth, srcHeight, srcDepth,
                     srcFormat, srcType, srcAddr,
                     srcPacking);
      pixels = tempImage;
      srcFormat = GL_RGBA;
   }
   else {
      pixels = _mesa_image_address2d(srcPacking, srcAddr, srcWidth, srcHeight,
                                     srcFormat, srcType, 0, 0);
   }

   dst = dstSlices[0];

   tx_compress_dxtn(4, srcWidth, srcHeight, pixels,
                    GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
                    dst, dstRowStride);

   free((void*) tempImage);

   return GL_TRUE;
}


/**
 * Store user's image in rgba_dxt3 format.
 */
GLboolean
_mesa_texstore_rgba_dxt3(TEXSTORE_PARAMS)
{
   const GLubyte *pixels;
   GLubyte *dst;
   const GLubyte *tempImage = NULL;

   assert(dstFormat == MESA_FORMAT_RGBA_DXT3 ||
          dstFormat == MESA_FORMAT_SRGBA_DXT3);

   if (srcFormat != GL_RGBA ||
       srcType != GL_UNSIGNED_BYTE ||
       ctx->_ImageTransferState ||
       ALIGN(srcPacking->RowLength, srcPacking->Alignment) != srcWidth ||
       srcPacking->SwapBytes) {
      /* convert image to RGBA/GLubyte */
      GLubyte *tempImageSlices[1];
      int rgbaRowStride = 4 * srcWidth * sizeof(GLubyte);
      tempImage = malloc(srcWidth * srcHeight * 4 * sizeof(GLubyte));
      if (!tempImage)
         return GL_FALSE; /* out of memory */
      tempImageSlices[0] = (GLubyte *) tempImage;
      _mesa_texstore(ctx, dims,
                     baseInternalFormat,
#if UTIL_ARCH_LITTLE_ENDIAN
                     MESA_FORMAT_R8G8B8A8_UNORM,
#else
                     MESA_FORMAT_A8B8G8R8_UNORM,
#endif
                     rgbaRowStride, tempImageSlices,
                     srcWidth, srcHeight, srcDepth,
                     srcFormat, srcType, srcAddr,
                     srcPacking);
      pixels = tempImage;
   }
   else {
      pixels = _mesa_image_address2d(srcPacking, srcAddr, srcWidth, srcHeight,
                                     srcFormat, srcType, 0, 0);
   }

   dst = dstSlices[0];

   tx_compress_dxtn(4, srcWidth, srcHeight, pixels,
                    GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
                    dst, dstRowStride);

   free((void *) tempImage);

   return GL_TRUE;
}


/**
 * Store user's image in rgba_dxt5 format.
 */
GLboolean
_mesa_texstore_rgba_dxt5(TEXSTORE_PARAMS)
{
   const GLubyte *pixels;
   GLubyte *dst;
   const GLubyte *tempImage = NULL;

   assert(dstFormat == MESA_FORMAT_RGBA_DXT5 ||
          dstFormat == MESA_FORMAT_SRGBA_DXT5);

   if (srcFormat != GL_RGBA ||
       srcType != GL_UNSIGNED_BYTE ||
       ctx->_ImageTransferState ||
       ALIGN(srcPacking->RowLength, srcPacking->Alignment) != srcWidth ||
       srcPacking->SwapBytes) {
      /* convert image to RGBA/GLubyte */
      GLubyte *tempImageSlices[1];
      int rgbaRowStride = 4 * srcWidth * sizeof(GLubyte);
      tempImage = malloc(srcWidth * srcHeight * 4 * sizeof(GLubyte));
      if (!tempImage)
         return GL_FALSE; /* out of memory */
      tempImageSlices[0] = (GLubyte *) tempImage;
      _mesa_texstore(ctx, dims,
                     baseInternalFormat,
#if UTIL_ARCH_LITTLE_ENDIAN
                     MESA_FORMAT_R8G8B8A8_UNORM,
#else
                     MESA_FORMAT_A8B8G8R8_UNORM,
#endif
                     rgbaRowStride, tempImageSlices,
                     srcWidth, srcHeight, srcDepth,
                     srcFormat, srcType, srcAddr,
                     srcPacking);
      pixels = tempImage;
   }
   else {
      pixels = _mesa_image_address2d(srcPacking, srcAddr, srcWidth, srcHeight,
                                     srcFormat, srcType, 0, 0);
   }

   dst = dstSlices[0];

   tx_compress_dxtn(4, srcWidth, srcHeight, pixels,
                    GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
                    dst, dstRowStride);

   free((void *) tempImage);

   return GL_TRUE;
}


static void
fetch_rgb_dxt1(const GLubyte *map,
               GLint rowStride, GLint i, GLint j, GLfloat *texel)
{
   GLubyte tex[4];
   fetch_2d_texel_rgb_dxt1(rowStride, map, i, j, tex);
   texel[RCOMP] = UBYTE_TO_FLOAT(tex[RCOMP]);
   texel[GCOMP] = UBYTE_TO_FLOAT(tex[GCOMP]);
   texel[BCOMP] = UBYTE_TO_FLOAT(tex[BCOMP]);
   texel[ACOMP] = UBYTE_TO_FLOAT(tex[ACOMP]);
}

static void
fetch_rgba_dxt1(const GLubyte *map,
                GLint rowStride, GLint i, GLint j, GLfloat *texel)
{
   GLubyte tex[4];
   fetch_2d_texel_rgba_dxt1(rowStride, map, i, j, tex);
   texel[RCOMP] = UBYTE_TO_FLOAT(tex[RCOMP]);
   texel[GCOMP] = UBYTE_TO_FLOAT(tex[GCOMP]);
   texel[BCOMP] = UBYTE_TO_FLOAT(tex[BCOMP]);
   texel[ACOMP] = UBYTE_TO_FLOAT(tex[ACOMP]);
}

static void
fetch_rgba_dxt3(const GLubyte *map,
                GLint rowStride, GLint i, GLint j, GLfloat *texel)
{
   GLubyte tex[4];
   fetch_2d_texel_rgba_dxt3(rowStride, map, i, j, tex);
   texel[RCOMP] = UBYTE_TO_FLOAT(tex[RCOMP]);
   texel[GCOMP] = UBYTE_TO_FLOAT(tex[GCOMP]);
   texel[BCOMP] = UBYTE_TO_FLOAT(tex[BCOMP]);
   texel[ACOMP] = UBYTE_TO_FLOAT(tex[ACOMP]);
}

static void
fetch_rgba_dxt5(const GLubyte *map,
                GLint rowStride, GLint i, GLint j, GLfloat *texel)
{
   GLubyte tex[4];
   fetch_2d_texel_rgba_dxt5(rowStride, map, i, j, tex);
   texel[RCOMP] = UBYTE_TO_FLOAT(tex[RCOMP]);
   texel[GCOMP] = UBYTE_TO_FLOAT(tex[GCOMP]);
   texel[BCOMP] = UBYTE_TO_FLOAT(tex[BCOMP]);
   texel[ACOMP] = UBYTE_TO_FLOAT(tex[ACOMP]);
}


static void
fetch_srgb_dxt1(const GLubyte *map,
                GLint rowStride, GLint i, GLint j, GLfloat *texel)
{
   GLubyte tex[4];
   fetch_2d_texel_rgb_dxt1(rowStride, map, i, j, tex);
   texel[RCOMP] = util_format_srgb_8unorm_to_linear_float(tex[RCOMP]);
   texel[GCOMP] = util_format_srgb_8unorm_to_linear_float(tex[GCOMP]);
   texel[BCOMP] = util_format_srgb_8unorm_to_linear_float(tex[BCOMP]);
   texel[ACOMP] = UBYTE_TO_FLOAT(tex[ACOMP]);
}

static void
fetch_srgba_dxt1(const GLubyte *map,
                 GLint rowStride, GLint i, GLint j, GLfloat *texel)
{
   GLubyte tex[4];
   fetch_2d_texel_rgba_dxt1(rowStride, map, i, j, tex);
   texel[RCOMP] = util_format_srgb_8unorm_to_linear_float(tex[RCOMP]);
   texel[GCOMP] = util_format_srgb_8unorm_to_linear_float(tex[GCOMP]);
   texel[BCOMP] = util_format_srgb_8unorm_to_linear_float(tex[BCOMP]);
   texel[ACOMP] = UBYTE_TO_FLOAT(tex[ACOMP]);
}

static void
fetch_srgba_dxt3(const GLubyte *map,
                 GLint rowStride, GLint i, GLint j, GLfloat *texel)
{
   GLubyte tex[4];
   fetch_2d_texel_rgba_dxt3(rowStride, map, i, j, tex);
   texel[RCOMP] = util_format_srgb_8unorm_to_linear_float(tex[RCOMP]);
   texel[GCOMP] = util_format_srgb_8unorm_to_linear_float(tex[GCOMP]);
   texel[BCOMP] = util_format_srgb_8unorm_to_linear_float(tex[BCOMP]);
   texel[ACOMP] = UBYTE_TO_FLOAT(tex[ACOMP]);
}

static void
fetch_srgba_dxt5(const GLubyte *map,
                 GLint rowStride, GLint i, GLint j, GLfloat *texel)
{
   GLubyte tex[4];
   fetch_2d_texel_rgba_dxt5(rowStride, map, i, j, tex);
   texel[RCOMP] = util_format_srgb_8unorm_to_linear_float(tex[RCOMP]);
   texel[GCOMP] = util_format_srgb_8unorm_to_linear_float(tex[GCOMP]);
   texel[BCOMP] = util_format_srgb_8unorm_to_linear_float(tex[BCOMP]);
   texel[ACOMP] = UBYTE_TO_FLOAT(tex[ACOMP]);
}



compressed_fetch_func
_mesa_get_dxt_fetch_func(mesa_format format)
{
   switch (format) {
   case MESA_FORMAT_RGB_DXT1:
      return fetch_rgb_dxt1;
   case MESA_FORMAT_RGBA_DXT1:
      return fetch_rgba_dxt1;
   case MESA_FORMAT_RGBA_DXT3:
      return fetch_rgba_dxt3;
   case MESA_FORMAT_RGBA_DXT5:
      return fetch_rgba_dxt5;
   case MESA_FORMAT_SRGB_DXT1:
      return fetch_srgb_dxt1;
   case MESA_FORMAT_SRGBA_DXT1:
      return fetch_srgba_dxt1;
   case MESA_FORMAT_SRGBA_DXT3:
      return fetch_srgba_dxt3;
   case MESA_FORMAT_SRGBA_DXT5:
      return fetch_srgba_dxt5;
   default:
      return NULL;
   }
}
