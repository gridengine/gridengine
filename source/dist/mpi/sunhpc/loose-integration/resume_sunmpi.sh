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
#  License at http://www.gridengine.sunsource.net/license.html
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
#
# Resume an MPI job when its parent batch job is resumed
#
# It assumes that CRE JID is saved by suspend method.
#
#----------------
cre_jid=`cat $TMPDIR/resume_cre_jid`
job_pid=$1

#
# Resume the CRE job
#
/opt/SUNWhpc/bin/mpkill -CONT $cre_jid
#
# Resume the job script
#
kill -CONT -$job_pid 

#
# Delete the CRE job id file
#
/bin/rm $TMPDIR/resume_cre_jid

# signal success to caller
exit 0
