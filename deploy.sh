#!/bin/bash
echo -e "\n\n\nThis scripts reads a GitHub repo, compiles with VS2013 then uploads the package to dropbox"
echo "Time: `date`"

if [ $# -ne 3 ]; then
  # usage:
  echo "Usage: $0 RepoName TempDir branches"
  echo "Example: $0 ArcCASPER d:/Archive/arccasperdeploy 'master dev'"
  exit -1
fi

# define some vars
curDir="`pwd`"
scriptDir="`dirname $0`"
tempdir="`cygpath $2`"
repo=$1
branches=$3

[ -d $tempdir ] || mkdir $tempdir

for branch in $branches
do
  cd $tempdir

  # update / clone the repo and then switch to the right branch
  if [ -d "$repo-$branch" ]; then
    echo -e "\n\nrepo already cloned. going to pull."
  else
    echo -e "\n\nrepo will be cloned."
    git clone git@github.com:kaveh096/$repo.git "$repo-$branch"
  fi

  # making sure repo is up-to-date
  cd "$repo-$branch"
  git checkout $branch
  git pull

  # Check if this branch has new commits
  echo "repo is ready for build on branch $branch"

  revi=`git describe`
  # out=$repo-$revi.zip
  if [ "$branch" == "master" ]; then
    out=$repo-$revi-stable.zip
  else
    out=$repo-$revi-nightly.zip
  fi

  if [ `$scriptDir/dropbox_uploader.sh list | grep $out | wc -l` -ge 1 ]; then
    echo "There are no new changes to be built"
  else
    echo "The $branch branch has new commits"

    # update the gitdescribe.h header file before build
    echo "post-commit script: Writing version to gitdescribe.h"
    echo "#ifndef GIT_DESCRIBE" > src/gitdescribe.h
    echo "#define GIT_DESCRIBE \"`git describe`\"" >> src/gitdescribe.h
    echo "#endif" >> src/gitdescribe.h

    # cleanup then rebuild
    rm -f Package/EvcSolver32.* Package/EvcSolver64.* Package/opensteer32.* Package/opensteer64.* Package/README.md
    "/cygdrive/c/Program Files (x86)/MSBuild/12.0/Bin/MSBuild.exe" /m /target:rebuild /p:Configuration=Release /p:Platform=Win32 /p:NOREG=1 ArcCASPER.sln
    "/cygdrive/c/Program Files (x86)/MSBuild/12.0/Bin/MSBuild.exe" /m /target:rebuild /p:Configuration=Release /p:Platform=x64 /p:NOREG=1 ArcCASPER.sln

    # Generate readme file
    sed -e "s/REV/$revi/g" README.md > Package/README.md
    unix2dos Package/README.md
    
    # upload
    echo "Build is complete and ready for zipping and uploading"
    cd Package
    "/cygdrive/c/Program Files/7-Zip/7z.exe" a $out -i@file.list
    if [ $? -eq 0 ]; then
      $scriptDir/dropbox_uploader.sh upload $out $out
    else
      echo "Some files are missing so we do not upload the zip file"
    fi
  fi
done
cd $curDir
