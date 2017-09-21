# -*- coding: utf-8 -*-

import os
import sys

#需要统计行数的文件后缀
Suffix = {".c",".cpp",".h",".hpp",".cxx"}
#目录路径
Path = ""

if len(sys.argv) > 1:
    Path = sys.argv[1]

if not Path:
    print("Path is empty")
    exit()

if not os.path.isdir(Path):
    print("Path is not exists")
    exit()

def CountFileLines(FileName):
    count=0
    fp=open(FileName,"r")
    last_chr='\n'
    while 1:
        buffer=fp.read(8*1024*1024)
        if not buffer:
            break
        count+=buffer.count('\n')
        last_chr=buffer[-1]
    fp.close()
    if last_chr != '\n':
        count+=1
    return count

def listdir(dir,Targets):
    files = os.listdir(dir)
    for file in files:
        filepath = os.path.join(dir,file)
        if os.path.isdir(filepath):
            listdir(filepath,Targets)
        elif os.path:
            suffix = os.path.splitext(file)[1]
            if Targets.has_key(suffix):
                if not Targets[suffix].has_key("TotalFiles"):
                    Targets[suffix]["TotalFiles"] = 0
                Targets[suffix]["TotalFiles"]+=1
                lines = CountFileLines(filepath)
                if not Targets[suffix].has_key("TotalLines"):
                    Targets[suffix]["TotalLines"] = 0
                Targets[suffix]["TotalLines"] += lines
                if not Targets[suffix].has_key("TheMaxLines"):
                    Targets[suffix]["TheMaxLines"] = 0
                if Targets[suffix]["TheMaxLines"] < lines:
                    Targets[suffix]["TheMaxLines"] = lines

Targets = {}
files = 0;
lines = 0;
for val in Suffix:
    Targets[val] = {}
                    
listdir(Path,Targets)

for suffix_val in Targets.keys():
    if not Targets[suffix_val]:
        continue
    print(suffix_val+":")
    for suffix_item in Targets[suffix_val].keys():
        if suffix_item == "TotalFiles":
            files += Targets[suffix_val][suffix_item]
        if suffix_item == "TotalLines":
            lines += Targets[suffix_val][suffix_item]
        print("\t"+suffix_item+"\t:"+"{:,}".format(Targets[suffix_val][suffix_item]))
    print('\n')
    
print("Total:")
print("\tFiles\t:"+"{:,}".format(files))
print("\tLines\t:"+"{:,}".format(lines))
