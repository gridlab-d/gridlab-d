@echo off

set exe=svn.exe
for %%a in ( "%PATH:;=" "%" ) do ( if exist "%%a\svn.exe" set exe="%%a\svn.exe" )
if "%exe:"=%" == "svn.exe" goto noexe

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
goto done

:noexe
echo %exe% not found: try reinstalling Tortoise with the command line version enabled.
goto done

:done
echo Current build is %newrev%