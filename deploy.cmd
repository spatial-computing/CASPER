@ECHO OFF

:: setup my variables
set OLDDIR=%CD%
SET "dir="D:\Archive\arccasperdeploy""
SET url=git@github.com:kaveh096/ArcCASPER.git

:: clean enviroment
IF NOT EXIST %dir% ( git clone %url% %dir% )
cd %dir%

:: call build function
call:buildFunc dev nightly
call:buildFunc master stable

goto :exit

:: ------------------------------------- build and post function
:buildFunc

:: switch to branch
git checkout %1
git pull

:: get hash of head
for /f %%i in ('git describe') do set hash=%%i
set out=ArcCASPER-%2%-%rev%.zip

:: TODO create changle.log from 'git shortlog --no-merges %1 --not v10.1'

:: check if there is new commit to be built
for /f %%i in ('d:\cygwin64\bin\bash.exe -c "/home/kshahabi/dropbox_uploader.sh list | grep %rev% | wc -l"') do set iscommit=%%i
if %iscommit%==1 (
  echo No new code is commited to %1% branch
  goto :eof
)

:: build clean
set NOREG=1
del /Q Package\EvcSolver32.*
del /Q Package\EvcSolver64.*
msbuild.exe /m /target:build /p:Configuration=Release /p:Platform=Win32 USC.GISResearchLab.ArcCASPER.sln
msbuild.exe /m /target:build /p:Configuration=Release /p:Platform=x64   USC.GISResearchLab.ArcCASPER.sln

:: Generate readme file
"c:\Program Files (x86)\GnuWin32\bin\sed.exe" -e "s/REV/%rev%/g" README.md > Package\README.md

:: post to dropbox
cd Package
"c:\Program Files\7-Zip\7z.exe" a %out% -i@file.list
if %errorlevel%==0 (
  echo Uploading to dropbox for public access
  d:\cygwin64\bin\bash.exe -c '/home/kshahabi/dropbox_uploader.sh upload `cygpath -u "%CD%\%out%"` %out%'
) else (
  echo Some of the files are not present
)
goto :eof
:: ------------------------------------- end of build function

:: restore current directory and then exit
:exit
chdir /d %OLDDIR%
exit 0