#!/bin/sh

#$ -S /bin/sh
#$ -cwd
#$ -o log.txt -j y

if [ $#ARGV -ne 4 ]; then
	echo "Usage: transferjobs.sh <ftp site> <path> <filename> <destdir>"
	exit 1
fi

FTP_SITE=$1
FTP_PATH=$2
FTP_FILE=$3
FTP_DEST=$4

cd $FTP_DEST

# NOTE: the program 'rftp' does FTP across a firewall
# you need to have a .netrc file in your home directory
# which does the authentication for the ftp login
#
/usr/dist/exe/rftp $FTP_SITE << END
cd $FTP_PATH
get $FTP_FILE
quit
END

