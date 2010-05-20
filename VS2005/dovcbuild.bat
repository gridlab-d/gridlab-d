REM Doing this bececause vcbuild requires the Release|Win32 argument, and buildbot doesn't seem to send it correctly.

vcbuild /useenv /platform:Win32 /nologo /rebuild gridlabd.sln "Release|%1"

exit /b %errorlevel%