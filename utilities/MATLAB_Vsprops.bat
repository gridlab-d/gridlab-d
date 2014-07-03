@echo off
if "%3" == "" goto EMPTYMATLAB
goto FULLMATLAB

:EMPTYMATLAB
echo ^<?xml version="1.0" encoding="Windows-1252"?^>> %2
echo ^<VisualStudioPropertySheet>> %2
echo		ProjectType="Visual C++">> %2
echo 	Version="8.00">> %2
echo 	Name="matlab_specific_%1">> %2
echo 	^>>> %2
echo ^</VisualStudioPropertySheet^>>> %2

goto END

:FULLMATLAB
echo ^<?xml version="1.0" encoding="Windows-1252"?^>> %2
echo ^<VisualStudioPropertySheet>> %2
echo		ProjectType="Visual C++">> %2
echo 	Version="8.00">> %2
echo 	Name="matlab_specific_%1">> %2
echo 	^>>> %2
echo 	^<Tool>> %2
echo 		Name="VCCLCompilerTool">> %2
echo 		AdditionalIncludeDirectories=%3>> %2
echo 	/^>>> %2
echo 	^<Tool>> %2
echo 		Name="VCLinkerTool">> %2
echo 		AdditionalDependencies="libmx.lib libeng.lib">> %2
echo 		AdditionalLibraryDirectories=%4>> %2
echo 	/^>>> %2
echo ^</VisualStudioPropertySheet^>>> %2

:END