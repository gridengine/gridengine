#!/bin/sh
#
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

# ----------------------------------------
#
# example for a load sensor script
#
# Be careful: Load sensor scripts are started with root permissions.
# In an admin_user system euid=0 and uid=admin_user
#


TEST_HOSTNAME=`cat $SGE_ROOT/testsuite_dns_domain`
PATH=/bin:/usr/bin
ARCH=`$SGE_ROOT/util/arch`
HOST=`$SGE_ROOT/utilbin/$ARCH/gethostname -name`

end=false
while [ $end = false ]; do

   # ---------------------------------------- 
   # wait for an input
   #
   read input
   result=$?
   if [ $result != 0 ]; then
      end=true
      break
   fi
   
   if [ "$input" = "quit" ]; then
      end=true
      break
   fi

   echo "begin"

   echo "$HOST:test_string:TEST_ARCH"

   echo "$HOST:test_hostname:$HOST.$TEST_HOSTNAME"

   echo "$HOST:test_double:55.55"

   echo "$HOST:test_integer:123456"
  
   echo "$HOST:test_memory:123456789"

   echo "end"
done
