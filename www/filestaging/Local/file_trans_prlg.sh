#!/bin/sh

# prolog script to transfer files to local system

if [ "X$SGE_IN" = "X" ]; then
# no files specified, so just exit
	exit 0
fi

for file in $SGE_IN ;
do
	echo "now transferring $file..."
	rcp $SGE_O_HOST:/$SGE_O_WORKDIR/$file $TMPDIR
	echo "done transferring $file"
done

