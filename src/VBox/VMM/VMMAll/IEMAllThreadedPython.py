#!/usr/bin/env python
# -*- coding: utf-8 -*-
# $Id$
# pylint: disable=invalid-name

"""
Annotates and generates threaded functions from IEMAllInstructions*.cpp.h.
"""

from __future__ import print_function;

__copyright__ = \
"""
Copyright (C) 2023 Oracle and/or its affiliates.

This file is part of VirtualBox base platform packages, as
available from https://www.virtualbox.org.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, in version 3 of the
License.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <https://www.gnu.org/licenses>.

SPDX-License-Identifier: GPL-3.0-only
"""
__version__ = "$Revision$"

# Standard python imports.
import copy;
import datetime;
import os;
import re;
import sys;
import argparse;

import IEMAllInstructionsPython as iai;


# Python 3 hacks:
if sys.version_info[0] >= 3:
    long = int;     # pylint: disable=redefined-builtin,invalid-name

## Number of generic parameters for the thread functions.
g_kcThreadedParams = 3;

g_kdTypeInfo = {
    # type name:    (cBits, fSigned, C-type       )
    'int8_t':       (    8,    True, 'uint8_t',   ),
    'int16_t':      (   16,    True, 'int16_t',   ),
    'int32_t':      (   32,    True, 'int32_t',   ),
    'int64_t':      (   64,    True, 'int64_t',   ),
    'uint4_t':      (    4,   False, 'uint8_t',   ),
    'uint8_t':      (    8,   False, 'uint8_t',   ),
    'uint16_t':     (   16,   False, 'uint16_t',  ),
    'uint32_t':     (   32,   False, 'uint32_t',  ),
    'uint64_t':     (   64,   False, 'uint64_t',  ),
    'uintptr_t':    (   64,   False, 'uintptr_t', ), # ASSUMES 64-bit host pointer size.
    'bool':         (    1,   False, 'bool',      ),
    'IEMMODE':      (    2,   False, 'IEMMODE',   ),
};

g_kdIemFieldToType = {
    # Illegal ones:
    'offInstrNextByte':     ( None, ),
    'cbInstrBuf':           ( None, ),
    'pbInstrBuf':           ( None, ),
    'uInstrBufPc':          ( None, ),
    'cbInstrBufTotal':      ( None, ),
    'offCurInstrStart':     ( None, ),
    'cbOpcode':             ( None, ),
    'offOpcode':            ( None, ),
    'offModRm':             ( None, ),
    # Okay ones.
    'fPrefixes':            ( 'uint32_t', ),
    'uRexReg':              ( 'uint8_t', ),
    'uRexB':                ( 'uint8_t', ),
    'uRexIndex':            ( 'uint8_t', ),
    'iEffSeg':              ( 'uint8_t', ),
    'enmEffOpSize':         ( 'IEMMODE', ),
    'enmDefAddrMode':       ( 'IEMMODE', ),
    'enmEffAddrMode':       ( 'IEMMODE', ),
    'enmDefOpSize':         ( 'IEMMODE', ),
    'idxPrefix':            ( 'uint8_t', ),
    'uVex3rdReg':           ( 'uint8_t', ),
    'uVexLength':           ( 'uint8_t', ),
    'fEvexStuff':           ( 'uint8_t', ),
    'uFpuOpcode':           ( 'uint16_t', ),
};

class ThreadedParamRef(object):
    """
    A parameter reference for a threaded function.
    """

    def __init__(self, sOrgRef, sType, oStmt, iParam = None, offParam = 0, sStdRef = None):
        ## The name / reference in the original code.
        self.sOrgRef    = sOrgRef;
        ## Normalized name to deal with spaces in macro invocations and such.
        self.sStdRef    = sStdRef if sStdRef else ''.join(sOrgRef.split());
        ## Indicates that sOrgRef may not match the parameter.
        self.fCustomRef = sStdRef is not None;
        ## The type (typically derived).
        self.sType      = sType;
        ## The statement making the reference.
        self.oStmt      = oStmt;
        ## The parameter containing the references. None if implicit.
        self.iParam     = iParam;
        ## The offset in the parameter of the reference.
        self.offParam   = offParam;

        ## The variable name in the threaded function.
        self.sNewName     = 'x';
        ## The this is packed into.
        self.iNewParam    = 99;
        ## The bit offset in iNewParam.
        self.offNewParam  = 1024


class ThreadedFunctionVariation(object):
    """ Threaded function variation. """

    ## @name Variations.
    ## These variations will match translation block selection/distinctions as well.
    ## @note Effective operand size is generally handled in the decoder, at present
    ##       we only do variations on addressing and memory accessing.
    ## @todo Blocks without addressing should have 64-bit and 32-bit PC update
    ##       variations to reduce code size (see iemRegAddToRip).
    ## @{
    ksVariation_Default     = '';               ##< No variations - only used by IEM_MC_DEFER_TO_CIMPL_X_RET.
    ksVariation_16          = '_16';            ##< 16-bit mode code (386+).
    ksVariation_16_Addr32   = '_16_Addr32';     ##< 16-bit mode code (386+), address size prefixed to 32-bit addressing.
    ksVariation_16_Pre386   = '_16_Pre386';     ##< 16-bit mode code, pre-386 CPU target.
    ksVariation_32          = '_32';            ##< 32-bit mode code (386+).
    ksVariation_32_Flat     = '_32_Flat';       ##< 32-bit mode code (386+) with CS, DS, E,S and SS flat and 4GB wide.
    ksVariation_32_Addr16   = '_32_Addr16';     ##< 32-bit mode code (386+), address size prefixed to 16-bit addressing.
    ksVariation_64          = '_64';            ##< 64-bit mode code.
    ksVariation_64_Addr32   = '_64_Addr32';     ##< 64-bit mode code, address size prefixed to 32-bit addressing.
    kasVariations           = (
        ksVariation_Default,
        ksVariation_16,
        ksVariation_16_Addr32,
        ksVariation_16_Pre386,
        ksVariation_32,
        ksVariation_32_Flat,
        ksVariation_32_Addr16,
        ksVariation_64,
        ksVariation_64_Addr32,
    );
    kasVariationsWithoutAddress = (
        ksVariation_16,
        ksVariation_16_Pre386,
        ksVariation_32,
        ksVariation_64,
    );
    kasVariationsWithAddress = (
        ksVariation_16,
        ksVariation_16_Addr32,
        ksVariation_16_Pre386,
        ksVariation_32,
        ksVariation_32_Flat,
        ksVariation_32_Addr16,
        ksVariation_64,
        ksVariation_64_Addr32,
    );
    kasVariationsEmitOrder = (
        ksVariation_Default,
        ksVariation_64,
        ksVariation_32_Flat,
        ksVariation_32,
        ksVariation_16,
        ksVariation_16_Addr32,
        ksVariation_16_Pre386,
        ksVariation_32_Addr16,
        ksVariation_64_Addr32,
    );
    kdVariationNames = {
        ksVariation_Default:    'defer-to-cimpl',
        ksVariation_16:         '16-bit',
        ksVariation_16_Addr32:  '16-bit w/ address prefix (Addr32)',
        ksVariation_16_Pre386:  '16-bit on pre-386 CPU',
        ksVariation_32:         '32-bit',
        ksVariation_32_Flat:    '32-bit flat and wide open CS, SS, DS and ES',
        ksVariation_32_Addr16:  '32-bit w/ address prefix (Addr16)',
        ksVariation_64:         '64-bit',
        ksVariation_64_Addr32:  '64-bit w/ address prefix (Addr32)',

    };
    ## @}

    ## IEM_CIMPL_F_XXX flags that we know.
    kdCImplFlags = {
        'IEM_CIMPL_F_MODE':             True,
        'IEM_CIMPL_F_BRANCH':           False,
        'IEM_CIMPL_F_RFLAGS':           False,
        'IEM_CIMPL_F_STATUS_FLAGS':     False,
        'IEM_CIMPL_F_VMEXIT':           False,
        'IEM_CIMPL_F_FPU':              False,
        'IEM_CIMPL_F_REP':              False,
        'IEM_CIMPL_F_END_TB':           False,
        'IEM_CIMPL_F_XCPT':             True,
    };

    def __init__(self, oThreadedFunction, sVariation = ksVariation_Default):
        self.oParent        = oThreadedFunction # type: ThreadedFunction
        ##< ksVariation_Xxxx.
        self.sVariation     = sVariation

        ## Threaded function parameter references.
        self.aoParamRefs    = []            # type: list(ThreadedParamRef)
        ## Unique parameter references.
        self.dParamRefs     = {}            # type: dict(str,list(ThreadedParamRef))
        ## Minimum number of parameters to the threaded function.
        self.cMinParams     = 0;

        ## List/tree of statements for the threaded function.
        self.aoStmtsForThreadedFunction = [] # type: list(McStmt)

        ## Dictionary with any IEM_CIMPL_F_XXX flags associated to the code block.
        self.dsCImplFlags   = { }           # type: dict(str, bool)

        ## Function enum number, for verification. Set by generateThreadedFunctionsHeader.
        self.iEnumValue     = -1;

    def getIndexName(self):
        sName = self.oParent.oMcBlock.sFunction;
        if sName.startswith('iemOp_'):
            sName = sName[len('iemOp_'):];
        if self.oParent.oMcBlock.iInFunction == 0:
            return 'kIemThreadedFunc_%s%s' % ( sName, self.sVariation, );
        return 'kIemThreadedFunc_%s_%s%s' % ( sName, self.oParent.oMcBlock.iInFunction, self.sVariation, );

    def getFunctionName(self):
        sName = self.oParent.oMcBlock.sFunction;
        if sName.startswith('iemOp_'):
            sName = sName[len('iemOp_'):];
        if self.oParent.oMcBlock.iInFunction == 0:
            return 'iemThreadedFunc_%s%s' % ( sName, self.sVariation, );
        return 'iemThreadedFunc_%s_%s%s' % ( sName, self.oParent.oMcBlock.iInFunction, self.sVariation, );

    #
    # Analysis and code morphing.
    #

    def raiseProblem(self, sMessage):
        """ Raises a problem. """
        self.oParent.raiseProblem(sMessage);

    def warning(self, sMessage):
        """ Emits a warning. """
        self.oParent.warning(sMessage);

    def analyzeReferenceToType(self, sRef):
        """
        Translates a variable or structure reference to a type.
        Returns type name.
        Raises exception if unable to figure it out.
        """
        ch0 = sRef[0];
        if ch0 == 'u':
            if sRef.startswith('u32'):
                return 'uint32_t';
            if sRef.startswith('u8') or sRef == 'uReg':
                return 'uint8_t';
            if sRef.startswith('u64'):
                return 'uint64_t';
            if sRef.startswith('u16'):
                return 'uint16_t';
        elif ch0 == 'b':
            return 'uint8_t';
        elif ch0 == 'f':
            return 'bool';
        elif ch0 == 'i':
            if sRef.startswith('i8'):
                return 'int8_t';
            if sRef.startswith('i16'):
                return 'int16_t';
            if sRef.startswith('i32'):
                return 'int32_t';
            if sRef.startswith('i64'):
                return 'int64_t';
            if sRef in ('iReg', 'iFixedReg', 'iGReg', 'iSegReg', 'iSrcReg', 'iDstReg', 'iCrReg'):
                return 'uint8_t';
        elif ch0 == 'p':
            if sRef.find('-') < 0:
                return 'uintptr_t';
            if sRef.startswith('pVCpu->iem.s.'):
                sField = sRef[len('pVCpu->iem.s.') : ];
                if sField in g_kdIemFieldToType:
                    if g_kdIemFieldToType[sField][0]:
                        return g_kdIemFieldToType[sField][0];
        elif ch0 == 'G' and sRef.startswith('GCPtr'):
            return 'uint64_t';
        elif ch0 == 'e':
            if sRef == 'enmEffOpSize':
                return 'IEMMODE';
        elif ch0 == 'o':
            if sRef.startswith('off32'):
                return 'uint32_t';
        elif sRef == 'cbFrame':  # enter
            return 'uint16_t';
        elif sRef == 'cShift': ## @todo risky
            return 'uint8_t';

        self.raiseProblem('Unknown reference: %s' % (sRef,));
        return None; # Shut up pylint 2.16.2.

    def analyzeCallToType(self, sFnRef):
        """
        Determins the type of an indirect function call.
        """
        assert sFnRef[0] == 'p';

        #
        # Simple?
        #
        if sFnRef.find('-') < 0:
            oDecoderFunction = self.oParent.oMcBlock.oFunction;

            # Try the argument list of the function defintion macro invocation first.
            iArg = 2;
            while iArg < len(oDecoderFunction.asDefArgs):
                if sFnRef == oDecoderFunction.asDefArgs[iArg]:
                    return oDecoderFunction.asDefArgs[iArg - 1];
                iArg += 1;

            # Then check out line that includes the word and looks like a variable declaration.
            oRe = re.compile(' +(P[A-Z0-9_]+|const +IEMOP[A-Z0-9_]+ *[*]) +(const |) *' + sFnRef + ' *(;|=)');
            for sLine in oDecoderFunction.asLines:
                oMatch = oRe.match(sLine);
                if oMatch:
                    if not oMatch.group(1).startswith('const'):
                        return oMatch.group(1);
                    return 'PC' + oMatch.group(1)[len('const ') : -1].strip();

        #
        # Deal with the pImpl->pfnXxx:
        #
        elif sFnRef.startswith('pImpl->pfn'):
            sMember   = sFnRef[len('pImpl->') : ];
            sBaseType = self.analyzeCallToType('pImpl');
            offBits   = sMember.rfind('U') + 1;
            if sBaseType == 'PCIEMOPBINSIZES':          return 'PFNIEMAIMPLBINU'        + sMember[offBits:];
            if sBaseType == 'PCIEMOPUNARYSIZES':        return 'PFNIEMAIMPLUNARYU'      + sMember[offBits:];
            if sBaseType == 'PCIEMOPSHIFTSIZES':        return 'PFNIEMAIMPLSHIFTU'      + sMember[offBits:];
            if sBaseType == 'PCIEMOPSHIFTDBLSIZES':     return 'PFNIEMAIMPLSHIFTDBLU'   + sMember[offBits:];
            if sBaseType == 'PCIEMOPMULDIVSIZES':       return 'PFNIEMAIMPLMULDIVU'     + sMember[offBits:];
            if sBaseType == 'PCIEMOPMEDIAF3':           return 'PFNIEMAIMPLMEDIAF3U'    + sMember[offBits:];
            if sBaseType == 'PCIEMOPMEDIAOPTF3':        return 'PFNIEMAIMPLMEDIAOPTF3U' + sMember[offBits:];
            if sBaseType == 'PCIEMOPMEDIAOPTF2':        return 'PFNIEMAIMPLMEDIAOPTF2U' + sMember[offBits:];
            if sBaseType == 'PCIEMOPMEDIAOPTF3IMM8':    return 'PFNIEMAIMPLMEDIAOPTF3U' + sMember[offBits:] + 'IMM8';
            if sBaseType == 'PCIEMOPBLENDOP':           return 'PFNIEMAIMPLAVXBLENDU'   + sMember[offBits:];

            self.raiseProblem('Unknown call reference: %s::%s (%s)' % (sBaseType, sMember, sFnRef,));

        self.raiseProblem('Unknown call reference: %s' % (sFnRef,));
        return None; # Shut up pylint 2.16.2.

    def analyze8BitGRegStmt(self, oStmt):
        """
        Gets the 8-bit general purpose register access details of the given statement.
        ASSUMES the statement is one accessing an 8-bit GREG.
        """
        idxReg = 0;
        if (   oStmt.sName.find('_FETCH_') > 0
            or oStmt.sName.find('_REF_') > 0
            or oStmt.sName.find('_TO_LOCAL') > 0):
            idxReg = 1;

        sRegRef = oStmt.asParams[idxReg];
        if sRegRef.startswith('IEM_GET_MODRM_RM') >= 0:
            sOrgExpr = 'IEM_GET_MODRM_RM_EX8(pVCpu, %s)' % (sRegRef,);
        elif sRegRef.startswith('IEM_GET_MODRM_REG') >= 0:
            sOrgExpr = 'IEM_GET_MODRM_REG_EX8(pVCpu, %s)' % (sRegRef,);
        else:
            sOrgExpr = '((%s) < 4 || (pVCpu->iem.s.fPrefixes & IEM_OP_PRF_REX) ? (%s) : (%s) + 12)' % (sRegRef, sRegRef, sRegRef);

        if sRegRef.find('IEM_GET_MODRM_RM') >= 0:    sStdRef = 'bRmRm8Ex';
        elif sRegRef.find('IEM_GET_MODRM_REG') >= 0: sStdRef = 'bRmReg8Ex';
        elif sRegRef == 'X86_GREG_xAX':              sStdRef = 'bGregXAx8Ex';
        elif sRegRef == 'X86_GREG_xCX':              sStdRef = 'bGregXCx8Ex';
        elif sRegRef == 'X86_GREG_xSP':              sStdRef = 'bGregXSp8Ex';
        elif sRegRef == 'iFixedReg':                 sStdRef = 'bFixedReg8Ex';
        else:
            self.warning('analyze8BitGRegStmt: sRegRef=%s -> bOther8Ex; %s %s; sOrgExpr=%s'
                         % (sRegRef, oStmt.sName, oStmt.asParams, sOrgExpr,));
            sStdRef = 'bOther8Ex';

        #print('analyze8BitGRegStmt: %s %s; sRegRef=%s\n -> idxReg=%s sOrgExpr=%s sStdRef=%s'
        #      % (oStmt.sName, oStmt.asParams, sRegRef, idxReg, sOrgExpr, sStdRef));
        return (idxReg, sOrgExpr, sStdRef);


    def analyzeMorphStmtForThreaded(self, aoStmts, iParamRef = 0):
        """
        Transforms (copy) the statements into those for the threaded function.

        Returns list/tree of statements (aoStmts is not modified) and the new
        iParamRef value.
        """
        #
        # We'll be traversing aoParamRefs in parallel to the statements, so we
        # must match the traversal in analyzeFindThreadedParamRefs exactly.
        #
        #print('McBlock at %s:%s' % (os.path.split(self.oMcBlock.sSrcFile)[1], self.oMcBlock.iBeginLine,));
        aoThreadedStmts = [];
        for oStmt in aoStmts:
            # Skip C++ statements that is purely related to decoding.
            if not oStmt.isCppStmt() or not oStmt.fDecode:
                # Copy the statement. Make a deep copy to make sure we've got our own
                # copies of all instance variables, even if a bit overkill at the moment.
                oNewStmt = copy.deepcopy(oStmt);
                aoThreadedStmts.append(oNewStmt);
                #print('oNewStmt %s %s' % (oNewStmt.sName, len(oNewStmt.asParams),));

                # If the statement has parameter references, process the relevant parameters.
                # We grab the references relevant to this statement and apply them in reserve order.
                if iParamRef < len(self.aoParamRefs) and self.aoParamRefs[iParamRef].oStmt == oStmt:
                    iParamRefFirst = iParamRef;
                    while True:
                        iParamRef += 1;
                        if iParamRef >= len(self.aoParamRefs) or self.aoParamRefs[iParamRef].oStmt != oStmt:
                            break;

                    #print('iParamRefFirst=%s iParamRef=%s' % (iParamRefFirst, iParamRef));
                    for iCurRef in range(iParamRef - 1, iParamRefFirst - 1, -1):
                        oCurRef = self.aoParamRefs[iCurRef];
                        if oCurRef.iParam is not None:
                            assert oCurRef.oStmt == oStmt;
                            #print('iCurRef=%s iParam=%s sOrgRef=%s' % (iCurRef, oCurRef.iParam, oCurRef.sOrgRef));
                            sSrcParam = oNewStmt.asParams[oCurRef.iParam];
                            assert (   sSrcParam[oCurRef.offParam : oCurRef.offParam + len(oCurRef.sOrgRef)] == oCurRef.sOrgRef
                                    or oCurRef.fCustomRef), \
                                   'offParam=%s sOrgRef=%s iParam=%s oStmt.sName=%s sSrcParam=%s<eos>' \
                                   % (oCurRef.offParam, oCurRef.sOrgRef, oCurRef.iParam, oStmt.sName, sSrcParam);
                            oNewStmt.asParams[oCurRef.iParam] = sSrcParam[0 : oCurRef.offParam] \
                                                              + oCurRef.sNewName \
                                                              + sSrcParam[oCurRef.offParam + len(oCurRef.sOrgRef) : ];

                # Morph IEM_MC_CALC_RM_EFF_ADDR into IEM_MC_CALC_RM_EFF_ADDR_THREADED ...
                if oNewStmt.sName == 'IEM_MC_CALC_RM_EFF_ADDR':
                    assert self.sVariation != self.ksVariation_Default;
                    oNewStmt.sName = 'IEM_MC_CALC_RM_EFF_ADDR_THREADED' + self.sVariation.upper();
                    assert len(oNewStmt.asParams) == 3;

                    if self.sVariation in (self.ksVariation_16, self.ksVariation_16_Pre386, self.ksVariation_32_Addr16):
                        oNewStmt.asParams = [
                            oNewStmt.asParams[0], oNewStmt.asParams[1], self.dParamRefs['u16Disp'][0].sNewName,
                        ];
                    else:
                        sSibAndMore = self.dParamRefs['bSib'][0].sNewName; # Merge bSib and 2nd part of cbImmAndRspOffset.
                        if oStmt.asParams[2] not in ('0', '1', '2', '4'):
                            sSibAndMore = '(%s) | ((%s) & 0x0f00)' % (self.dParamRefs['bSib'][0].sNewName, oStmt.asParams[2]);

                        if self.sVariation in (self.ksVariation_32, self.ksVariation_32_Flat, self.ksVariation_16_Addr32):
                            oNewStmt.asParams = [
                                oNewStmt.asParams[0], oNewStmt.asParams[1], sSibAndMore, self.dParamRefs['u32Disp'][0].sNewName,
                            ];
                        else:
                            oNewStmt.asParams = [
                                oNewStmt.asParams[0], self.dParamRefs['bRmEx'][0].sNewName, sSibAndMore,
                                self.dParamRefs['u32Disp'][0].sNewName, self.dParamRefs['cbInstr'][0].sNewName,
                            ];
                # ... and IEM_MC_ADVANCE_RIP_AND_FINISH into *_THREADED and maybe *_LM64/_NOT64 ...
                elif oNewStmt.sName in ('IEM_MC_ADVANCE_RIP_AND_FINISH', 'IEM_MC_REL_JMP_S8_AND_FINISH',
                                        'IEM_MC_REL_JMP_S16_AND_FINISH', 'IEM_MC_REL_JMP_S32_AND_FINISH'):
                    oNewStmt.asParams.append(self.dParamRefs['cbInstr'][0].sNewName);
                    if (    oNewStmt.sName in ('IEM_MC_REL_JMP_S8_AND_FINISH', )
                        and self.sVariation != self.ksVariation_16_Pre386):
                        oNewStmt.asParams.append(self.dParamRefs['pVCpu->iem.s.enmEffOpSize'][0].sNewName);
                    oNewStmt.sName += '_THREADED';
                    if self.sVariation in (self.ksVariation_64, self.ksVariation_64_Addr32):
                        oNewStmt.sName += '_PC64';
                    elif self.sVariation == self.ksVariation_16_Pre386:
                        oNewStmt.sName += '_PC16';
                    elif self.sVariation != self.ksVariation_Default:
                        oNewStmt.sName += '_PC32';

                # ... and IEM_MC_*_GREG_U8 into *_THREADED w/ reworked index taking REX into account
                elif oNewStmt.sName.startswith('IEM_MC_') and oNewStmt.sName.find('_GREG_U8') > 0:
                    (idxReg, _, sStdRef) = self.analyze8BitGRegStmt(oStmt); # Don't use oNewStmt as it has been modified!
                    oNewStmt.asParams[idxReg] = self.dParamRefs[sStdRef][0].sNewName;
                    oNewStmt.sName += '_THREADED';

                # ... and IEM_MC_CALL_CIMPL_[0-5] and IEM_MC_DEFER_TO_CIMPL_[0-5]_RET into *_THREADED ...
                elif oNewStmt.sName.startswith('IEM_MC_CALL_CIMPL_') or oNewStmt.sName.startswith('IEM_MC_DEFER_TO_CIMPL_'):
                    oNewStmt.sName += '_THREADED';
                    oNewStmt.asParams.insert(0, self.dParamRefs['cbInstr'][0].sNewName);

                # Process branches of conditionals recursively.
                if isinstance(oStmt, iai.McStmtCond):
                    (oNewStmt.aoIfBranch, iParamRef) = self.analyzeMorphStmtForThreaded(oStmt.aoIfBranch,   iParamRef);
                    if oStmt.aoElseBranch:
                        (oNewStmt.aoElseBranch, iParamRef) = self.analyzeMorphStmtForThreaded(oStmt.aoElseBranch, iParamRef);

        return (aoThreadedStmts, iParamRef);


    def analyzeCodeOperation(self, aoStmts):
        """
        Analyzes the code looking clues as to additional side-effects.

        Currently this is simply looking for any IEM_IMPL_C_F_XXX flags and
        collecting these in self.dsCImplFlags.
        """
        for oStmt in aoStmts:
            # Pick up hints from CIMPL calls and deferals.
            if oStmt.sName.startswith('IEM_MC_CALL_CIMPL_') or oStmt.sName.startswith('IEM_MC_DEFER_TO_CIMPL_'):
                sFlagsSansComments = iai.McBlock.stripComments(oStmt.asParams[0]);
                for sFlag in sFlagsSansComments.split('|'):
                    sFlag = sFlag.strip();
                    if sFlag != '0':
                        if sFlag in self.kdCImplFlags:
                            self.dsCImplFlags[sFlag] = True;
                        else:
                            self.raiseProblem('Unknown CIMPL flag value: %s' % (sFlag,));

            # Set IEM_IMPL_C_F_BRANCH if we see any branching MCs.
            if (   oStmt.sName.startswith('IEM_MC_SET_RIP')
                or oStmt.sName.startswith('IEM_MC_REL_JMP')):
                self.dsCImplFlags['IEM_CIMPL_F_BRANCH'] = True;

            # Process branches of conditionals recursively.
            if isinstance(oStmt, iai.McStmtCond):
                self.analyzeCodeOperation(oStmt.aoIfBranch);
                if oStmt.aoElseBranch:
                    self.analyzeCodeOperation(oStmt.aoElseBranch);

        return True;


    def analyzeConsolidateThreadedParamRefs(self):
        """
        Consolidate threaded function parameter references into a dictionary
        with lists of the references to each variable/field.
        """
        # Gather unique parameters.
        self.dParamRefs = {};
        for oRef in self.aoParamRefs:
            if oRef.sStdRef not in self.dParamRefs:
                self.dParamRefs[oRef.sStdRef] = [oRef,];
            else:
                self.dParamRefs[oRef.sStdRef].append(oRef);

        # Generate names for them for use in the threaded function.
        dParamNames = {};
        for sName, aoRefs in self.dParamRefs.items():
            # Morph the reference expression into a name.
            if sName.startswith('IEM_GET_MODRM_REG'):           sName = 'bModRmRegP';
            elif sName.startswith('IEM_GET_MODRM_RM'):          sName = 'bModRmRmP';
            elif sName.startswith('IEM_GET_MODRM_REG_8'):       sName = 'bModRmReg8P';
            elif sName.startswith('IEM_GET_MODRM_RM_8'):        sName = 'bModRmRm8P';
            elif sName.startswith('IEM_GET_EFFECTIVE_VVVV'):    sName = 'bEffVvvvP';
            elif sName.find('.') >= 0 or sName.find('->') >= 0:
                sName = sName[max(sName.rfind('.'), sName.rfind('>')) + 1 : ] + 'P';
            else:
                sName += 'P';

            # Ensure it's unique.
            if sName in dParamNames:
                for i in range(10):
                    if sName + str(i) not in dParamNames:
                        sName += str(i);
                        break;
            dParamNames[sName] = True;

            # Update all the references.
            for oRef in aoRefs:
                oRef.sNewName = sName;

        # Organize them by size too for the purpose of optimize them.
        dBySize = {}        # type: dict(str,str)
        for sStdRef, aoRefs in self.dParamRefs.items():
            if aoRefs[0].sType[0] != 'P':
                cBits = g_kdTypeInfo[aoRefs[0].sType][0];
                assert(cBits <= 64);
            else:
                cBits = 64;

            if cBits not in dBySize:
                dBySize[cBits] = [sStdRef,]
            else:
                dBySize[cBits].append(sStdRef);

        # Pack the parameters as best as we can, starting with the largest ones
        # and ASSUMING a 64-bit parameter size.
        self.cMinParams = 0;
        offNewParam     = 0;
        for cBits in sorted(dBySize.keys(), reverse = True):
            for sStdRef in dBySize[cBits]:
                if offNewParam == 0 or offNewParam + cBits > 64:
                    self.cMinParams += 1;
                    offNewParam      = cBits;
                else:
                    offNewParam     += cBits;
                assert(offNewParam <= 64);

                for oRef in self.dParamRefs[sStdRef]:
                    oRef.iNewParam   = self.cMinParams - 1;
                    oRef.offNewParam = offNewParam - cBits;

        # Currently there are a few that requires 4 parameters, list these so we can figure out why:
        if self.cMinParams >= 4:
            print('debug: cMinParams=%s cRawParams=%s - %s:%d'
                  % (self.cMinParams, len(self.dParamRefs), self.oParent.oMcBlock.sSrcFile, self.oParent.oMcBlock.iBeginLine,));

        return True;

    ksHexDigits = '0123456789abcdefABCDEF';

    def analyzeFindThreadedParamRefs(self, aoStmts): # pylint: disable=too-many-statements
        """
        Scans the statements for things that have to passed on to the threaded
        function (populates self.aoParamRefs).
        """
        for oStmt in aoStmts:
            # Some statements we can skip alltogether.
            if isinstance(oStmt, iai.McCppPreProc):
                continue;
            if oStmt.isCppStmt() and oStmt.fDecode:
                continue;

            if isinstance(oStmt, iai.McStmtVar):
                if oStmt.sConstValue is None:
                    continue;
                aiSkipParams = { 0: True, 1: True, 3: True };
            else:
                aiSkipParams = {};

            # Several statements have implicit parameters and some have different parameters.
            if oStmt.sName in ('IEM_MC_ADVANCE_RIP_AND_FINISH', 'IEM_MC_REL_JMP_S8_AND_FINISH', 'IEM_MC_REL_JMP_S16_AND_FINISH',
                               'IEM_MC_REL_JMP_S32_AND_FINISH', 'IEM_MC_CALL_CIMPL_0', 'IEM_MC_CALL_CIMPL_1',
                               'IEM_MC_CALL_CIMPL_2', 'IEM_MC_CALL_CIMPL_3', 'IEM_MC_CALL_CIMPL_4', 'IEM_MC_CALL_CIMPL_5',
                               'IEM_MC_DEFER_TO_CIMPL_0_RET', 'IEM_MC_DEFER_TO_CIMPL_1_RET', 'IEM_MC_DEFER_TO_CIMPL_2_RET',
                               'IEM_MC_DEFER_TO_CIMPL_3_RET', 'IEM_MC_DEFER_TO_CIMPL_4_RET', 'IEM_MC_DEFER_TO_CIMPL_5_RET', ):
                self.aoParamRefs.append(ThreadedParamRef('IEM_GET_INSTR_LEN(pVCpu)', 'uint4_t', oStmt, sStdRef = 'cbInstr'));

            if (    oStmt.sName in ('IEM_MC_REL_JMP_S8_AND_FINISH',)
                and self.sVariation != self.ksVariation_16_Pre386):
                self.aoParamRefs.append(ThreadedParamRef('pVCpu->iem.s.enmEffOpSize', 'IEMMODE', oStmt));

            if oStmt.sName == 'IEM_MC_CALC_RM_EFF_ADDR':
                # This is being pretty presumptive about bRm always being the RM byte...
                assert len(oStmt.asParams) == 3;
                assert oStmt.asParams[1] == 'bRm';

                if self.sVariation in (self.ksVariation_16, self.ksVariation_16_Pre386, self.ksVariation_32_Addr16):
                    self.aoParamRefs.append(ThreadedParamRef('bRm',     'uint8_t',  oStmt));
                    self.aoParamRefs.append(ThreadedParamRef('(uint16_t)uEffAddrInfo' ,
                                                             'uint16_t', oStmt, sStdRef = 'u16Disp'));
                elif self.sVariation in (self.ksVariation_32, self.ksVariation_32_Flat, self.ksVariation_16_Addr32):
                    self.aoParamRefs.append(ThreadedParamRef('bRm',     'uint8_t',  oStmt));
                    self.aoParamRefs.append(ThreadedParamRef('(uint8_t)(uEffAddrInfo >> 32)',
                                                             'uint8_t',  oStmt, sStdRef = 'bSib'));
                    self.aoParamRefs.append(ThreadedParamRef('(uint32_t)uEffAddrInfo',
                                                             'uint32_t', oStmt, sStdRef = 'u32Disp'));
                else:
                    assert self.sVariation in (self.ksVariation_64, self.ksVariation_64_Addr32);
                    self.aoParamRefs.append(ThreadedParamRef('IEM_GET_MODRM_EX(pVCpu, bRm)',
                                                             'uint8_t',  oStmt, sStdRef = 'bRmEx'));
                    self.aoParamRefs.append(ThreadedParamRef('(uint8_t)(uEffAddrInfo >> 32)',
                                                             'uint8_t',  oStmt, sStdRef = 'bSib'));
                    self.aoParamRefs.append(ThreadedParamRef('(uint32_t)uEffAddrInfo',
                                                             'uint32_t', oStmt, sStdRef = 'u32Disp'));
                    self.aoParamRefs.append(ThreadedParamRef('IEM_GET_INSTR_LEN(pVCpu)',
                                                             'uint4_t',  oStmt, sStdRef = 'cbInstr'));
                    aiSkipParams[1] = True; # Skip the bRm parameter as it is being replaced by bRmEx.

            # 8-bit register accesses needs to have their index argument reworked to take REX into account.
            if oStmt.sName.startswith('IEM_MC_') and oStmt.sName.find('_GREG_U8') > 0:
                (idxReg, sOrgRef, sStdRef) = self.analyze8BitGRegStmt(oStmt);
                self.aoParamRefs.append(ThreadedParamRef(sOrgRef, 'uint16_t', oStmt, idxReg, sStdRef = sStdRef));
                aiSkipParams[idxReg] = True; # Skip the parameter below.

            # Inspect the target of calls to see if we need to pass down a
            # function pointer or function table pointer for it to work.
            if isinstance(oStmt, iai.McStmtCall):
                if oStmt.sFn[0] == 'p':
                    self.aoParamRefs.append(ThreadedParamRef(oStmt.sFn, self.analyzeCallToType(oStmt.sFn), oStmt, oStmt.idxFn));
                elif (    oStmt.sFn[0] != 'i'
                      and not oStmt.sFn.startswith('IEMTARGETCPU_EFL_BEHAVIOR_SELECT')
                      and not oStmt.sFn.startswith('IEM_SELECT_HOST_OR_FALLBACK') ):
                    self.raiseProblem('Bogus function name in %s: %s' % (oStmt.sName, oStmt.sFn,));
                aiSkipParams[oStmt.idxFn] = True;

                # Skip the hint parameter (first) for IEM_MC_CALL_CIMPL_X.
                if oStmt.sName.startswith('IEM_MC_CALL_CIMPL_'):
                    assert oStmt.idxFn == 1;
                    aiSkipParams[0] = True;


            # Check all the parameters for bogus references.
            for iParam, sParam in enumerate(oStmt.asParams):
                if iParam not in aiSkipParams  and  sParam not in self.oParent.dVariables:
                    # The parameter may contain a C expression, so we have to try
                    # extract the relevant bits, i.e. variables and fields while
                    # ignoring operators and parentheses.
                    offParam = 0;
                    while offParam < len(sParam):
                        # Is it the start of an C identifier? If so, find the end, but don't stop on field separators (->, .).
                        ch = sParam[offParam];
                        if ch.isalpha() or ch == '_':
                            offStart = offParam;
                            offParam += 1;
                            while offParam < len(sParam):
                                ch = sParam[offParam];
                                if not ch.isalnum() and ch != '_' and ch != '.':
                                    if ch != '-' or sParam[offParam + 1] != '>':
                                        # Special hack for the 'CTX_SUFF(pVM)' bit in pVCpu->CTX_SUFF(pVM)->xxxx:
                                        if (    ch == '('
                                            and sParam[offStart : offParam + len('(pVM)->')] == 'pVCpu->CTX_SUFF(pVM)->'):
                                            offParam += len('(pVM)->') - 1;
                                        else:
                                            break;
                                    offParam += 1;
                                offParam += 1;
                            sRef = sParam[offStart : offParam];

                            # For register references, we pass the full register indexes instead as macros
                            # like IEM_GET_MODRM_REG implicitly references pVCpu->iem.s.uRexReg and the
                            # threaded function will be more efficient if we just pass the register index
                            # as a 4-bit param.
                            if (   sRef.startswith('IEM_GET_MODRM')
                                or sRef.startswith('IEM_GET_EFFECTIVE_VVVV') ):
                                offParam = iai.McBlock.skipSpacesAt(sParam, offParam, len(sParam));
                                if sParam[offParam] != '(':
                                    self.raiseProblem('Expected "(" following %s in "%s"' % (sRef, oStmt.renderCode(),));
                                (asMacroParams, offCloseParam) = iai.McBlock.extractParams(sParam, offParam);
                                if asMacroParams is None:
                                    self.raiseProblem('Unable to find ")" for %s in "%s"' % (sRef, oStmt.renderCode(),));
                                offParam = offCloseParam + 1;
                                self.aoParamRefs.append(ThreadedParamRef(sParam[offStart : offParam], 'uint8_t',
                                                                         oStmt, iParam, offStart));

                            # We can skip known variables.
                            elif sRef in self.oParent.dVariables:
                                pass;

                            # Skip certain macro invocations.
                            elif sRef in ('IEM_GET_HOST_CPU_FEATURES',
                                          'IEM_GET_GUEST_CPU_FEATURES',
                                          'IEM_IS_GUEST_CPU_AMD',
                                          'IEM_IS_16BIT_CODE',
                                          'IEM_IS_32BIT_CODE',
                                          'IEM_IS_64BIT_CODE',
                                          ):
                                offParam = iai.McBlock.skipSpacesAt(sParam, offParam, len(sParam));
                                if sParam[offParam] != '(':
                                    self.raiseProblem('Expected "(" following %s in "%s"' % (sRef, oStmt.renderCode(),));
                                (asMacroParams, offCloseParam) = iai.McBlock.extractParams(sParam, offParam);
                                if asMacroParams is None:
                                    self.raiseProblem('Unable to find ")" for %s in "%s"' % (sRef, oStmt.renderCode(),));
                                offParam = offCloseParam + 1;

                                # Skip any dereference following it, unless it's a predicate like IEM_IS_GUEST_CPU_AMD.
                                if sRef not in ('IEM_IS_GUEST_CPU_AMD',
                                                'IEM_IS_16BIT_CODE',
                                                'IEM_IS_32BIT_CODE',
                                                'IEM_IS_64BIT_CODE',
                                                ):
                                    offParam = iai.McBlock.skipSpacesAt(sParam, offParam, len(sParam));
                                    if offParam + 2 <= len(sParam) and sParam[offParam : offParam + 2] == '->':
                                        offParam = iai.McBlock.skipSpacesAt(sParam, offParam + 2, len(sParam));
                                        while offParam < len(sParam) and (sParam[offParam].isalnum() or sParam[offParam] in '_.'):
                                            offParam += 1;

                            # Skip constants, globals, types (casts), sizeof and macros.
                            elif (   sRef.startswith('IEM_OP_PRF_')
                                  or sRef.startswith('IEM_ACCESS_')
                                  or sRef.startswith('IEMINT_')
                                  or sRef.startswith('X86_GREG_')
                                  or sRef.startswith('X86_SREG_')
                                  or sRef.startswith('X86_EFL_')
                                  or sRef.startswith('X86_FSW_')
                                  or sRef.startswith('X86_FCW_')
                                  or sRef.startswith('X86_XCPT_')
                                  or sRef.startswith('IEMMODE_')
                                  or sRef.startswith('IEM_F_')
                                  or sRef.startswith('IEM_CIMPL_F_')
                                  or sRef.startswith('g_')
                                  or sRef.startswith('iemAImpl_')
                                  or sRef in ( 'int8_t',    'int16_t',    'int32_t',
                                               'INT8_C',    'INT16_C',    'INT32_C',    'INT64_C',
                                               'UINT8_C',   'UINT16_C',   'UINT32_C',   'UINT64_C',
                                               'UINT8_MAX', 'UINT16_MAX', 'UINT32_MAX', 'UINT64_MAX',
                                               'INT8_MAX',  'INT16_MAX',  'INT32_MAX',  'INT64_MAX',
                                               'INT8_MIN',  'INT16_MIN',  'INT32_MIN',  'INT64_MIN',
                                               'sizeof',    'NOREF',      'RT_NOREF',   'IEMMODE_64BIT',
                                               'RT_BIT_32', 'true',       'false',      'NIL_RTGCPTR',) ):
                                pass;

                            # Skip certain macro invocations.
                            # Any variable (non-field) and decoder fields in IEMCPU will need to be parameterized.
                            elif (   (    '.' not in sRef
                                      and '-' not in sRef
                                      and sRef not in ('pVCpu', ) )
                                  or iai.McBlock.koReIemDecoderVars.search(sRef) is not None):
                                self.aoParamRefs.append(ThreadedParamRef(sRef, self.analyzeReferenceToType(sRef),
                                                                         oStmt, iParam, offStart));
                        # Number.
                        elif ch.isdigit():
                            if (    ch == '0'
                                and offParam + 2 <= len(sParam)
                                and sParam[offParam + 1] in 'xX'
                                and sParam[offParam + 2] in self.ksHexDigits ):
                                offParam += 2;
                                while offParam < len(sParam) and sParam[offParam] in self.ksHexDigits:
                                    offParam += 1;
                            else:
                                while offParam < len(sParam) and sParam[offParam].isdigit():
                                    offParam += 1;
                        # Comment?
                        elif (    ch == '/'
                              and offParam + 4 <= len(sParam)
                              and sParam[offParam + 1] == '*'):
                            offParam += 2;
                            offNext = sParam.find('*/', offParam);
                            if offNext < offParam:
                                self.raiseProblem('Unable to find "*/" in "%s" ("%s")' % (sRef, oStmt.renderCode(),));
                            offParam = offNext + 2;
                        # Whatever else.
                        else:
                            offParam += 1;

            # Traverse the branches of conditionals.
            if isinstance(oStmt, iai.McStmtCond):
                self.analyzeFindThreadedParamRefs(oStmt.aoIfBranch);
                self.analyzeFindThreadedParamRefs(oStmt.aoElseBranch);
        return True;

    def analyzeVariation(self, aoStmts):
        """
        2nd part of the analysis, done on each variation.

        The variations may differ in parameter requirements and will end up with
        slightly different MC sequences. Thus this is done on each individually.

        Returns dummy True - raises exception on trouble.
        """
        # Now scan the code for variables and field references that needs to
        # be passed to the threaded function because they are related to the
        # instruction decoding.
        self.analyzeFindThreadedParamRefs(aoStmts);
        self.analyzeConsolidateThreadedParamRefs();

        # Scan the code for IEM_CIMPL_F_ and other clues.
        self.analyzeCodeOperation(aoStmts);

        # Morph the statement stream for the block into what we'll be using in the threaded function.
        (self.aoStmtsForThreadedFunction, iParamRef) = self.analyzeMorphStmtForThreaded(aoStmts);
        if iParamRef != len(self.aoParamRefs):
            raise Exception('iParamRef=%s, expected %s!' % (iParamRef, len(self.aoParamRefs),));

        return True;

    def emitThreadedCallStmts(self, cchIndent):
        """
        Produces generic C++ statments that emits a call to the thread function
        variation and any subsequent checks that may be necessary after that.
        """
        # The call to the threaded function.
        sCode = 'IEM_MC2_EMIT_CALL_%s(%s' % (self.cMinParams, self.getIndexName(), );
        for iParam in range(self.cMinParams):
            asFrags = [];
            for aoRefs in self.dParamRefs.values():
                oRef = aoRefs[0];
                if oRef.iNewParam == iParam:
                    sCast = '(uint64_t)'
                    if oRef.sType in ('int8_t', 'int16_t', 'int32_t'): # Make sure these doesn't get sign-extended.
                        sCast = '(uint64_t)(u' + oRef.sType + ')';
                    if oRef.offNewParam == 0:
                        asFrags.append(sCast + '(' + oRef.sOrgRef + ')');
                    else:
                        asFrags.append('(%s(%s) << %s)' % (sCast, oRef.sOrgRef, oRef.offNewParam));
            assert asFrags;
            sCode += ', ' + ' | '.join(asFrags);
        sCode += ');';

        aoStmts = [
            iai.McCppGeneric('IEM_MC2_BEGIN_EMIT_CALLS();', cchIndent = cchIndent), # Scope and a hook for various stuff.
            iai.McCppGeneric(sCode, cchIndent = cchIndent),
        ];

        # For CIMPL stuff, we need to consult the associated IEM_CIMPL_F_XXX
        # mask and maybe emit additional checks.
        if 'IEM_CIMPL_F_MODE' in self.dsCImplFlags or 'IEM_CIMPL_F_XCPT' in self.dsCImplFlags:
            aoStmts.append(iai.McCppGeneric('IEM_MC2_EMIT_CALL_1(kIemThreadedFunc_CheckMode, pVCpu->iem.s.fExec);',
                                            cchIndent = cchIndent));

        aoStmts.append(iai.McCppGeneric('IEM_MC2_END_EMIT_CALLS();', cchIndent = cchIndent)); # For closing the scope.
        return aoStmts;


class ThreadedFunction(object):
    """
    A threaded function.
    """

    def __init__(self, oMcBlock):
        self.oMcBlock       = oMcBlock      # type: IEMAllInstructionsPython.McBlock
        ## Variations for this block. There is at least one.
        self.aoVariations   = []            # type: list(ThreadedFunctionVariation)
        ## Variation dictionary containing the same as aoVariations.
        self.dVariations    = {}            # type: dict(str,ThreadedFunctionVariation)
        ## Dictionary of local variables (IEM_MC_LOCAL[_CONST]) and call arguments (IEM_MC_ARG*).
        self.dVariables     = {}            # type: dict(str,McStmtVar)

    @staticmethod
    def dummyInstance():
        """ Gets a dummy instance. """
        return ThreadedFunction(iai.McBlock('null', 999999999, 999999999,
                                            iai.DecoderFunction('null', 999999999, 'nil', ('','')), 999999999));

    def raiseProblem(self, sMessage):
        """ Raises a problem. """
        raise Exception('%s:%s: error: %s' % (self.oMcBlock.sSrcFile, self.oMcBlock.iBeginLine, sMessage, ));

    def warning(self, sMessage):
        """ Emits a warning. """
        print('%s:%s: warning: %s' % (self.oMcBlock.sSrcFile, self.oMcBlock.iBeginLine, sMessage, ));

    def analyzeFindVariablesAndCallArgs(self, aoStmts):
        """ Scans the statements for MC variables and call arguments. """
        for oStmt in aoStmts:
            if isinstance(oStmt, iai.McStmtVar):
                if oStmt.sVarName in self.dVariables:
                    raise Exception('Variable %s is defined more than once!' % (oStmt.sVarName,));
                self.dVariables[oStmt.sVarName] = oStmt.sVarName;

            # There shouldn't be any variables or arguments declared inside if/
            # else blocks, but scan them too to be on the safe side.
            if isinstance(oStmt, iai.McStmtCond):
                cBefore = len(self.dVariables);
                self.analyzeFindVariablesAndCallArgs(oStmt.aoIfBranch);
                self.analyzeFindVariablesAndCallArgs(oStmt.aoElseBranch);
                if len(self.dVariables) != cBefore:
                    raise Exception('Variables/arguments defined in conditional branches!');
        return True;

    def analyze(self):
        """
        Analyzes the code, identifying the number of parameters it requires and such.

        Returns dummy True - raises exception on trouble.
        """

        # Decode the block into a list/tree of McStmt objects.
        aoStmts = self.oMcBlock.decode();

        # Scan the statements for local variables and call arguments (self.dVariables).
        self.analyzeFindVariablesAndCallArgs(aoStmts);

        # Create variations if needed.
        if iai.McStmt.findStmtByNames(aoStmts,
                                      { 'IEM_MC_DEFER_TO_CIMPL_0_RET': True,
                                        'IEM_MC_DEFER_TO_CIMPL_1_RET': True,
                                        'IEM_MC_DEFER_TO_CIMPL_2_RET': True,
                                        'IEM_MC_DEFER_TO_CIMPL_3_RET': True, }):
            self.aoVariations = [ThreadedFunctionVariation(self, ThreadedFunctionVariation.ksVariation_Default),];

        elif iai.McStmt.findStmtByNames(aoStmts, {'IEM_MC_CALC_RM_EFF_ADDR' : True,}):
            self.aoVariations = [ThreadedFunctionVariation(self, sVar)
                                 for sVar in ThreadedFunctionVariation.kasVariationsWithAddress];
        else:
            self.aoVariations = [ThreadedFunctionVariation(self, sVar)
                                 for sVar in ThreadedFunctionVariation.kasVariationsWithoutAddress];

        # Dictionary variant of the list.
        self.dVariations = { oVar.sVariation: oVar for oVar in self.aoVariations };

        # Continue the analysis on each variation.
        for oVariation in self.aoVariations:
            oVariation.analyzeVariation(aoStmts);

        return True;

    def emitThreadedCallStmts(self):
        """
        Worker for morphInputCode that returns a list of statements that emits
        the call to the threaded functions for the block.
        """
        # Special case for only default variation:
        if len(self.aoVariations) == 1:
            assert  self.aoVariations[0].sVariation == ThreadedFunctionVariation.ksVariation_Default;
            return self.aoVariations[0].emitThreadedCallStmts(0);

        # Currently only have variations for address mode.
        dByVari = self.dVariations;

        sExecMask = 'IEM_F_MODE_CPUMODE_MASK';
        if (   ThreadedFunctionVariation.ksVariation_64_Addr32 in dByVari
            or ThreadedFunctionVariation.ksVariation_32_Addr16 in dByVari
            or ThreadedFunctionVariation.ksVariation_32_Flat   in dByVari
            or ThreadedFunctionVariation.ksVariation_16_Addr32 in dByVari):
            sExecMask = '(IEM_F_MODE_CPUMODE_MASK | IEM_F_MODE_X86_FLAT_OR_PRE_386_MASK)';
        aoStmts = [
            iai.McCppGeneric('switch (pVCpu->iem.s.fExec & %s)' % (sExecMask,)),
            iai.McCppGeneric('{'),
        ];

        if ThreadedFunctionVariation.ksVariation_64_Addr32 in dByVari:
            aoStmts.extend([
                iai.McCppGeneric('case IEMMODE_64BIT:', cchIndent = 4),
                iai.McCppCond('RT_LIKELY(pVCpu->iem.s.enmEffAddrMode == IEMMODE_64BIT)', fDecode = True, cchIndent = 8,
                              aoIfBranch   = dByVari[ThreadedFunctionVariation.ksVariation_64].emitThreadedCallStmts(0),
                              aoElseBranch = dByVari[ThreadedFunctionVariation.ksVariation_64_Addr32].emitThreadedCallStmts(0)),
                iai.McCppGeneric('break;', cchIndent = 8),
            ]);
        elif ThreadedFunctionVariation.ksVariation_64 in dByVari:
            aoStmts.append(iai.McCppGeneric('case IEMMODE_64BIT:', cchIndent = 4));
            aoStmts.extend(dByVari[ThreadedFunctionVariation.ksVariation_64].emitThreadedCallStmts(8));
            aoStmts.append(iai.McCppGeneric('break;', cchIndent = 8));

        if ThreadedFunctionVariation.ksVariation_32_Addr16 in dByVari:
            aoStmts.extend([
                iai.McCppGeneric('case IEMMODE_32BIT | IEM_F_MODE_X86_FLAT_OR_PRE_386_MASK:', cchIndent = 4),
                iai.McCppCond('RT_LIKELY(pVCpu->iem.s.enmEffAddrMode == IEMMODE_32BIT)', fDecode = True, cchIndent = 8,
                              aoIfBranch   = dByVari[ThreadedFunctionVariation.ksVariation_32_Flat].emitThreadedCallStmts(0),
                              aoElseBranch = dByVari[ThreadedFunctionVariation.ksVariation_32_Addr16].emitThreadedCallStmts(0)),
                iai.McCppGeneric('break;', cchIndent = 8),
                iai.McCppGeneric('case IEMMODE_32BIT:', cchIndent = 4),
                iai.McCppCond('RT_LIKELY(pVCpu->iem.s.enmEffAddrMode == IEMMODE_32BIT)', fDecode = True, cchIndent = 8,
                              aoIfBranch   = dByVari[ThreadedFunctionVariation.ksVariation_32].emitThreadedCallStmts(0),
                              aoElseBranch = dByVari[ThreadedFunctionVariation.ksVariation_32_Addr16].emitThreadedCallStmts(0)),
                iai.McCppGeneric('break;', cchIndent = 8),
            ]);
        elif ThreadedFunctionVariation.ksVariation_32 in dByVari:
            aoStmts.append(iai.McCppGeneric('case IEMMODE_32BIT:', cchIndent = 4));
            aoStmts.extend(dByVari[ThreadedFunctionVariation.ksVariation_32].emitThreadedCallStmts(8));
            aoStmts.append(iai.McCppGeneric('break;', cchIndent = 8));

        if ThreadedFunctionVariation.ksVariation_16_Addr32 in dByVari:
            aoStmts.extend([
                iai.McCppGeneric('case IEMMODE_16BIT:', cchIndent = 4),
                iai.McCppCond('RT_LIKELY(pVCpu->iem.s.enmEffAddrMode == IEMMODE_16BIT)', fDecode = True, cchIndent = 8,
                              aoIfBranch   = dByVari[ThreadedFunctionVariation.ksVariation_16].emitThreadedCallStmts(0),
                              aoElseBranch = dByVari[ThreadedFunctionVariation.ksVariation_16_Addr32].emitThreadedCallStmts(0)),
                iai.McCppGeneric('break;', cchIndent = 8),
            ]);
        elif ThreadedFunctionVariation.ksVariation_16 in dByVari:
            aoStmts.append(iai.McCppGeneric('case IEMMODE_16BIT:', cchIndent = 4));
            aoStmts.extend(dByVari[ThreadedFunctionVariation.ksVariation_16].emitThreadedCallStmts(8));
            aoStmts.append(iai.McCppGeneric('break;', cchIndent = 8));

        if ThreadedFunctionVariation.ksVariation_16_Pre386 in dByVari:
            aoStmts.append(iai.McCppGeneric('case IEMMODE_16BIT | IEM_F_MODE_X86_FLAT_OR_PRE_386_MASK:', cchIndent = 4));
            aoStmts.extend(dByVari[ThreadedFunctionVariation.ksVariation_16_Pre386].emitThreadedCallStmts(8));
            aoStmts.append(iai.McCppGeneric('break;', cchIndent = 8));

        aoStmts.extend([
            iai.McCppGeneric('IEM_NOT_REACHED_DEFAULT_CASE_RET();', cchIndent = 4),
            iai.McCppGeneric('}'),
        ]);

        return aoStmts;

    def morphInputCode(self, aoStmts, fCallEmitted = False, cDepth = 0):
        """
        Adjusts (& copies) the statements for the input/decoder so it will emit
        calls to the right threaded functions for each block.

        Returns list/tree of statements (aoStmts is not modified) and updated
        fCallEmitted status.
        """
        #print('McBlock at %s:%s' % (os.path.split(self.oMcBlock.sSrcFile)[1], self.oMcBlock.iBeginLine,));
        aoDecoderStmts = [];

        # Take a very simple approach to problematic instructions for now.
        if cDepth == 0:
            dsCImplFlags = {};
            for oVar in self.aoVariations:
                dsCImplFlags.update(oVar.dsCImplFlags);
            if (   'IEM_CIMPL_F_BRANCH' in dsCImplFlags
                or 'IEM_CIMPL_F_MODE'   in dsCImplFlags
                or 'IEM_CIMPL_F_REP'    in dsCImplFlags):
                aoDecoderStmts.append(iai.McCppGeneric('pVCpu->iem.s.fEndTb = true;'));

        for oStmt in aoStmts:
            # Copy the statement. Make a deep copy to make sure we've got our own
            # copies of all instance variables, even if a bit overkill at the moment.
            oNewStmt = copy.deepcopy(oStmt);
            aoDecoderStmts.append(oNewStmt);
            #print('oNewStmt %s %s' % (oNewStmt.sName, len(oNewStmt.asParams),));

            # If we haven't emitted the threaded function call yet, look for
            # statements which it would naturally follow or preceed.
            if not fCallEmitted:
                if not oStmt.isCppStmt():
                    if (   oStmt.sName.startswith('IEM_MC_MAYBE_RAISE_') \
                        or (oStmt.sName.endswith('_AND_FINISH') and oStmt.sName.startswith('IEM_MC_'))
                        or oStmt.sName.startswith('IEM_MC_CALL_CIMPL_')
                        or oStmt.sName.startswith('IEM_MC_DEFER_TO_CIMPL_')
                        or oStmt.sName in ('IEM_MC_RAISE_DIVIDE_ERROR',)):
                        aoDecoderStmts.pop();
                        aoDecoderStmts.extend(self.emitThreadedCallStmts());
                        aoDecoderStmts.append(oNewStmt);
                        fCallEmitted = True;
                elif (    oStmt.fDecode
                      and (   oStmt.asParams[0].find('IEMOP_HLP_DONE_') >= 0
                           or oStmt.asParams[0].find('IEMOP_HLP_DECODED_') >= 0)):
                    aoDecoderStmts.extend(self.emitThreadedCallStmts());
                    fCallEmitted = True;

            # Process branches of conditionals recursively.
            if isinstance(oStmt, iai.McStmtCond):
                (oNewStmt.aoIfBranch, fCallEmitted1) = self.morphInputCode(oStmt.aoIfBranch, fCallEmitted, cDepth + 1);
                if oStmt.aoElseBranch:
                    (oNewStmt.aoElseBranch, fCallEmitted2) = self.morphInputCode(oStmt.aoElseBranch, fCallEmitted, cDepth + 1);
                else:
                    fCallEmitted2 = False;
                fCallEmitted = fCallEmitted or (fCallEmitted1 and fCallEmitted2);

        if not fCallEmitted and cDepth == 0:
            self.raiseProblem('Unable to insert call to threaded function.');

        return (aoDecoderStmts, fCallEmitted);


    def generateInputCode(self):
        """
        Modifies the input code.
        """
        cchIndent = (self.oMcBlock.cchIndent + 3) // 4 * 4;

        if len(self.oMcBlock.aoStmts) == 1:
            # IEM_MC_DEFER_TO_CIMPL_X_RET - need to wrap in {} to make it safe to insert into random code.
            sCode = iai.McStmt.renderCodeForList(self.morphInputCode(self.oMcBlock.aoStmts)[0],
                                                 cchIndent = cchIndent).replace('\n', ' /* gen */\n', 1);
            sCode = ' ' * (min(cchIndent, 2) - 2) + '{\n' \
                  + sCode \
                  + ' ' * (min(cchIndent, 2) - 2) + '}\n';
            return sCode;

        # IEM_MC_BEGIN/END block
        assert len(self.oMcBlock.asLines) > 2, "asLines=%s" % (self.oMcBlock.asLines,);
        return iai.McStmt.renderCodeForList(self.morphInputCode(self.oMcBlock.aoStmts)[0],
                                            cchIndent = cchIndent).replace('\n', ' /* gen */\n', 1);


class IEMThreadedGenerator(object):
    """
    The threaded code generator & annotator.
    """

    def __init__(self):
        self.aoThreadedFuncs = []       # type: list(ThreadedFunction)
        self.oOptions        = None     # type: argparse.Namespace
        self.aoParsers       = []       # type: list(IEMAllInstructionsPython.SimpleParser)

    #
    # Processing.
    #

    def processInputFiles(self):
        """
        Process the input files.
        """

        # Parse the files.
        self.aoParsers = iai.parseFiles(self.oOptions.asInFiles);

        # Create threaded functions for the MC blocks.
        self.aoThreadedFuncs = [ThreadedFunction(oMcBlock) for oMcBlock in iai.g_aoMcBlocks];

        # Analyze the threaded functions.
        dRawParamCounts = {};
        dMinParamCounts = {};
        for oThreadedFunction in self.aoThreadedFuncs:
            oThreadedFunction.analyze();
            for oVariation in oThreadedFunction.aoVariations:
                dRawParamCounts[len(oVariation.dParamRefs)] = dRawParamCounts.get(len(oVariation.dParamRefs), 0) + 1;
                dMinParamCounts[oVariation.cMinParams]      = dMinParamCounts.get(oVariation.cMinParams,      0) + 1;
        print('debug: param count distribution, raw and optimized:', file = sys.stderr);
        for cCount in sorted({cBits: True for cBits in list(dRawParamCounts.keys()) + list(dMinParamCounts.keys())}.keys()):
            print('debug:     %s params: %4s raw, %4s min'
                  % (cCount, dRawParamCounts.get(cCount, 0), dMinParamCounts.get(cCount, 0)),
                  file = sys.stderr);

        return True;

    #
    # Output
    #

    def generateLicenseHeader(self):
        """
        Returns the lines for a license header.
        """
        return [
            '/*',
            ' * Autogenerated by $Id$ ',
            ' * Do not edit!',
            ' */',
            '',
            '/*',
            ' * Copyright (C) 2023-' + str(datetime.date.today().year) + ' Oracle and/or its affiliates.',
            ' *',
            ' * This file is part of VirtualBox base platform packages, as',
            ' * available from https://www.virtualbox.org.',
            ' *',
            ' * This program is free software; you can redistribute it and/or',
            ' * modify it under the terms of the GNU General Public License',
            ' * as published by the Free Software Foundation, in version 3 of the',
            ' * License.',
            ' *',
            ' * This program is distributed in the hope that it will be useful, but',
            ' * WITHOUT ANY WARRANTY; without even the implied warranty of',
            ' * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU',
            ' * General Public License for more details.',
            ' *',
            ' * You should have received a copy of the GNU General Public License',
            ' * along with this program; if not, see <https://www.gnu.org/licenses>.',
            ' *',
            ' * The contents of this file may alternatively be used under the terms',
            ' * of the Common Development and Distribution License Version 1.0',
            ' * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included',
            ' * in the VirtualBox distribution, in which case the provisions of the',
            ' * CDDL are applicable instead of those of the GPL.',
            ' *',
            ' * You may elect to license modified versions of this file under the',
            ' * terms and conditions of either the GPL or the CDDL or both.',
            ' *',
            ' * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0',
            ' */',
            '',
            '',
            '',
        ];


    def generateThreadedFunctionsHeader(self, oOut):
        """
        Generates the threaded functions header file.
        Returns success indicator.
        """

        asLines = self.generateLicenseHeader();

        # Generate the threaded function table indexes.
        asLines += [
            'typedef enum IEMTHREADEDFUNCS',
            '{',
            '    kIemThreadedFunc_Invalid = 0,',
            '',
            '    /*'
            '     * Predefined'
            '     */'
            '    kIemThreadedFunc_CheckMode,',
            '    kIemThreadedFunc_CheckCsLim,',
        ];
        iThreadedFunction = 1;
        for sVariation in ThreadedFunctionVariation.kasVariationsEmitOrder:
            asLines += [
                    '',
                    '    /*',
                    '     * Variation: ' + ThreadedFunctionVariation.kdVariationNames[sVariation] + '',
                    '     */',
            ];
            for oThreadedFunction in self.aoThreadedFuncs:
                oVariation = oThreadedFunction.dVariations.get(sVariation, None);
                if oVariation:
                    iThreadedFunction += 1;
                    oVariation.iEnumValue = iThreadedFunction;
                    asLines.append('    ' + oVariation.getIndexName() + ',');
        asLines += [
            '    kIemThreadedFunc_End',
            '} IEMTHREADEDFUNCS;',
            '',
        ];

        # Prototype the function table.
        sFnType = 'typedef IEM_DECL_IMPL_TYPE(VBOXSTRICTRC, FNIEMTHREADEDFUNC, (PVMCPU pVCpu';
        for iParam in range(g_kcThreadedParams):
            sFnType += ', uint64_t uParam' + str(iParam);
        sFnType += '));'

        asLines += [
            sFnType,
            'typedef FNIEMTHREADEDFUNC *PFNIEMTHREADEDFUNC;',
            '',
            'extern const PFNIEMTHREADEDFUNC g_apfnIemThreadedFunctions[kIemThreadedFunc_End];',
            '#if defined(IN_RING3) || defined(LOG_ENABLED)',
            'extern const char * const       g_apszIemThreadedFunctions[kIemThreadedFunc_End];',
            '#endif',
        ];

        oOut.write('\n'.join(asLines));
        return True;

    ksBitsToIntMask = {
        1:  "UINT64_C(0x1)",
        2:  "UINT64_C(0x3)",
        4:  "UINT64_C(0xf)",
        8:  "UINT64_C(0xff)",
        16: "UINT64_C(0xffff)",
        32: "UINT64_C(0xffffffff)",
    };
    def generateThreadedFunctionsSource(self, oOut):
        """
        Generates the threaded functions source file.
        Returns success indicator.
        """

        asLines = self.generateLicenseHeader();
        oOut.write('\n'.join(asLines));

        # Prepare the fixed bits.
        sParamList = '(PVMCPU pVCpu';
        for iParam in range(g_kcThreadedParams):
            sParamList += ', uint64_t uParam' + str(iParam);
        sParamList += '))\n';

        #
        # Emit the function definitions.
        #
        for sVariation in ThreadedFunctionVariation.kasVariationsEmitOrder:
            sVarName = ThreadedFunctionVariation.kdVariationNames[sVariation];
            oOut.write(  '\n'
                       + '\n'
                       + '\n'
                       + '\n'
                       + '/*' + '*' * 128 + '\n'
                       + '*   Variation: ' + sVarName + ' ' * (129 - len(sVarName) - 15) + '*\n'
                       + '*' * 128 + '*/\n');

            for oThreadedFunction in self.aoThreadedFuncs:
                oVariation = oThreadedFunction.dVariations.get(sVariation, None);
                if oVariation:
                    oMcBlock = oThreadedFunction.oMcBlock;

                    # Function header
                    oOut.write(  '\n'
                               + '\n'
                               + '/**\n'
                               + ' * #%u: %s at line %s offset %s in %s%s\n'
                                  % (oVariation.iEnumValue, oMcBlock.sFunction, oMcBlock.iBeginLine, oMcBlock.offBeginLine,
                                     os.path.split(oMcBlock.sSrcFile)[1],
                                     ' (macro expansion)' if oMcBlock.iBeginLine == oMcBlock.iEndLine else '')
                               + ' */\n'
                               + 'static IEM_DECL_IMPL_DEF(VBOXSTRICTRC, ' + oVariation.getFunctionName() + ',\n'
                               + '                         ' + sParamList
                               + '{\n');

                    aasVars = [];
                    for aoRefs in oVariation.dParamRefs.values():
                        oRef  = aoRefs[0];
                        if oRef.sType[0] != 'P':
                            cBits = g_kdTypeInfo[oRef.sType][0];
                            sType = g_kdTypeInfo[oRef.sType][2];
                        else:
                            cBits = 64;
                            sType = oRef.sType;

                        sTypeDecl = sType + ' const';

                        if cBits == 64:
                            assert oRef.offNewParam == 0;
                            if sType == 'uint64_t':
                                sUnpack = 'uParam%s;' % (oRef.iNewParam,);
                            else:
                                sUnpack = '(%s)uParam%s;' % (sType, oRef.iNewParam,);
                        elif oRef.offNewParam == 0:
                            sUnpack = '(%s)(uParam%s & %s);' % (sType, oRef.iNewParam, self.ksBitsToIntMask[cBits]);
                        else:
                            sUnpack = '(%s)((uParam%s >> %s) & %s);' \
                                    % (sType, oRef.iNewParam, oRef.offNewParam, self.ksBitsToIntMask[cBits]);

                        sComment = '/* %s - %s ref%s */' % (oRef.sOrgRef, len(aoRefs), 's' if len(aoRefs) != 1 else '',);

                        aasVars.append([ '%s:%02u' % (oRef.iNewParam, oRef.offNewParam),
                                         sTypeDecl, oRef.sNewName, sUnpack, sComment ]);
                    acchVars = [0, 0, 0, 0, 0];
                    for asVar in aasVars:
                        for iCol, sStr in enumerate(asVar):
                            acchVars[iCol] = max(acchVars[iCol], len(sStr));
                    sFmt = '    %%-%ss %%-%ss = %%-%ss %%s\n' % (acchVars[1], acchVars[2], acchVars[3]);
                    for asVar in sorted(aasVars):
                        oOut.write(sFmt % (asVar[1], asVar[2], asVar[3], asVar[4],));

                    # RT_NOREF for unused parameters.
                    if oVariation.cMinParams < g_kcThreadedParams:
                        oOut.write('    RT_NOREF('
                                   + ', '.join(['uParam%u' % (i,) for i in range(oVariation.cMinParams, g_kcThreadedParams)])
                                   + ');\n');

                    # Now for the actual statements.
                    oOut.write(iai.McStmt.renderCodeForList(oVariation.aoStmtsForThreadedFunction, cchIndent = 4));

                    oOut.write('}\n');


        #
        # Emit the function table.
        #
        oOut.write(  '\n'
                   + '\n'
                   + '/**\n'
                   + ' * Function table.\n'
                   + ' */\n'
                   + 'const PFNIEMTHREADEDFUNC g_apfnIemThreadedFunctions[kIemThreadedFunc_End] =\n'
                   + '{\n'
                   + '    /*Invalid*/ NULL,\n'
                   + '\n'
                   + '    /*\n'
                   + '     * Predefined.\n'
                   + '     */'
                   + '    iemThreadedFunc_BltIn_CheckMode,\n'
                   + '    iemThreadedFunc_BltIn_CheckCsLim,\n'
                   );
        iThreadedFunction = 1;
        for sVariation in ThreadedFunctionVariation.kasVariationsEmitOrder:
            oOut.write(  '\n'
                       + '    /*\n'
                       + '     * Variation: ' + ThreadedFunctionVariation.kdVariationNames[sVariation] + '\n'
                       + '     */\n');
            for oThreadedFunction in self.aoThreadedFuncs:
                oVariation = oThreadedFunction.dVariations.get(sVariation, None);
                if oVariation:
                    iThreadedFunction += 1;
                    assert oVariation.iEnumValue == iThreadedFunction;
                    oOut.write('    /*%4u*/ %s,\n' % (iThreadedFunction, oVariation.getFunctionName(),));
        oOut.write('};\n');

        #
        # Emit the function name table.
        #
        oOut.write(  '\n'
                   + '\n'
                   + '#if defined(IN_RING3) || defined(LOG_ENABLED)\n'
                   + '/**\n'
                   + ' * Function table.\n'
                   + ' */\n'
                   + 'const char * const g_apszIemThreadedFunctions[kIemThreadedFunc_End] =\n'
                   + '{\n'
                   + '    "Invalid",\n'
                   + '\n'
                   + '    /*\n'
                   + '     * Predefined.\n'
                   + '     */'
                   + '    "BltIn_CheckMode",\n'
                   + '    "BltIn_CheckCsLim",\n'
                   );
        iThreadedFunction = 1;
        for sVariation in ThreadedFunctionVariation.kasVariationsEmitOrder:
            oOut.write(  '\n'
                       + '    /*\n'
                       + '     * Variation: ' + ThreadedFunctionVariation.kdVariationNames[sVariation] + '\n'
                       + '     */\n');
            for oThreadedFunction in self.aoThreadedFuncs:
                oVariation = oThreadedFunction.dVariations.get(sVariation, None);
                if oVariation:
                    iThreadedFunction += 1;
                    assert oVariation.iEnumValue == iThreadedFunction;
                    sName = oVariation.getFunctionName();
                    if sName.startswith('iemThreadedFunc_'):
                        sName = sName[len('iemThreadedFunc_'):];
                    oOut.write('    /*%4u*/ "%s",\n' % (iThreadedFunction, sName,));
        oOut.write(  '};\n'
                   + '#endif /* IN_RING3 || LOG_ENABLED */\n');

        return True;

    def getThreadedFunctionByIndex(self, idx):
        """
        Returns a ThreadedFunction object for the given index.  If the index is
        out of bounds, a dummy is returned.
        """
        if idx < len(self.aoThreadedFuncs):
            return self.aoThreadedFuncs[idx];
        return ThreadedFunction.dummyInstance();

    def generateModifiedInput(self, oOut):
        """
        Generates the combined modified input source/header file.
        Returns success indicator.
        """
        #
        # File header.
        #
        oOut.write('\n'.join(self.generateLicenseHeader()));

        #
        # ASSUMING that g_aoMcBlocks/self.aoThreadedFuncs are in self.aoParsers
        # order, we iterate aoThreadedFuncs in parallel to the lines from the
        # parsers and apply modifications where required.
        #
        iThreadedFunction = 0;
        oThreadedFunction = self.getThreadedFunctionByIndex(0);
        for oParser in self.aoParsers: # type: IEMAllInstructionsPython.SimpleParser
            oOut.write("\n\n/* ****** BEGIN %s ******* */\n" % (oParser.sSrcFile,));

            iLine = 0;
            while iLine < len(oParser.asLines):
                sLine = oParser.asLines[iLine];
                iLine += 1;                 # iBeginLine and iEndLine are 1-based.

                # Can we pass it thru?
                if (   iLine not in [oThreadedFunction.oMcBlock.iBeginLine, oThreadedFunction.oMcBlock.iEndLine]
                    or oThreadedFunction.oMcBlock.sSrcFile != oParser.sSrcFile):
                    oOut.write(sLine);
                #
                # Single MC block.  Just extract it and insert the replacement.
                #
                elif oThreadedFunction.oMcBlock.iBeginLine != oThreadedFunction.oMcBlock.iEndLine:
                    assert sLine.count('IEM_MC_') == 1;
                    oOut.write(sLine[:oThreadedFunction.oMcBlock.offBeginLine]);
                    sModified = oThreadedFunction.generateInputCode().strip();
                    oOut.write(sModified);

                    iLine = oThreadedFunction.oMcBlock.iEndLine;
                    sLine = oParser.asLines[iLine - 1];
                    assert sLine.count('IEM_MC_') == 1 or len(oThreadedFunction.oMcBlock.aoStmts) == 1;
                    oOut.write(sLine[oThreadedFunction.oMcBlock.offAfterEnd : ]);

                    # Advance
                    iThreadedFunction += 1;
                    oThreadedFunction  = self.getThreadedFunctionByIndex(iThreadedFunction);
                #
                # Macro expansion line that have sublines and may contain multiple MC blocks.
                #
                else:
                    offLine = 0;
                    while iLine == oThreadedFunction.oMcBlock.iBeginLine:
                        oOut.write(sLine[offLine : oThreadedFunction.oMcBlock.offBeginLine]);

                        sModified = oThreadedFunction.generateInputCode().strip();
                        assert (   sModified.startswith('IEM_MC_BEGIN')
                                or (sModified.find('IEM_MC_DEFER_TO_CIMPL_') > 0 and sModified.strip().startswith('{\n'))
                                or sModified.startswith('pVCpu->iem.s.fEndTb = true')
                                ), 'sModified="%s"' % (sModified,);
                        oOut.write(sModified);

                        offLine = oThreadedFunction.oMcBlock.offAfterEnd;

                        # Advance
                        iThreadedFunction += 1;
                        oThreadedFunction  = self.getThreadedFunctionByIndex(iThreadedFunction);

                    # Last line segment.
                    if offLine < len(sLine):
                        oOut.write(sLine[offLine : ]);

            oOut.write("/* ****** END %s ******* */\n" % (oParser.sSrcFile,));

        return True;

    #
    # Main
    #

    def main(self, asArgs):
        """
        C-like main function.
        Returns exit code.
        """

        #
        # Parse arguments
        #
        sScriptDir = os.path.dirname(__file__);
        oParser = argparse.ArgumentParser(add_help = False);
        oParser.add_argument('asInFiles',       metavar = 'input.cpp.h',        nargs = '*',
                             default = [os.path.join(sScriptDir, asFM[0]) for asFM in iai.g_aasAllInstrFilesAndDefaultMap],
                             help = "Selection of VMMAll/IEMAllInstructions*.cpp.h files to use as input.");
        oParser.add_argument('--out-funcs-hdr', metavar = 'file-funcs.h',       dest = 'sOutFileFuncsHdr', action = 'store',
                             default = '-', help = 'The output header file for the functions.');
        oParser.add_argument('--out-funcs-cpp', metavar = 'file-funcs.cpp',     dest = 'sOutFileFuncsCpp', action = 'store',
                             default = '-', help = 'The output C++ file for the functions.');
        oParser.add_argument('--out-mod-input', metavar = 'file-instr.cpp.h',   dest = 'sOutFileModInput', action = 'store',
                             default = '-', help = 'The output C++/header file for the modified input instruction files.');
        oParser.add_argument('--help', '-h', '-?', action = 'help', help = 'Display help and exit.');
        oParser.add_argument('--version', '-V', action = 'version',
                             version = 'r%s (IEMAllThreadedPython.py), r%s (IEMAllInstructionsPython.py)'
                                     % (__version__.split()[1], iai.__version__.split()[1],),
                             help = 'Displays the version/revision of the script and exit.');
        self.oOptions = oParser.parse_args(asArgs[1:]);
        print("oOptions=%s" % (self.oOptions,));

        #
        # Process the instructions specified in the IEM sources.
        #
        if self.processInputFiles():
            #
            # Generate the output files.
            #
            aaoOutputFiles = (
                 ( self.oOptions.sOutFileFuncsHdr, self.generateThreadedFunctionsHeader ),
                 ( self.oOptions.sOutFileFuncsCpp, self.generateThreadedFunctionsSource ),
                 ( self.oOptions.sOutFileModInput, self.generateModifiedInput ),
            );
            fRc = True;
            for sOutFile, fnGenMethod in aaoOutputFiles:
                if sOutFile == '-':
                    fRc = fnGenMethod(sys.stdout) and fRc;
                else:
                    try:
                        oOut = open(sOutFile, 'w');                 # pylint: disable=consider-using-with,unspecified-encoding
                    except Exception as oXcpt:
                        print('error! Failed open "%s" for writing: %s' % (sOutFile, oXcpt,), file = sys.stderr);
                        return 1;
                    fRc = fnGenMethod(oOut) and fRc;
                    oOut.close();
            if fRc:
                return 0;

        return 1;


if __name__ == '__main__':
    sys.exit(IEMThreadedGenerator().main(sys.argv));

