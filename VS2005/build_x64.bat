call "c:\Program Files (x86)\Microsoft Visual Studio 8\VC\vcvarsall.bat" x86_amd64
MSBuild gridlabd.sln /t:Rebuild /p:Platform=x64;Configuration=Release /nologo