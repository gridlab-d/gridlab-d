import os,stat,time,sys,re
from datetime import datetime

#######################################################################
# Delete any files older than 30 days.
#
#
day_secs = 60 * 60 * 24 # one day in seconds



def testFile(fileName,days):
	pattern = re.compile("gridlabd-win32-([0-9]{4})_([0-9]{2})_([0-9]{2})-nightly\\.exe")
	result = pattern.match(fileName)
	
	d = datetime(int(result.group(1)),int(result.group(2)),int(result.group(3)))
	td = datetime.today() - d
	
	return td.days >= days

if __name__ == "__main__":
	SF_PATH = "L:\\"
	days=30
	DRY_RUN = False
	if(len(sys.argv) > 1):
		SF_PATH=sys.argv[1]
	if(len(sys.argv) > 2):
		days = int(sys.argv[2])
	if(len(sys.argv) > 3):
		DRY_RUN = True
	
	for filename in os.listdir(SF_PATH):
		#print "Testing",SF_PATH + "\\" +filename
		if testFile(filename,days):
			print "delete",SF_PATH + "/" +filename
			if(not DRY_RUN):
				os.unlink(SF_PATH + "/" +filename)
