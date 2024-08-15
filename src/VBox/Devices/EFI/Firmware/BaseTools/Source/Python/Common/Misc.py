## @file
# Common routines used by all tools
#
# Copyright (c) 2007 - 2019, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import absolute_import

import sys
import string
import threading
import time
import re
import pickle
import array
import shutil
import filecmp
from random import sample
from struct import pack
import uuid
import subprocess
import tempfile
from collections import OrderedDict

import Common.LongFilePathOs as os
from Common import EdkLogger as EdkLogger
from Common import GlobalData as GlobalData
from Common.DataType import *
from Common.BuildToolError import *
from CommonDataClass.DataClass import *
from Common.Parsing import GetSplitValueList
from Common.LongFilePathSupport import OpenLongFilePath as open
from Common.LongFilePathSupport import CopyLongFilePath as CopyLong
from Common.LongFilePathSupport import LongFilePath as LongFilePath
from Common.MultipleWorkspace import MultipleWorkspace as mws
from CommonDataClass.Exceptions import BadExpression
from Common.caching import cached_property
import struct

ArrayIndex = re.compile(r"\[\s*[0-9a-fA-FxX]*\s*\]")
## Regular expression used to find out place holders in string template
gPlaceholderPattern = re.compile(r"\$\{([^$()\s]+)\}", re.MULTILINE | re.UNICODE)

## regular expressions for map file processing
startPatternGeneral = re.compile(r"^Start[' ']+Length[' ']+Name[' ']+Class")
addressPatternGeneral = re.compile(r"^Address[' ']+Publics by Value[' ']+Rva\+Base")
valuePatternGcc = re.compile(r'^([\w_\.]+) +([\da-fA-Fx]+) +([\da-fA-Fx]+)$')
pcdPatternGcc = re.compile(r'^([\da-fA-Fx]+) +([\da-fA-Fx]+)')
secReGeneral = re.compile(r'^([\da-fA-F]+):([\da-fA-F]+) +([\da-fA-F]+)[Hh]? +([.\w\$]+) +(\w+)', re.UNICODE)

StructPattern = re.compile(r'[_a-zA-Z][0-9A-Za-z_]*$')

## Dictionary used to store dependencies of files
gDependencyDatabase = {}    # arch : {file path : [dependent files list]}

#
# If a module is built more than once with different PCDs or library classes
# a temporary INF file with same content is created, the temporary file is removed
# when build exits.
#
_TempInfs = []

# VBox: BEGIN
#
#       Monkey patching copyxattr to also ignore errno.EACCES for older python versions.
#       Otherwise building in a docker container fails due to a PermissionError because SELinux
#       denies copying the security context extended attributes. This was just fixed 3 months ago
#       in python so all our python versions in the containers are too old to have it.
#
# [vbox@tinderlin ~]$ sudo ausearch -m AVC,USER_AVC -ts recent
# time->Tue Jul 11 07:44:16 2023
# type=AVC msg=audit(1689061456.361:1300563): avc:  denied  { relabelto } for  pid=1160256 comm="python3"
# name="PcdPeim.uni" dev="sda1" ino=186911506 scontext=system_u:system_r:container_t:s0:c100,c482
# tcontext=unconfined_u:object_r:container_file_t:s0 tclass=file permissive=0
#
# See for further reference:
#    https://github.com/python/cpython/issues/82814
#    https://bugs.python.org/issue38893
#    https://github.com/python/cpython/pull/21430
#
import errno
orig_copyxattr = shutil._copyxattr
def patched_copyxattr(src, dst, *, follow_symlinks=True):
    try:
        orig_copyxattr(src, dst, follow_symlinks=follow_symlinks)
    except OSError as ex:
        if ex.errno != errno.EACCES: raise
shutil._copyxattr = patched_copyxattr
# VBox: END

def GetVariableOffset(mapfilepath, efifilepath, varnames):
    """ Parse map file to get variable offset in current EFI file
    @param mapfilepath    Map file absolution path
    @param efifilepath:   EFI binary file full path
    @param varnames       iteratable container whose elements are variable names to be searched

    @return List whos elements are tuple with variable name and raw offset
    """
    lines = []
    try:
        f = open(mapfilepath, 'r')
        lines = f.readlines()
        f.close()
    except:
        return None

    if len(lines) == 0: return None
    firstline = lines[0].strip()
    if re.match(r'^\s*Address\s*Size\s*Align\s*Out\s*In\s*Symbol\s*$', firstline):
        return _parseForXcodeAndClang9(lines, efifilepath, varnames)
    if (firstline.startswith("Archive member included ") and
        firstline.endswith(" file (symbol)")):
        return _parseForGCC(lines, efifilepath, varnames)
    if firstline.startswith("# Path:"):
        return _parseForXcodeAndClang9(lines, efifilepath, varnames)
    return _parseGeneral(lines, efifilepath, varnames)

def _parseForXcodeAndClang9(lines, efifilepath, varnames):
    status = 0
    ret = []
    for line in lines:
        line = line.strip()
        if status == 0 and (re.match(r'^\s*Address\s*Size\s*Align\s*Out\s*In\s*Symbol\s*$', line) \
            or line == "# Symbols:"):
            status = 1
            continue
        if status == 1 and len(line) != 0:
            for varname in varnames:
                if varname in line:
                    # cannot pregenerate this RegEx since it uses varname from varnames.
                    m = re.match(r'^([\da-fA-FxX]+)([\s\S]*)([_]*%s)$' % varname, line)
                    if m is not None:
                        ret.append((varname, m.group(1)))
    return ret

def _parseForGCC(lines, efifilepath, varnames):
    """ Parse map file generated by GCC linker """
    status = 0
    sections = []
    varoffset = []
    for index, line in enumerate(lines):
        line = line.strip()
        # status machine transection
        if status == 0 and line == "Memory Configuration":
            status = 1
            continue
        elif status == 1 and line == 'Linker script and memory map':
            status = 2
            continue
        elif status ==2 and line == 'START GROUP':
            status = 3
            continue

        # status handler
        if status == 3:
            m = valuePatternGcc.match(line)
            if m is not None:
                sections.append(m.groups(0))
            for varname in varnames:
                Str = ''
                m = re.match("^.data.(%s)" % varname, line)
                if m is not None:
                    m = re.match(".data.(%s)$" % varname, line)
                    if m is not None:
                        Str = lines[index + 1]
                    else:
                        Str = line[len(".data.%s" % varname):]
                    if Str:
                        m = pcdPatternGcc.match(Str.strip())
                        if m is not None:
                            varoffset.append((varname, int(m.groups(0)[0], 16), int(sections[-1][1], 16), sections[-1][0]))

    if not varoffset:
        return []
    # get section information from efi file
    efisecs = PeImageClass(efifilepath).SectionHeaderList
    if efisecs is None or len(efisecs) == 0:
        return []
    #redirection
    redirection = 0
    for efisec in efisecs:
        for section in sections:
            if section[0].strip() == efisec[0].strip() and section[0].strip() == '.text':
                redirection = int(section[1], 16) - efisec[1]

    ret = []
    for var in varoffset:
        for efisec in efisecs:
            if var[1] >= efisec[1] and var[1] < efisec[1]+efisec[3]:
                ret.append((var[0], hex(efisec[2] + var[1] - efisec[1] - redirection)))
    return ret

def _parseGeneral(lines, efifilepath, varnames):
    status = 0    #0 - beginning of file; 1 - PE section definition; 2 - symbol table
    secs  = []    # key = section name
    varoffset = []
    symRe = re.compile(r'^([\da-fA-F]+):([\da-fA-F]+) +([\.:\\\\\w\?@\$-]+) +([\da-fA-F]+)', re.UNICODE)

    for line in lines:
        line = line.strip()
        if startPatternGeneral.match(line):
            status = 1
            continue
        if addressPatternGeneral.match(line):
            status = 2
            continue
        if line.startswith("entry point at"):
            status = 3
            continue
        if status == 1 and len(line) != 0:
            m =  secReGeneral.match(line)
            assert m is not None, "Fail to parse the section in map file , line is %s" % line
            sec_no, sec_start, sec_length, sec_name, sec_class = m.groups(0)
            secs.append([int(sec_no, 16), int(sec_start, 16), int(sec_length, 16), sec_name, sec_class])
        if status == 2 and len(line) != 0:
            for varname in varnames:
                m = symRe.match(line)
                assert m is not None, "Fail to parse the symbol in map file, line is %s" % line
                sec_no, sym_offset, sym_name, vir_addr = m.groups(0)
                sec_no     = int(sec_no,     16)
                sym_offset = int(sym_offset, 16)
                vir_addr   = int(vir_addr,   16)
                # cannot pregenerate this RegEx since it uses varname from varnames.
                m2 = re.match('^[_]*(%s)' % varname, sym_name)
                if m2 is not None:
                    # fond a binary pcd entry in map file
                    for sec in secs:
                        if sec[0] == sec_no and (sym_offset >= sec[1] and sym_offset < sec[1] + sec[2]):
                            varoffset.append([varname, sec[3], sym_offset, vir_addr, sec_no])

    if not varoffset: return []

    # get section information from efi file
    efisecs = PeImageClass(efifilepath).SectionHeaderList
    if efisecs is None or len(efisecs) == 0:
        return []

    ret = []
    for var in varoffset:
        index = 0
        for efisec in efisecs:
            index = index + 1
            if var[1].strip() == efisec[0].strip():
                ret.append((var[0], hex(efisec[2] + var[2])))
            elif var[4] == index:
                ret.append((var[0], hex(efisec[2] + var[2])))

    return ret

## Routine to process duplicated INF
#
#  This function is called by following two cases:
#  Case 1 in DSC:
#    [components.arch]
#    Pkg/module/module.inf
#    Pkg/module/module.inf {
#      <Defines>
#        FILE_GUID = 0D1B936F-68F3-4589-AFCC-FB8B7AEBC836
#    }
#  Case 2 in FDF:
#    INF Pkg/module/module.inf
#    INF FILE_GUID = 0D1B936F-68F3-4589-AFCC-FB8B7AEBC836 Pkg/module/module.inf
#
#  This function copies Pkg/module/module.inf to
#  Conf/.cache/0D1B936F-68F3-4589-AFCC-FB8B7AEBC836module.inf
#
#  @param Path     Original PathClass object
#  @param BaseName New file base name
#
#  @retval         return the new PathClass object
#
def ProcessDuplicatedInf(Path, BaseName, Workspace):
    Filename = os.path.split(Path.File)[1]
    if '.' in Filename:
        Filename = BaseName + Path.BaseName + Filename[Filename.rfind('.'):]
    else:
        Filename = BaseName + Path.BaseName

    DbDir = os.path.split(GlobalData.gDatabasePath)[0]

    #
    # A temporary INF is copied to database path which must have write permission
    # The temporary will be removed at the end of build
    # In case of name conflict, the file name is
    # FILE_GUIDBaseName (0D1B936F-68F3-4589-AFCC-FB8B7AEBC836module.inf)
    #
    TempFullPath = os.path.join(DbDir,
                                Filename)
    RtPath = PathClass(Path.File, Workspace)
    #
    # Modify the full path to temporary path, keep other unchanged
    #
    # To build same module more than once, the module path with FILE_GUID overridden has
    # the file name FILE_GUIDmodule.inf, but the relative path (self.MetaFile.File) is the real path
    # in DSC which is used as relative path by C files and other files in INF.
    # A trick was used: all module paths are PathClass instances, after the initialization
    # of PathClass, the PathClass.Path is overridden by the temporary INF path.
    #
    # The reason for creating a temporary INF is:
    # Platform.Modules which is the base to create ModuleAutoGen objects is a dictionary,
    # the key is the full path of INF, the value is an object to save overridden library instances, PCDs.
    # A different key for the same module is needed to create different output directory,
    # retrieve overridden PCDs, library instances.
    #
    # The BaseName is the FILE_GUID which is also the output directory name.
    #
    #
    RtPath.Path = TempFullPath
    RtPath.BaseName = BaseName
    RtPath.OriginalPath = Path
    #
    # If file exists, compare contents
    #
    if os.path.exists(TempFullPath):
        with open(str(Path), 'rb') as f1, open(TempFullPath, 'rb') as f2:
            if f1.read() == f2.read():
                return RtPath
    _TempInfs.append(TempFullPath)
    shutil.copy2(str(Path), TempFullPath)
    return RtPath

## Remove temporary created INFs whose paths were saved in _TempInfs
#
def ClearDuplicatedInf():
    while _TempInfs:
        File = _TempInfs.pop()
        if os.path.exists(File):
            os.remove(File)

## Convert GUID string in xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx style to C structure style
#
#   @param      Guid    The GUID string
#
#   @retval     string  The GUID string in C structure style
#
def GuidStringToGuidStructureString(Guid):
    GuidList = Guid.split('-')
    Result = '{'
    for Index in range(0, 3, 1):
        Result = Result + '0x' + GuidList[Index] + ', '
    Result = Result + '{0x' + GuidList[3][0:2] + ', 0x' + GuidList[3][2:4]
    for Index in range(0, 12, 2):
        Result = Result + ', 0x' + GuidList[4][Index:Index + 2]
    Result += '}}'
    return Result

## Convert GUID structure in byte array to xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
#
#   @param      GuidValue   The GUID value in byte array
#
#   @retval     string      The GUID value in xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx format
#
def GuidStructureByteArrayToGuidString(GuidValue):
    guidValueString = GuidValue.lower().replace("{", "").replace("}", "").replace(" ", "").replace(";", "")
    guidValueList = guidValueString.split(",")
    if len(guidValueList) != 16:
        return ''
        #EdkLogger.error(None, None, "Invalid GUID value string %s" % GuidValue)
    try:
        return "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x" % (
                int(guidValueList[3], 16),
                int(guidValueList[2], 16),
                int(guidValueList[1], 16),
                int(guidValueList[0], 16),
                int(guidValueList[5], 16),
                int(guidValueList[4], 16),
                int(guidValueList[7], 16),
                int(guidValueList[6], 16),
                int(guidValueList[8], 16),
                int(guidValueList[9], 16),
                int(guidValueList[10], 16),
                int(guidValueList[11], 16),
                int(guidValueList[12], 16),
                int(guidValueList[13], 16),
                int(guidValueList[14], 16),
                int(guidValueList[15], 16)
                )
    except:
        return ''

## Convert GUID string in C structure style to xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
#
#   @param      GuidValue   The GUID value in C structure format
#
#   @retval     string      The GUID value in xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx format
#
def GuidStructureStringToGuidString(GuidValue):
    if not GlobalData.gGuidCFormatPattern.match(GuidValue):
        return ''
    guidValueString = GuidValue.lower().replace("{", "").replace("}", "").replace(" ", "").replace(";", "")
    guidValueList = guidValueString.split(",")
    if len(guidValueList) != 11:
        return ''
        #EdkLogger.error(None, None, "Invalid GUID value string %s" % GuidValue)
    try:
        return "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x" % (
                int(guidValueList[0], 16),
                int(guidValueList[1], 16),
                int(guidValueList[2], 16),
                int(guidValueList[3], 16),
                int(guidValueList[4], 16),
                int(guidValueList[5], 16),
                int(guidValueList[6], 16),
                int(guidValueList[7], 16),
                int(guidValueList[8], 16),
                int(guidValueList[9], 16),
                int(guidValueList[10], 16)
                )
    except:
        return ''

## Convert GUID string in C structure style to xxxxxxxx_xxxx_xxxx_xxxx_xxxxxxxxxxxx
#
#   @param      GuidValue   The GUID value in C structure format
#
#   @retval     string      The GUID value in xxxxxxxx_xxxx_xxxx_xxxx_xxxxxxxxxxxx format
#
def GuidStructureStringToGuidValueName(GuidValue):
    guidValueString = GuidValue.lower().replace("{", "").replace("}", "").replace(" ", "")
    guidValueList = guidValueString.split(",")
    if len(guidValueList) != 11:
        EdkLogger.error(None, FORMAT_INVALID, "Invalid GUID value string [%s]" % GuidValue)
    return "%08x_%04x_%04x_%02x%02x_%02x%02x%02x%02x%02x%02x" % (
            int(guidValueList[0], 16),
            int(guidValueList[1], 16),
            int(guidValueList[2], 16),
            int(guidValueList[3], 16),
            int(guidValueList[4], 16),
            int(guidValueList[5], 16),
            int(guidValueList[6], 16),
            int(guidValueList[7], 16),
            int(guidValueList[8], 16),
            int(guidValueList[9], 16),
            int(guidValueList[10], 16)
            )

## Create directories
#
#   @param      Directory   The directory name
#
def CreateDirectory(Directory):
    if Directory is None or Directory.strip() == "":
        return True
    try:
        if not os.access(Directory, os.F_OK):
            os.makedirs(Directory)
    except:
        return False
    return True

## Remove directories, including files and sub-directories in it
#
#   @param      Directory   The directory name
#
def RemoveDirectory(Directory, Recursively=False):
    if Directory is None or Directory.strip() == "" or not os.path.exists(Directory):
        return
    if Recursively:
        CurrentDirectory = os.getcwd()
        os.chdir(Directory)
        for File in os.listdir("."):
            if os.path.isdir(File):
                RemoveDirectory(File, Recursively)
            else:
                os.remove(File)
        os.chdir(CurrentDirectory)
    os.rmdir(Directory)

## Store content in file
#
#  This method is used to save file only when its content is changed. This is
#  quite useful for "make" system to decide what will be re-built and what won't.
#
#   @param      File            The path of file
#   @param      Content         The new content of the file
#   @param      IsBinaryFile    The flag indicating if the file is binary file or not
#
#   @retval     True            If the file content is changed and the file is renewed
#   @retval     False           If the file content is the same
#
def SaveFileOnChange(File, Content, IsBinaryFile=True, FileLock=None):

    # Convert to long file path format
    File = LongFilePath(File)

    if os.path.exists(File):
        if IsBinaryFile:
            try:
                with open(File, "rb") as f:
                    if Content == f.read():
                        return False
            except:
                EdkLogger.error(None, FILE_OPEN_FAILURE, ExtraData=File)
        else:
            try:
                with open(File, "r") as f:
                    if Content == f.read():
                        return False
            except:
                EdkLogger.error(None, FILE_OPEN_FAILURE, ExtraData=File)

    DirName = os.path.dirname(File)
    if not CreateDirectory(DirName):
        EdkLogger.error(None, FILE_CREATE_FAILURE, "Could not create directory %s" % DirName)
    else:
        if DirName == '':
            DirName = os.getcwd()
        if not os.access(DirName, os.W_OK):
            EdkLogger.error(None, PERMISSION_FAILURE, "Do not have write permission on directory %s" % DirName)

    OpenMode = "w"
    if IsBinaryFile:
        OpenMode = "wb"

    # use default file_lock if no input new lock
    if not FileLock:
        FileLock = GlobalData.file_lock
    if FileLock:
        FileLock.acquire()


    if GlobalData.gIsWindows and not os.path.exists(File):
        try:
            with open(File, OpenMode) as tf:
                tf.write(Content)
        except IOError as X:
            if GlobalData.gBinCacheSource:
                EdkLogger.quiet("[cache error]:fails to save file with error: %s" % (X))
            else:
                EdkLogger.error(None, FILE_CREATE_FAILURE, ExtraData='IOError %s' % X)
        finally:
            if FileLock:
                FileLock.release()
    else:
        try:
            with open(File, OpenMode) as Fd:
                Fd.write(Content)
        except IOError as X:
            if GlobalData.gBinCacheSource:
                EdkLogger.quiet("[cache error]:fails to save file with error: %s" % (X))
            else:
                EdkLogger.error(None, FILE_CREATE_FAILURE, ExtraData='IOError %s' % X)
        finally:
            if FileLock:
                FileLock.release()

    return True

## Copy source file only if it is different from the destination file
#
#  This method is used to copy file only if the source file and destination
#  file content are different. This is quite useful to avoid duplicated
#  file writing.
#
#   @param      SrcFile   The path of source file
#   @param      Dst       The path of destination file or folder
#
#   @retval     True      The two files content are different and the file is copied
#   @retval     False     No copy really happen
#
def CopyFileOnChange(SrcFile, Dst, FileLock=None):

    # Convert to long file path format
    SrcFile = LongFilePath(SrcFile)
    Dst = LongFilePath(Dst)

    if os.path.isdir(SrcFile):
        EdkLogger.error(None, FILE_COPY_FAILURE, ExtraData='CopyFileOnChange SrcFile is a dir, not a file: %s' % SrcFile)
        return False

    if os.path.isdir(Dst):
        DstFile = os.path.join(Dst, os.path.basename(SrcFile))
    else:
        DstFile = Dst

    if os.path.exists(DstFile) and filecmp.cmp(SrcFile, DstFile, shallow=False):
        return False

    DirName = os.path.dirname(DstFile)
    if not CreateDirectory(DirName):
        EdkLogger.error(None, FILE_CREATE_FAILURE, "Could not create directory %s" % DirName)
    else:
        if DirName == '':
            DirName = os.getcwd()
        if not os.access(DirName, os.W_OK):
            EdkLogger.error(None, PERMISSION_FAILURE, "Do not have write permission on directory %s" % DirName)

    # use default file_lock if no input new lock
    if not FileLock:
        FileLock = GlobalData.file_lock
    if FileLock:
        FileLock.acquire()

    try:
        CopyLong(SrcFile, DstFile)
    except IOError as X:
        if GlobalData.gBinCacheSource:
            EdkLogger.quiet("[cache error]:fails to copy file with error: %s" % (X))
        else:
            EdkLogger.error(None, FILE_COPY_FAILURE, ExtraData='IOError %s' % X)
    finally:
        if FileLock:
            FileLock.release()

    return True

## Retrieve and cache the real path name in file system
#
#   @param      Root    The root directory of path relative to
#
#   @retval     str     The path string if the path exists
#   @retval     None    If path doesn't exist
#
class DirCache:
    _CACHE_ = set()
    _UPPER_CACHE_ = {}

    def __init__(self, Root):
        self._Root = Root
        for F in os.listdir(Root):
            self._CACHE_.add(F)
            self._UPPER_CACHE_[F.upper()] = F

    # =[] operator
    def __getitem__(self, Path):
        Path = Path[len(os.path.commonprefix([Path, self._Root])):]
        if not Path:
            return self._Root
        if Path and Path[0] == os.path.sep:
            Path = Path[1:]
        if Path in self._CACHE_:
            return os.path.join(self._Root, Path)
        UpperPath = Path.upper()
        if UpperPath in self._UPPER_CACHE_:
            return os.path.join(self._Root, self._UPPER_CACHE_[UpperPath])

        IndexList = []
        LastSepIndex = -1
        SepIndex = Path.find(os.path.sep)
        while SepIndex > -1:
            Parent = UpperPath[:SepIndex]
            if Parent not in self._UPPER_CACHE_:
                break
            LastSepIndex = SepIndex
            SepIndex = Path.find(os.path.sep, LastSepIndex + 1)

        if LastSepIndex == -1:
            return None

        Cwd = os.getcwd()
        os.chdir(self._Root)
        SepIndex = LastSepIndex
        while SepIndex > -1:
            Parent = Path[:SepIndex]
            ParentKey = UpperPath[:SepIndex]
            if ParentKey not in self._UPPER_CACHE_:
                os.chdir(Cwd)
                return None

            if Parent in self._CACHE_:
                ParentDir = Parent
            else:
                ParentDir = self._UPPER_CACHE_[ParentKey]
            for F in os.listdir(ParentDir):
                Dir = os.path.join(ParentDir, F)
                self._CACHE_.add(Dir)
                self._UPPER_CACHE_[Dir.upper()] = Dir

            SepIndex = Path.find(os.path.sep, SepIndex + 1)

        os.chdir(Cwd)
        if Path in self._CACHE_:
            return os.path.join(self._Root, Path)
        elif UpperPath in self._UPPER_CACHE_:
            return os.path.join(self._Root, self._UPPER_CACHE_[UpperPath])
        return None

def RealPath(File, Dir='', OverrideDir=''):
    NewFile = os.path.normpath(os.path.join(Dir, File))
    NewFile = GlobalData.gAllFiles[NewFile]
    if not NewFile and OverrideDir:
        NewFile = os.path.normpath(os.path.join(OverrideDir, File))
        NewFile = GlobalData.gAllFiles[NewFile]
    return NewFile

## Get GUID value from given packages
#
#   @param      CName           The CName of the GUID
#   @param      PackageList     List of packages looking-up in
#   @param      Inffile         The driver file
#
#   @retval     GuidValue   if the CName is found in any given package
#   @retval     None        if the CName is not found in all given packages
#
def GuidValue(CName, PackageList, Inffile = None):
    for P in PackageList:
        GuidKeys = list(P.Guids.keys())
        if Inffile and P._PrivateGuids:
            if not Inffile.startswith(P.MetaFile.Dir):
                GuidKeys = [x for x in P.Guids if x not in P._PrivateGuids]
        if CName in GuidKeys:
            return P.Guids[CName]
    return None

## A string template class
#
#  This class implements a template for string replacement. A string template
#  looks like following
#
#       ${BEGIN} other_string ${placeholder_name} other_string ${END}
#
#  The string between ${BEGIN} and ${END} will be repeated as many times as the
#  length of "placeholder_name", which is a list passed through a dict. The
#  "placeholder_name" is the key name of the dict. The ${BEGIN} and ${END} can
#  be not used and, in this case, the "placeholder_name" must not a list and it
#  will just be replaced once.
#
class TemplateString(object):
    _REPEAT_START_FLAG = "BEGIN"
    _REPEAT_END_FLAG = "END"

    class Section(object):
        _LIST_TYPES = [type([]), type(set()), type((0,))]

        def __init__(self, TemplateSection, PlaceHolderList):
            self._Template = TemplateSection
            self._PlaceHolderList = []

            # Split the section into sub-sections according to the position of placeholders
            if PlaceHolderList:
                self._SubSectionList = []
                SubSectionStart = 0
                #
                # The placeholders passed in must be in the format of
                #
                #   PlaceHolderName, PlaceHolderStartPoint, PlaceHolderEndPoint
                #
                for PlaceHolder, Start, End in PlaceHolderList:
                    self._SubSectionList.append(TemplateSection[SubSectionStart:Start])
                    self._SubSectionList.append(TemplateSection[Start:End])
                    self._PlaceHolderList.append(PlaceHolder)
                    SubSectionStart = End
                if SubSectionStart < len(TemplateSection):
                    self._SubSectionList.append(TemplateSection[SubSectionStart:])
            else:
                self._SubSectionList = [TemplateSection]

        def __str__(self):
            return self._Template + " : " + str(self._PlaceHolderList)

        def Instantiate(self, PlaceHolderValues):
            RepeatTime = -1
            RepeatPlaceHolders = {}
            NonRepeatPlaceHolders = {}

            for PlaceHolder in self._PlaceHolderList:
                if PlaceHolder not in PlaceHolderValues:
                    continue
                Value = PlaceHolderValues[PlaceHolder]
                if type(Value) in self._LIST_TYPES:
                    if RepeatTime < 0:
                        RepeatTime = len(Value)
                    elif RepeatTime != len(Value):
                        EdkLogger.error(
                                    "TemplateString",
                                    PARAMETER_INVALID,
                                    "${%s} has different repeat time from others!" % PlaceHolder,
                                    ExtraData=str(self._Template)
                                    )
                    RepeatPlaceHolders["${%s}" % PlaceHolder] = Value
                else:
                    NonRepeatPlaceHolders["${%s}" % PlaceHolder] = Value

            if NonRepeatPlaceHolders:
                StringList = []
                for S in self._SubSectionList:
                    if S not in NonRepeatPlaceHolders:
                        StringList.append(S)
                    else:
                        StringList.append(str(NonRepeatPlaceHolders[S]))
            else:
                StringList = self._SubSectionList

            if RepeatPlaceHolders:
                TempStringList = []
                for Index in range(RepeatTime):
                    for S in StringList:
                        if S not in RepeatPlaceHolders:
                            TempStringList.append(S)
                        else:
                            TempStringList.append(str(RepeatPlaceHolders[S][Index]))
                StringList = TempStringList

            return "".join(StringList)

    ## Constructor
    def __init__(self, Template=None):
        self.String = []
        self.IsBinary = False
        self._Template = Template
        self._TemplateSectionList = self._Parse(Template)

    ## str() operator
    #
    #   @retval     string  The string replaced
    #
    def __str__(self):
        return "".join(self.String)

    ## Split the template string into fragments per the ${BEGIN} and ${END} flags
    #
    #   @retval     list    A list of TemplateString.Section objects
    #
    def _Parse(self, Template):
        SectionStart = 0
        SearchFrom = 0
        MatchEnd = 0
        PlaceHolderList = []
        TemplateSectionList = []
        while Template:
            MatchObj = gPlaceholderPattern.search(Template, SearchFrom)
            if not MatchObj:
                if MatchEnd <= len(Template):
                    TemplateSection = TemplateString.Section(Template[SectionStart:], PlaceHolderList)
                    TemplateSectionList.append(TemplateSection)
                break

            MatchString = MatchObj.group(1)
            MatchStart = MatchObj.start()
            MatchEnd = MatchObj.end()

            if MatchString == self._REPEAT_START_FLAG:
                if MatchStart > SectionStart:
                    TemplateSection = TemplateString.Section(Template[SectionStart:MatchStart], PlaceHolderList)
                    TemplateSectionList.append(TemplateSection)
                SectionStart = MatchEnd
                PlaceHolderList = []
            elif MatchString == self._REPEAT_END_FLAG:
                TemplateSection = TemplateString.Section(Template[SectionStart:MatchStart], PlaceHolderList)
                TemplateSectionList.append(TemplateSection)
                SectionStart = MatchEnd
                PlaceHolderList = []
            else:
                PlaceHolderList.append((MatchString, MatchStart - SectionStart, MatchEnd - SectionStart))
            SearchFrom = MatchEnd
        return TemplateSectionList

    ## Replace the string template with dictionary of placeholders and append it to previous one
    #
    #   @param      AppendString    The string template to append
    #   @param      Dictionary      The placeholder dictionaries
    #
    def Append(self, AppendString, Dictionary=None):
        if Dictionary:
            SectionList = self._Parse(AppendString)
            self.String.append( "".join(S.Instantiate(Dictionary) for S in SectionList))
        else:
            if isinstance(AppendString,list):
                self.String.extend(AppendString)
            else:
                self.String.append(AppendString)

    ## Replace the string template with dictionary of placeholders
    #
    #   @param      Dictionary      The placeholder dictionaries
    #
    #   @retval     str             The string replaced with placeholder values
    #
    def Replace(self, Dictionary=None):
        return "".join(S.Instantiate(Dictionary) for S in self._TemplateSectionList)

## Progress indicator class
#
#  This class makes use of thread to print progress on console.
#
class Progressor:
    # for avoiding deadloop
    _StopFlag = None
    _ProgressThread = None
    _CheckInterval = 0.25

    ## Constructor
    #
    #   @param      OpenMessage     The string printed before progress characters
    #   @param      CloseMessage    The string printed after progress characters
    #   @param      ProgressChar    The character used to indicate the progress
    #   @param      Interval        The interval in seconds between two progress characters
    #
    def __init__(self, OpenMessage="", CloseMessage="", ProgressChar='.', Interval=1.0):
        self.PromptMessage = OpenMessage
        self.CodaMessage = CloseMessage
        self.ProgressChar = ProgressChar
        self.Interval = Interval
        if Progressor._StopFlag is None:
            Progressor._StopFlag = threading.Event()

    ## Start to print progress character
    #
    #   @param      OpenMessage     The string printed before progress characters
    #
    def Start(self, OpenMessage=None):
        if OpenMessage is not None:
            self.PromptMessage = OpenMessage
        Progressor._StopFlag.clear()
        if Progressor._ProgressThread is None:
            Progressor._ProgressThread = threading.Thread(target=self._ProgressThreadEntry)
            Progressor._ProgressThread.setDaemon(False)
            Progressor._ProgressThread.start()

    ## Stop printing progress character
    #
    #   @param      CloseMessage    The string printed after progress characters
    #
    def Stop(self, CloseMessage=None):
        OriginalCodaMessage = self.CodaMessage
        if CloseMessage is not None:
            self.CodaMessage = CloseMessage
        self.Abort()
        self.CodaMessage = OriginalCodaMessage

    ## Thread entry method
    def _ProgressThreadEntry(self):
        sys.stdout.write(self.PromptMessage + " ")
        sys.stdout.flush()
        TimeUp = 0.0
        while not Progressor._StopFlag.isSet():
            if TimeUp <= 0.0:
                sys.stdout.write(self.ProgressChar)
                sys.stdout.flush()
                TimeUp = self.Interval
            time.sleep(self._CheckInterval)
            TimeUp -= self._CheckInterval
        sys.stdout.write(" " + self.CodaMessage + "\n")
        sys.stdout.flush()

    ## Abort the progress display
    @staticmethod
    def Abort():
        if Progressor._StopFlag is not None:
            Progressor._StopFlag.set()
        if Progressor._ProgressThread is not None:
            Progressor._ProgressThread.join()
            Progressor._ProgressThread = None


## Dictionary using prioritized list as key
#
class tdict:
    _ListType = type([])
    _TupleType = type(())
    _Wildcard = 'COMMON'
    _ValidWildcardList = ['COMMON', 'DEFAULT', 'ALL', TAB_STAR, 'PLATFORM']

    def __init__(self, _Single_=False, _Level_=2):
        self._Level_ = _Level_
        self.data = {}
        self._Single_ = _Single_

    # =[] operator
    def __getitem__(self, key):
        KeyType = type(key)
        RestKeys = None
        if KeyType == self._ListType or KeyType == self._TupleType:
            FirstKey = key[0]
            if len(key) > 1:
                RestKeys = key[1:]
            elif self._Level_ > 1:
                RestKeys = [self._Wildcard for i in range(0, self._Level_ - 1)]
        else:
            FirstKey = key
            if self._Level_ > 1:
                RestKeys = [self._Wildcard for i in range(0, self._Level_ - 1)]

        if FirstKey is None or str(FirstKey).upper() in self._ValidWildcardList:
            FirstKey = self._Wildcard

        if self._Single_:
            return self._GetSingleValue(FirstKey, RestKeys)
        else:
            return self._GetAllValues(FirstKey, RestKeys)

    def _GetSingleValue(self, FirstKey, RestKeys):
        Value = None
        #print "%s-%s" % (FirstKey, self._Level_) ,
        if self._Level_ > 1:
            if FirstKey == self._Wildcard:
                if FirstKey in self.data:
                    Value = self.data[FirstKey][RestKeys]
                if Value is None:
                    for Key in self.data:
                        Value = self.data[Key][RestKeys]
                        if Value is not None: break
            else:
                if FirstKey in self.data:
                    Value = self.data[FirstKey][RestKeys]
                if Value is None and self._Wildcard in self.data:
                    #print "Value=None"
                    Value = self.data[self._Wildcard][RestKeys]
        else:
            if FirstKey == self._Wildcard:
                if FirstKey in self.data:
                    Value = self.data[FirstKey]
                if Value is None:
                    for Key in self.data:
                        Value = self.data[Key]
                        if Value is not None: break
            else:
                if FirstKey in self.data:
                    Value = self.data[FirstKey]
                elif self._Wildcard in self.data:
                    Value = self.data[self._Wildcard]
        return Value

    def _GetAllValues(self, FirstKey, RestKeys):
        Value = []
        if self._Level_ > 1:
            if FirstKey == self._Wildcard:
                for Key in self.data:
                    Value += self.data[Key][RestKeys]
            else:
                if FirstKey in self.data:
                    Value += self.data[FirstKey][RestKeys]
                if self._Wildcard in self.data:
                    Value += self.data[self._Wildcard][RestKeys]
        else:
            if FirstKey == self._Wildcard:
                for Key in self.data:
                    Value.append(self.data[Key])
            else:
                if FirstKey in self.data:
                    Value.append(self.data[FirstKey])
                if self._Wildcard in self.data:
                    Value.append(self.data[self._Wildcard])
        return Value

    ## []= operator
    def __setitem__(self, key, value):
        KeyType = type(key)
        RestKeys = None
        if KeyType == self._ListType or KeyType == self._TupleType:
            FirstKey = key[0]
            if len(key) > 1:
                RestKeys = key[1:]
            else:
                RestKeys = [self._Wildcard for i in range(0, self._Level_ - 1)]
        else:
            FirstKey = key
            if self._Level_ > 1:
                RestKeys = [self._Wildcard for i in range(0, self._Level_ - 1)]

        if FirstKey in self._ValidWildcardList:
            FirstKey = self._Wildcard

        if FirstKey not in self.data and self._Level_ > 0:
            self.data[FirstKey] = tdict(self._Single_, self._Level_ - 1)

        if self._Level_ > 1:
            self.data[FirstKey][RestKeys] = value
        else:
            self.data[FirstKey] = value

    def SetGreedyMode(self):
        self._Single_ = False
        if self._Level_ > 1:
            for Key in self.data:
                self.data[Key].SetGreedyMode()

    def SetSingleMode(self):
        self._Single_ = True
        if self._Level_ > 1:
            for Key in self.data:
                self.data[Key].SetSingleMode()

    def GetKeys(self, KeyIndex=0):
        assert KeyIndex >= 0
        if KeyIndex == 0:
            return set(self.data.keys())
        else:
            keys = set()
            for Key in self.data:
                keys |= self.data[Key].GetKeys(KeyIndex - 1)
            return keys

def AnalyzePcdExpression(Setting):
    RanStr = ''.join(sample(string.ascii_letters + string.digits, 8))
    Setting = Setting.replace('\\\\', RanStr).strip()
    # There might be escaped quote in a string: \", \\\" , \', \\\'
    Data = Setting
    # There might be '|' in string and in ( ... | ... ), replace it with '-'
    NewStr = ''
    InSingleQuoteStr = False
    InDoubleQuoteStr = False
    Pair = 0
    for Index, ch in enumerate(Data):
        if ch == '"' and not InSingleQuoteStr:
            if Data[Index - 1] != '\\':
                InDoubleQuoteStr = not InDoubleQuoteStr
        elif ch == "'" and not InDoubleQuoteStr:
            if Data[Index - 1] != '\\':
                InSingleQuoteStr = not InSingleQuoteStr
        elif ch == '(' and not (InSingleQuoteStr or InDoubleQuoteStr):
            Pair += 1
        elif ch == ')' and not (InSingleQuoteStr or InDoubleQuoteStr):
            Pair -= 1

        if (Pair > 0 or InSingleQuoteStr or InDoubleQuoteStr) and ch == TAB_VALUE_SPLIT:
            NewStr += '-'
        else:
            NewStr += ch
    FieldList = []
    StartPos = 0
    while True:
        Pos = NewStr.find(TAB_VALUE_SPLIT, StartPos)
        if Pos < 0:
            FieldList.append(Setting[StartPos:].strip())
            break
        FieldList.append(Setting[StartPos:Pos].strip())
        StartPos = Pos + 1
    for i, ch in enumerate(FieldList):
        if RanStr in ch:
            FieldList[i] = ch.replace(RanStr,'\\\\')
    return FieldList

def ParseFieldValue (Value):
    def ParseDevPathValue (Value):
        if '\\' in Value:
            Value.replace('\\', '/').replace(' ', '')

        Cmd = 'DevicePath ' + '"' + Value + '"'
        try:
            p = subprocess.Popen(Cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
            out, err = p.communicate()
        except Exception as X:
            raise BadExpression("DevicePath: %s" % (str(X)) )
        finally:
            subprocess._cleanup()
            p.stdout.close()
            p.stderr.close()
        if err:
            raise BadExpression("DevicePath: %s" % str(err))
        out = out.decode()
        Size = len(out.split())
        out = ','.join(out.split())
        return '{' + out + '}', Size

    if "{CODE(" in Value:
        return Value, len(Value.split(","))
    if isinstance(Value, type(0)):
        return Value, (Value.bit_length() + 7) // 8
    if not isinstance(Value, type('')):
        raise BadExpression('Type %s is %s' %(Value, type(Value)))
    Value = Value.strip()
    if Value.startswith(TAB_UINT8) and Value.endswith(')'):
        Value, Size = ParseFieldValue(Value.split('(', 1)[1][:-1])
        if Size > 1:
            raise BadExpression('Value (%s) Size larger than %d' %(Value, Size))
        return Value, 1
    if Value.startswith(TAB_UINT16) and Value.endswith(')'):
        Value, Size = ParseFieldValue(Value.split('(', 1)[1][:-1])
        if Size > 2:
            raise BadExpression('Value (%s) Size larger than %d' %(Value, Size))
        return Value, 2
    if Value.startswith(TAB_UINT32) and Value.endswith(')'):
        Value, Size = ParseFieldValue(Value.split('(', 1)[1][:-1])
        if Size > 4:
            raise BadExpression('Value (%s) Size larger than %d' %(Value, Size))
        return Value, 4
    if Value.startswith(TAB_UINT64) and Value.endswith(')'):
        Value, Size = ParseFieldValue(Value.split('(', 1)[1][:-1])
        if Size > 8:
            raise BadExpression('Value (%s) Size larger than %d' % (Value, Size))
        return Value, 8
    if Value.startswith(TAB_GUID) and Value.endswith(')'):
        Value = Value.split('(', 1)[1][:-1].strip()
        if Value[0] == '{' and Value[-1] == '}':
            TmpValue = GuidStructureStringToGuidString(Value)
            if not TmpValue:
                raise BadExpression("Invalid GUID value string %s" % Value)
            Value = TmpValue
        if Value[0] == '"' and Value[-1] == '"':
            Value = Value[1:-1]
        try:
            Value = uuid.UUID(Value).bytes_le
            ValueL, ValueH = struct.unpack('2Q', Value)
            Value = (ValueH << 64 ) | ValueL

        except ValueError as Message:
            raise BadExpression(Message)
        return Value, 16
    if Value.startswith('L"') and Value.endswith('"'):
        # Unicode String
        # translate escape character
        Value = Value[1:]
        try:
            Value = eval(Value)
        except:
            Value = Value[1:-1]
        List = list(Value)
        List.reverse()
        Value = 0
        for Char in List:
            Value = (Value << 16) | ord(Char)
        return Value, (len(List) + 1) * 2
    if Value.startswith('"') and Value.endswith('"'):
        # ASCII String
        # translate escape character
        try:
            Value = eval(Value)
        except:
            Value = Value[1:-1]
        List = list(Value)
        List.reverse()
        Value = 0
        for Char in List:
            Value = (Value << 8) | ord(Char)
        return Value, len(List) + 1
    if Value.startswith("L'") and Value.endswith("'"):
        # Unicode Character Constant
        # translate escape character
        Value = Value[1:]
        try:
            Value = eval(Value)
        except:
            Value = Value[1:-1]
        List = list(Value)
        if len(List) == 0:
            raise BadExpression('Length %s is %s' % (Value, len(List)))
        List.reverse()
        Value = 0
        for Char in List:
            Value = (Value << 16) | ord(Char)
        return Value, len(List) * 2
    if Value.startswith("'") and Value.endswith("'"):
        # Character constant
        # translate escape character
        try:
            Value = eval(Value)
        except:
            Value = Value[1:-1]
        List = list(Value)
        if len(List) == 0:
            raise BadExpression('Length %s is %s' % (Value, len(List)))
        List.reverse()
        Value = 0
        for Char in List:
            Value = (Value << 8) | ord(Char)
        return Value, len(List)
    if Value.startswith('{') and Value.endswith('}'):
        # Byte array
        Value = Value[1:-1]
        List = [Item.strip() for Item in Value.split(',')]
        List.reverse()
        Value = 0
        RetSize = 0
        for Item in List:
            ItemValue, Size = ParseFieldValue(Item)
            RetSize += Size
            for I in range(Size):
                Value = (Value << 8) | ((ItemValue >> 8 * I) & 0xff)
        return Value, RetSize
    if Value.startswith('DEVICE_PATH(') and Value.endswith(')'):
        Value = Value.replace("DEVICE_PATH(", '').rstrip(')')
        Value = Value.strip().strip('"')
        return ParseDevPathValue(Value)
    if Value.lower().startswith('0x'):
        try:
            Value = int(Value, 16)
        except:
            raise BadExpression("invalid hex value: %s" % Value)
        if Value == 0:
            return 0, 1
        return Value, (Value.bit_length() + 7) // 8
    if Value[0].isdigit():
        Value = int(Value, 10)
        if Value == 0:
            return 0, 1
        return Value, (Value.bit_length() + 7) // 8
    if Value.lower() == 'true':
        return 1, 1
    if Value.lower() == 'false':
        return 0, 1
    return Value, 1

## AnalyzeDscPcd
#
#  Analyze DSC PCD value, since there is no data type info in DSC
#  This function is used to match functions (AnalyzePcdData) used for retrieving PCD value from database
#  1. Feature flag: TokenSpace.PcdCName|PcdValue
#  2. Fix and Patch:TokenSpace.PcdCName|PcdValue[|VOID*[|MaxSize]]
#  3. Dynamic default:
#     TokenSpace.PcdCName|PcdValue[|VOID*[|MaxSize]]
#     TokenSpace.PcdCName|PcdValue
#  4. Dynamic VPD:
#     TokenSpace.PcdCName|VpdOffset[|VpdValue]
#     TokenSpace.PcdCName|VpdOffset[|MaxSize[|VpdValue]]
#  5. Dynamic HII:
#     TokenSpace.PcdCName|HiiString|VariableGuid|VariableOffset[|HiiValue]
#  PCD value needs to be located in such kind of string, and the PCD value might be an expression in which
#    there might have "|" operator, also in string value.
#
#  @param Setting: String contain information described above with "TokenSpace.PcdCName|" stripped
#  @param PcdType: PCD type: feature, fixed, dynamic default VPD HII
#  @param DataType: The datum type of PCD: VOID*, UNIT, BOOL
#  @retval:
#    ValueList: A List contain fields described above
#    IsValid:   True if conforming EBNF, otherwise False
#    Index:     The index where PcdValue is in ValueList
#
def AnalyzeDscPcd(Setting, PcdType, DataType=''):
    FieldList = AnalyzePcdExpression(Setting)

    IsValid = True
    if PcdType in (MODEL_PCD_FIXED_AT_BUILD, MODEL_PCD_PATCHABLE_IN_MODULE, MODEL_PCD_DYNAMIC_DEFAULT, MODEL_PCD_DYNAMIC_EX_DEFAULT):
        Value = FieldList[0]
        Size = ''
        if len(FieldList) > 1 and FieldList[1]:
            DataType = FieldList[1]
            if FieldList[1] != TAB_VOID and StructPattern.match(FieldList[1]) is None:
                IsValid = False
        if len(FieldList) > 2:
            Size = FieldList[2]
        if IsValid:
            if DataType == "":
                IsValid = (len(FieldList) <= 1)
            else:
                IsValid = (len(FieldList) <= 3)

        if Size:
            try:
                int(Size, 16) if Size.upper().startswith("0X") else int(Size)
            except:
                IsValid = False
                Size = -1
        return [str(Value), DataType, str(Size)], IsValid, 0
    elif PcdType == MODEL_PCD_FEATURE_FLAG:
        Value = FieldList[0]
        Size = ''
        IsValid = (len(FieldList) <= 1)
        return [Value, DataType, str(Size)], IsValid, 0
    elif PcdType in (MODEL_PCD_DYNAMIC_VPD, MODEL_PCD_DYNAMIC_EX_VPD):
        VpdOffset = FieldList[0]
        Value = Size = ''
        if not DataType == TAB_VOID:
            if len(FieldList) > 1:
                Value = FieldList[1]
        else:
            if len(FieldList) > 1:
                Size = FieldList[1]
            if len(FieldList) > 2:
                Value = FieldList[2]
        if DataType == "":
            IsValid = (len(FieldList) <= 1)
        else:
            IsValid = (len(FieldList) <= 3)
        if Size:
            try:
                int(Size, 16) if Size.upper().startswith("0X") else int(Size)
            except:
                IsValid = False
                Size = -1
        return [VpdOffset, str(Size), Value], IsValid, 2
    elif PcdType in (MODEL_PCD_DYNAMIC_HII, MODEL_PCD_DYNAMIC_EX_HII):
        IsValid = (3 <= len(FieldList) <= 5)
        HiiString = FieldList[0]
        Guid = Offset = Value = Attribute = ''
        if len(FieldList) > 1:
            Guid = FieldList[1]
        if len(FieldList) > 2:
            Offset = FieldList[2]
        if len(FieldList) > 3:
            Value = FieldList[3]
        if len(FieldList) > 4:
            Attribute = FieldList[4]
        return [HiiString, Guid, Offset, Value, Attribute], IsValid, 3
    return [], False, 0

## AnalyzePcdData
#
#  Analyze the pcd Value, Datum type and TokenNumber.
#  Used to avoid split issue while the value string contain "|" character
#
#  @param[in] Setting:  A String contain value/datum type/token number information;
#
#  @retval   ValueList: A List contain value, datum type and toke number.
#
def AnalyzePcdData(Setting):
    ValueList = ['', '', '']

    ValueRe = re.compile(r'^\s*L?\".*\|.*\"')
    PtrValue = ValueRe.findall(Setting)

    ValueUpdateFlag = False

    if len(PtrValue) >= 1:
        Setting = re.sub(ValueRe, '', Setting)
        ValueUpdateFlag = True

    TokenList = Setting.split(TAB_VALUE_SPLIT)
    ValueList[0:len(TokenList)] = TokenList

    if ValueUpdateFlag:
        ValueList[0] = PtrValue[0]

    return ValueList

## check format of PCD value against its the datum type
#
# For PCD value setting
#
def CheckPcdDatum(Type, Value):
    if Type == TAB_VOID:
        ValueRe = re.compile(r'\s*L?\".*\"\s*$')
        if not (((Value.startswith('L"') or Value.startswith('"')) and Value.endswith('"'))
                or (Value.startswith('{') and Value.endswith('}')) or (Value.startswith("L'") or Value.startswith("'") and Value.endswith("'"))
               ):
            return False, "Invalid value [%s] of type [%s]; must be in the form of {...} for array"\
                          ", \"...\" or \'...\' for string, L\"...\" or L\'...\' for unicode string" % (Value, Type)
        elif ValueRe.match(Value):
            # Check the chars in UnicodeString or CString is printable
            if Value.startswith("L"):
                Value = Value[2:-1]
            else:
                Value = Value[1:-1]
            Printset = set(string.printable)
            Printset.remove(TAB_PRINTCHAR_VT)
            Printset.add(TAB_PRINTCHAR_BS)
            Printset.add(TAB_PRINTCHAR_NUL)
            if not set(Value).issubset(Printset):
                PrintList = sorted(Printset)
                return False, "Invalid PCD string value of type [%s]; must be printable chars %s." % (Type, PrintList)
    elif Type == 'BOOLEAN':
        if Value not in ['TRUE', 'True', 'true', '0x1', '0x01', '1', 'FALSE', 'False', 'false', '0x0', '0x00', '0']:
            return False, "Invalid value [%s] of type [%s]; must be one of TRUE, True, true, 0x1, 0x01, 1"\
                          ", FALSE, False, false, 0x0, 0x00, 0" % (Value, Type)
    elif Type in [TAB_UINT8, TAB_UINT16, TAB_UINT32, TAB_UINT64]:
        if Value.startswith('0') and not Value.lower().startswith('0x') and len(Value) > 1 and Value.lstrip('0'):
            Value = Value.lstrip('0')
        try:
            if Value and int(Value, 0) < 0:
                return False, "PCD can't be set to negative value[%s] for datum type [%s]" % (Value, Type)
            Value = int(Value, 0)
            if Value > MAX_VAL_TYPE[Type]:
                return False, "Too large PCD value[%s] for datum type [%s]" % (Value, Type)
        except:
            return False, "Invalid value [%s] of type [%s];"\
                          " must be a hexadecimal, decimal or octal in C language format." % (Value, Type)
    else:
        return True, "StructurePcd"

    return True, ""

def CommonPath(PathList):
    P1 = min(PathList).split(os.path.sep)
    P2 = max(PathList).split(os.path.sep)
    for Index in range(min(len(P1), len(P2))):
        if P1[Index] != P2[Index]:
            return os.path.sep.join(P1[:Index])
    return os.path.sep.join(P1)

class PathClass(object):
    def __init__(self, File='', Root='', AlterRoot='', Type='', IsBinary=False,
                 Arch='COMMON', ToolChainFamily='', Target='', TagName='', ToolCode=''):
        self.Arch = Arch
        self.File = str(File)
        if os.path.isabs(self.File):
            self.Root = ''
            self.AlterRoot = ''
        else:
            self.Root = str(Root)
            self.AlterRoot = str(AlterRoot)

        # Remove any '.' and '..' in path
        if self.Root:
            self.Root = mws.getWs(self.Root, self.File)
            self.Path = os.path.normpath(os.path.join(self.Root, self.File))
            self.Root = os.path.normpath(CommonPath([self.Root, self.Path]))
            # eliminate the side-effect of 'C:'
            if self.Root[-1] == ':':
                self.Root += os.path.sep
            # file path should not start with path separator
            if self.Root[-1] == os.path.sep:
                self.File = self.Path[len(self.Root):]
            else:
                self.File = self.Path[len(self.Root) + 1:]
        else:
            self.Path = os.path.normpath(self.File)

        self.SubDir, self.Name = os.path.split(self.File)
        self.BaseName, self.Ext = os.path.splitext(self.Name)

        if self.Root:
            if self.SubDir:
                self.Dir = os.path.join(self.Root, self.SubDir)
            else:
                self.Dir = self.Root
        else:
            self.Dir = self.SubDir

        if IsBinary:
            self.Type = Type
        else:
            self.Type = self.Ext.lower()

        self.IsBinary = IsBinary
        self.Target = Target
        self.TagName = TagName
        self.ToolCode = ToolCode
        self.ToolChainFamily = ToolChainFamily
        self.OriginalPath = self

    ## Convert the object of this class to a string
    #
    #  Convert member Path of the class to a string
    #
    #  @retval string Formatted String
    #
    def __str__(self):
        return self.Path

    ## Override __eq__ function
    #
    # Check whether PathClass are the same
    #
    # @retval False The two PathClass are different
    # @retval True  The two PathClass are the same
    #
    def __eq__(self, Other):
        return self.Path == str(Other)

    ## Override __cmp__ function
    #
    # Customize the comparison operation of two PathClass
    #
    # @retval 0     The two PathClass are different
    # @retval -1    The first PathClass is less than the second PathClass
    # @retval 1     The first PathClass is Bigger than the second PathClass
    def __cmp__(self, Other):
        OtherKey = str(Other)

        SelfKey = self.Path
        if SelfKey == OtherKey:
            return 0
        elif SelfKey > OtherKey:
            return 1
        else:
            return -1

    ## Override __hash__ function
    #
    # Use Path as key in hash table
    #
    # @retval string Key for hash table
    #
    def __hash__(self):
        return hash(self.Path)

    @cached_property
    def Key(self):
        return self.Path.upper()

    @property
    def TimeStamp(self):
        return os.stat(self.Path)[8]

    def Validate(self, Type='', CaseSensitive=True):
        def RealPath2(File, Dir='', OverrideDir=''):
            NewFile = None
            if OverrideDir:
                NewFile = GlobalData.gAllFiles[os.path.normpath(os.path.join(OverrideDir, File))]
                if NewFile:
                    if OverrideDir[-1] == os.path.sep:
                        return NewFile[len(OverrideDir):], NewFile[0:len(OverrideDir)]
                    else:
                        return NewFile[len(OverrideDir) + 1:], NewFile[0:len(OverrideDir)]
            if GlobalData.gAllFiles:
                NewFile = GlobalData.gAllFiles[os.path.normpath(os.path.join(Dir, File))]
            if not NewFile:
                NewFile = os.path.normpath(os.path.join(Dir, File))
                if not os.path.exists(NewFile):
                    return None, None
            if NewFile:
                if Dir:
                    if Dir[-1] == os.path.sep:
                        return NewFile[len(Dir):], NewFile[0:len(Dir)]
                    else:
                        return NewFile[len(Dir) + 1:], NewFile[0:len(Dir)]
                else:
                    return NewFile, ''

            return None, None

        if GlobalData.gCaseInsensitive:
            CaseSensitive = False
        if Type and Type.lower() != self.Type:
            return FILE_TYPE_MISMATCH, '%s (expect %s but got %s)' % (self.File, Type, self.Type)

        RealFile, RealRoot = RealPath2(self.File, self.Root, self.AlterRoot)
        if not RealRoot and not RealFile:
            RealFile = self.File
            if self.AlterRoot:
                RealFile = os.path.join(self.AlterRoot, self.File)
            elif self.Root:
                RealFile = os.path.join(self.Root, self.File)
            if len (mws.getPkgPath()) == 0:
                return FILE_NOT_FOUND, os.path.join(self.AlterRoot, RealFile)
            else:
                return FILE_NOT_FOUND, "%s is not found in packages path:\n\t%s" % (self.File, '\n\t'.join(mws.getPkgPath()))

        ErrorCode = 0
        ErrorInfo = ''
        if RealRoot != self.Root or RealFile != self.File:
            if CaseSensitive and (RealFile != self.File or (RealRoot != self.Root and RealRoot != self.AlterRoot)):
                ErrorCode = FILE_CASE_MISMATCH
                ErrorInfo = self.File + '\n\t' + RealFile + " [in file system]"

            self.SubDir, self.Name = os.path.split(RealFile)
            self.BaseName, self.Ext = os.path.splitext(self.Name)
            if self.SubDir:
                self.Dir = os.path.join(RealRoot, self.SubDir)
            else:
                self.Dir = RealRoot
            self.File = RealFile
            self.Root = RealRoot
            self.Path = os.path.join(RealRoot, RealFile)
        return ErrorCode, ErrorInfo

## Parse PE image to get the required PE information.
#
class PeImageClass():
    ## Constructor
    #
    #   @param  File FilePath of PeImage
    #
    def __init__(self, PeFile):
        self.FileName   = PeFile
        self.IsValid    = False
        self.Size       = 0
        self.EntryPoint = 0
        self.SectionAlignment  = 0
        self.SectionHeaderList = []
        self.ErrorInfo = ''
        try:
            PeObject = open(PeFile, 'rb')
        except:
            self.ErrorInfo = self.FileName + ' can not be found\n'
            return
        # Read DOS header
        ByteArray = array.array('B')
        ByteArray.fromfile(PeObject, 0x3E)
        ByteList = ByteArray.tolist()
        # DOS signature should be 'MZ'
        if self._ByteListToStr (ByteList[0x0:0x2]) != 'MZ':
            self.ErrorInfo = self.FileName + ' has no valid DOS signature MZ'
            return

        # Read 4 byte PE Signature
        PeOffset = self._ByteListToInt(ByteList[0x3C:0x3E])
        PeObject.seek(PeOffset)
        ByteArray = array.array('B')
        ByteArray.fromfile(PeObject, 4)
        # PE signature should be 'PE\0\0'
        if ByteArray.tolist() != [ord('P'), ord('E'), 0, 0]:
            self.ErrorInfo = self.FileName + ' has no valid PE signature PE00'
            return

        # Read PE file header
        ByteArray = array.array('B')
        ByteArray.fromfile(PeObject, 0x14)
        ByteList = ByteArray.tolist()
        SecNumber = self._ByteListToInt(ByteList[0x2:0x4])
        if SecNumber == 0:
            self.ErrorInfo = self.FileName + ' has no section header'
            return

        # Read PE optional header
        OptionalHeaderSize = self._ByteListToInt(ByteArray[0x10:0x12])
        ByteArray = array.array('B')
        ByteArray.fromfile(PeObject, OptionalHeaderSize)
        ByteList = ByteArray.tolist()
        self.EntryPoint       = self._ByteListToInt(ByteList[0x10:0x14])
        self.SectionAlignment = self._ByteListToInt(ByteList[0x20:0x24])
        self.Size             = self._ByteListToInt(ByteList[0x38:0x3C])

        # Read each Section Header
        for Index in range(SecNumber):
            ByteArray = array.array('B')
            ByteArray.fromfile(PeObject, 0x28)
            ByteList = ByteArray.tolist()
            SecName  = self._ByteListToStr(ByteList[0:8])
            SecVirtualSize = self._ByteListToInt(ByteList[8:12])
            SecRawAddress  = self._ByteListToInt(ByteList[20:24])
            SecVirtualAddress = self._ByteListToInt(ByteList[12:16])
            self.SectionHeaderList.append((SecName, SecVirtualAddress, SecRawAddress, SecVirtualSize))
        self.IsValid = True
        PeObject.close()

    def _ByteListToStr(self, ByteList):
        String = ''
        for index in range(len(ByteList)):
            if ByteList[index] == 0:
                break
            String += chr(ByteList[index])
        return String

    def _ByteListToInt(self, ByteList):
        Value = 0
        for index in range(len(ByteList) - 1, -1, -1):
            Value = (Value << 8) | int(ByteList[index])
        return Value

class DefaultStore():
    def __init__(self, DefaultStores ):

        self.DefaultStores = DefaultStores
    def DefaultStoreID(self, DefaultStoreName):
        for key, value in self.DefaultStores.items():
            if value == DefaultStoreName:
                return key
        return None
    def GetDefaultDefault(self):
        if not self.DefaultStores or "0" in self.DefaultStores:
            return "0", TAB_DEFAULT_STORES_DEFAULT
        else:
            minvalue = min(int(value_str) for value_str in self.DefaultStores)
            return (str(minvalue), self.DefaultStores[str(minvalue)])
    def GetMin(self, DefaultSIdList):
        if not DefaultSIdList:
            return TAB_DEFAULT_STORES_DEFAULT
        storeidset = {storeid for storeid, storename in self.DefaultStores.values() if storename in DefaultSIdList}
        if not storeidset:
            return ""
        minid = min(storeidset )
        for sid, name in self.DefaultStores.values():
            if sid == minid:
                return name

class SkuClass():
    DEFAULT = 0
    SINGLE = 1
    MULTIPLE =2

    def __init__(self,SkuIdentifier='', SkuIds=None):
        if SkuIds is None:
            SkuIds = {}

        for SkuName in SkuIds:
            SkuId = SkuIds[SkuName][0]
            skuid_num = int(SkuId, 16) if SkuId.upper().startswith("0X") else int(SkuId)
            if skuid_num > 0xFFFFFFFFFFFFFFFF:
                EdkLogger.error("build", PARAMETER_INVALID,
                            ExtraData = "SKU-ID [%s] value %s exceeds the max value of UINT64"
                                      % (SkuName, SkuId))

        self.AvailableSkuIds = OrderedDict()
        self.SkuIdSet = []
        self.SkuIdNumberSet = []
        self.SkuData = SkuIds
        self._SkuInherit = {}
        self._SkuIdentifier = SkuIdentifier
        if SkuIdentifier == '' or SkuIdentifier is None:
            self.SkuIdSet = ['DEFAULT']
            self.SkuIdNumberSet = ['0U']
        elif SkuIdentifier == 'ALL':
            self.SkuIdSet = list(SkuIds.keys())
            self.SkuIdNumberSet = [num[0].strip() + 'U' for num in SkuIds.values()]
        else:
            r = SkuIdentifier.split('|')
            self.SkuIdSet=[(r[k].strip()).upper() for k in range(len(r))]
            k = None
            try:
                self.SkuIdNumberSet = [SkuIds[k][0].strip() + 'U' for k in self.SkuIdSet]
            except Exception:
                EdkLogger.error("build", PARAMETER_INVALID,
                            ExtraData = "SKU-ID [%s] is not supported by the platform. [Valid SKU-ID: %s]"
                                      % (k, " | ".join(SkuIds.keys())))
        for each in self.SkuIdSet:
            if each in SkuIds:
                self.AvailableSkuIds[each] = SkuIds[each][0]
            else:
                EdkLogger.error("build", PARAMETER_INVALID,
                            ExtraData="SKU-ID [%s] is not supported by the platform. [Valid SKU-ID: %s]"
                                      % (each, " | ".join(SkuIds.keys())))
        if self.SkuUsageType != SkuClass.SINGLE:
            self.AvailableSkuIds.update({'DEFAULT':0, 'COMMON':0})
        if self.SkuIdSet:
            GlobalData.gSkuids = (self.SkuIdSet)
            if 'COMMON' in GlobalData.gSkuids:
                GlobalData.gSkuids.remove('COMMON')
            if self.SkuUsageType == self.SINGLE:
                if len(GlobalData.gSkuids) != 1:
                    if 'DEFAULT' in GlobalData.gSkuids:
                        GlobalData.gSkuids.remove('DEFAULT')
            if GlobalData.gSkuids:
                GlobalData.gSkuids.sort()

    def GetNextSkuId(self, skuname):
        if not self._SkuInherit:
            self._SkuInherit = {}
            for item in self.SkuData.values():
                self._SkuInherit[item[1]]=item[2] if item[2] else "DEFAULT"
        return self._SkuInherit.get(skuname, "DEFAULT")

    def GetSkuChain(self, sku):
        if sku == "DEFAULT":
            return ["DEFAULT"]
        skulist = [sku]
        nextsku = sku
        while True:
            nextsku = self.GetNextSkuId(nextsku)
            skulist.append(nextsku)
            if nextsku == "DEFAULT":
                break
        skulist.reverse()
        return skulist
    def SkuOverrideOrder(self):
        skuorderset = []
        for skuname in self.SkuIdSet:
            skuorderset.append(self.GetSkuChain(skuname))

        skuorder = []
        for index in range(max(len(item) for item in skuorderset)):
            for subset in skuorderset:
                if index > len(subset)-1:
                    continue
                if subset[index] in skuorder:
                    continue
                skuorder.append(subset[index])

        return skuorder

    @property
    def SkuUsageType(self):
        if self._SkuIdentifier.upper() == "ALL":
            return SkuClass.MULTIPLE

        if len(self.SkuIdSet) == 1:
            if self.SkuIdSet[0] == 'DEFAULT':
                return SkuClass.DEFAULT
            return SkuClass.SINGLE
        if len(self.SkuIdSet) == 2 and 'DEFAULT' in self.SkuIdSet:
            return SkuClass.SINGLE
        return SkuClass.MULTIPLE

    def DumpSkuIdArrary(self):
        if self.SkuUsageType == SkuClass.SINGLE:
            return "{0x0}"
        ArrayStrList = []
        for skuname in self.AvailableSkuIds:
            if skuname == "COMMON":
                continue
            while skuname != "DEFAULT":
                ArrayStrList.append(hex(int(self.AvailableSkuIds[skuname])))
                skuname = self.GetNextSkuId(skuname)
            ArrayStrList.append("0x0")
        return "{{{myList}}}".format(myList=",".join(ArrayStrList))

    @property
    def AvailableSkuIdSet(self):
        return self.AvailableSkuIds

    @property
    def SystemSkuId(self):
        if self.SkuUsageType == SkuClass.SINGLE:
            if len(self.SkuIdSet) == 1:
                return self.SkuIdSet[0]
            else:
                return self.SkuIdSet[0] if self.SkuIdSet[0] != 'DEFAULT' else self.SkuIdSet[1]
        else:
            return 'DEFAULT'

##  Get the integer value from string like "14U" or integer like 2
#
#   @param      Input   The object that may be either a integer value or a string
#
#   @retval     Value    The integer value that the input represents
#
def GetIntegerValue(Input):
    if not isinstance(Input, str):
        return Input
    String = Input
    if String.endswith("U"):
        String = String[:-1]
    if String.endswith("ULL"):
        String = String[:-3]
    if String.endswith("LL"):
        String = String[:-2]

    if String.startswith("0x") or String.startswith("0X"):
        return int(String, 16)
    elif String == '':
        return 0
    else:
        return int(String)

#
# Pack a GUID (registry format) list into a buffer and return it
#
def PackGUID(Guid):
    return pack(PACK_PATTERN_GUID,
                int(Guid[0], 16),
                int(Guid[1], 16),
                int(Guid[2], 16),
                int(Guid[3][-4:-2], 16),
                int(Guid[3][-2:], 16),
                int(Guid[4][-12:-10], 16),
                int(Guid[4][-10:-8], 16),
                int(Guid[4][-8:-6], 16),
                int(Guid[4][-6:-4], 16),
                int(Guid[4][-4:-2], 16),
                int(Guid[4][-2:], 16)
                )

#
# Pack a GUID (byte) list into a buffer and return it
#
def PackByteFormatGUID(Guid):
    return pack(PACK_PATTERN_GUID,
                Guid[0],
                Guid[1],
                Guid[2],
                Guid[3],
                Guid[4],
                Guid[5],
                Guid[6],
                Guid[7],
                Guid[8],
                Guid[9],
                Guid[10],
                )

## DeepCopy dict/OrderedDict recusively
#
#   @param      ori_dict    a nested dict or ordereddict
#
#   @retval     new dict or orderdict
#
def CopyDict(ori_dict):
    dict_type = ori_dict.__class__
    if dict_type not in (dict,OrderedDict):
        return ori_dict
    new_dict = dict_type()
    for key in ori_dict:
        if isinstance(ori_dict[key],(dict,OrderedDict)):
            new_dict[key] = CopyDict(ori_dict[key])
        else:
            new_dict[key] = ori_dict[key]
    return new_dict

#
# Remove the c/c++ comments: // and /* */
#
def RemoveCComments(ctext):
    return re.sub('//.*?\n|/\\*.*?\\*/', '\n', ctext, flags=re.S)
