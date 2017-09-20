# -*- coding: utf-8 -*-

import os

#需要统计行数的文件后缀
Suffix = {".cpp",".h",".hpp",".cxx"}
#目录路径
Path = "E:\\test"

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
				if not Targets[suffix].has_key("total_files"):
					Targets[suffix]["total_files"] = 0
				Targets[suffix]["total_files"]+=1
				lines = CountFileLines(filepath)
				if not Targets[suffix].has_key("total_lines"):
					Targets[suffix]["total_lines"] = 0
				Targets[suffix]["total_lines"] += lines
				if not Targets[suffix].has_key("max_lines_file"):
					Targets[suffix]["max_lines_file"] = 0
				if Targets[suffix]["max_lines_file"] < lines:
					Targets[suffix]["max_lines_file"] = lines

Targets = {}
files = 0;
lines = 0;
for val in Suffix:
	Targets[val] = {}
					
listdir(Path,Targets)

for suffix_val in Targets.keys():
	print suffix_val+":"
	for suffix_item in Targets[suffix_val].keys():
		if suffix_item == "total_files":
			files += Targets[suffix_val][suffix_item]
		if suffix_item == "total_lines":
			lines += Targets[suffix_val][suffix_item]
		print "\t"+suffix_item+"\t:"+str(Targets[suffix_val][suffix_item])
	print '\n'
	
print "Total:"
print "\tFiles\t:"+str(files)
print "\tLines\t:"+str(lines)
