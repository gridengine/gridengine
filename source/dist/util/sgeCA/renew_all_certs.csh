#!/bin/csh
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

# extend the validity of the CA certificate by
set CADAYS = 365 
# extend the validity of the daemon certificate by
set DAEMONDAYS = 365
# extend the validity of the user certificate by
set USERDAYS = 365



if ( ! ($?SGE_ROOT && $?SGE_CELL) ) then
   echo "SGE_ROOT environment not set"
   exit 1
endif

if ( $?SGE_QMASTER_PORT ) then
   set CA_PORT = port$SGE_QMASTER_PORT
else
   set CA_PORT = sge_qmaster
endif

set CERT = "/var/sgeCA/$CA_PORT/$SGE_CELL/userkeys"

echo $CERT

# renew the ca certificate
$SGE_ROOT/util/sgeCA/sge_ca -renew_ca -days $CADAYS

# renew the daemon certificate
$SGE_ROOT/util/sgeCA/sge_ca -renew_sys -days $DAEMONDAYS 

# renew all user certificates
foreach i ($CERT/*)
   set user = `basename $i`
   $SGE_ROOT/util/sgeCA/sge_ca -renew $user -days $USERDAYS
end
