#!/bin/sh
#
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


#----------------------------------------------------------------------------     
# ErrUsage
ErrUsage()
{
   $INFOTEXT "Usage: %s [-h {u|o|s},..] job_task_list|'all'|-u user_list|-uall\n" \
             "  job_task_list         job_tasks [,job_tasks, ...]\n" \
             "  job_tasks             job_id['.'task_id_range]\n" \
             "  task_id_range         task_id['-'task_id[':'step]]\n" \
             "  user_list             user['-'user, ...]\n\n" $cmdname
   exit 1
}

#----------------------------------------------------------------------------
# MAIN MAIN MAIN MAIN MAIN MAIN 
#----------------------------------------------------------------------------     

cmdname=`basename $0`

# ensure we can execute qalter binary 
if [ -z "$SGE_ROOT" -o ! -d "$SGE_ROOT" ]; then
   echo
   echo ERROR: Please set your \$SGE_ROOT environment variable first. Exit.
   echo
   exit 1
fi

if [ ! -x "$SGE_ROOT/util/arch" ]; then
   echo
   echo ERROR: The shell script \"\$SGE_ROOT/util/arch\" does not exist.
   echo Please verify your distribution and restart this script. Exit.
   echo
   exit 1
fi

if [ ! -f $SGE_ROOT/util/arch_variables ]; then
   echo
   echo ERROR: Missing shell script \"\$SGE_ROOT/util/arch_variables\".
   echo Please verify your distribution and restart this script. Exit.
   echo
   exit 1
fi

. $SGE_ROOT/util/arch_variables


#---------------------------------------
# setup INFOTEXT begin
#---------------------------------------

V5BIN=$SGE_ROOT/bin/$ARCH
V5UTILBIN=$SGE_ROOT/utilbin/$ARCH
# INFOTXT_DUMMY is needed by message parsing script
# which is looking for $INFOTEXT and would report
# errors in the next command. Please use INFOTXT_DUMMY
# instead of using $INFOTEXT

INFOTXT_DUMMY=$V5UTILBIN/infotext
INFOTEXT=$INFOTXT_DUMMY
if [ ! -x "$INFOTXT_DUMMY" ]; then
   echo
   echo "can't find binary \"$INFOTXT_DUMMY\""
   echo
   exit 1         
fi

SGE_INFOTEXT_MAX_COLUMN=5000; export SGE_INFOTEXT_MAX_COLUMN

#---------------------------------------
# setup INFOTEXT end
#---------------------------------------

QALTER=$SGE_ROOT/bin/`$SGE_ROOT/util/arch`/qalter
if [ ! -x $QALTER ]; then
   $INFOTEXT -e "%s: can't execute %s\n" $cmdname $QALTER
   exit 1
fi


# parse command line
state=start
user_list=""
all_users=""
hold_types=u   
jobid_list=""
while [ "$1" != "" ]; do
   case "$1" in
   -h)
      if [ $state != "start" ]; then
         ErrUsage
      fi
      shift
      hold_types=$1   
      shift
      state=got_hold_list
      ;;
   -u)
      shift
      user_list=$1
      shift
      ;;
   -uall)
      all_users="1"
      shift
      ;;
   [0-9,]*)
      jobid_list="$jobid_list $1"
      shift
      ;;  
   all)
      jobid_list="$jobid_list $1"
      shift
      ;;
   *)
      ErrUsage
      ;;
   esac
done

# invalid parameter - do not start 
if [ "$jobid_list" = "" -a "$user_list" = "" -a "$all_users" = "" ]; then
   $INFOTEXT "%s: list of usernames or jids expected" $cmdname
   ErrUsage
fi

if [ "$user_list" != "" -a "$all_users" != "" ]; then
   $INFOTEXT "%s: selection of usernames and -uall switch are not allowed together" $cmdname
   ErrUsage
fi

if [ \( "$user_list" != "" -o "$all_users" != "" \) -a "$jobid_list" != "" ]; then
   $INFOTEXT "%s: selection of usernames and jids not allowed together" $cmdname
   ErrUsage
fi

# ensure valid hold types are given
valid=`expr $hold_types : '[uso]*$'`
if [ $valid = "0" ]; then
   $INFOTEXT "%s: invalid list of hold types" $cmdname
   ErrUsage
fi

# map qrls hold_types into qalter hold_types
if [ "$cmdname" = "qrls" ]; then
   hold_types="`echo $hold_types|tr 'uos' 'UOS'`"
fi


# build switches for qalter
hold_param=""
user_list_param=""
jobid_list_param=""
all_users_param=""

if [ "$hold_types" != "" ]; then
   hold_param="-h $hold_types"
fi
if [ "$user_list" != "" ]; then
   user_list_param="-u $user_list"
fi
if [ "$all_users" = "1" ]; then
   all_users_param="-uall"
fi
if [ "$jobid_list" != "" ]; then
   jobid_list_param="$jobid_list"
fi

$QALTER $hold_param $user_list_param $all_users_param $jobid_list_param
