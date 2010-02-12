#!/usr/bin/python

import glob
import sys
import tarfile
import os
from zipfile import ZipFile

def main():
	fname = sys.argv[1]
	fp = open(fname,'r')
	fnames = []
	
	for line in fp:
		fnames.append(line.rstrip())
	fp.close()
		
	createArchiveFiles(fnames)
	
def generatePathList(fnames):
	pathlist = []
	for fname in fnames:
		globline = "./*/autotest/"+fname
		temp = glob.glob(globline)
		if len(temp) > 0:
			pathname = temp[0]
			pathlist.append(pathname)
			
	return pathlist
	
def createArchiveFiles(fnames):
	paths = generatePathList(fnames)
	tar = tarfile.open("failed_tests.tar","w")
	zfile = ZipFile("failed_tests.zip","w")
	for path in paths:
		tar.add(path)
		for fname in os.listdir(path):
			zfile.write(os.path.join(path,fname))
	tar.close()
	zfile.close()
	
def createTarFile(fnames):
	paths = generatePathList(fnames)
	tar = tarfile.open("failed_tests.tar","w")
	for path in paths:
		tar.add(path)
	tar.close()
	
def createZipFile(paths):
	paths = generatePathList(fnames)
	zfile = ZipFile("failed_tests.zip","w")
	for dirpath in paths:
		for fname in os.listdir(dirpath):
			zfile.write(os.path.join(path,fname))
	zfile.close()

if __name__ == '__main__':
	main()
