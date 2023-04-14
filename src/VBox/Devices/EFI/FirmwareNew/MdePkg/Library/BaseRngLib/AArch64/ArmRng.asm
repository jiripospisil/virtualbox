;------------------------------------------------------------------------------
;
; ArmRndr() for AArch64
;
; Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

#include "BaseRngLibInternals.h"

  EXPORT ArmRndr
  AREA BaseLib_LowLevel, CODE, READONLY


;/**
;  Generates a random number using RNDR.
;  Returns TRUE on success; FALSE on failure.
;
;  @param[out] Rand     Buffer pointer to store the 64-bit random value.
;
;  @retval TRUE         Random number generated successfully.
;  @retval FALSE        Failed to generate the random number.
;
;**/
;BOOLEAN
;EFIAPI
;ArmRndr (
;  OUT UINT64 *Rand
;  );
;
ArmRndr
  mrs  x1, RNDR
  str  x1, [x0]
  cset x0, ne    // RNDR sets NZCV to 0b0100 on failure
  ret

  END
