@echo off
setlocal enableextensions 
setlocal enabledelayedexpansion

set force=no
set warn=no
set verbose=no
set buildnum=yes
set search=C:\
set source=.
set config=config.h
set build=build.h

rem ********************************************************************************************
:Parse
if "%1" == "" goto Done1
  if /i not %1 == /w goto Parse1
  set warn=yes
  shift /1
  goto Parse
:Parse1
  if /i not %1 == /f goto Parse2
  set force=yes
  shift /1
  goto Parse
:Parse2
  if /i not %1 == /v goto Parse3
  set verbose=yes
  shift /1
  goto Parse
:Parse3
  if /i not %1 == /b goto Parse4
  set buildnum=no
  shift /1
  goto Parse
:Parse4
  if not %1 == /? goto Parse5
  echo Syntax: CONFIGURE /F /W /V /B SOURCE=dir SEARCH=dir CONFIG=file BUILD=file
  echo.
  echo Configures GridLAB-D for Visual Studio 2005 compiles by searching for 
  echo optional components that may be installed on the system and creating
  echo the SOURCE\core\CONFIG file needed to build a version of GridLAB-D.
  echo.
  echo If the config file exists it is normally not replaced.
  echo.
  echo   CONFIGURE /F forces a new search and update of CONFIG
  echo   CONFIGURE /W print warning messages (if any)
  echo   CONFIGURE /V enable verbose search output
  echo   CONFIGURE /B prevents updating BUILD with revision number of SOURCE
  echo.
  echo The default values are
  echo.
  echo   SEARCH = %search%
  echo   SOURCE = %source%
  echo   CONFIG = %config%
  echo   BUILD  = %build%
  echo.
  goto Return
:Parse5
  if "%2" == "" goto Missing2
  set %1=%2
  shift /1
  shift /1
  goto Parse
:Missing2
  echo Missing argument to %1
  goto Return
:Done1
if "%1" == "" goto Begin
echo %1 unexpected 
goto Return

:Begin
rem ********************************************************************************************
rem
rem - Only do configuration search if config.h not found
rem
if %force% == yes goto Search
if exist %config% goto Buildnum

rem ********************************************************************************************
:Search
echo Configure is building config.h...
echo // config.h automatically created %DATE% %TIME% >%config%
echo // %0: config=%config% >>%config%
echo // %0: search=%search% >>%config%
echo // %0: source=%source% >>%config%

rem 
rem - Check for existence of MySQL
rem   Finds the path to include\mysql.h and extracts the include and lib paths from it
rem
set target=include\mysql.h
echo Searching for MySQL target %target%
set MYSQL=_
if %verbose% == yes echo on
for /r "%search%" %%f in ( %target% ) do if exist %%f set MYSQL=%%~fsf
if "%MYSQL: =_%" == "_" ( 
	echo // %: MYSQL target include\mysql.h not found >>%config% 
	goto MYSQLdone
)
echo Found MYSQL target %target% at [%MYSQL%]
echo // >>%config%
echo // MYSQL configuration >>%config%
echo // >>%config%
echo // %0: found %MYSQL%/ >>%config%
set MYSQL_INCLUDE=%MYSQL:\mysql.h=%
set MYSQL_CNX=%MYSQL_INCLUDE:\include=%
echo #define HAVE_MYSQL "%MYSQL_CNX%" >>%config%
echo #define MYSQL_INCLUDE "%MYSQL_INCLUDE%" >>%config%
echo #ifdef _DEBUG>>%config%
echo #define MYSQL_LIB "%MYSQL_CNX%\lib\debug" >>%config%
echo #else>>%config%
echo #define MYSQL_LIB "%MYSQL_CNX%\lib\opt" >>%config%
echo #endif >>%config%
:MYSQLdone
@if %verbose% == yes echo off

rem
rem - Check for matlab
rem   Finds the path to extern\include\mex.h and extracts the include and lib paths from it
rem
set target=extern\include\mex.h
echo Searching for MATLAB target %target%
set MATLAB=_
if %verbose% == yes echo on
for /r "%search%" %%f in ( %target% ) do if exist %%f set MATLAB=%%~fsf
if "%MATLAB: =_%" == "_" (
	echo // %0: MATLAB target extern\include\mex.h not found >>%config% 
	goto MATLABdone
)
echo Found MATLAB target %target% at [%MATLAB%]
echo // >>%config%
echo // MATLAB configuration >>%config%
echo // >>%config%
echo // %0: found %MATLAB% >>%config%
set MATLAB_INCLUDE=%MATLAB:\mex.h=%
echo #define MATLAB_INCLUDE "%MATLAB_INCLUDE%" >>%config%
set MATLAB_EXTERN=%MATLAB_INCLUDE:\include=%
set MATLAB_LIB=%MATLAB_EXTERN%\lib
for /r "%MATLAB_EXTERN%\lib" %%f in ( libmex.lib ) do if exist %%f set MATLAB_LIBMEX=%%~fsf
echo #define MATLAB_LIB "%MATLAB_LIBMEX:\libmex.lib=%" >>%config%
set MATLAB_DIR=%MATLAB_EXTERN:\extern=%
echo #define HAVE_MATLAB "%MATLAB_DIR%" >>%config%
:MATLABdone
@if %verbose% == yes echo off

rem
rem - Check for existence of PowerWorld
rem 
echo Searching for Powerworld
echo // TODO: POWERWORLD search not implemented >>%config%

echo Configuration done

rem ********************************************************************************************
:Buildnum
if %buildnum% == no goto Return
rem
rem - Update the build number
rem
rem
rem - Check the PATH to ensure SVN.EXE is available on this system
rem 
set exe=svn.exe
for %%a in ( "%PATH:;=" "%" ) do ( if exist "%%a\svn.exe" set exe="%%a\svn.exe" )
if "%exe:"=%" == "svn.exe" goto noexe

rem
rem - Obtain the version number reported by subversion
rem
set new=
for /f "delims=" %%a in ('svn info %source% ^| findstr /b "Revision: "') do set new=%%a 
set newrev=%new:Revision: =%

rem
rem - Obtain the version number stored in build.h
rem
if not exist %build% goto update
set old=
for /f "delims=" %%a in ('type %build%') do set old=%%a
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
echo %build% updated
echo #define BUILDNUM %newrev% >%build%
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

rem ********************************************************************************************
:Return