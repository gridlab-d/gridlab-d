@echo off
setlocal enableextensions 
setlocal enabledelayedexpansion

set force=no
set warn=no
set verbose=no
set buildnum=yes
set fullsearch=no
set search=C:\
set source=..
set config=config_external.h
set build=build.h
set matlabfile=MATLAB_specific.vsprops
set MATLAB=_
set MATLABFILEOUT=no
set mysqlfile=MYSQL_specific.vsprops
set MYSQL=_
set MYSQLFILEOUT=no
set PWRWORLD=_

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
  if /i not %1 == /a goto Parse5
  set fullsearch=yes
  shift /1
  goto Parse
:Parse5
  if not %1 == /? goto Parse6
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
  echo   CONFIGURE /A enables all search operations to fully parse the SEARCH path
  echo.
  echo The default values are
  echo.
  echo   SEARCH = %search%
  echo   SOURCE = %source%
  echo   CONFIG = %config%
  echo   BUILD  = %build%
  echo.
  echo Note that parameters with spaces should be surrounded with quotation marks.
  echo e.g., SOURCE="C:\My Code\GridLAB-D"
  echo.
  echo Also note that final build.h and config.h files are copied to where the /core
  echo project is expected to be, %source%\core
  echo.
  echo Specifiable locations are:
  echo MYSQL    = MySQL base folder (e.g., C:\Program Files\MySQL\MySQL Connector C)
  echo MATLAB   = MATLAB (e.g., C:\Program Files\MATLAB\R2011a)
  echo PWRWORLD = PowerWorld SimAuto (e.g., C:\Program Files\PowerWorld\Simulator17)
  goto Return
:Parse6
  if "%~2" == "" goto Missing2
  set %1=%~2
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
if exist %source%\core\%config% goto BuildNum

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
if not "%MYSQL: =_%" == "_" goto MYSQLSPECIFIED
if %fullsearch% == yes goto MYSQLFULL
goto MYSQLSMART
REM Full search for MySQL
:MYSQLFULL
set target=include\mysql.h
echo Searching for MySQL target %target%
if %verbose% == yes echo on
for /r "%search%" %%f in ( %target% ) do if exist %%f set MYSQL=%%~fsf
goto MYSQLTEST
REM Constrained search for MySQL
:MYSQLSMART
set target=include\mysql.h
echo Searching for MySQL in expected locations
if %verbose% == yes echo on
if exist "C:\Program Files\MySQL\MySQL Connector C" (
for /r "C:\Program Files\MySQL\MySQL Connector C" %%f in ( %target% ) do if exist %%f (
	set MYSQL=%%~fsf
	goto MYSQLTEST
	)
)
if exist "C:\Program Files (x86)\MySQL\MySQL Connector C" (
for /r "C:\Program Files (x86)\MySQL\MySQL Connector C" %%f in ( %target% ) do if exist %%f (
	set MYSQL=%%~fsf
	goto MYSQLTEST
	)
)
goto MYSQLTEST
REM Batch-file specified search for MySQL
:MYSQLSPECIFIED
echo Searching for MySQL in the specified location
set MYSQL_TEMP=_
for /r "%MYSQL%" %%f in ( include\mysql.h ) do if exist %%f (
	set MYSQL=%%~fsf
	goto MYSQLTEST
	)
if "%MYSQL_TEMP: =_%" == "_" (
	set MYSQL=_
	echo // %: MYSQL target include\mysql.h not found >>%config% 
	goto MYSQLdone
)
REM Test to see if anything was found
:MYSQLTEST
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

REM Flag to generate options
set MYSQLFILEOUT=yes

:MYSQLdone
if %MYSQLFILEOUT% == yes (
	call MYSQL_Vsprops %mysqlfile% "%MYSQL:\mysql.h=%" "%MYSQL_CNX%\lib\opt" 
) else (
	call MYSQL_Vsprops %mysqlfile%
)
@if %verbose% == yes echo off

rem
rem - Check for matlab
rem   Finds the path to extern\include\mex.h and extracts the include and lib paths from it
rem
if not "%MATLAB: =_%" == "_" goto MATLABSPECIFIED
if %fullsearch% == yes goto MATLABFULL
goto MATLABSMART
REM Full search for MATLAB
:MATLABFULL
set target=extern\include\mex.h
echo Searching for MATLAB target %target%
if %verbose% == yes echo on
for /r "%search%" %%f in ( %target% ) do if exist %%f set MATLAB=%%~fsf
goto MATLABTEST
REM Constrained search for MATLAB
:MATLABSMART
set target=extern\include\mex.h
echo Searching for MATLAB in expected locations
if %verbose% == yes echo on
if exist "C:\Program Files\MATLAB" (
for /r "C:\Program Files\MATLAB" %%f in ( %target% ) do if exist %%f (
	set MATLAB=%%~fsf
	goto MATLABTEST
	)
)
if exist "C:\Program Files (x86)\MATLAB" (
for /r "C:\Program Files (x86)\MATLAB" %%f in ( %target% ) do if exist %%f (
	set MATLAB=%%~fsf
	goto MATLABTEST
	)
)
if exist "C:\MATLAB" (
for /r "C:\MATLAB" %%f in ( %target% ) do if exist %%f (
	set MATLAB=%%~fsf
	goto MATLABTEST
	)
)
if exist "C:\Program Files\Mathworks" (
for /r "C:\Program Files\Mathworks" %%f in ( %target% ) do if exist %%f (
	set MATLAB=%%~fsf
	goto MATLABTEST
	)
)
if exist "C:\Program Files (x86)\Mathworks" (
for /r "C:\Program Files (x86)\Mathworks" %%f in ( %target% ) do if exist %%f (
	set MATLAB=%%~fsf
	goto MATLABTEST
	)
)
goto MATLABTEST
REM Batch file specified search for MATLAB
:MATLABSPECIFIED
echo Searching for MATLAB in specified location
set MATLAB_TEMP=_
for /r "%MATLAB%" %%f in ( extern\include\mex.h ) do if exist %%f (
	set MATLAB=%%~fsf
	goto MATLABTEST
	)
if "%MATLAB_TEMP: =_%" == "_" (
	set MATLAB=_
	echo // %0: MATLAB target not found in specified location >>%config% 
	goto MATLABDONE
)
REM Test to see if anything was found
:MATLABTEST
if "%MATLAB: =_%" == "_" (
	echo // %0: MATLAB target extern\include\mex.h not found >>%config% 
	goto MATLABDONE
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
for /r "%MATLAB_LIB%" %%f in ( microsoft\libmx.lib ) do if exist %%f set MATLAB_LIBMEX=%%~fsf
echo #define MATLAB_LIB "%MATLAB_LIBMEX:\libmx.lib=%" >>%config%
set MATLAB_DIR=%MATLAB_EXTERN:\extern=%
echo #define HAVE_MATLAB "%MATLAB_DIR%" >>%config%

REM Flag to generate options
set MATLABFILEOUT=yes

:MATLABDONE
if %MATLABFILEOUT% == yes (
	call MATLAB_Vsprops %matlabfile% "%MATLAB_INCLUDE%" "%MATLAB_LIBMEX:\libmx.lib=%" 
) else (
	call MATLAB_Vsprops %matlabfile%
)
@if %verbose% == yes echo off

rem
rem - Check for existence of PowerWorld
rem 
if not "%PWRWORLD: =_%" == "_" goto POWERWORLDSPECIFIED
if %fullsearch% == yes goto POWERWORLDFULL
goto POWERWORLDSMART
REM Full search for PowerWorld Simulator
:POWERWORLDFULL
echo Searching for PowerWorld - full search
set target=RegSimAuto.exe
echo Searching for PowerWorld SimAuto target %target%
if %verbose% == yes echo on
for /r "%search%" %%f in ( %target% ) do if exist %%f set PWRWORLD=%%~fsf
goto POWERWORLDTEST
REM Constrained search for PowerWorld Simulator
:POWERWORLDSMART
echo Searching for PowerWorld - expected locations
set target=RegSimAuto.exe
echo Searching for PowerWorld SimAuto target %target%
if %verbose% == yes echo on
if exist "C:\Powerworld" (
for /r "C:\PowerWorld" %%f in ( %target% ) do if exist %%f (
	set PWRWORLD=%%~fsf
	goto POWERWORLDTEST
	)
)
if exist "C:\Program Files (x86)\PowerWorld" (
for /r "C:\Program Files (x86)\PowerWorld" %%f in ( %target% ) do if exist %%f (
	set PWRWORLD=%%~fsf
	goto POWERWORLDTEST
	)
)
if exist "C:\Program Files\PowerWorld" (
for /r "C:\Program Files\PowerWorld" %%f in ( %target% ) do if exist %%f (
	set PWRWORLD=%%~fsf
	goto POWERWORLDTEST
	)
)
goto POWERWORLDTEST
REM Batch file specified search for PowerWorld Simulator
:POWERWORLDSPECIFIED
echo Searching for PowerWorld in specified location
for /r "%PWRWORLD%" %%f in ( RegSimAuto.exe ) do if exist %%f goto POWERWORLDTEST
	set PWRWORLD=_
	echo // %0: PowerWorld SimAuto target RegSimAuto.exe not found in specified location >>%config% 
	goto POWERWORLDDONE
REM Test to see if PowerWorld was ever found
:POWERWORLDTEST
if "%PWRWORLD: =_%" == "_" (
	echo // %0: PowerWorld SimAuto target RegSimAuto.exe not found >>%config% 
	goto POWERWORLDdone
)
echo Found PowerWorld target %target% at [%PWRWORLD%]
echo // >>%config%
echo // PowerWorld configuration >>%config%
echo // >>%config%
echo // %0: found %PWRWORLD% >>%config%
echo #define HAVE_POWERWORLD >>%config%
:POWERWORLDdone
@if %verbose% == yes echo off

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
echo Moving files to proper location
if exist %config% move /y %config% "%source%\core"
if exist %build% move /y %build% "%source%\core"
if exist %matlabfile% move /y %matlabfile% "%source%\core\link\matlab"
if exist %mysqlfile% move /y %mysqlfile% "%source%\mysql"

rem ********************************************************************************************
:Return