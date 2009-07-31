import os,stat,time

#######################################################################
# Delete any files older than 30 days.
#
#

thirty_days = 60 * 60 * 24 * 30 # Thirty days in seconds
SF_PATH = "J:\\"

def testFile(fileName):
	ct = int(time.time())
	file_time = os.stat(fileName)[stat.ST_CTIME]
	
	return (ct - thirty_days) > file_time

if __name__ == "__main__":
	for filename in os.listdir(SF_PATH):
		if testFile(SF_PATH + filename):
			os.unlink(SF_PATH + filename)
