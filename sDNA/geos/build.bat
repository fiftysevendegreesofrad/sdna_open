xcopy drop x86\ /E
xcopy drop x64\ /E

rem path must be set to include c:\Program Files\Microsoft Visual Studio 9.0\VC\bin

cd x64
call autogen.bat

call "C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\Vcvarsall.bat" amd64

nmake /f makefile.vc MSVC_VER=1500
cd ..

cd x86
call autogen.bat

call "C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\Vcvarsall.bat" x86

nmake /f makefile.vc MSVC_VER=1500
cd ..

