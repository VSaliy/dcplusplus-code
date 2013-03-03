@echo off
choice /M "%0 - Make sure you have Cygwin and Visual Studio - Make sure you have an 'openssl-*.tar.gz' file in this directory - Continue?"
echo on
if errorlevel 2 goto end

set VCDIR=C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC

mv ../include/openssl/opensslconf.h .
rm -rf ../include ../lib
mkdir ..\include\openssl ..\lib\x64 ..\lib\ia64
mv opensslconf.h ../include/openssl

rm -rf */
tar -xzf *.tar.gz
cd openssl*
patch -p1 -i../patch.patch
bash --login -i -c "cd '%CD%' && ./Configure mingw --cross-compile-prefix=i686-pc-mingw32- && make"
if errorlevel 1 goto end
mv crypto/opensslconf.h ../../include/openssl/opensslconf-mingw-x86.h
mv libcrypto.a libssl.a ../../lib
cd ..

rm -rf */
tar -xzf *.tar.gz
cd openssl*
patch -p1 -i../patch.patch
bash --login -i -c "cd '%CD%' && ./Configure mingw64 --cross-compile-prefix=x86_64-w64-mingw32- && make"
if errorlevel 1 goto end
mv crypto/opensslconf.h ../../include/openssl/opensslconf-mingw-x64.h
mv libcrypto.a libssl.a ../../lib/x64
cd ..

rm -rf */
tar -xzf *.tar.gz
cd openssl*
patch -p1 -i../patch.patch
call "%VCDIR%\vcvarsall.bat" x86
echo on
perl Configure VC-WIN32 --prefix=.
call ms\do_nasm
nmake -f ms\nt.mak clean install
if errorlevel 1 goto end
mv inc32/openssl/opensslconf.h ../../include/openssl/opensslconf-msvc-x86.h
mv inc32/openssl/* ../../include/openssl
mv lib/libeay32.lib lib/ssleay32.lib tmp32/lib.pdb ../../lib
nmake -f ms\ntdebug.mak clean install
if errorlevel 1 goto end
mv lib/libeay32.lib ../../lib/libeay32d.lib
mv lib/ssleay32.lib ../../lib/ssleay32d.lib
mv tmp32.dbg/libd.pdb ../../lib
cd ..

rm -rf */
tar -xzf *.tar.gz
cd openssl*
patch -p1 -i../patch.patch
call "%VCDIR%\vcvarsall.bat" amd64
echo on
perl Configure VC-WIN64A --prefix=.
call ms\do_win64a
nmake -f ms\nt.mak clean install
if errorlevel 1 goto end
mv inc32/openssl/opensslconf.h ../../include/openssl/opensslconf-msvc-x64.h
mv lib/libeay32.lib lib/ssleay32.lib tmp32/lib.pdb ../../lib/x64
nmake -f ms\ntdebug.mak clean install
if errorlevel 1 goto end
mv lib/libeay32.lib ../../lib/x64/libeay32d.lib
mv lib/ssleay32.lib ../../lib/x64/ssleay32d.lib
mv tmp32.dbg/libd.pdb ../../lib/x64
cd ..

rm -rf */
:end
pause
