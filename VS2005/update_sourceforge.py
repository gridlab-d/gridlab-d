import os
import sys
import re
import getopt
from datetime import datetime


#
# Delete any files older than 30 days.
#
#
day_secs = 60 * 60 * 24  # one day in seconds


def do_help():
    print("update_sourceforge.py - GridLAB-D autotest/validation script")
    print(
        "     update_sourceforge.py [--rdir=<dir>] [--days=<days>] [--prefix=<prefix>]")
    return 0

def testFile(pattern,fileName,days,prefix):
	result = pattern.match(fileName)
    if result is None:
        print "No files match pattern", pattern.pattern
        return

    d = datetime(int(result.group(1)), int(
        result.group(2)), int(result.group(3)))
    td = datetime.today() - d

    return td.days >= days

if __name__ == "__main__":
    SF_PATH = "../../releases"
    days = 5
    DRY_RUN = False
    prefix = "gridlabd-win32-"

    opts, args = getopt.getopt(
        sys.argv[1:], "", ["rdir=", 'days=', 'prefix=', 'dry-run', 'help'])

    for o, a in opts:
        if "--help" in o or "-h" in o:
            do_help()
            sys.exit(0)
        elif "--rdir" in o:
            SF_PATH = a
        elif "--days" in o:
            days = int(a)
        elif "--prefix" in o:
            prefix = a
        elif "--dry-run" in o:
            DRY_RUN = True
        else:
            print "Invalid argument"
            do_help()
    pattern = re.compile(
        prefix + "([0-9]{4})_([0-9]{2})_([0-9]{2})-nightly\\.exe")
    count = 1
    dirlist = os.listdir(SF_PATH)
    dirlist.reverse()
    for filename in dirlist:
        # print "Testing",SF_PATH + "\\" +filename
        if testFile(pattern, filename, days, prefix) and (count > days):
            print "delete", SF_PATH + "/" + filename
            if(not DRY_RUN):
                os.unlink(SF_PATH + "/" + filename)
        count += 1
