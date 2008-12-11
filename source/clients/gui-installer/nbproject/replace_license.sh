#!/bin/sh
FILE=$1
TMP_FILE=${FILE}.new

#Remove existing license
sed -e "/___INFO__MARK_BEGIN__/,/___INFO__MARK_END__/d" $FILE > $TMP_FILE

#Insert new license
sed '1 i\
<!-- (c) 2008 Sun Microsystems, Inc. All rights reserved. Use is subject to license terms. -->' $TMP_FILE > $FILE

#Remove temp file
rm -f $TMP_FILE