@ECHO OFF

:: setup my variables
set OLDDIR=%CD%
SET "dir="D:\Archive\arccasperdeploy""
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
set out=ArcCASPER-nigthly-%hash%.zip

:: TODO use 'git describe branchname' for release archive name
:: TODO create changle.log from 'git shortlog --no-merges master --not v1.0.1'

:: check if there is new commit to be built
for /f %%i in ('d:\cygwin64\bin\bash.exe -c "/home/kshahabi/dropbox_uploader.sh list | grep %hash% | wc -l"') do set iscommit=%%i
if %iscommit%==1 (
  echo No new code is commited to dev branch
  goto :stable
)

:: build clean
set NOREG=1
del /Q Package\EvcSolver32.*
del /Q Package\EvcSolver64.*
msbuild.exe /m /target:build /p:Configuration=ReleaseNoFlock /p:Platform=Win32 USC.GISResearchLab.ArcCASPER.sln
msbuild.exe /m /target:build /p:Configuration=ReleaseNoFlock /p:Platform=x64   USC.GISResearchLab.ArcCASPER.sln

:: post to dropbox
cd Package
"c:\Program Files\7-Zip\7z.exe" a %out%  -i@file.list
if %errorlevel%==0 (
  echo Uploading to dropbox for public access
  d:\cygwin64\bin\bash.exe -c '/home/kshahabi/dropbox_uploader.sh upload `cygpath -u "%CD%\%out%"` %out%'
) else (
  echo Some of the files are not present
)

:stable
:: checkout master for stable release

:: update the dev branch
cd %dir%
git checkout master
git pull

:: get hash of dev head
for /f %%i in ('git rev-parse --short HEAD') do set hash=%%i
set out=ArcCASPER-stable-%hash%.zip

:: check if there is new commit to be built
for /f %%i in ('d:\cygwin64\bin\bash.exe -c "/home/kshahabi/dropbox_uploader.sh list | grep %hash% | wc -l"') do set iscommit=%%i
if %iscommit%==1 (
  set "msg=No new code is commited to master branch"
  goto :exit
)

:: build clean
set NOREG=1
del /Q Package\EvcSolver32.*
del /Q Package\EvcSolver64.*
msbuild.exe /m /target:build /p:Configuration=Release /p:Platform=Win32 USC.GISResearchLab.ArcCASPER.sln
msbuild.exe /m /target:build /p:Configuration=Release /p:Platform=x64   USC.GISResearchLab.ArcCASPER.sln

:: TODO Generate readme file

:: post to dropbox
cd Package
"c:\Program Files\7-Zip\7z.exe" a %out%  -i@file.list
if %errorlevel%==0 (
  echo Uploading to dropbox for public access
  d:\cygwin64\bin\bash.exe -c '/home/kshahabi/dropbox_uploader.sh upload `cygpath -u "%CD%\%out%"` %out%'
) else (
  echo Some of the files are not present
)

:: restore current directory and then exit
:exit
echo %msg%
chdir /d %OLDDIR%
