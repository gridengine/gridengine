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
#  Copyright: 2008 by Sun Microsystems, Inc.
#
#  All Rights Reserved.
#
##########################################################################
#___INFO__MARK_END__

########################################################################### 
#
# example for a job verification script 
#
# Be careful:  Job verification scripts are started with sgeadmin 
#              permissions if they are executed within the master process
#

PATH=/bin:/usr/bin
ARCH=`$SGE_ROOT/util/arch`

logging_enabled="false" # is logging enabled?
logfile="/tmp/jsv_$$.log"  # logfile

# Current state of the script
# Might be "initialized", "started" or "verifying"
state="initialized"       

# Following strings are switch names of command line clients (qsub, qrsh, ...) 
# and these strings will also be used as variable suffixes in this script
jsv_cli_params="a ar A b ckpt cwd C display dl e hard h hold_jid\
                hold_jid_ad i inherit j js m M masterq notify\
                now N noshell nostdin o ot P p pe pty R r shell sync S t\
                terse u w wd"

# These names are the suffixes of variable names which will contain
# the information of following submit client switches:
#     ac: summarized job context information (-sc -ac -dc)
#     l_hard: hard resource request (-hard -l ...)
#     l_soft: soft resource requests (-soft -l ...)
#     q_hard: hard queue resource request (-hard -q ...)
#     q_soft: soft queue resource request (-soft -q ...)
jsv_mod_params="ac l_hard l_soft q_hard q_soft"

# Here are the suffixes of variable names which do not directly appear
# as named switches in a client.
#     CLIENT: submit client which was used
#     CONTEXT: in which context is this script executed
#     ...
jsv_add_params="CLIENT CONTEXT GROUP VERSION JOB_ID SCRIPT CMDARGS USER"

# Values specified with the list below will be available in this script 
# as variables with the name "jsv_param_<name>". If a corresponding value
# was specified during submission then this variable will have the
# same value. Find more information in jsv(5) and submit(1) man page.
jsv_all_params="$jsv_add_params $jsv_cli_params $jsv_mod_params"

# This is a space separated list of environment variable names which
# will built up during runtime, when this script receives the job env.
jsv_all_envs=""

undef="variable_is_undefined"
quit="false"
saved_ifs="$IFS"

jsv_clear_params()
{
   for i in $jsv_all_params; do
      name="jsv_param_$i"
      command=`eval "echo \$\{$name\:\-$undef\}"`
      isdef=`eval "echo $command"`
      if [ "$isdef" != "$undef" ]; then
         unset $name
      fi
      unset name
      unset command
      unset isdef
   done
}

jsv_clear_envs()
{
   for i in $jsv_all_envs; do
      name="jsv_env_$i"
      command=`eval "echo \$\{$name\:\-$undef\}"`
      isdef=`eval "echo $command"`
      if [ "$isdef" != "$undef" ]; then
         unset $name
      fi
      unset name
      unset command
      unset isdef
   done
   jsv_all_envs=""
}

jsv_show_params()
{
   for i in $jsv_all_params; do
      name="jsv_param_$i"
      command=`eval "echo \$\{$name\:\-$undef\}"`
      isdef=`eval "echo $command"`
      if [ "$isdef" != "$undef" ]; then
         eval "echo LOG INFO got param $i=\${$name}"
      fi
      unset name
      unset command
      unset isdef
   done
}

jsv_show_envs()
{
   for i in $jsv_all_envs; do
      name="jsv_env_$i"
      command=`eval "echo \$\{$name\:\-$undef\}"`
      isdef=`eval "echo $command"`
      if [ "$isdef" != "$undef" ]; then
         eval "echo LOG INFO got env $i=\${$name}"
      fi
      unset name
      unset command
      unset isdef
   done
}

jsv_is_env() 
{
   name="jsv_env_$1"
   command=`eval "echo \$\{$name\:\-$undef\}"`
   isdef=`eval "echo $command"`
   if [ "$isdef" != "$undef" ]; then
      echo true 
   else
      echo false
   fi
   unset name
   unset command
   unset isdef
}

jsv_get_env() 
{
   name="jsv_env_$1"
   command=`eval "echo \$\{$name\:\-$undef\}"`
   isdef=`eval "echo $command"`
   if [ "$isdef" != "$undef" ]; then
      eval "echo \${$name}" 
   fi
   unset name
   unset command
   unset isdef
}

jsv_add_env() 
{
   name="jsv_env_$1"
   eval "$name=$2"
   jsv_send_command ENV ADD "$1" "$2"
   unset name
}

jsv_mod_env() 
{
   name="jsv_env_$1"
   eval "$name=$2"
   jsv_send_command ENV MOD "$1" "$2"
   unset name
}

jsv_del_env() 
{
   name="jsv_env_$1"
   command=`eval "echo \$\{$name\:\-$undef\}"`
   isdef=`eval "echo $command"`
   if [ "$isdef" != "$undef" ]; then
      eval "unset $name" 
      jsv_send_command ENV DEL "$1" 
   fi
   unset name
   unset command
   unset isdef
}

jsv_is_param() 
{
   name="jsv_param_$1"
   command=`eval "echo \$\{$name\:\-$undef\}"`
   isdef=`eval "echo $command"`
   if [ "$isdef" != "$undef" ]; then
      echo true 
   else
      echo false
   fi
   unset name
   unset command
   unset isdef
}

jsv_get_param() 
{
   name="jsv_param_$1"
   command=`eval "echo \$\{$name\:\-$undef\}"`
   isdef=`eval "echo $command"`
   if [ "$isdef" != "$undef" ]; then
      eval "echo \${$name}" 
   fi
   unset name
   unset command
   unset isdef
}

jsv_set_param()
{
   name="jsv_param_$1"
   eval "$name=$2"
   jsv_send_command PARAM "$1" "$2"
   unset name
}

jsv_del_param() 
{
   name="jsv_param_$1"
   value="$2"
   command=`eval "echo \$\{$name\:\-$undef\}"`
   isdef=`eval "echo $command"`
   if [ "$isdef" != "$undef" ]; then
      eval "unset \${$name}" 
      jsv_send_command PARAM "$1" ""
   fi
   unset name
   unset command
   unset isdef
}

jsv_sub_is_param() {
   ret="false"
   name="jsv_param_$1"
   sub_name="$2" 
   command=`eval "echo \$\{$name\:\-$undef\}"`
   list=`eval "echo $command"`
   if [ "$list" != "$undef" ]; then
      IFS=","
      for i in $list; do
         IFS="="
         for j in $i; do
            IFS="$saved_ifs"
            if [ "$j" = "$sub_name" ]; then
               ret="true"
               break
            else
               break
            fi
            IFS="="
         done
	 IFS="$saved_ifs"
         if [ "$ret" = "true" ]; then
            break
         fi
         IFS=","
      done
      IFS="$saved_ifs"
   fi
   unset name
   unset sub_name
   unset command
   unset list
   echo $ret
   return
}

jsv_sub_del_param() 
{
   param_name="$1"
   name="jsv_param_$param_name"
   sub_name="$2" 
   command=`eval "echo \$\{$name\:\-$undef\}"`
   list=`eval "echo $command"`
   new_param=""
   if [ "$list" != "$undef" ]; then

      # split token between ',' character
      IFS=","
      for i in $list; do
         found="false"

         # split the first string before '=' character
         # This is the variable name and if it is the
         # one which should be deleted then set "found" 
         # to "true" 
         IFS="="
         for j in $i; do
            IFS="$saved_ifs"
            if [ "$j" = "$sub_name" ]; then
               found="true"
               break
            else
               break
            fi
            IFS="="
         done
         IFS="$saved_ifs"
         # append all entries to new_param
         # skip only the entry which should be deleted
         if [ "$found" != "true" ]; then
            if [ "$new_param" = "" ]; then  
               new_param="$i"
            else
               new_param="${new_param},$i"
            fi
         fi
         IFS=","
      done
      IFS="$saved_ifs"

      # set local variable and send modification to client/master
      eval "$name=\"\$new_param\""
      jsv_send_command PARAM $param_name "$new_param" 
   fi
   unset new_param
   unset list
   unset command
   unset name
   unset sub_name
   unset param_name
   return
}

jsv_sub_get_param() 
{
   param_name="$1"
   name="jsv_param_$param_name"
   sub_name="$2" 
   command=`eval "echo \$\{$name\:\-$undef\}"`
   list=`eval "echo $command"`
   if [ "$list" != "$undef" ]; then

      # split token between ',' character
      IFS=","
      for i in $list; do
         found="false"

         # split the first string before '=' character
         # This is the variable name and if it is the
         # one which should be deleted then set "found" 
         # to "true" 
         IFS="="
         for j in $i; do
            IFS="$saved_ifs"
            if [ "$found" = "true" ]; then
               echo "$j"
            else
               if [ "$j" = "$sub_name" ]; then
                  found="true"
               fi
            fi
            IFS="="
         done
         IFS="$saved_ifs"
         if [ "$found" = "true" ]; then
            break;
         fi
         IFS=","
      done
      IFS="$saved_ifs"
   fi
   unset list
   unset command
   unset name
   unset sub_name
   unset param_name
   return
}

jsv_sub_add_param() 
{
   param_name="$1"
   name="jsv_param_$param_name"
   sub_name="$2" 
   value="$3"
   command=`eval "echo \$\{$name\:\-$undef\}"`
   list=`eval "echo $command"`
   new_param=""
   if [ "$list" = "$undef" ]; then
      list=""
   fi
   found="false"

   # split token between ',' character
   IFS=","
   for i in $list; do
      # split the first string before '=' character
      # This is the variable name and if it is the
      # one which should be deleted then set "found" 
      # to "true" 
      IFS="="
      for j in $i; do
         IFS="$saved_ifs"
         if [ "$j" = "$sub_name" ]; then
            found="true"
            if [ "$value" = "" ]; then
               if [ "$new_param" = "" ]; then
                  new_param="$j"
               else
                  new_param="${new_param},$j"
               fi
            else
               if [ "$new_param" = "" ]; then  
                  new_param="${j}=${value}"
               else
                  new_param="${new_param},${j}=${value}"
               fi
            fi
         else
            if [ "$new_param" = "" ]; then  
               new_param="$i"
            else
               new_param="${new_param},$i"
            fi
         fi
         IFS="="
         break;
      done
      IFS="$saved_ifs"
   done
   if [ "$found" = "false" ]; then
      if [ "$value" = "" ]; then
         if [ "$new_param" = "" ]; then  
            new_param="$sub_name"
         else
            new_param="${new_param},$sub_name"
         fi
      else 
         if [ "$new_param" = "" ]; then  
            new_param="${sub_name}=$value"
         else
            new_param="${new_param},${sub_name}=$value"
         fi
      fi
   fi
   IFS="$saved_ifs"

   # set local variable and send modification to client/master
   eval "$name=\"\$new_param\""
   jsv_send_command PARAM "$param_name" "$new_param"
   unset new_param
   unset list
   unset command
   unset name
   unset sub_name
   unset param_name
   return
}

# executed whenever the JSV can be done
jsv_handle_start_command()
{
   if [ "$state" = "initialized" ]; then
      jsv_on_start
      jsv_send_command "STARTED"
      state="started"
   else
      jsv_send_command "ERROR JSV script got START command but is in state $state"
   fi
}

jsv_handle_begin_command()
{
   if [ "$state" = "started" ]; then
      state="verifying"
      jsv_on_verify
      jsv_clear_params
      jsv_clear_envs
   else
      jsv_send_command "ERROR JSV script got BEGIN command but is in state $state"
   fi
}

jsv_handle_param_command()
{
   if [ "$state" = "started" ]; then
      param="$1"
      value="$2"

      eval "jsv_param_$param=\"\${value}\""
      unset param
      unset value
   else
      jsv_send_command "ERROR JSV script got PARAM command but is in state $state"
   fi
}

jsv_handle_env_command()
{
   if [ "$state" = "started" ]; then
      action="$1"
      name="$2"
      data="$3"
      if [ "$action" = "ADD" ]; then
         jsv_all_envs="$jsv_all_envs $name"
# TODO: EB: escape characeters are not handled correctly at least for linux amd64
#         eval "jsv_env_${name}=\"\`$SGE_ROOT/utilbin/$ARCH/echo_raw -e \${data}\`\""
         eval "jsv_env_${name}=\"\${data}\""
      fi
      unset action
      unset name
      unset data
   else
      jsv_send_command "ERROR JSV script got ENV command but is in state $state"
   fi
}

jsv_send_command()
{
   echo "$@"

   jsv_script_log "<<< $@"
}

jsv_send_env()
{
   jsv_send_command "SEND ENV"
}

jsv_accept()
{
   if [ "$state" = "verifying" ]; then
      jsv_send_command "RESULT STATE ACCEPT $*"
      state="initialized"
   else
      jsv_send_command "ERROR JSV script will send RESULT command but is in state $state"
   fi
}

jsv_correct()
{
   if [ "$state" = "verifying" ]; then
      jsv_send_command "RESULT STATE CORRECT $*"
      state="initialized"
   else
      jsv_send_command "ERROR JSV script will send RESULT command but is in state $state"
   fi
}

jsv_reject()
{
   if [ "$state" = "verifying" ]; then
      jsv_send_command "RESULT STATE REJECT $*"
      state="initialized"
   else
      jsv_send_command "ERROR JSV script will send RESULT command but is in state $state"
   fi
}

jsv_reject_wait()
{
   if [ "$state" = "verifying" ]; then
      jsv_send_command "RESULT STATE REJECT_WAIT $*"
      state="initialized"
   else
      jsv_send_command "ERROR JSV script will send RESULT command but is in state $state"
   fi
}

jsv_log_info() 
{
   jsv_send_command "LOG INFO $*"
}

jsv_log_warning() 
{
   jsv_send_command "LOG WARNING $*"
}

jsv_log_error() 
{
   jsv_send_command "LOG ERROR $*"
}

jsv_script_log()
{
   if [ $logging_enabled = true ]; then
      echo "$@" >>$logfile
   fi
}

jsv_main()
{
   jsv_script_log "$0 started on `date`"
   jsv_script_log ""
   jsv_script_log "This file contains logging output from a GE JSV script. Lines beginning"
   jsv_script_log "with >>> contain the data which was send by a command line client or"
   jsv_script_log "sge_qmaster to the JSV script. Lines beginning with <<< contain data"
   jsv_script_log "which is send for this JSV script to the client or sge_qmaster"
   jsv_script_log ""

   while [ $quit = false ]; do
      # simple read can't be used here because it does backslash escaping
      # read -r (raw mode) is not available on all platforms
      # therefore we have to use our own implementation 
      if [ "$ARCH" = "darwin-x86" ]; then
         read -r input
      else
         input=`$SGE_ROOT/utilbin/$ARCH/read_raw`
      fi 
      result=$?

      if [ $result != 0 ]; then
         quit=true
      else
         if [ "$input" != "" ]; then
            jsv_script_log ">>> $input"     
            first=`$SGE_ROOT/utilbin/$ARCH/echo_raw $input | cut -d' ' -f 1`
            second=`$SGE_ROOT/utilbin/$ARCH/echo_raw $input | cut -d' ' -f 2`
            remaining=`$SGE_ROOT/utilbin/$ARCH/echo_raw $input | cut -d' ' -f 3-`
            if [ "$first" = "QUIT" ]; then
               quit=true
            elif [ "$first" = "PARAM" ]; then
               jsv_handle_param_command "$second" "$remaining"
            elif [ "$first" = "ENV" ]; then
               third=`$SGE_ROOT/utilbin/$ARCH/echo_raw $input | cut -d' ' -f 3`
               len=`$SGE_ROOT/utilbin/$ARCH/echo_raw "$first $second $third " | wc -c`
               data=`$SGE_ROOT/utilbin/$ARCH/echo_raw $input | cut -c ${len}-`
               jsv_handle_env_command "$second" "$third" "$data"
            elif [ "$first" = "START" ]; then
               jsv_handle_start_command 
            elif [ "$first" = "BEGIN" ]; then
               jsv_handle_begin_command 
            elif [ "$first" = "SHOW" ]; then
               jsv_show_params
               jsv_show_envs
            else
               jsv_send_command "ERROR JSV script got unknown command \"$first\""
            fi
         fi
      fi
      unset result
   done
   jsv_script_log "$0 is terminating on `date`"
}

