#!/bin/sh
#
# update cluster config to Grid Engine 5.3
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
#
# $1 = "sge" or "sgeee"
# $2 = file to read
# $3 = file to write
# $4 = gid_range
# $5 = compatibility mode var variables
#
# Only add parameters which aren't set already.
# This makes it safe to call the script more than one time without
# duplicating existing entries.
#
# The entries in the output file are "sorted"
#

PATH=/bin:/usr/bin

ErrUsage()
{
   echo
   echo "usage: `basename $0` sge|sgeee inpfile outfile gid_range compat_mode"
   echo
   exit 1
}

if [ $# -lt 5 ]; then
   ErrUsage
fi

if [ $1 != sge -a $1 != sgeee ]; then
   ErrUsage
fi

if [ ! -f $2 ]; then
   echo "can't read file: $2"
   exit 1
fi


echo "conf_version              0"    >> $3
grep '^qmaster_spool_dir'       $2 >> $3   
grep '^execd_spool_dir'         $2 >> $3   
grep '^binary_path'             $2 >> $3   
grep '^mailer'                  $2 >> $3   
grep '^xterm'                   $2 >> $3   
grep '^load_sensor'             $2 >> $3   
grep '^prolog'                  $2 >> $3   
grep '^epilog'                  $2 >> $3   
grep '^shell_start_mode'        $2 >> $3   
grep '^login_shells'            $2 >> $3   
grep '^min_uid'                 $2 >> $3   
grep '^min_gid'                 $2 >> $3   
grep '^user_lists'              $2 >> $3   
grep '^xuser_lists'             $2 >> $3   

if [ $1 = sgeee ]; then
   grep '^projects' $2 2>&1 >/dev/null
   if [ $? != 0 ]; then
      echo "projects                  none" >> $3
   else
      grep '^projects'          $2 >> $3
   fi

   grep '^xprojects' $2 2>&1 >/dev/null
   if [ $? != 0 ]; then
      echo "xprojects                 none" >> $3
   else
      grep '^xprojects'         $2 >> $3
   fi

   grep '^enforce_project' $2 2>&1 >/dev/null
   if [ $? != 0 ]; then
      echo "enforce_project           false" >> $3
   else
      grep '^enforce_project'   $2 >> $3
   fi

   grep '^enforce_user' $2 2>&1 >/dev/null
   if [ $? != 0 ]; then
      echo "enforce_user              false" >> $3
   else
      grep '^enforce_user'      $2 >> $3
   fi
fi

grep '^load_report_time'        $2 >> $3
grep '^stat_log_time'           $2 >> $3
grep '^max_unheard'             $2 >> $3

# Attention: This resets "reschedule_unknown" from SGE 5.2.x "execd_params"
#
grep '^reschedule_unknown' $2 2>&1 >/dev/null
if [ $? != 0 ]; then
   echo "reschedule_unknown       00:00:00" >> $3
else
   grep '^reschedule_unknown'   $2 >> $3
fi

grep '^loglevel'                $2 >> $3
grep '^administrator_mail'      $2 >> $3
grep '^set_token_cmd'           $2 >> $3
grep '^pag_cmd'                 $2 >> $3
grep '^token_extend_time'       $2 >> $3
grep '^shepherd_cmd'            $2 >> $3
grep '^qmaster_params'          $2 >> $3
grep '^schedd_params'           $2 >> $3
if [ "$5" = none ]; then
   grep '^execd_params'         $2 >> $3
else
   line=`env LC_ALL=C grep '^execd_params' $2`
   param=`echo $line | env LC_ALL=C awk '{print $2}'| env LC_ALL=C tr '[A-Z]' '[a-z]'`
   if [ "$param" != none ]; then
      echo "$line $5" >> $3
   else
      echo "execd_params              $5" >> $3
   fi
fi

grep '^finished_jobs'           $2 >> $3

grep '^gid_range' $2 2>&1 >/dev/null
if [ $? != 0 ]; then
   echo "gid_range                 $4" >> $3
else
   grep '^gid_range'            $2 >> $3
fi

grep '^admin_user'              $2 >> $3
grep '^qlogin_command'          $2 >> $3
grep '^qlogin_daemon'           $2 >> $3
grep '^rlogin_daemon'           $2 >> $3

grep '^default_domain' $2 2>&1 >/dev/null
if [ $? != 0 ]; then
   echo "default_domain            none" >> $3
else
   grep '^default_domain'          $2 >> $3
fi


# Try to set "ignore_fqdn" from SGE 5.2.x "qmaster_params" section
#
grep '^ignore_fqdn' $2 2>&1 >/dev/null
if [ $? != 0 ]; then
   grep -i 'qmaster_params.*ignore_fqdn=true' $2 2>&1 >/dev/null
   ret=$?
   grep -i 'qmaster_params.*ignore_fqdn=1' $2 2>&1 >/dev/null
   if [ $? = 0 -o $ret = 0 ]; then
      echo "ignore_fqdn               true" >> $3
   else
      echo "ignore_fqdn               false" >> $3   
   fi
else
   grep '^ignore_fqdn'          $2 >> $3
fi


grep '^max_aj_instances' $2 2>&1 >/dev/null
if [ $? != 0 ]; then
   echo "max_aj_instances          2000" >> $3
else
   grep '^max_aj_instances'     $2 >> $3
fi

grep '^max_aj_tasks' $2 2>&1 >/dev/null
if [ $? != 0 ]; then
   echo "max_aj_tasks              75000" >> $3
else
   grep '^max_aj_tasks'         $2 >> $3
fi

grep '^max_u_jobs' $2 2>&1 >/dev/null
if [ $? != 0 ]; then
   echo "max_u_jobs                0" >> $3
else
   grep '^max_u_jobs'           $2 >> $3
fi

