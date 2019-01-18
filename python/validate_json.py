import os
import csv
import glm

os.system('find .. -name gridlabd.json -print > validate.json')
with open('validate.json') as fh:
	reader = csv.reader(fh)
	for filename in reader:
		try :
			fname = filename[0]
			model = glm.GLM(filename=fname)
			if model.check_parents() :
				print("ERROR %s: parent check failed" % fname)
			else:
				print("%s: ok" % fname)
		except Exception as e:
			print("ERROR %s: load failed -- %s" % (fname,e))
			os.system("open %s" % fname)
