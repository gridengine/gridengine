#!/bin/sh

# epilog script to transfer files to submit host

echo
echo EPILOG - fs_epilog.sh
echo =====================

echo fs_epilog.sh startet `date` on host `hostname`

FsMergeStderr=$1
FsOutputHost=$2
FsOutputPath=$3
FsOutputTmpPath=$4
FsOutputFileStaging=$5
FsStdOut=$6

shift 6

FsErrorHost=$1
FsErrorPath=$2
FsErrorTmpPath=$3
FsErrorFileStaging=$4
FsStdErr=$5

echo FsMergeStderr=$FsMergeStderr
echo FsOutputHost=$FsOutputHost
echo FsOutputPath=$FsOutputPath
echo FsOutputTmpPath=$FsOutputTmpPath
echo FsOutputFileStaging=$FsOutputFileStaging
echo FsStdOut=$FsStdOut

echo FsErrorHost=$FsErrorHost
echo FsErrorPath=$FsErrorPath
echo FsErrorTmpPath=$FsErrorTmpPath
echo FsErrorFileStaging=$FsErrorFileStaging
echo FsStdErr=$FsStdErr

if [ "$FsErrorFileStaging" = "1" -a "$FsMergeStderr"
!= "1" ]; then 
   DestPath=$FsErrorHost:$FsErrorPath
   echo "now transferring  (`hostname`):$FsErrorTmpPath to $DestPath"
   rcp $FsErrorTmpPath $DestPath

   echo "done transferring $FsErrorPath"
else
   if [ "$FsMergeStderr" = "1" ]; then
      echo "no error file staging needed (error file merged into output file)"
   else
      echo "no error file staging wanted"
   fi
fi

if [ "$FsOutputFileStaging" = "1" ]; then
   DestPath=$FsOutputHost:$FsOutputPath
   echo "now transferring (`hostname`):$FsOutputTmpPath to $DestPath"
   rcp $FsOutputTmpPath $DestPath

   echo "done transferring (`hostname`):$FsOutputTmpPath"
else
   echo "no output file staging wanted"
fi

echo fs_epilog.sh ended `date` on host `hostname`
