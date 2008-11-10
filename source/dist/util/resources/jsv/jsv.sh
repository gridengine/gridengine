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

jsv_on_start()
{
   # trigger client/master to send also the job environment
   # 

   # EB: TODO: add security note
   jsv_send_env
   return
}

jsv_on_verify()
{
   # Logging into the logfile of this script can be done with:
   #
   #     jsv_script_log "script logging"

   # log all received parameters and environment variables into the the master
   # message file or print them to stdout of the client with:
   #
   #     jsv_show_params
   #     jsv_show_envs

   # logging into the master message file or into stdout of the submit client:
   #
   #     jsv_log_info "This is a JSV info message"
   #     jsv_log_warning "This is a JSV warning message"
   #     jsv_log_error " This is a JSV error message"

   # Any time a job can be accepted, rejected with one of these commands
   #
   #     jsv_accept
   #     jsv_correct
   #     jsv_reject
   #     jsv_reject_wait


   # EXAMPLE:
   #
   # Following section demonstrates how to access job paramaters, how
   # it is possible to modify job parameters and environment variables.
   # 
   #     - all binary jobs will be rejected
   #     - pe jobs should use a multiple of 16 slots otherwise they 
   #       are rejected
   #     - "h_vmem" is not allowed in hard resource requests.
   #       It will be removed but later on the job will be rejected with
   #       reject_wait.
   #     - "h_data" is not allowed in hard resource requests.
   #       It will be removed but job will be accepted with the correction. 
   #     - add/modify a=1 to the job context of each job
   #     - print the result to stdout of the client but not into the
   #       message file of the master if this script is a master side 
   #       script
   do_correct="false" 
   do_wait="false"
   if [ "`jsv_get_param b`" = "y" ]; then
      jsv_reject "Binary job is rejected."
      return
   fi 

   if [ "`jsv_get_param pe_name`" != "" ]; then
      slots=`jsv_get_param pe_slots`
      i=`echo "$slots % 16" | bc`

      if [ $i -gt 0 ]; then
         jsv_reject "Parallel job does not request a multiple of 16 slots"
      fi
   fi

   l_hard=`jsv_get_param l_hard`
   if [ "$l_hard" != "" ]; then
      context=`jsv_get_param CONTEXT` 
      has_h_vmem=`jsv_sub_is_param l_hard h_vmem`
      has_h_data=`jsv_sub_is_param l_hard h_data`

   
      if [ "$has_h_vmem" = "true" ]; then
         jsv_sub_del_param l_hard h_vmem
         do_wait="true"
         if [ "$context" = "client" ]; then
            jsv_log_info "h_vmem as hard resource requirement has been deleted"
         fi 
      fi
      if [ "$has_h_data" = "true" ]; then
         jsv_sub_del_param l_hard h_data
         do_correct="true"
         if [ "$context" = "client" ]; then
            jsv_log_info "h_data as hard resource requirement has been deleted"
         fi 
      fi
   fi

   c=`jsv_get_param c`
   if [ "$c" != "" ]; then
      context=`jsv_get_param CONTEXT`
      has_c_a=`jsv_sub_is_param c a`
      has_c_b=`jsv_sub_is_param c b`

      if [ "$has_c_a" = "true" ]; then
         c_a_value=`jsv_sub_get_param c a`
         new_value=`echo "$c_a_value + 1" | bc`

         jsv_sub_add_param c a $new_value
      else
         jsv_sub_add_param c a 1
      fi
      if [ "$has_c_b" = "true" ]; then
         jsv_sub_del_param c b 
      fi
      jsv_sub_add_param c c
   fi

   if [ "$do_wait" = "true" ]; then
      jsv_reject_wait "Job is rejected. It might be submitted later."
   elif [ "$do_correct" = "true" ]; then
      jsv_correct "Job was modified before it was accepted"
   else
      jsv_accept "Job is accepted"
   fi
}

. ${SGE_ROOT}/util/resources/jsv/jsv_include.sh

jsv_main

