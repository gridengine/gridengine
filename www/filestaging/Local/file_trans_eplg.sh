#!/bin/sh

# epilog script to transfer files to submit host

if [ "X$SGE_OUT" = "X" ]; then
# no files specified, so just exit
	exit 0
fi

for file in $SGE_OUT ;
do
	echo "now transferring $file..."
	rcp $TMPDIR/$file $SGE_O_HOST:/$SGE_O_WORKDIR
	echo "done transferring $file"
done

