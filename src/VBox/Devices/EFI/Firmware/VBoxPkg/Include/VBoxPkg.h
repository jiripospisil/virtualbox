/* $Id$ */
/** @file
 * VBoxPkg.h - Common header, must be include before IPRT and VBox headers.
 */

/*
 * Copyright (C) 2009-2024 Oracle and/or its affiliates.
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

#ifndef ___VBoxPkg_h
#define ___VBoxPkg_h

/*
 * IPRT configuration.
 */
#define IN_RING0
/** @todo detect this */
//#include <iprt/cdefs.h>
#if !defined(ARCH_BITS) || !defined(HC_ARCH_BITS)
//# error "please add right bitness"
#endif

/*
 * VBox and IPRT headers.
 */
#include <VBox/version.h>
#include <iprt/types.h>
#ifdef _MSC_VER
# pragma warning(disable: 4389)
# pragma warning(disable: 4245)
# pragma warning(disable: 4244)
#endif
#include <iprt/asm.h>
#include <iprt/asm-amd64-x86.h>
#ifdef _MSC_VER
# pragma warning(default: 4244)
# pragma warning(default: 4245)
# pragma warning(default: 4389)
#endif

#endif

