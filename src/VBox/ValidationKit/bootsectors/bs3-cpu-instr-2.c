/* $Id$ */
/** @file
 * BS3Kit - bs3-cpu-instr-2, 16-bit C code.
 */

/*
 * Copyright (C) 2007-2022 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <bs3kit.h>


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_mul);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_imul);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_div);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_idiv);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_bsf_tzcnt);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_bsr_lzcnt);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_andn);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_bextr);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_blsr);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_blsmsk);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_blsi);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_rorx);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_shlx);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_sarx);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_shrx);
BS3TESTMODE_PROTOTYPES_CMN_64(bs3CpuInstr2_cmpxchg16b);
BS3TESTMODE_PROTOTYPES_CMN_64(bs3CpuInstr2_wrfsbase);
BS3TESTMODE_PROTOTYPES_CMN_64(bs3CpuInstr2_wrgsbase);
BS3TESTMODE_PROTOTYPES_CMN_64(bs3CpuInstr2_rdfsbase);
BS3TESTMODE_PROTOTYPES_CMN_64(bs3CpuInstr2_rdgsbase);


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static const BS3TESTMODEENTRY g_aModeTests[] =
{
#if 1
    BS3TESTMODEENTRY_CMN("mul", bs3CpuInstr2_mul),
    BS3TESTMODEENTRY_CMN("imul", bs3CpuInstr2_imul),
    BS3TESTMODEENTRY_CMN("div", bs3CpuInstr2_div),
    BS3TESTMODEENTRY_CMN("idiv", bs3CpuInstr2_idiv),
#endif
#if 1 /* BSF/BSR (386+) & TZCNT/LZCNT (BMI1,ABM) */
    BS3TESTMODEENTRY_CMN("bsf/tzcnt",  bs3CpuInstr2_bsf_tzcnt),
    BS3TESTMODEENTRY_CMN("bsr/lzcnt",  bs3CpuInstr2_bsr_lzcnt),
#endif
#if 1 /* BMI1 */
    BS3TESTMODEENTRY_CMN("andn",    bs3CpuInstr2_andn),
    BS3TESTMODEENTRY_CMN("bextr",   bs3CpuInstr2_bextr),
    BS3TESTMODEENTRY_CMN("blsr",    bs3CpuInstr2_blsr),
    BS3TESTMODEENTRY_CMN("blsmsk",  bs3CpuInstr2_blsmsk),
    BS3TESTMODEENTRY_CMN("blsi",    bs3CpuInstr2_blsi),
#endif
#if 1 /* BMI2 */
    BS3TESTMODEENTRY_CMN("rorx",  bs3CpuInstr2_rorx),
    BS3TESTMODEENTRY_CMN("shlx",  bs3CpuInstr2_shlx),
    BS3TESTMODEENTRY_CMN("sarx",  bs3CpuInstr2_sarx),
    BS3TESTMODEENTRY_CMN("shrx",  bs3CpuInstr2_shrx),
#endif
#if 1
    BS3TESTMODEENTRY_CMN_64("cmpxchg16b", bs3CpuInstr2_cmpxchg16b),
    BS3TESTMODEENTRY_CMN_64("wrfsbase", bs3CpuInstr2_wrfsbase),
    BS3TESTMODEENTRY_CMN_64("wrgsbase", bs3CpuInstr2_wrgsbase),
    BS3TESTMODEENTRY_CMN_64("rdfsbase", bs3CpuInstr2_rdfsbase),
    BS3TESTMODEENTRY_CMN_64("rdgsbase", bs3CpuInstr2_rdgsbase),
#endif
};


BS3_DECL(void) Main_rm()
{
    Bs3InitAll_rm();
    Bs3TestInit("bs3-cpu-instr-2");

    Bs3TestDoModes_rm(g_aModeTests, RT_ELEMENTS(g_aModeTests));

    Bs3TestTerm();
}

