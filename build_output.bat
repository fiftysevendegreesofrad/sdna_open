call "C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\vcvarsall" amd64

c:\windows\Microsoft.NET\Framework64\v4.0.30319\MSBuild.exe build_output.proj /t:rebuild /p:Configuration=release

pause