@echo off

set dir=%1
if '%dir%' == '' set dir=..
set file=.\build.h

for /f "delims=" %%a in ('svn info %dir% ^| findstr /b "Revision: "') do set new=%%a 
set newrev=%new:Revision: =%
REM echo Last build is %newrev%

if not exist %file% goto update

for /f "delims=" %%a in ('type %file%') do set old=%%a
if "%old%" == "" goto update
set oldrev=%old:#define BUILDNUM =% 

REM echo This build is %oldrev%
if %oldrev% == %newrev% goto done

:update
echo %file% updated
echo #define BUILDNUM %newrev% >%file%

:done
echo Current build is %newrev%