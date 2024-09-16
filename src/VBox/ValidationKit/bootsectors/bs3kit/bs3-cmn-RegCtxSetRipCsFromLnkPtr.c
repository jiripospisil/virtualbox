/* $Id$ */
/** @file
 * BS3Kit - Bs3RegCtxSetRipCsFromLnkPtr
 */

/*
 * Copyright (C) 2007-2024 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include "bs3kit-template-header.h"


#undef Bs3RegCtxSetRipCsFromLnkPtr
BS3_CMN_DEF(void, Bs3RegCtxSetRipCsFromLnkPtr,(PBS3REGCTX pRegCtx, FPFNBS3FAR pfnCode))
{
    if (BS3_MODE_IS_16BIT_CODE(pRegCtx->bMode))
    {
#if ARCH_BITS == 16
        pRegCtx->rip.u = BS3_FP_OFF(pfnCode);
        if (BS3_MODE_IS_RM_OR_V86(pRegCtx->bMode))
            pRegCtx->cs = BS3_FP_SEG(pfnCode);
        else
            pRegCtx->cs = Bs3SelRealModeCodeToProtMode(BS3_FP_SEG(pfnCode));
#else
        uint32_t uFar1616;
        if (BS3_MODE_IS_RM_OR_V86(pRegCtx->bMode))
            uFar1616 = Bs3SelFlatCodeToRealMode((uint32_t)(uintptr_t)pfnCode);
        else
            uFar1616 = Bs3SelFlatCodeToProtFar16((uint32_t)(uintptr_t)pfnCode);
        pRegCtx->rip.u = uFar1616 & UINT16_MAX;
        pRegCtx->cs    = uFar1616 >> 16;
#endif
    }
    else
    {
#if ARCH_BITS == 16
        pRegCtx->rip.u = Bs3SelRealModeCodeToFlat(pfnCode);
#else
        pRegCtx->rip.u = (uintptr_t)pfnCode;
#endif
        if (BS3_MODE_IS_32BIT_CODE(pRegCtx->bMode))
            pRegCtx->cs = BS3_SEL_R0_CS32;
        else
            pRegCtx->cs = BS3_SEL_R0_CS64;
    }

    /* Adjust CS to the right ring, if not ring-0 or V86 context. */
    if (   pRegCtx->bCpl != 0
        && !BS3_MODE_IS_RM_OR_V86(pRegCtx->bMode)
        && BS3_SEL_IS_IN_R0_RANGE(pRegCtx->cs))
    {
        pRegCtx->cs += (uint16_t)pRegCtx->bCpl << BS3_SEL_RING_SHIFT;
        pRegCtx->cs |= pRegCtx->bCpl;
    }
}

