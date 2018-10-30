#!/usr/bin/python
import json
import string

version = "4.1.0"

def convert(jsonfile,glmfile) :
	fd = open(jsonfile,'r')
	glm = json.load(fd)
	fd.close()

	if "application" not in glm.keys() or glm["application"] != "gridlabd" :
		raise Exception("input is not a gridlabd model")
	if glm["version"] > version :
		raise Exception("input version is newer than convert code")

	fd = open(glmfile,"w")

	modules = glm["modules"]
	globals = glm["globals"]
	classes = glm["classes"]
	objects = glm["objects"]

	fd.write("clock {\n")
	fd.write("\ttimezone {};\n".format(globals["timezone_locale"]["value"]))
	fd.write("\tstarttime '{}';\n".format(globals["starttime"]["value"]))
	fd.write("\tstoptime '{}';\n".format(globals["stoptime"]["value"]))
	fd.write("}\n")
	for mod in modules.keys() :
		fd.write("module {} {}\n".format(mod,'{'))
		for var in globals.keys():
			spec = string.split(var,"::")
			if spec[0] == mod :
				fd.write("\t{} {};\n".format(spec[1],globals[var]["value"]))
		fd.write("}\n")

	for obj,data in objects.items() :
		fd.write("object {} {}\n".format(obj,'{'))
		oclass = data["class"];
		for name,value in data.items() :
			ignore = ["class","id"]
			myclass = classes[oclass]
			if ( name not in ignore) :
				if ( name not in myclass or myclass[name]["access"] == "PUBLIC" ) :
					fd.write("\t{} {};\n".format(name,value))
		fd.write("}\n")

	fd.close()

convert('../autotest/test_json.json','../autotest/test_json_out.glm')