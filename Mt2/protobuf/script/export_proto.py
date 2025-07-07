# -*- coding: utf-8 -*-

import os
from . import config
from shutil import move

## fixxes the automatically created "import <protofile> as <name>" from google-protobuf to be renamed to "from . import <protofile> as <name>"
def py_fix_import(fileName):
    with open(fileName, "r") as _fh:
        data = _fh.readlines()

    newLines = []
    found = False
    for line in data:
        if line.startswith("import "):
            pos1 = line.find("_pb2")
            pos2 = line.rfind("__pb2")
            if pos1 > 0 and pos2 > 0 and pos1-1 != pos2:
                line = "from . " + line
                found = True

        newLines.append(line)

    if found:
        with open(fileName, "w") as _fh:
            for line in newLines:
                _fh.write(line)

def cpp_fix_import(fileName):
    with open(fileName, "r") as _fh:
        data = _fh.readlines()

    newLines = []
    found = False
    for line in data:
        if line.startswith("#include ") and line.find(".pb.h") > 0:
            pos1 = line.find("\"")
            pos2 = line.rfind("\"")
            if pos1 > 0 and pos2 > 0 and pos1 != pos2:
                name = line[pos1+1:pos2]
                line = "#include \"protobuf_" + name[:-len(".pb.h")] + ".h" + "\"" + line[pos2+1:]
                found = True

        newLines.append(line)

    if found:
        with open(fileName, "w") as _fh:
            for line in newLines:
                _fh.write(line)

def export():
    fileNames = ""
    for file in os.listdir("."):
        if file.endswith(".proto"):
            fileNames += file + " "
    if not fileNames:
        return

    for exportData in config.EXPORT_LIST:
        print("[PROTO] Exporting to ", exportData['proto_path'])
        cmd = "protoc.exe --%s_out=%s %s" % (exportData["lang"], exportData["proto_path"], fileNames)
        os.system(cmd)

        print(cmd)
        ## fix imports
        if exportData["lang"] == "python":
            for fileName in fileNames.split(" "):
                if not fileName:
                    continue
                py_fix_import(os.path.join(exportData["proto_path"], os.path.splitext(fileName)[0] + "_pb2.py"))
        ## rename cpp
        if exportData["lang"] == "cpp":
            for fileName in fileNames.split(" "):
                if not fileName:
                    continue
                fileName = os.path.splitext(fileName)[0]
                srcPath = os.path.join(exportData["proto_path"], fileName + "%s")
                dstPath = os.path.join(exportData["proto_path"], "protobuf_" + fileName + "%s")
                move(srcPath % ".pb.cc", dstPath % ".cpp")
                move(srcPath % ".pb.h", dstPath % ".h")

                cpp_fix_import(dstPath % ".cpp")
                cpp_fix_import(dstPath % ".h")
