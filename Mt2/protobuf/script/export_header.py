# -*- coding: utf-8 -*-

import os
from . import config

SPACE_PER_TAB = 4
TAB_AS_SPACE = "".join(" " for i in range(SPACE_PER_TAB))
def makeTab(i):
    return "".join(TAB_AS_SPACE for i in range(i))

## Decodes a value that's only one word or if it's in quotation marks (") everything that is in them
def DecodeValue(value):
    if len(value) == 0:
        return value

    # tuple support
    if value[0] == '(' and len(value) > 1 and value.rfind(')') > value.find(",") and value.find(",") > 0:
        value = value[1:value.rfind(")")]
        ret = ()
        while 1:
            curVal, endPos = DecodeValue(value)
            ret += (curVal,)

            if endPos == -1:
                break

            value = value[endPos:]
            nextValPos = value.find(",")
            if nextValPos < 0:
                break

            value = value[nextValPos+1:].lstrip()
            if not value:
                break

        return ret, -1

    # quotation mark support
    useQuotMarks = False
    if value[0] == '"':
        useQuotMarks = True

    # normal value finding
    posStart = 0
    posEndAdd = 0
    if useQuotMarks:
        posStart = 1
        posEnd = posStart
        while 1:
            posEnd = value.find("\"", posEnd)
            if posEnd < 0:
                print("no quot end found : " + value)
                return "", -1

            if value[posEnd-1] != '\\':
                break

            ## remove \" => "
            value = value[:posEnd-1] + value[posEnd:]
            posEndAdd += 1
    else:
        posEnd1 = value.find(" ")
        posEnd2 = value.find(",")
        if posEnd1 == -1:
            posEnd = posEnd2
        elif posEnd2 == -1:
            posEnd = posEnd1
        else:
            posEnd = min(posEnd1, posEnd2)
        if posEnd < 0:
            posEnd = len(value)

    return value[posStart:posEnd], posEnd

## Decodes a key-value pair (like key=value or key="value" or key = value or key = (value1, value2, value3) as tuple)
def DecodeKVPair(pairString, getEndPos = False):
    pos = pairString.find("=")

    addPos = 0

    tmpPos = pos - 1
    while tmpPos >= 0 and pairString[tmpPos] == ' ':
        pairString = pairString[:tmpPos] + pairString[tmpPos+1:]
        pos -= 1
        addPos += 1
    tmpPos = pos + 1
    while tmpPos < len(pairString) and pairString[tmpPos] == ' ':
        pairString = pairString[:tmpPos] + pairString[tmpPos+1:]
        addPos += 1

    key = pairString[:pos]
    value = pairString[pos+1:]

    key = DecodeValue(key[::-1])[0][::-1]
    value, endPos = DecodeValue(value)

    if getEndPos:
        return (key, value, pos+1 + addPos + endPos)
    else:
        return (key, value)

## Checks whether an string value is true or false
def IsStringTrue(value):
    if value.lower() == 'true':
        return True
    if value == "1":
        return True

    return False

########################################
## Single Header-File
########################################
class Exporter():

    ########################################
    ## Single Import
    ########################################
    class ImportInfo():

        def __init__(self, fileName, aliasName):
            self._fileName = fileName
            self._aliasName = aliasName
            self._packetList = []

        def GetFileName(self):
            return self._fileName

        def GetAliasName(self):
            return self._aliasName

        def AddPacket(self, packetName):
            self._packetList.append(packetName)

        def GetPacketCount(self):
            return len(self._packetList)

        def HasPacket(self, packetName):
            return packetName in self._packetList

    ########################################
    ## Single Header-Group
    ########################################
    class Group():

        def __init__(self, name):
            self._name = name
            self._headerList = []
            self._options = {}

        def GetName(self):
            return self._name

        def GetPacketGroupName(self):
            name = self.GetName()
            if name.lower().endswith("header"):
                name = name[:-len("header")]

            return name + "Packet"

        def SetOption(self, key, value):
            self._options[key.lower()] = value

        def HasOption(self, key):
            return key.lower() in self._options

        def GetOption(self, key, default_value = ""):
            key = key.lower()
            if not key in self._options:
                return default_value

            return self._options[key]

        def AddHeader(self, headerName, packetInfo):
            self._headerList.append({
                "header" : headerName,
                "packet" : packetInfo,
            })

        def GetHeaderCount(self):
            return len(self._headerList)

        def GetHeaderList(self):
            return self._headerList

    ########################################
    ## Single Prototype-File
    ########################################
    class PrototypeFile():

        class FuncParams():

            def __init__(self):
                self._content = {}

            def Add(self, key, value):
                self._content[key.lower()] = value

            def Get(self, key, default_value = None):
                key = key.lower()
                if not key in self._content:
                    return default_value

                return self._content[key]

        PATH = "prototype"

        VAR_BEGIN_KEY = "[<"
        VAR_END_KEY = ">]"

        def __init__(self, fileName):
            targetFile = None
            targetFilePos = fileName.find("=>")
            if targetFilePos > 0:
                targetFile = fileName[targetFilePos+2:].lstrip()
                fileName = fileName[:targetFilePos].rstrip()

            filePath = os.path.join(os.path.dirname(os.path.realpath(__file__)), self.PATH)

            self._fileName = os.path.join(filePath, fileName)
            self._targetFileName = targetFile
            self._content = []
            self._paramDict = {}

            self._Read()

        def _Read(self):
            with open(self._fileName, "r") as _fh:
                self._content = _fh.readlines()

        def _EvalError(self, line, title, message):
            print("[ProtoEval] ", title, " [", self._fileName, ":", line, "]: ", message)

        def GetTargetFile(self):
            return self._targetFileName

        def AddVariable(self, varName, func):
            self._paramDict[varName.lower()] = func

        def _GetParameter(self, params, lineIndex, paramString):
            while 1:
                pos = paramString.find("=")
                if pos < 0:
                    break

                (key, value, endPos) = DecodeKVPair(paramString, True)
                if not key or not value:
                    self._EvalError(lineIndex, "Invalid line", "parameter invalid (key or value missing): " + paramString)
                    break

                curData = paramString[:endPos]
                paramString = paramString[endPos+1:]

                # convert value to number/bool if possible
                if value.lower() == 'true':
                    value = True
                elif value.lower() == 'false':
                    value = False
                else:
                    try:
                        value = int(value)
                    except ValueError:
                        pass

                params.Add(key, value)

        def Eval(self):
            retData = []

            lineIndex = 0
            for line in self._content:
                lineIndex += 1

                pos = 0
                while 1:
                    posStart = line.find(self.VAR_BEGIN_KEY, pos)
                    if posStart < 0:
                        break

                    ## find endPos
                    posEnd = posStart + len(self.VAR_BEGIN_KEY) - 1
                    isInQuot = False
                    while 1:
                        posEnd += 1
                        if posEnd >= len(line):
                            posEnd = -1
                            break

                        if line[posEnd] == '\\':
                            posEnd += 1
                        elif isInQuot:
                            if line[posEnd] == '"':
                                isInQuot = False
                        else:
                            if line[posEnd] == '"':
                                isInQuot = True
                            elif line[posEnd:posEnd+len(self.VAR_END_KEY)] == self.VAR_END_KEY:
                                break

                    if posEnd < 0:
                        self._EvalError(lineIndex, "Invalid line", "variable starts at " + str(posStart) + " and doesn't end : " + line)
                        break

                    varData = line[posStart+len(self.VAR_BEGIN_KEY):posEnd].strip()
                    pos = posEnd + len(self.VAR_END_KEY)

                    tokens = varData.split(" ", 1)
                    varName = tokens[0].lower()
                    if len(varName) == 0:
                        self._EvalError(lineIndex, "Invalid line", "no variable name given (pos " + str(posStart) + " - " + str(posEnd) + ")")
                        continue
                    if not varName in self._paramDict:
                        self._EvalError(lineIndex, "Invalid line", "unknown variable name: \"" + tokens[0] + "\"")
                        continue

                    params = self.FuncParams()
                    # get parameter
                    if len(tokens) == 2:
                        self._GetParameter(params, lineIndex, tokens[1])

                    # eval variable
                    replaceData = self._paramDict[varName](params)
                    line = line[:posStart] + replaceData + line[posEnd + len(self.VAR_END_KEY):]
                    pos = posStart

                retData.append(line)

            return retData

    ########################################
    ## General
    ########################################

    IMPORT_KEY = "import"
    INCLUDE_KEY = "include"
    OPTION_KEY = "option"
    GROUP_KEY = "group"

    ## for imports:
    IMP_MESSAGE_KEY = "message"

    def __init__(self, fileName):
        self._fileName = fileName
        self._currentTarget = ""
        self._imports = {}
        self._options = {}
        self._groups = []

        self._Read()

    def _IsValidVarName(self, name, allowUnderscore = True):
        underscoreName = name
        if allowUnderscore:
            underscoreName = name.replace("_", "")

        return len(name) > 0 and name[0].isalpha() and underscoreName.isalnum()

    ########################################
    ## Reading
    ########################################

    def _ReadError(self, line, title, message, fileName = None):
        if not fileName:
            fileName = self._fileName

        print(title," [", fileName, ":", line, "]: ", message)

    def _AddImport(self, importInfo):
        self._imports[importInfo.GetAliasName()] = importInfo

    def _HasImport(self, aliasName):
        return aliasName in self._imports

    def _GetImport(self, aliasName):
        if self._HasImport(aliasName):
            return self._imports[aliasName]

        return None

    def _GetImports(self):
        return self._imports

    def _IsImportUsed(self, aliasName):
        for group in self._GetGroups():
            disableTargets = group.GetOption("disable_target", "")
            if disableTargets and self._currentTarget in disableTargets:
                continue

            for headerData in group.GetHeaderList():
                packetInfo = headerData["packet"]
                if packetInfo and packetInfo["alias"] == aliasName:
                    return True

        return False

    def _AddOption(self, key, value):
        self._options[key.lower()] = value

    def _HasOption(self, key):
        if key.lower() in self._options:
            return True

        return False

    def _GetOption(self, key):
        return self._options[key.lower()]

    def _AddGroup(self, group):
        self._groups.append(group)

    def _GetGroupCount(self):
        return len(self._groups)

    def _GetGroup(self, index):
        return self._groups[index]

    def _GetGroups(self):
        return self._groups

    def _ReadImport(self, fileName, aliasName):
        if self._HasImport(aliasName):
            return

        importInfo = self.ImportInfo(fileName, aliasName)

        with open(fileName, "r") as _fh:
            lineIndex = 0
            for line in _fh.read().splitlines():
                lineIndex += 1
                line = line.strip()

                if line.lower().startswith(self.IMP_MESSAGE_KEY):
                    line = line[len(self.IMP_MESSAGE_KEY + " "):]

                    packetName = line.lstrip().split(" ", 1)[0]
                    if not self._IsValidVarName(packetName):
                        self._ReadError(lineIndex, "Invalid import line", "invalid message name: \"" + packetName + "\"", fileName)
                        continue

                    importInfo.AddPacket(packetName)

        if importInfo.GetPacketCount() == 0:
            self._ReadError(lineIndex, "Strange import", "no packets found in import", fileName)
            return

        self._AddImport(importInfo)

    def _Read(self):
        readingGroup = None

        with open(self._fileName, "r") as _fh:
            lineIndex = 0
            lineList = _fh.read().splitlines() # use the splitlines() so we don't have newline-characters

            while lineIndex < len(lineList): # len(lineList) may change while reading, so we cannot use a constant
                line = lineList[lineIndex].strip() # we do not need spaces
                lineIndex += 1

                ## ignore empty and comment lines
                if len(line) == 0 or line.startswith("#") or line.startswith("//"):
                    continue

                ## no readingGroup -> find anything to read (options, import, include or new group)
                if readingGroup == None:
                    ## check for import
                    if line.lower().startswith(self.IMPORT_KEY + " "):
                        line = line[len(self.IMPORT_KEY + " "):].strip()

                        importFileName, endPos = DecodeValue(line)
                        if not importFileName:
                            self._ReadError(lineIndex, "Invalid line", "no file name for import found")
                            continue
                        if not os.path.exists(importFileName):
                            self._ReadError(lineIndex, "Invalid line", "import file does not exist: \"" + importFileName + "\"")
                            continue

                        line = line[endPos+1:]
                        aliasName = importFileName
                        aliasPos = line.find(" as ")
                        ## use alias if it's given
                        if aliasPos >= 0:
                            aliasName = line[aliasPos+4:].strip()
                            if not self._IsValidVarName(aliasName):
                                self._ReadError(lineIndex, "Invalid line", "invalid import alias: \"" + aliasName + "\"")
                                continue

                        ## got all necessary info -> create import info and load
                        self._ReadImport(importFileName, aliasName)

                    ## check for import
                    elif line.lower().startswith(self.INCLUDE_KEY + " "):
                        line = line[len(self.INCLUDE_KEY + " "):]

                        fileName = DecodeValue(line.strip())[0]
                        if not fileName:
                            self._ReadError(lineIndex, "Invalid line", "no file name for include found")
                            continue

                        baseDir = os.path.dirname(os.path.abspath(self._fileName))
                        includeFileName = os.path.join(baseDir, fileName)
                        if not os.path.exists(includeFileName):
                            self._ReadError(lineIndex, "Invalid line", "include file not found: \"" + includeFileName + "\"")
                            continue

                        # get file lines
                        with open(includeFileName, "r") as _include_fh:
                            includeLineList = _include_fh.read().splitlines()

                        # copy lines into current lineList
                        lineList = lineList[:lineIndex] + includeLineList + lineList[lineIndex:]

                    ## check for option
                    elif line.lower().startswith(self.OPTION_KEY + " "):
                        line = line[len(self.OPTION_KEY + " "):]

                        optionKey, optionValue = DecodeKVPair(line.lstrip())
                        if len(optionKey) == 0 or len(optionValue) == 0:
                            self._ReadError(lineIndex, "Invalid line", "no option key or value")
                            continue

                        self._AddOption(optionKey, optionValue)

                    ## check for new group
                    elif line.lower().startswith(self.GROUP_KEY + " "):
                        line = line[len(self.GROUP_KEY + " "):]

                        tokens = line.split(" ", 1)
                        if len(tokens) < 2 or tokens[1].strip() != "{":
                            self._ReadError(lineIndex, "Invalid line", "invalid group start")
                            continue

                        groupName = tokens[0]
                        if not self._IsValidVarName(groupName):
                            self._ReadError(lineIndex, "Invalid line", "invalid group name: \"" + groupName + "\"")
                            continue

                        readingGroup = self.Group(groupName)

                    ## there shouldn't be anything else out there currently...
                    else:
                        self._ReadError(lineIndex, "Strange line", "expected option or group start")

                ## we have a readingGroup -> find header/option in the group or end of the group
                else:
                    ## check for header
                    if line[0].isalpha():
                        tokens = line.split(" ", 1)
                        headerName = tokens[0]
                        if headerName == line:
                            headerName = headerName[:-1]
                        if not self._IsValidVarName(headerName):
                            self._ReadError(lineIndex, "Invalid line", "invalid header name: \"" + headerName + "\"")
                            continue

                        ## get packet name and alias
                        packetInfo = None
                        if len(tokens) == 2:
                            packetName = tokens[1]
                            allocationPos = packetName.find("=>")
                            if allocationPos >= 0:
                                packetName = packetName[allocationPos+2:].strip()
                                packetNameEnd = packetName.find(",")
                                if packetNameEnd >= 0:
                                    packetName = packetName[:packetNameEnd]

                                aliasNameEnd = packetName.find(".")
                                if aliasNameEnd > 0:
                                    aliasName = packetName[:aliasNameEnd]
                                    packetName = packetName[aliasNameEnd+1:]

                                    importInfo = self._GetImport(aliasName)
                                    if not importInfo:
                                        self._ReadError(lineIndex, "Invalid line", "no import found by name: \"" + aliasName + "\"")
                                        continue

                                    if not importInfo.HasPacket(packetName):
                                        self._ReadError(lineIndex, "Invalid line", "packet not found in import: import[\"" + aliasName + "\"] packet[\"" + packetName + "\"]")
                                        continue

                                    packetInfo = {
                                        "alias" : aliasName,
                                        "name" : packetName,
                                    }

#                        if not packetInfo:
#                            self._ReadError(lineIndex, "Invalid line", "invalid header, no corresponding packet found: \"" + headerName + "\"")
#                            continue

                        readingGroup.AddHeader(headerName, packetInfo)

                    ## check for option
                    elif line[0] == '[':
                        key, value = DecodeKVPair(line)
                        if len(key) < 3 or key[0] != '[' or key[-1] != ']':
                            self._ReadError(lineIndex, "Invalid line", "invalid group option name: " + key)
                            continue
                        if len(value) == 0:
                            self._ReadError(lineIndex, "Invalid line", "group option value is empty for key: " + key)
                            continue

                        readingGroup.SetOption(key[1:-1], value)

                    ## check for end of group
                    elif line[0] == '}':
                        if readingGroup.GetHeaderCount() == 0:
                            self._ReadError(lineIndex, "Strange group", "empty group will be ignored: \"" + readingGroup.GetName() + "\"")
                            readingGroup = None
                            continue

                        self._AddGroup(readingGroup)
                        readingGroup = None

                    ## there shouldn't be anything else out there currently....
                    else:
                        self._ReadError(lineIndex, "Strange line", "expected header or end of group")

        if readingGroup != None:
            self._ReadError(lineIndex, "Invalid group", "file end without group end, group will be ignored: \"" + readingGroup.GetName() + "\"")

    ########################################
    ## Writing
    ########################################

    def _Save_TargetInclude(self, params):
        allowTarget = params.Get("target", "")
        content = params.Get("content", "")

        if allowTarget in self._currentTarget:
            return content

        return ""

    def _Save_TargetExclude(self, params):
        disallowTarget = params.Get("target", "")
        content = params.Get("content", "")

        if not disallowTarget in self._currentTarget:
            return content

        return ""

    def Save(self, targetPath, lang, target):
        self._currentTarget = target

        targetFile = os.path.splitext(self._fileName)[0]

        protoData = None
        if self._HasOption("%s_prototype" % lang):
            protoFileName = self._GetOption("%s_prototype" % lang)
            protoData = []
            if type(protoFileName) == tuple:
                for fileName in protoFileName:
                    protoData.append(self.PrototypeFile(fileName))
            else:
                protoData.append(self.PrototypeFile(protoFileName))

            for proto in protoData:
                proto.AddVariable("TARGET_INCLUDE", self._Save_TargetInclude)
                proto.AddVariable("TARGET_EXCLUDE", self._Save_TargetExclude)

        if lang == "python":
            self._SaveAsPython(targetPath, targetFile, protoData)
        elif lang == "csharp":
            self._SaveAsCSharp(targetPath, targetFile)
        elif lang == "cpp":
            self._SaveAsCPP(targetPath, targetFile, protoData)

    ########################################
    ## CSharp Writing
    ########################################

    def _SaveAsCSharp(self, targetPath, targetFile):
        data = ""
        tabcount = 0

        if self._HasOption("csharp_namespace"):
            data += makeTab(tabcount) + "namespace " + self._GetOption("csharp_namespace") + "\n"
            data += "{\n"
            tabcount += 1

        cnt = 0
        for group in self._GetGroups():
            disableTargets = group.GetOption("disable_target", "")
            if disableTargets and self._currentTarget in disableTargets:
                continue

            if cnt > 0:
                data += "\n"

            data += makeTab(tabcount) + "public enum " + group.GetName() + "\n"
            data += makeTab(tabcount) + "{\n"
            tabcount += 1

            data += makeTab(tabcount) + "NONE,\n"
            for headerData in group.GetHeaderList():
                data += makeTab(tabcount) + headerData["header"] + ",\n"

            tabcount -= 1
            data += makeTab(tabcount) + "}\n"

            cnt += 1

        if self._HasOption("csharp_namespace"):
            data += "}\n"
            tabcount -= 1

        if self._HasOption("csharp_target"):
            targetFile = self._GetOption("csharp_target")

        with open(os.path.join(targetPath, targetFile + ".cs"), "w") as _fh:
            _fh.write(data)

    ########################################
    ## C++ Writing
    ########################################

    def _SaveCPP_Basic(self, targetPath, targetFile):
        data = ""
        tabcount = 0

        data += "#pragma once\n\n"

        if self._HasOption("cpp_namespace"):
            data += makeTab(tabcount) + "namespace " + self._GetOption("cpp_namespace") + "\n"
            data += "{\n"
            tabcount += 1

        cnt = 0
        for group in self._GetGroups():
            disableTargets = group.GetOption("disable_target", "")
            if disableTargets and self._currentTarget in disableTargets:
                continue

            if cnt > 0:
                data += "\n"

            data += makeTab(tabcount) + "enum class T" + group.GetName() + "\n"
            data += makeTab(tabcount) + "{\n"
            tabcount += 1

            data += makeTab(tabcount) + "NONE,\n"
            for headerData in group.GetHeaderList():
                data += makeTab(tabcount) + headerData["header"] + ",\n"

            tabcount -= 1
            data += makeTab(tabcount) + "};\n"

            cnt += 1

        if self._HasOption("cpp_namespace"):
            data += "}\n"
            tabcount -= 1

        if self._HasOption("cpp_target"):
            targetFile = self._GetOption("cpp_target")

        with open(os.path.join(targetPath, targetFile + ".hpp"), "w") as _fh:
            _fh.write(data)

    def _SaveCPP_IncludeNameList(self, params):
        ret = ""
        for aliasName in self._GetImports():
            ## check if import used
            if not self._IsImportUsed(aliasName):
                continue

            ## add import
            importInfo = self._GetImport(aliasName)
            importFileName = os.path.splitext(importInfo.GetFileName())[0]

            # python modules get the extension "_pb2"
            ret += "#include \"protobuf_" + importFileName + ".h\"\n"

        return ret[:-1]

    def _SaveCPP_ClassList(self, params):
        parentClass = params.Get("parentClass", "")
        specialParentClass = params.Get("specialParentClass", "")
        unknownPacketRaise = params.Get("unknownPacketRaise", "std::runtime_error()")
        headerVarName = params.Get("headerVarName", "header")
        tabs = makeTab(params.Get("tabcount", 0))

        if specialParentClass:
            splitData = specialParentClass.split("=")
            if len(splitData) >= 2:
                tmp = splitData[0].split(",")
                specialParentClass = {}
                for data in tmp:
                    specialParentClass[data] = splitData[1]
            else:
                specialParentClass = {}
        else:
            specialParentClass = {}

        ret = ""
        for i, group in enumerate(self._GetGroups()):
            disableTargets = group.GetOption("disable_target", "")
            if disableTargets and self._currentTarget in disableTargets:
                continue

            headerTypeName = group.GetName()[:2]
            currentClassName = headerTypeName + parentClass

            realParentClass = parentClass
            for key in specialParentClass:
                if headerTypeName.startswith(key):
                    realParentClass = specialParentClass[key]
                    break

            ## current header base struct
            ret += tabs + "template <typename T>\n"
            ret += tabs + "struct " + currentClassName + " : " + realParentClass + "<T> {\n"
            ret += tabs + makeTab(1) + currentClassName + "()\n"
            ret += tabs + makeTab(1) + "{\n"
            ret += tabs + makeTab(2) + "throw " + unknownPacketRaise + ";\n"
            ret += tabs + makeTab(1) + "}\n"
            ret += tabs + "};\n"

            ## each header
            for headerData in group.GetHeaderList():
                if not headerData["packet"]:
                    continue

                packetName = headerData["packet"]["name"]
                ret += tabs + "template <>\n"
                ret += tabs + "struct " + currentClassName + "<" + packetName + "> : " + realParentClass + "<" + packetName + "> {\n"
                ret += tabs + makeTab(1) + currentClassName + "()\n"
                ret += tabs + makeTab(1) + "{\n"
                ret += tabs + makeTab(2) + headerVarName + " = static_cast<uint16_t>(T" + group.GetName() + "::" + headerData["header"] + ");\n"
                ret += tabs + makeTab(1) + "}\n"
                ret += tabs + "};\n"

            ret += "\n"

        return ret[:-2]

    def _SaveAsCPP(self, targetPath, targetFile, protoData):
        self._SaveCPP_Basic(targetPath, targetFile + "_basic")

        if protoData:
            for data in protoData:
                data.AddVariable("INCLUDE_NAME_LIST", self._SaveCPP_IncludeNameList)
                data.AddVariable("CLASS_LIST", self._SaveCPP_ClassList)

                curTargetFile = targetFile + ".hpp"
                if data.GetTargetFile():
                    curTargetFile = data.GetTargetFile()

                with open(os.path.join(targetPath, curTargetFile), "w") as _fh:
                    for line in data.Eval():
                        _fh.write(line)

    ########################################
    ## Python Writing
    ########################################

    def _SavePy_HeaderGroupList(self, params):
        isLinesplit = params.Get("linesplit", False) == True
        ret = ""
        tabs = makeTab(params.Get("tabcount", 0))

        if isLinesplit:
            ret += "\n"

        for group in self._GetGroups():
            disableTargets = group.GetOption("disable_target", "")
            if disableTargets and self._currentTarget in disableTargets:
                continue

            name = group.GetName()
            if params.Get("packetNames", False) == True:
                name = group.GetPacketGroupName()

            ret += tabs + name + ","
            if isLinesplit:
                ret += "\n"
            else:
                ret += " "

        if len(ret) > 0:
            ret = ret[:-2]
            if isLinesplit:
                ret += "\n"

        return ret

    def _SavePy_HeaderGroupListStr(self, params):
        isLinesplit = params.Get("linesplit", False) == True
        ret = ""
        tabs = makeTab(params.Get("tabcount", 0))

        if isLinesplit:
            ret += "\n"

        for group in self._GetGroups():
            disableTargets = group.GetOption("disable_target", "")
            if disableTargets and self._currentTarget in disableTargets:
                continue

            name = group.GetName()
            if params.Get("packetNames", False) == True:
                name = group.GetPacketGroupName()

            ret += tabs + "'" + name + "'" + ","
            if isLinesplit:
                ret += "\n"
            else:
                ret += " "

        if len(ret) > 0:
            ret = ret[:-2]
            if isLinesplit:
                ret += "\n"

        return ret

    def _SavePy_ImportNameList(self, params):
        splitCnt = params.Get("linesplit", 0)
        if type(splitCnt) != int:
            splitCnt = 0

        ret = ""
        importCnt = 0
        for aliasName in self._GetImports():
            ## check if import used
            if not self._IsImportUsed(aliasName):
                continue

            ## add import
            if splitCnt > 0 and importCnt > 0 and importCnt % splitCnt == 0:
                ret = ret[:-1]
                ret += "\\\n" + TAB_AS_SPACE

            importCnt += 1
            importInfo = self._GetImport(aliasName)
            importFileName = os.path.splitext(importInfo.GetFileName())[0]

            # python modules get the extension "_pb2"
            ret += importFileName + "_pb2 as " + aliasName + ", "

        return ret[:-2]

    def _SavePy_ClassList(self, params):
        parentClass = params.Get("parentClass", "")

        ret = ""
        for i, group in enumerate(self._GetGroups()):
            disableTargets = group.GetOption("disable_target", "")
            if disableTargets and self._currentTarget in disableTargets:
                continue

            ## class header
            ret += "@enum.unique\n"
            ret += "class " + group.GetName() + "(" + str(parentClass) + "):\n"

            ## comment
            if group.HasOption("comment"):
                ret += makeTab(1) + "\"\"\" " + group.GetOption("comment") + " \"\"\"\n"

            ## header list
            ret += makeTab(1) + "NONE = 0\n"
            for headerData in group.GetHeaderList():
                ret += makeTab(1) + headerData["header"] + " = enum.auto()\n"

            ## convert header -> packet function
            ret += "\n"
            # declaration
            ret += makeTab(1) + "def " + params.Get("packetMethodName", "packet_func") + "(self, **kwargs) -> " + group.GetPacketGroupName() + ":\n"
            # comment
            ret += makeTab(2) + "\"\"\" Returns the packet belonging to the header. \"\"\"\n"

            # header converter list
            cnt = 0
            for headerData in group.GetHeaderList():
                if not headerData["packet"]:
                    continue

                ret += makeTab(2)
                if cnt > 0:
                    ret += "el"
                ret += "if self is " + group.GetName() + "." + headerData["header"] + ":\n"
                ret += makeTab(3) + "return " + headerData["packet"]["name"] + "(**kwargs)\n"

                cnt += 1

            # raise error if no packet found
            ret += makeTab(2) + "else:\n"
            ret += makeTab(3) + "raise " + params.Get("unknownHeaderRaise", "")
            if i == len(self._GetGroups()) - 1:
                ret += "\n"
            else:
                ret += "\n\n\n"

        return ret[:-1]

    def _SavePy_PacketNameList(self, params):
        isLinesplit = params.Get("linesplit", False) == True
        tabs = makeTab(params.Get("tabcount", 1) + int(isLinesplit))
        ret = ""

        if isLinesplit:
            ret += "\n"
        for group in self._GetGroups():
            if params.Get("group", group.GetName()).lower() != group.GetName().lower():
                continue
            disableTargets = group.GetOption("disable_target", "")
            if disableTargets and self._currentTarget in disableTargets:
                continue

            for headerData in group.GetHeaderList():
                if not headerData["packet"]:
                    continue

                ret += tabs + headerData["packet"]["name"] + ","
                if isLinesplit:
                    ret += "\n"
            ret += makeTab(1)
        return ret

    def _SaveAsPython(self, targetPath, targetFile, protoData):
        if protoData:
            for data in protoData:
                data.AddVariable("HEADER_GROUP_LIST", self._SavePy_HeaderGroupList)
                data.AddVariable("HEADER_GROUP_LIST_STR", self._SavePy_HeaderGroupListStr)
                data.AddVariable("IMPORT_NAME_LIST", self._SavePy_ImportNameList)
                data.AddVariable("CLASS_LIST", self._SavePy_ClassList)
                data.AddVariable("PACKET_NAME_LIST", self._SavePy_PacketNameList)

                curTargetFile = targetFile + ".py"
                if data.GetTargetFile():
                    curTargetFile = data.GetTargetFile()

                with open(os.path.join(targetPath, curTargetFile), "w") as _fh:
                    for line in data.Eval():
                        _fh.write(line)

        else:
            print("Error: " + targetFile)
            print("Saving header files in python without a prototype is not supported.")

########################################
## Export function for all header files
########################################
def export():
    exporterList = []
    for file in os.listdir("."):
        if file.endswith(".hproto"):
            exporterList.append(Exporter(file))
    if len(exporterList) == 0:
        return

    for exportData in config.EXPORT_LIST:
        print("[HEADER] Exporting to ", exportData['header_path'])
        for exporter in exporterList:
            exporter.Save(exportData["header_path"], exportData["lang"], exportData["target"])
