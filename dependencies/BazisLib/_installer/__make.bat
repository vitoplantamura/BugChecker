REM @echo off
set DEVENVPATH=%VS90COMNTOOLS%\..\IDE

REM All sources and setup program binary should be placed into "bzslib" subdirectory. Then, the following command should be run:
rmdir /s /q ..\..\bzslib.tmp2
rmdir /s /q ..\..\bzslib.inst
mkdir ..\..\bzslib.inst
mkdir ..\..\bzslib.inst\bzslib

cd ..\support\BazisLibSetup
"%DEVENVPATH%\devenv" BazisLibSetup.sln /Build Release
if not exist bin\Release\BazisLibSetup.exe goto error
if not exist bin\Release\ICSharpCode.SharpZipLib.dll goto error
copy /y bin\Release\BazisLibSetup.exe ..\..\..\bzslib.inst\bzslib\
copy /y bin\Release\ICSharpCode.SharpZipLib.dll ..\..\..\bzslib.inst\bzslib\

mkdir ..\..\..\bzslib.tmp2
cd ..\..\..\bzslib.tmp2
tortoiseproc /command:checkout /path:. /closeonend:1 /url:https://sys1:36841/svn/sysprogs/trunk/bzslib
rmdir /s /q .svn
cd ..\bzslib.inst
xcopy /e ..\bzslib.tmp2\*.* bzslib\*.*
xcopy ..\bzslib.tmp2\_installer\*.* .
rmdir /s /q ..\bzslib.tmp2
cd bzslib
..\..\utils\perl support\convert_vcproj.pl bzscmn\bzscmn.vcproj
..\..\utils\perl support\convert_vcproj.pl bzsddk\bzsddk.vcproj
..\..\utils\perl support\convert_vcproj.pl bzsnet\bzsnet.vcproj
..\..\utils\perl support\convert_vcproj.pl bzswin\bzswin.vcproj
cd ..
..\utils\ssibuilder BazisLib.xit BazisLib.exe /LINKCABS /EXESTUB:\projects\svn\utils\ssisfx.exe
cd bzslib
..\..\utils\zip ..\_BazisLib-PutToSysprogs.zip -r * 
cd ..
goto end

:error
echo ERROR building BazisLib installer
pause
:end