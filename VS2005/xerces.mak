
WORKDIR=..\third_party
XERCES=xerces-c-3.1.1

all: $(WORKDIR)\$(XERCES)\credits.txt
	"$(VCTOOLSDIR)vcbuild.exe" /nologo /platform:$(PLATFORM) xerces-lib.vcproj $(CONFIG)
#	msbuild.exe /nologo /t:Build /p:Platform=$(PLATFORM),Configuration=$(CONFIG) xerces-lib.vcproj

clean:
	"$(VCTOOLSDIR)vcbuild.exe" /clean /nologo /platform:$(PLATFORM) xerces-lib.vcproj $(CONFIG)
#	msbuild.exe /nologo /t:Clean /p:Platform=$(PLATFORM),Configuration=$(CONFIG) xerces-lib.vcproj

# Because of the pipe, gzip failures are ignored unless they also cause
# tar to fail.  The existance test checks the last file in the archive to
# verify that all files were extracted.  When the tarball is replaced, be
# sure to update the test, if needed, to check for the last file in the
# archive.  Appending to credits.txt is performed in place of touching the
# file since we have no guarantee that touch is available (it isn't by
# default).
$(WORKDIR)\$(XERCES)\credits.txt: $(WORKDIR)\$(XERCES).tar.gz
	cd $(WORKDIR)
	bin\gzip.exe -dqc $(XERCES).tar.gz | bin\tar.exe -x
	@if exist $(XERCES)\credits.txt echo >> $(XERCES)\credits.txt
	cd "$(MAKEDIR)"

# List the contents of the archive to update the target above.
list:
	$(WORKDIR)\bin\gzip.exe -dqc $(WORKDIR)\$(XERCES).tar.gz | $(WORKDIR)\bin\tar.exe -t

