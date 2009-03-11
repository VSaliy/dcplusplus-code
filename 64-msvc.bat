call "C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\vcvarsall.bat" x64
echo on
call scons tools=default arch=x64
echo on
pause
