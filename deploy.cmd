@ECHO OFF

:: setup my variables
set OLDDIR=%CD%
SET "dir="D:\Archive (no backup)\arccasperdeploy""
SET url=git@github.com:kaveh096/ArcCASPER.git
::SET dropbox

:: clean enviroment
IF NOT EXIST %dir% ( git clone %url% %dir% )

:: update the dev branch
cd %dir%
git checkout dev
git pull

:: build clean
msbuild.exe /target:rebuild /p:Configuration=Debug /p:Platform=Win32 USC.GISResearchLab.ArcCASPER.sln
msbuild.exe /target:rebuild /p:Configuration=Debug /p:Platform=x64" USC.GISResearchLab.ArcCASPER.sln

:: restore current directory
chdir /d %OLDDIR%