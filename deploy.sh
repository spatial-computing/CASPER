#!/bin/bash
echo "This scripts reads a GitHub repo, compiles with with VS2013 then uploads the package to dropbox"

if [ $# -ne 3 ]; then
  # usage:
  echo "Usage: $0 RepoName TempDir branches"
  echo "Example: $0 ArcCASPER d:/Archive/arccasperdeploy 'master dev'"
  exit -1
fi

# define some vars
curdir="`pwd`"
tempdir="`cygpath $2`"
repo=$1
branches=$3

[ -d $tempdir ] || mkdir $tempdir

for branch in $branches
do
  cd $tempdir

  # update / clone ther repo and switch to the right branch
  if [ -d "$repo-$branch" ]; then
    echo "repo already cloned. going to pull."
  else
    echo "repo will be cloned."
    git clone git@github.com:kaveh096/$repo.git "$repo-$branch"
  fi

  # makeing sure repo is up-to-date
  cd "$repo-$branch"
  git checkout $branch
  git pull

  # Check if this branch has new commits
  echo "repo is ready for build on branch $branch"

  revi=`git describe`
  out=$repo-$revi.zip
  if [ "$branch" -eq "master" ]; then
    set out=$repo-$revi-stable.zip
  else
    set out=$repo-$revi-nightly.zip
  fi

  if [ `$curdir/dropbox_uploader.sh list | grep $out | wc -l` -eq 1 ]; then
    echo "There are no new changes to be built"
  else
    echo "The $branch branch has new commits"

    # cleanup then rebuild
    rm -f Package/EvcSolver32.* Package/EvcSolver64.* Package/README.md
    "/cygdrive/c/Program Files (x86)/MSBuild/12.0/Bin/MSBuild.exe" /m /target:rebuild /p:Configuration=Release /p:Platform=Win32 /p:NOREG=1 ArcCASPER.sln
    "/cygdrive/c/Program Files (x86)/MSBuild/12.0/Bin/MSBuild.exe" /m /target:rebuild /p:Configuration=Release /p:Platform=x64 /p:NOREG=1 ArcCASPER.sln

    # Generate readme file
    sed -e "s/REV/$revi/g" README.md > Package/README.md
    
    # upload
    echo "Build is complete and ready for zipping and uploading"
    cd Package
    "/cygdrive/c/Program Files/7-Zip/7z.exe" a $out -i@file.list
    if [ $? -eq 0 ]; then
      $curdir/dropbox_uploader.sh upload $out $out
    else
      echo "Some files are missing so we do not upload the zip file"
    fi
  fi
done
cd $curdir
