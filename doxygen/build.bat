@echo off

setlocal enableextensions

:: Name temporary file
:temps
set CONFIGIN=%TEMP%\dx-build-in.%RANDOM%.tmp
set CONFIGOUT=%TEMP%\dx-build.%RANDOM%.tmp
if exist "%CONFIGIN%" goto temps
if exist "%CONFIGOUT%" goto temps
set TEMPS="%CONFIGIN%" "%CONFIGOUT%"

:: Clear variables from the environment
set CONFIG=
set EXTRAPATH=
set WORKINGDIR=

:: Parse command-line for options and config
:args
if %1.==. goto main
set KEY=%~1
if %KEY:~0,1%==/ goto option
if %KEY:~0,1%==- goto option
if %2.==. goto config
echo %KEY% = %2 >> "%CONFIGIN%"
goto shift

:option
set KEY=%KEY:~1%
if /I %KEY%==h goto usage
if /I %KEY%==help goto usage
if %KEY%==? goto usage
echo :WORKINGDIR:EXTRAPATH: | find /I ":%KEY%:" > nul
if errorlevel 1 goto usage
set %KEY%=%~2

:shift
shift /1
shift /1
goto args


:: A configuration file was specified on the command-line
:config
set CONFIG=%~fs1

:main
if "%WORKINGDIR%"=="" set WORKINGDIR=%~dps0..
if "%CONFIG%"=="" set CONFIG=%~dps0gridlabd.conf
if not "%EXTRAPATH%"=="" set PATH=%PATH%;%EXTRAPATH%
if exist %~dps0..\..\tools\doxygen\doxygen.exe set PATH=%PATH%;%~dp0..\..\tools\doxygen

if not exist "%CONFIG%" (
	echo %~n0: ERROR: missing configuration file: %CONFIG%
	del /q %TEMPS% > nul
	exit /b 1
)

pushd "%WORKINGDIR%"
if exist "%CONFIGIN%" (
	copy /y "%CONFIG%"+"%CONFIGIN%" "%CONFIGOUT%" > nul
	doxygen.exe "%CONFIGOUT%"
	set RESULT=%errorlevel%
	del /q %TEMPS% > nul
) else (
	doxygen.exe "%CONFIG%"
	set RESULT=%errorlevel%
)

exit /b %RESULT%

:usage
echo Build Doxygen documenation according to the given CONFIG file.  KEY/VALUE pairs
echo given on the command-line override those in the CONFIG file.
echo.
echo %~n0 [OPTIONS]... [KEY VALUE]... [CONFIG]
echo.
echo   CONFIG           Doxygen config file containing default options.
echo   /extrapath PATH  PATH is appended to the PATH environment variable.  Use to
echo                    to set or override the search path for doxygen.exe.
echo   /workingdir DIR  Change to DIR before executing doxygen.exe.
echo.
echo The default CONFIG file is %~dp0gridlabd.conf.

del /q %TEMPS% > nul
exit /b 1

