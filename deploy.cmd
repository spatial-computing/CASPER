@ECHO OFF

:: setup my variables
set OLDDIR=%CD%
SET dir=d:\Archive\arccasperdeploy
SET cydir=d:/Archive/arccasperdeploy
SET url=git@github.com:kaveh096/ArcCASPER.git
set PATH=%PATH%;D:\cygwin64\bin

:: clean enviroment
IF NOT EXIST %dir% ( git clone %url% %cydir% )

:: call build function
echo Going to build a nightly version
cd %dir%
call:buildFunc dev nightly

echo Going to build an stable version
cd %dir%
call:buildFunc master stable

goto :exit

:: ------------------------------------- build and post function
:buildFunc

:: switch to branch
git checkout %1
git pull

:: get hash of head
for /f %%i in ('git describe') do set revi=%%i
set "out=CASPER4GIS-%revi%-%2%.zip"

:: TODO create changle.log from 'git shortlog --no-merges %1 --not v10.1'

:: check if there is new commit to be built
for /f %%i in ('d:\cygwin64\bin\bash.exe -c "/home/kshahabi/dropbox_uploader.sh list | grep %out% | wc -l"') do set iscommit=%%i
if %iscommit%==1 (
  echo No new code is commited to %1% branch
  goto :eof
)

:: build clean
set NOREG=1
del /Q Package\EvcSolver32.*
del /Q Package\EvcSolver64.*
del /Q Package\README.md
msbuild.exe /m /target:rebuild /p:Configuration=Release /p:Platform=Win32 ArcCASPER.sln
msbuild.exe /m /target:rebuild /p:Configuration=Release /p:Platform=x64   ArcCASPER.sln

:: Generate readme file
"c:\Program Files (x86)\GnuWin32\bin\sed.exe" -e "s/REV/%revi%/g" README.md > Package\README.md

:: post to dropbox
cd Package
"c:\Program Files\7-Zip\7z.exe" a %out% -i@file.list
if %errorlevel%==0 (
  d:\cygwin64\bin\bash.exe -c '/home/kshahabi/dropbox_uploader.sh upload `cygpath -u "%CD%\%out%"` %out%'
) else (
  echo Some of the files are not present
)
goto :eof
:: ------------------------------------- end of build function

:: restore current directory and then exit
:exit
chdir /d %OLDDIR%
