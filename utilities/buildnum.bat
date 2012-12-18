@echo off
for /f "delims=" %%a in ('svn info . ^| findstr /b "Revision: "') do set new=%%a 
REM echo Last build %new:Revision: =%
for /f "delims=" %%a in ('type build.h') do @set old=%%a
REM echo This build %old:#define BUILDNUM =%
if not %old:#define BUILDNUM =% == %new:Revision: =% echo #define BUILDNUM %new:Revision: =% >build.h