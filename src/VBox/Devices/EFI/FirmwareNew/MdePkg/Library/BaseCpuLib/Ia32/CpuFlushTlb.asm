;------------------------------------------------------------------------------ ;
; Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;   CpuFlushTlb.Asm
;
; Abstract:
;
;   CpuFlushTlb function
;
; Notes:
;
;------------------------------------------------------------------------------

    .386p
    .model  flat,C
    .code

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; CpuFlushTlb (
;   VOID
;   );
;------------------------------------------------------------------------------
CpuFlushTlb PROC
    mov     eax, cr3
    mov     cr3, eax                    ; moving to CR3 flushes TLB
    ret
CpuFlushTlb ENDP

    END
