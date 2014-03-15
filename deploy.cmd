@ECHO ON

:: setup my variables
set OLDDIR=%CD%
SET "dir="D:\Archive (no backup)\arccasperdeploy""
SET url=git@github.com:kaveh096/ArcCASPER.git
SET msg=""

:: clean enviroment
IF NOT EXIST %dir% ( git clone %url% %dir% )

:: update the dev branch
cd %dir%
git checkout dev
git pull

:: get hash of dev head
for /f %%i in ('git rev-parse --short HEAD') do set hash=%%i

:: check if there is new commit to be built
for /f %%i in ('d:\cygwin64\bin\bash.exe -c "/home/kshahabi/dropbox_uploader.sh list | grep %hash% | wc -l"') do set iscommit=%%i
if %iscommit%==1 (
  SET "msg=No new code is commited to dev branch"
  goto :exit
)

:: build clean
set NOREG=1
del /Q Package\EvcSolver32.*
del /Q Package\EvcSolver64.*
::msbuild.exe /target:rebuild /p:Configuration=Release /p:Platform=Win32 USC.GISResearchLab.ArcCASPER.sln
::msbuild.exe /target:rebuild /p:Configuration=Release /p:Platform=x64   USC.GISResearchLab.ArcCASPER.sln

:: post to dropbox
cd Package
set ERRORLEVEL=
"c:\Program Files\7-Zip\7z.exe" a ArcCASPER-nigthly-%hash%.zip EvcSolver32.dll EvcSolver64.dll glut32.dll glut64.dll Manual.pdf readme.txt install.cmd uninstall.cmd
echo %ERRORLEVEL%
if %ERRORLEVEL%==0 (
  d:\cygwin64\bin\bash.exe -c "/home/kshahabi/dropbox_uploader.sh upload \"$(cygpath -u \"%CD%\ArcCASPER-nigthly-%hash%.zip\")\" ArcCASPER-nigthly-%hash%.zip"
) else (
  SET "msg=Some of the files are not present"
  goto :exit
)

:: restore current directory and then exit
:exit
echo %msg%
chdir /d %OLDDIR%