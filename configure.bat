@echo off
rem This script does the job that configure does on Linux systems but for MS Windows system with the mingw compiler
set _cd=%CD%

rem Set to path to mingw tools
set mingw=c:\mingw\bin

rem set proper flags
set CFLAGS=-DMINGW
set CPPFLAGS=-DMINGW

rem set target list
rem Check that automake is present
set automake=%CD%\automake
if exist %automake%.bat (echo automake ok) else (call :ERROR %automake%.bat not found)

rem Check that mingw is installed correctly
set checklist=gcc make
for %%i in (%checklist%) do set %%i=%mingw%\%%i.exe
for %%i in (%checklist%) do if exist %gcc% (echo %%i ok) else (call :ERROR "%%i.exe: file not found")

rem Read target list
for /F "delims=;" %%I in (makefile.win) do set %%I

rem Create target folder
if not exist %OUTDIR% mkdir %OUTDIR%

rem Main targets
echo # generate by automake >makefile
echo+ >>makefile
echo all: >>makefile
for %%I in (%TARGETS%) do echo+	make -C %%I all >>makefile
echo+ >>makefile
echo clean: >>makefile
for %%I in (%TARGETS%) do echo+	make -C %%I clean >>makefile
echo+ >>makefile

rem Identify list of subfolders with targets
for %%I in (%TARGETS%) do call :automake %%I

cd %_cd%
goto :EOF

:automake
echo Updating %1%
set _T=%CD%
cd %1%
if exist makefile.win (call %automake% > makefile) else (call :ERROR %1%\makefile.win not found)
cd /d %_T%
goto :EOF

:ERROR
echo %*%
goto :EOF
cd /d %_cd%

