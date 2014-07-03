@echo off
mkdir "c:\program files\gridlab-d"
mkdir "c:\program files\gridlab-d\bin"
mkdir "c:\program files\gridlab-d\etc"
mkdir "c:\program files\gridlab-d\rt"
mkdir "c:\program files\gridlab-d\tmy"

copy win32\debug\*.exe "c:\program files\gridlab-d\bin"
copy win32\debug\*.dll "c:\program files\gridlab-d\lib"

copy win32\debug\xerces-c_2_8D.dll "c:\program files\gridlab-d\bin"
copy win32\debug\cppunitd.dll "c:\program files\gridlab-d\bin"
copy win32\debug\msvc*.dll "c:\program files\gridlab-d\bin"

copy ..\core\rt\*.* "c:\program files\gridlab-d\rt"
copy ..\core\rt\*.conf "c:\program files\gridlab-d\etc"
copy ..\core\*.txt "c:\program files\gridlab-d\etc"


