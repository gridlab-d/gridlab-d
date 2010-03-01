import os,stat,time,sys

#######################################################################
# Delete any files older than 30 days.
#
#

thirty_days = 60 * 60 * 24 * 30 # Thirty days in seconds


def testFile(fileName):
	ct = int(time.time())
	file_time = os.stat(fileName)[stat.ST_CTIME]
	
	#print file_time, ct-thirty_days,ct-thirty_days > file_time
	return (ct - thirty_days) > file_time

if __name__ == "__main__":
	SF_PATH = "L:\\"
	if(len(sys.argv) > 1):
		SF_PATH=sys.argv[1]
	
	for filename in os.listdir(SF_PATH):
		#print "Testing",SF_PATH + "\\" +filename
		if testFile(SF_PATH + "\\" +filename):
			print "delete",SF_PATH + "\\" +filename
			os.unlink(SF_PATH + "\\" +filename)
