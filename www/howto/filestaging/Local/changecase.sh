#!/bin/sh

# Sample job script which uses the file-staging facility
# This job converts the input file text to all uppercase and
# prints it to the output file
# The job script uses the SGE_IN variable to determine the input
# filename and SGE_OUT to specify the output.  These variables
# also specify the file to transfer

# Usage: qsub -v SGE_IN=<file to transform>,SGE_OUT=<output filename> \
#	changecase.sh

#$ -cwd
#$ -o log.txt -j y
#$ -S /bin/sh

# if we don't specify the input file then just exit
#
if [ ! -f $SGE_IN ]; then
	exit 0
fi

# actual job begins here

# it's recommended to work in $TMPDIR, since it's a unique directory
# and is automatically cleaned and removed by SGE after the epilog
#
cd $TMPDIR

# this sample job just does some character conversions
#
tr -s 'a-z' 'A-Z' < $SGE_IN > $SGE_OUT

