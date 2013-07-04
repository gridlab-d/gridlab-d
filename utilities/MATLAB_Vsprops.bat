@echo off
if "%2" == "" goto EMPTYMATLAB
goto FULLMATLAB

:EMPTYMATLAB
echo ^<?xml version="1.0" encoding="Windows-1252"?^>> %1
echo ^<VisualStudioPropertySheet>> %1
echo		ProjectType="Visual C++">> %1
echo 	Version="8.00">> %1
echo 	Name="matlab_specific">> %1
echo 	^>>> %1
echo ^</VisualStudioPropertySheet^>>> %1

goto END

:FULLMATLAB
echo ^<?xml version="1.0" encoding="Windows-1252"?^>> %1
echo ^<VisualStudioPropertySheet>> %1
echo		ProjectType="Visual C++">> %1
echo 	Version="8.00">> %1
echo 	Name="matlab_specific">> %1
echo 	^>>> %1
echo 	^<Tool>> %1
echo 		Name="VCCLCompilerTool">> %1
echo 		AdditionalIncludeDirectories=%2>> %1
echo 	/^>>> %1
echo 	^<Tool>> %1
echo 		Name="VCLinkerTool">> %1
echo 		AdditionalDependencies="libmx.lib libeng.lib">> %1
echo 		AdditionalLibraryDirectories=%3>> %1
echo 	/^>>> %1
echo ^</VisualStudioPropertySheet^>>> %1

:END