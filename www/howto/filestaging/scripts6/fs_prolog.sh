#!/bin/sh

# prolog script to transfer files to execution host

echo
echo PROLOG - fs_prolog.sh
echo =====================

echo fs_prolog.sh startet `date` on host `hostname`

FsInputHost=$1
FsInputPath=$2
FsInputTmpPath=$3
FsInputFileStaging=$4
StdinPath=$5

echo FsInputHost=$FsInputHost
echo FsInputPath=$FsInputPath
echo FsInputTmpPath=$FsInputTmpPath
echo FsInputFileStaging=$FsInputFileStaging
echo StdinPath=$StdinPath

if [ "$FsInputFileStaging" = "1" ]; then
   echo "now transferring $FsInputHost:$FsInputPath to (`hostname`:)$FsInputTmpPath"
   rcp $FsInputHost:$FsInputPath $FsInputTmpPath

   echo "done transferring $FsInputPath"
else
   echo "no input file staging wanted"
fi

echo fs_prolog.sh ended `date` on host `hostname`
