#!/bin/sh

# check for correct start directory
if [ ! -d tcl_files -o ! -d checktree ]; then
   printf "Please start $0 from the testsuite root directory.\n"
   exit 1
fi

# check adoc installation
ADOC=`type adoc`
if [ $? -ne 0 ]; then
   printf "The adoc utility is not available. Please check PATH.\n"
   exit 1
fi

# check texi2html installation
#ADOC=`type texi2html`
#if [ $? -ne 0 ]; then
#   printf "The texi2html utility is not available. Please check PATH.\n"
#   exit 1
#fi

TCL_FILES="check.exp tcl_files/*.tcl"

PROJECT="Grid Engine Testsuite"
EDITION="1.0"
REVISION="Testsuite 1.3" # better grep testsuite revision from tcl_files/config.tcl
COPYRIGHT="The Grid Engine Team"
AUTHOR="The Grid Engine Team"

# create documentation
adoc --no-warnings -f -b -DPROJECT "${PROJECT}" -DEDITION "${EDITION}" -DREVISION "${REVISION}" -DCOPYRIGHT "${COPYRIGHT}" -DAUTHOR "${AUTHOR}" -I -xon -xitemize -2 -o reference.texi `echo ${TCL_FILES}`

#texi2html reference.texi
# -glossary -menu -number -split chapter|section
