@echo off

rem
rem - Check the PATH to ensure SVN.EXE is available on this system
rem 
set exe=svn.exe
for %%a in ( "%PATH:;=" "%" ) do ( if exist "%%a\svn.exe" set exe="%%a\svn.exe" )
if "%exe:"=%" == "svn.exe" goto noexe

rem
rem - Set the target directory for the version number
rem
set dir=%1
if '%dir%' == '' set dir=..
set file=.\build.h

rem
rem - Obtain the version number reported by subversion
rem
set new=
for /f "delims=" %%a in ('svn info %dir% ^| findstr "Rev: "') do set new=%%a 
set newrev=%new:Last Changed Rev: =%

rem
rem - Obtain the version number stored in build.h
rem
if not exist %file% goto update
set old=
for /f "delims=" %%a in ('type %file%') do set old=%%a
if "%old%" == "" goto update
set oldrev=%old:#define BUILDNUM =%

rem
rem - If the numbers are the same exit
rem
if "%oldrev: =%" == "%newrev: =%" goto done

rem
rem - Update build.h
rem
:update
echo %file% updated
echo #define BUILDNUM %newrev% >%file%
goto done

rem
rem - Report problem locating svn.exe
rem
:noexe
echo %exe% not found: try reinstalling Tortoise with the command line version enabled
goto done

rem
rem - build.h number should match svn info
rem
:done
echo Current build is %newrev%
