/** @file
 * IPRT / No-CRT - Our own minimal stdbool.h header (needed by softfloat.h).
 */

/*
 * Copyright (C) 2022 Oracle Corporation
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

#ifndef IPRT_INCLUDED_nocrt_stdbool_h
#define IPRT_INCLUDED_nocrt_stdbool_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* The iprt/types.h header should take care of the basics. */
#include <iprt/types.h>

#ifndef __cplusplus
# ifndef bool
#  define bool  _Bool
# endif
# ifndef true
#  define true  (1)
# endif
# ifndef false
#  define false (0)
# endif
#endif

#endif /* !IPRT_INCLUDED_nocrt_stdbool_h */

