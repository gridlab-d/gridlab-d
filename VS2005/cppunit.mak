
WORKDIR=..\third_party
CPPUNIT=cppunit-1.12.0

all: $(WORKDIR)\$(CPPUNIT)\COPYING
	"$(VCTOOLSDIR)vcbuild.exe" /nologo /platform:$(PLATFORM) cppunit-lib.vcproj $(CONFIG)
#	msbuild.exe /nologo /t:Build /p:Platform=$(PLATFORM),Configuration=$(CONFIG) cppunit-lib.vcproj

clean: $(WORKDIR)\$(CPPUNIT)\COPYING
	"$(VCTOOLSDIR)vcbuild.exe" /clean /nologo /platform:$(PLATFORM) cppunit-lib.vcproj $(CONFIG)
#	msbuild.exe /nologo /t:Clean /p:Platform=$(PLATFORM),Configuration=$(CONFIG) cppunit-lib.vcproj

# Because of the pipe, gzip failures are ignored unless they also cause
# tar to fail.  The existance test checks the last file in the archive to
# verify that all files were extracted.  When the tarball is replaced, be
# sure to update the test, if needed, to check for the last file in the
# archive.  Appending to COPYING is performed in place of touching the
# file since we have no guarantee that touch is available (it isn't by
# default).
$(WORKDIR)\$(CPPUNIT)\COPYING: $(WORKDIR)\$(CPPUNIT).tar.gz
	cd $(WORKDIR)
	bin\gzip.exe -dqc $(CPPUNIT).tar.gz | bin\tar.exe -x
	@if exist $(CPPUNIT)\COPYING echo >> $(CPPUNIT)\COPYING
	cd "$(MAKEDIR)"

# List the contents of the archive to update the target above.
list:
	$(WORKDIR)\bin\gzip.exe -dqc $(WORKDIR)\$(CPPUNIT).tar.gz | $(WORKDIR)\bin\tar.exe -t

