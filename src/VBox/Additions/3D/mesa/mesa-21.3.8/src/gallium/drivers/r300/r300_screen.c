/*
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
 * Copyright 2010 Marek Olšák <maraeo@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include "util/format/u_format.h"
#include "util/format/u_format_s3tc.h"
#include "util/u_screen.h"
#include "util/u_memory.h"
#include "util/os_time.h"
#include "vl/vl_decoder.h"
#include "vl/vl_video_buffer.h"

#include "r300_context.h"
#include "r300_texture.h"
#include "r300_screen_buffer.h"
#include "r300_state_inlines.h"
#include "r300_public.h"

#include "draw/draw_context.h"

/* Return the identifier behind whom the brave coders responsible for this
 * amalgamation of code, sweat, and duct tape, routinely obscure their names.
 *
 * ...I should have just put "Corbin Simpson", but I'm not that cool.
 *
 * (Or egotistical. Yet.) */
static const char* r300_get_vendor(struct pipe_screen* pscreen)
{
    return "X.Org R300 Project";
}

static const char* r300_get_device_vendor(struct pipe_screen* pscreen)
{
    return "ATI";
}

static const char* chip_families[] = {
    "unknown",
    "ATI R300",
    "ATI R350",
    "ATI RV350",
    "ATI RV370",
    "ATI RV380",
    "ATI RS400",
    "ATI RC410",
    "ATI RS480",
    "ATI R420",
    "ATI R423",
    "ATI R430",
    "ATI R480",
    "ATI R481",
    "ATI RV410",
    "ATI RS600",
    "ATI RS690",
    "ATI RS740",
    "ATI RV515",
    "ATI R520",
    "ATI RV530",
    "ATI R580",
    "ATI RV560",
    "ATI RV570"
};

static const char* r300_get_family_name(struct r300_screen* r300screen)
{
    return chip_families[r300screen->caps.family];
}

static const char* r300_get_name(struct pipe_screen* pscreen)
{
    struct r300_screen* r300screen = r300_screen(pscreen);

    return r300_get_family_name(r300screen);
}

static void r300_disk_cache_create(struct r300_screen* r300screen)
{
    struct mesa_sha1 ctx;
    unsigned char sha1[20];
    char cache_id[20 * 2 + 1];

    _mesa_sha1_init(&ctx);
    if (!disk_cache_get_function_identifier(r300_disk_cache_create,
                                            &ctx))
        return;

    _mesa_sha1_final(&ctx, sha1);
    disk_cache_format_hex_id(cache_id, sha1, 20 * 2);

    r300screen->disk_shader_cache =
                    disk_cache_create(r300_get_family_name(r300screen),
                                      cache_id,
                                      r300screen->debug);
}

static struct disk_cache* r300_get_disk_shader_cache(struct pipe_screen* pscreen)
{
	struct r300_screen* r300screen = r300_screen(pscreen);
	return r300screen->disk_shader_cache;
}

static int r300_get_param(struct pipe_screen* pscreen, enum pipe_cap param)
{
    struct r300_screen* r300screen = r300_screen(pscreen);
    boolean is_r500 = r300screen->caps.is_r500;

    switch (param) {
        /* Supported features (boolean caps). */
        case PIPE_CAP_NPOT_TEXTURES:
        case PIPE_CAP_MIXED_FRAMEBUFFER_SIZES:
        case PIPE_CAP_MIXED_COLOR_DEPTH_BITS:
        case PIPE_CAP_ANISOTROPIC_FILTER:
        case PIPE_CAP_POINT_SPRITE:
        case PIPE_CAP_OCCLUSION_QUERY:
        case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
        case PIPE_CAP_TEXTURE_MIRROR_CLAMP_TO_EDGE:
        case PIPE_CAP_BLEND_EQUATION_SEPARATE:
        case PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR:
        case PIPE_CAP_TGSI_FS_COORD_ORIGIN_UPPER_LEFT:
        case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
        case PIPE_CAP_CONDITIONAL_RENDER:
        case PIPE_CAP_TEXTURE_BARRIER:
        case PIPE_CAP_TGSI_CAN_COMPACT_CONSTANTS:
        case PIPE_CAP_PREFER_BLIT_BASED_TEXTURE_TRANSFER:
        case PIPE_CAP_BUFFER_MAP_PERSISTENT_COHERENT:
        case PIPE_CAP_CLIP_HALFZ:
        case PIPE_CAP_ALLOW_MAPPED_BUFFERS_DURING_EXECUTION:
            return 1;

        case PIPE_CAP_MIN_MAP_BUFFER_ALIGNMENT:
            return R300_BUFFER_ALIGNMENT;

        case PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT:
            return 16;

        case PIPE_CAP_GLSL_FEATURE_LEVEL:
        case PIPE_CAP_GLSL_FEATURE_LEVEL_COMPATIBILITY:
            return 120;

        /* r300 cannot do swizzling of compressed textures. Supported otherwise. */
        case PIPE_CAP_TEXTURE_SWIZZLE:
            return r300screen->caps.dxtc_swizzle;

        /* We don't support color clamping on r500, so that we can use color
         * interpolators for generic varyings. */
        case PIPE_CAP_VERTEX_COLOR_CLAMPED:
            return !is_r500;

        /* Supported on r500 only. */
        case PIPE_CAP_VERTEX_COLOR_UNCLAMPED:
        case PIPE_CAP_MIXED_COLORBUFFER_FORMATS:
        case PIPE_CAP_FRAGMENT_SHADER_TEXTURE_LOD:
        case PIPE_CAP_FRAGMENT_SHADER_DERIVATIVES:
        case PIPE_CAP_VERTEX_SHADER_SATURATE:
            return is_r500 ? 1 : 0;

        case PIPE_CAP_GLSL_OPTIMIZE_CONSERVATIVELY:
            return 0;
        case PIPE_CAP_SHAREABLE_SHADERS:
            return 0;

        case PIPE_CAP_MAX_GS_INVOCATIONS:
            return 32;
       case PIPE_CAP_MAX_SHADER_BUFFER_SIZE:
            return 1 << 27;

        /* SWTCL-only features. */
        case PIPE_CAP_PRIMITIVE_RESTART:
        case PIPE_CAP_PRIMITIVE_RESTART_FIXED_INDEX:
        case PIPE_CAP_USER_VERTEX_BUFFERS:
        case PIPE_CAP_TGSI_VS_WINDOW_SPACE_POSITION:
            return !r300screen->caps.has_tcl;

        /* HWTCL-only features / limitations. */
        case PIPE_CAP_VERTEX_BUFFER_OFFSET_4BYTE_ALIGNED_ONLY:
        case PIPE_CAP_VERTEX_BUFFER_STRIDE_4BYTE_ALIGNED_ONLY:
        case PIPE_CAP_VERTEX_ELEMENT_SRC_OFFSET_4BYTE_ALIGNED_ONLY:
            return r300screen->caps.has_tcl;

        /* Texturing. */
        case PIPE_CAP_MAX_TEXTURE_2D_SIZE:
            return is_r500 ? 4096 : 2048;
        case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
        case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
            /* 13 == 4096, 12 == 2048 */
            return is_r500 ? 13 : 12;

        /* Render targets. */
        case PIPE_CAP_MAX_RENDER_TARGETS:
            return 4;
        case PIPE_CAP_ENDIANNESS:
            return PIPE_ENDIAN_LITTLE;

        case PIPE_CAP_MAX_VIEWPORTS:
            return 1;

        case PIPE_CAP_MAX_VERTEX_ATTRIB_STRIDE:
            return 2048;

        case PIPE_CAP_MAX_VARYINGS:
            return 10;

        case PIPE_CAP_VENDOR_ID:
                return 0x1002;
        case PIPE_CAP_DEVICE_ID:
                return r300screen->info.pci_id;
        case PIPE_CAP_ACCELERATED:
                return 1;
        case PIPE_CAP_VIDEO_MEMORY:
                return r300screen->info.vram_size >> 20;
        case PIPE_CAP_UMA:
                return 0;
        case PIPE_CAP_PCI_GROUP:
            return r300screen->info.pci_domain;
        case PIPE_CAP_PCI_BUS:
            return r300screen->info.pci_bus;
        case PIPE_CAP_PCI_DEVICE:
            return r300screen->info.pci_dev;
        case PIPE_CAP_PCI_FUNCTION:
            return r300screen->info.pci_func;
        default:
            return u_pipe_screen_get_param_defaults(pscreen, param);
    }
}

static int r300_get_shader_param(struct pipe_screen *pscreen,
                                 enum pipe_shader_type shader,
                                 enum pipe_shader_cap param)
{
   struct r300_screen* r300screen = r300_screen(pscreen);
   boolean is_r400 = r300screen->caps.is_r400;
   boolean is_r500 = r300screen->caps.is_r500;

   switch (shader) {
    case PIPE_SHADER_FRAGMENT:
        switch (param)
        {
        case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
            return is_r500 || is_r400 ? 512 : 96;
        case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
            return is_r500 || is_r400 ? 512 : 64;
        case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
            return is_r500 || is_r400 ? 512 : 32;
        case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
            return is_r500 ? 511 : 4;
        case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
            return is_r500 ? 64 : 0; /* Actually unlimited on r500. */
            /* Fragment shader limits. */
        case PIPE_SHADER_CAP_MAX_INPUTS:
            /* 2 colors + 8 texcoords are always supported
             * (minus fog and wpos).
             *
             * R500 has the ability to turn 3rd and 4th color into
             * additional texcoords but there is no two-sided color
             * selection then. However the facing bit can be used instead. */
            return 10;
        case PIPE_SHADER_CAP_MAX_OUTPUTS:
            return 4;
        case PIPE_SHADER_CAP_MAX_CONST_BUFFER_SIZE:
            return (is_r500 ? 256 : 32) * sizeof(float[4]);
        case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
        case PIPE_SHADER_CAP_TGSI_ANY_INOUT_DECL_RANGE:
            return 1;
        case PIPE_SHADER_CAP_MAX_TEMPS:
            return is_r500 ? 128 : is_r400 ? 64 : 32;
        case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
        case PIPE_SHADER_CAP_MAX_SAMPLER_VIEWS:
           return r300screen->caps.num_tex_units;
        case PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED:
        case PIPE_SHADER_CAP_TGSI_SQRT_SUPPORTED:
        case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
        case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
        case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
        case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
        case PIPE_SHADER_CAP_SUBROUTINES:
        case PIPE_SHADER_CAP_INTEGERS:
        case PIPE_SHADER_CAP_INT64_ATOMICS:
        case PIPE_SHADER_CAP_FP16:
        case PIPE_SHADER_CAP_FP16_DERIVATIVES:
        case PIPE_SHADER_CAP_FP16_CONST_BUFFERS:
        case PIPE_SHADER_CAP_INT16:
        case PIPE_SHADER_CAP_GLSL_16BIT_CONSTS:
        case PIPE_SHADER_CAP_TGSI_DROUND_SUPPORTED:
        case PIPE_SHADER_CAP_TGSI_DFRACEXP_DLDEXP_SUPPORTED:
        case PIPE_SHADER_CAP_TGSI_LDEXP_SUPPORTED:
        case PIPE_SHADER_CAP_TGSI_FMA_SUPPORTED:
        case PIPE_SHADER_CAP_MAX_SHADER_BUFFERS:
        case PIPE_SHADER_CAP_MAX_SHADER_IMAGES:
        case PIPE_SHADER_CAP_LOWER_IF_THRESHOLD:
        case PIPE_SHADER_CAP_TGSI_SKIP_MERGE_REGISTERS:
        case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTERS:
        case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTER_BUFFERS:
            return 0;
        case PIPE_SHADER_CAP_MAX_UNROLL_ITERATIONS_HINT:
            return 32;
        case PIPE_SHADER_CAP_PREFERRED_IR:
            return PIPE_SHADER_IR_TGSI;
        case PIPE_SHADER_CAP_SUPPORTED_IRS:
            return 1 << PIPE_SHADER_IR_TGSI;
        }
        break;
    case PIPE_SHADER_VERTEX:
        switch (param)
        {
        case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
        case PIPE_SHADER_CAP_MAX_SAMPLER_VIEWS:
        case PIPE_SHADER_CAP_SUBROUTINES:
            return 0;
        default:;
        }

        if (!r300screen->caps.has_tcl) {
            switch (param) {
            case PIPE_SHADER_CAP_MAX_SHADER_BUFFERS:
            case PIPE_SHADER_CAP_MAX_SHADER_IMAGES:
                return 0;
            default:
                return draw_get_shader_param(shader, param);
            }
        }

        switch (param)
        {
        case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
        case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
            return is_r500 ? 1024 : 256;
        case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
            return is_r500 ? 4 : 0; /* For loops; not sure about conditionals. */
        case PIPE_SHADER_CAP_MAX_INPUTS:
            return 16;
        case PIPE_SHADER_CAP_MAX_OUTPUTS:
            return 10;
        case PIPE_SHADER_CAP_MAX_CONST_BUFFER_SIZE:
            return 256 * sizeof(float[4]);
        case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
            return 1;
        case PIPE_SHADER_CAP_MAX_TEMPS:
            return 32;
        case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
        case PIPE_SHADER_CAP_TGSI_ANY_INOUT_DECL_RANGE:
            return 1;
        case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
        case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
        case PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED:
        case PIPE_SHADER_CAP_TGSI_SQRT_SUPPORTED:
        case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
        case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
        case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
        case PIPE_SHADER_CAP_SUBROUTINES:
        case PIPE_SHADER_CAP_INTEGERS:
        case PIPE_SHADER_CAP_FP16:
        case PIPE_SHADER_CAP_FP16_CONST_BUFFERS:
        case PIPE_SHADER_CAP_FP16_DERIVATIVES:
        case PIPE_SHADER_CAP_INT16:
        case PIPE_SHADER_CAP_GLSL_16BIT_CONSTS:
        case PIPE_SHADER_CAP_INT64_ATOMICS:
        case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
        case PIPE_SHADER_CAP_MAX_SAMPLER_VIEWS:
        case PIPE_SHADER_CAP_TGSI_DROUND_SUPPORTED:
        case PIPE_SHADER_CAP_TGSI_DFRACEXP_DLDEXP_SUPPORTED:
        case PIPE_SHADER_CAP_TGSI_LDEXP_SUPPORTED:
        case PIPE_SHADER_CAP_TGSI_FMA_SUPPORTED:
        case PIPE_SHADER_CAP_MAX_SHADER_BUFFERS:
        case PIPE_SHADER_CAP_MAX_SHADER_IMAGES:
        case PIPE_SHADER_CAP_LOWER_IF_THRESHOLD:
        case PIPE_SHADER_CAP_TGSI_SKIP_MERGE_REGISTERS:
        case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTERS:
        case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTER_BUFFERS:
            return 0;
        case PIPE_SHADER_CAP_MAX_UNROLL_ITERATIONS_HINT:
            return 32;
        case PIPE_SHADER_CAP_PREFERRED_IR:
            return PIPE_SHADER_IR_TGSI;
        case PIPE_SHADER_CAP_SUPPORTED_IRS:
            return 1 << PIPE_SHADER_IR_TGSI;
        }
        break;
    default:
        ; /* nothing */
    }
    return 0;
}

static float r300_get_paramf(struct pipe_screen* pscreen,
                             enum pipe_capf param)
{
    struct r300_screen* r300screen = r300_screen(pscreen);

    switch (param) {
        case PIPE_CAPF_MAX_LINE_WIDTH:
        case PIPE_CAPF_MAX_LINE_WIDTH_AA:
        case PIPE_CAPF_MAX_POINT_WIDTH:
        case PIPE_CAPF_MAX_POINT_WIDTH_AA:
            /* The maximum dimensions of the colorbuffer are our practical
             * rendering limits. 2048 pixels should be enough for anybody. */
            if (r300screen->caps.is_r500) {
                return 4096.0f;
            } else if (r300screen->caps.is_r400) {
                return 4021.0f;
            } else {
                return 2560.0f;
            }
        case PIPE_CAPF_MAX_TEXTURE_ANISOTROPY:
            return 16.0f;
        case PIPE_CAPF_MAX_TEXTURE_LOD_BIAS:
            return 16.0f;
        case PIPE_CAPF_MIN_CONSERVATIVE_RASTER_DILATE:
        case PIPE_CAPF_MAX_CONSERVATIVE_RASTER_DILATE:
        case PIPE_CAPF_CONSERVATIVE_RASTER_DILATE_GRANULARITY:
            return 0.0f;
        default:
            debug_printf("r300: Warning: Unknown CAP %d in get_paramf.\n",
                         param);
            return 0.0f;
    }
}

static int r300_get_video_param(struct pipe_screen *screen,
				enum pipe_video_profile profile,
				enum pipe_video_entrypoint entrypoint,
				enum pipe_video_cap param)
{
   switch (param) {
      case PIPE_VIDEO_CAP_SUPPORTED:
         return vl_profile_supported(screen, profile, entrypoint);
      case PIPE_VIDEO_CAP_NPOT_TEXTURES:
         return 0;
      case PIPE_VIDEO_CAP_MAX_WIDTH:
      case PIPE_VIDEO_CAP_MAX_HEIGHT:
         return vl_video_buffer_max_size(screen);
      case PIPE_VIDEO_CAP_PREFERED_FORMAT:
         return PIPE_FORMAT_NV12;
      case PIPE_VIDEO_CAP_PREFERS_INTERLACED:
         return false;
      case PIPE_VIDEO_CAP_SUPPORTS_INTERLACED:
         return false;
      case PIPE_VIDEO_CAP_SUPPORTS_PROGRESSIVE:
         return true;
      case PIPE_VIDEO_CAP_MAX_LEVEL:
         return vl_level_supported(screen, profile);
      default:
         return 0;
   }
}

/**
 * Whether the format matches:
 *   PIPE_FORMAT_?10?10?10?2_UNORM
 */
static inline boolean
util_format_is_rgba1010102_variant(const struct util_format_description *desc)
{
   static const unsigned size[4] = {10, 10, 10, 2};
   unsigned chan;

   if (desc->block.width != 1 ||
       desc->block.height != 1 ||
       desc->block.bits != 32)
      return FALSE;

   for (chan = 0; chan < 4; ++chan) {
      if(desc->channel[chan].type != UTIL_FORMAT_TYPE_UNSIGNED &&
         desc->channel[chan].type != UTIL_FORMAT_TYPE_VOID)
         return FALSE;
      if (desc->channel[chan].size != size[chan])
         return FALSE;
   }

   return TRUE;
}

static bool r300_is_blending_supported(struct r300_screen *rscreen,
                                       enum pipe_format format)
{
    int c;
    const struct util_format_description *desc =
        util_format_description(format);

    if (desc->layout != UTIL_FORMAT_LAYOUT_PLAIN)
        return false;

    c = util_format_get_first_non_void_channel(format);

    /* RGBA16F */
    if (rscreen->caps.is_r500 &&
        desc->nr_channels == 4 &&
        desc->channel[c].size == 16 &&
        desc->channel[c].type == UTIL_FORMAT_TYPE_FLOAT)
        return true;

    if (desc->channel[c].normalized &&
        desc->channel[c].type == UTIL_FORMAT_TYPE_UNSIGNED &&
        desc->channel[c].size >= 4 &&
        desc->channel[c].size <= 10) {
        /* RGB10_A2, RGBA8, RGB5_A1, RGBA4, RGB565 */
        if (desc->nr_channels >= 3)
            return true;

        if (format == PIPE_FORMAT_R8G8_UNORM)
            return true;

        /* R8, I8, L8, A8 */
        if (desc->nr_channels == 1)
            return true;
    }

    return false;
}

static bool r300_is_format_supported(struct pipe_screen* screen,
                                     enum pipe_format format,
                                     enum pipe_texture_target target,
                                     unsigned sample_count,
                                     unsigned storage_sample_count,
                                     unsigned usage)
{
    uint32_t retval = 0;
    boolean is_r500 = r300_screen(screen)->caps.is_r500;
    boolean is_r400 = r300_screen(screen)->caps.is_r400;
    boolean is_color2101010 = format == PIPE_FORMAT_R10G10B10A2_UNORM ||
                              format == PIPE_FORMAT_R10G10B10X2_SNORM ||
                              format == PIPE_FORMAT_B10G10R10A2_UNORM ||
                              format == PIPE_FORMAT_B10G10R10X2_UNORM ||
                              format == PIPE_FORMAT_R10SG10SB10SA2U_NORM;
    boolean is_ati1n = format == PIPE_FORMAT_RGTC1_UNORM ||
                       format == PIPE_FORMAT_RGTC1_SNORM ||
                       format == PIPE_FORMAT_LATC1_UNORM ||
                       format == PIPE_FORMAT_LATC1_SNORM;
    boolean is_ati2n = format == PIPE_FORMAT_RGTC2_UNORM ||
                       format == PIPE_FORMAT_RGTC2_SNORM ||
                       format == PIPE_FORMAT_LATC2_UNORM ||
                       format == PIPE_FORMAT_LATC2_SNORM;
    boolean is_half_float = format == PIPE_FORMAT_R16_FLOAT ||
                            format == PIPE_FORMAT_R16G16_FLOAT ||
                            format == PIPE_FORMAT_R16G16B16_FLOAT ||
                            format == PIPE_FORMAT_R16G16B16A16_FLOAT ||
                            format == PIPE_FORMAT_R16G16B16X16_FLOAT;
    const struct util_format_description *desc;

    if (MAX2(1, sample_count) != MAX2(1, storage_sample_count))
        return false;

    /* Check multisampling support. */
    switch (sample_count) {
        case 0:
        case 1:
            break;
        case 2:
        case 4:
        case 6:
            /* No texturing and scanout. */
            if (usage & (PIPE_BIND_SAMPLER_VIEW |
                         PIPE_BIND_DISPLAY_TARGET |
                         PIPE_BIND_SCANOUT)) {
                return false;
            }

            desc = util_format_description(format);

            if (is_r500) {
                /* Only allow depth/stencil, RGBA8, RGBA1010102, RGBA16F. */
                if (!util_format_is_depth_or_stencil(format) &&
                    !util_format_is_rgba8_variant(desc) &&
                    !util_format_is_rgba1010102_variant(desc) &&
                    format != PIPE_FORMAT_R16G16B16A16_FLOAT &&
                    format != PIPE_FORMAT_R16G16B16X16_FLOAT) {
                    return false;
                }
            } else {
                /* Only allow depth/stencil, RGBA8. */
                if (!util_format_is_depth_or_stencil(format) &&
                    !util_format_is_rgba8_variant(desc)) {
                    return false;
                }
            }
            break;
        default:
            return false;
    }

    /* Check sampler format support. */
    if ((usage & PIPE_BIND_SAMPLER_VIEW) &&
        /* these two are broken for an unknown reason */
        format != PIPE_FORMAT_R8G8B8X8_SNORM &&
        format != PIPE_FORMAT_R16G16B16X16_SNORM &&
        /* ATI1N is r5xx-only. */
        (is_r500 || !is_ati1n) &&
        /* ATI2N is supported on r4xx-r5xx. */
        (is_r400 || is_r500 || !is_ati2n) &&
        r300_is_sampler_format_supported(format)) {
        retval |= PIPE_BIND_SAMPLER_VIEW;
    }

    /* Check colorbuffer format support. */
    if ((usage & (PIPE_BIND_RENDER_TARGET |
                  PIPE_BIND_DISPLAY_TARGET |
                  PIPE_BIND_SCANOUT |
                  PIPE_BIND_SHARED |
                  PIPE_BIND_BLENDABLE)) &&
        /* 2101010 cannot be rendered to on non-r5xx. */
        (!is_color2101010 || is_r500) &&
        r300_is_colorbuffer_format_supported(format)) {
        retval |= usage &
            (PIPE_BIND_RENDER_TARGET |
             PIPE_BIND_DISPLAY_TARGET |
             PIPE_BIND_SCANOUT |
             PIPE_BIND_SHARED);

        if (r300_is_blending_supported(r300_screen(screen), format)) {
            retval |= usage & PIPE_BIND_BLENDABLE;
        }
    }

    /* Check depth-stencil format support. */
    if (usage & PIPE_BIND_DEPTH_STENCIL &&
        r300_is_zs_format_supported(format)) {
        retval |= PIPE_BIND_DEPTH_STENCIL;
    }

    /* Check vertex buffer format support. */
    if (usage & PIPE_BIND_VERTEX_BUFFER) {
        if (r300_screen(screen)->caps.has_tcl) {
            /* Half float is supported on >= R400. */
            if ((is_r400 || is_r500 || !is_half_float) &&
                r300_translate_vertex_data_type(format) != R300_INVALID_FORMAT) {
                retval |= PIPE_BIND_VERTEX_BUFFER;
            }
        } else {
            /* SW TCL */
            if (!util_format_is_pure_integer(format)) {
                retval |= PIPE_BIND_VERTEX_BUFFER;
            }
        }
    }

    if (usage & PIPE_BIND_INDEX_BUFFER) {
       if (format == PIPE_FORMAT_R8_UINT ||
           format == PIPE_FORMAT_R16_UINT ||
           format == PIPE_FORMAT_R32_UINT)
          retval |= PIPE_BIND_INDEX_BUFFER;
    }

    return retval == usage;
}

static void r300_destroy_screen(struct pipe_screen* pscreen)
{
    struct r300_screen* r300screen = r300_screen(pscreen);
    struct radeon_winsys *rws = radeon_winsys(pscreen);

    if (rws && !rws->unref(rws))
      return;

    mtx_destroy(&r300screen->cmask_mutex);
    slab_destroy_parent(&r300screen->pool_transfers);

    disk_cache_destroy(r300screen->disk_shader_cache);

    if (rws)
      rws->destroy(rws);

    FREE(r300screen);
}

static void r300_fence_reference(struct pipe_screen *screen,
                                 struct pipe_fence_handle **ptr,
                                 struct pipe_fence_handle *fence)
{
    struct radeon_winsys *rws = r300_screen(screen)->rws;

    rws->fence_reference(ptr, fence);
}

static bool r300_fence_finish(struct pipe_screen *screen,
                              struct pipe_context *ctx,
                              struct pipe_fence_handle *fence,
                              uint64_t timeout)
{
    struct radeon_winsys *rws = r300_screen(screen)->rws;

    return rws->fence_wait(rws, fence, timeout);
}

struct pipe_screen* r300_screen_create(struct radeon_winsys *rws,
                                       const struct pipe_screen_config *config)
{
    struct r300_screen *r300screen = CALLOC_STRUCT(r300_screen);

    if (!r300screen) {
        FREE(r300screen);
        return NULL;
    }

    rws->query_info(rws, &r300screen->info, false, false);

    r300_init_debug(r300screen);
    r300_parse_chipset(r300screen->info.pci_id, &r300screen->caps);

    if (SCREEN_DBG_ON(r300screen, DBG_NO_ZMASK))
        r300screen->caps.zmask_ram = 0;
    if (SCREEN_DBG_ON(r300screen, DBG_NO_HIZ))
        r300screen->caps.hiz_ram = 0;

    r300screen->rws = rws;
    r300screen->screen.destroy = r300_destroy_screen;
    r300screen->screen.get_name = r300_get_name;
    r300screen->screen.get_vendor = r300_get_vendor;
    r300screen->screen.get_device_vendor = r300_get_device_vendor;
    r300screen->screen.get_disk_shader_cache = r300_get_disk_shader_cache;
    r300screen->screen.get_param = r300_get_param;
    r300screen->screen.get_shader_param = r300_get_shader_param;
    r300screen->screen.get_paramf = r300_get_paramf;
    r300screen->screen.get_video_param = r300_get_video_param;
    r300screen->screen.is_format_supported = r300_is_format_supported;
    r300screen->screen.is_video_format_supported = vl_video_buffer_is_format_supported;
    r300screen->screen.context_create = r300_create_context;
    r300screen->screen.fence_reference = r300_fence_reference;
    r300screen->screen.fence_finish = r300_fence_finish;

    r300_init_screen_resource_functions(r300screen);

    r300_disk_cache_create(r300screen);

    slab_create_parent(&r300screen->pool_transfers, sizeof(struct pipe_transfer), 64);

    (void) mtx_init(&r300screen->cmask_mutex, mtx_plain);

    return &r300screen->screen;
}
