#!/bin/sh
#___INFO__MARK_BEGIN__
##########################################################################
#
#  The Contents of this file are made available subject to the terms of
#  the Sun Industry Standards Source License Version 1.2
#
#  Sun Microsystems Inc., March, 2001
#
#
#  Sun Industry Standards Source License Version 1.2
#  =================================================
#  The contents of this file are subject to the Sun Industry Standards
#  Source License Version 1.2 (the "License"); You may not use this file
#  except in compliance with the License. You may obtain a copy of the
#  License at http://gridengine.sunsource.net/Gridengine_SISSL_license.html
#
#  Software provided under this License is provided on an "AS IS" basis,
#  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
#  WITHOUT LIMITATION, WARRANTIES THAT THE SOFTWARE IS FREE OF DEFECTS,
#  MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE, OR NON-INFRINGING.
#  See the License for the specific provisions governing your rights and
#  obligations concerning the Software.
#
#  The Initial Developer of the Original Code is: Sun Microsystems, Inc.
#
#  Copyright: 2001 by Sun Microsystems, Inc.
#
#  All Rights Reserved.
#
##########################################################################
#___INFO__MARK_END__

echo "******************************************************"
echo "testsuite - debug startup script"
# SGE_DEBUG_LEVEL="3 0 0 0 0 0 0 0"  ;# full top layer debug
# SGE_DEBUG_LEVEL="2 0 0 0 0 0 0 0"    ;# normal debug
# SGE_DEBUG_LEVEL="3 3 3 3 3 3 3 3"  ;# full debug
OUTPUT_FILE=$1
SGE_DEBUG_LEVEL=$2
export SGE_DEBUG_LEVEL
SGE_ND=1
export SGE_ND
echo " SGE_DEBUG_LEVEL : \"$SGE_DEBUG_LEVEL\""
echo " SGE_ND          : 1"
echo " id output       : `id`"
echo " hostname        : `hostname`"
echo " output file     : ${OUTPUT_FILE}"
echo " SGE_ROOT        : ${SGE_ROOT}"
echo " COMMD_PORT      : ${COMMD_PORT}"
echo " SGE_QMASTER_PORT: ${SGE_QMASTER_PORT}"
echo " SGE_EXECD_PORT  : ${SGE_EXECD_PORT}"
echo "******************************************************"
echo "this terminal will execute following command:"
shift
shift
echo "$* 2>&1 | tee -a ${OUTPUT_FILE}"

echo ""
echo "continue in 5 seconds ..."
sleep 5
# echo "please press enter"
# read input
touch ${OUTPUT_FILE}
echo "\n\n\n\n\n\n\n\n\n"                                     >> ${OUTPUT_FILE}
echo "******************************************************" >> ${OUTPUT_FILE}
echo "*  debug from `date`"                                   >> ${OUTPUT_FILE}
echo "*  $*" >> ${OUTPUT_FILE}                                >> ${OUTPUT_FILE}
echo "*  SGE_DEBUG_LEVEL : \"$SGE_DEBUG_LEVEL\""              >> ${OUTPUT_FILE}
echo "*  id output       : `id`"                              >> ${OUTPUT_FILE}
echo "*  hostname        : `hostname`"                        >> ${OUTPUT_FILE}
echo "*  SGE_ROOT        : ${SGE_ROOT}"                       >> ${OUTPUT_FILE}
echo "*  COMMD_PORT      : ${COMMD_PORT}"                     >> ${OUTPUT_FILE}
echo "*  SGE_QMASTER_PORT: ${SGE_QMASTER_PORT}"               >> ${OUTPUT_FILE}
echo "*  SGE_EXECD_PORT  : ${SGE_EXECD_PORT}"                 >> ${OUTPUT_FILE}
echo "******************************************************" >> ${OUTPUT_FILE}
# rm ${OUTPUT_FILE}
# touch ${OUTPUT_FILE}
$* 2>&1 | tee -a ${OUTPUT_FILE} 

echo "done. terminal will exit in 300 seconds ;-)"
sleep 300
