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


#****** gettext_procedures/sge_macro() *****************************************
#  NAME
#     sge_macro() -- return sge macro string
#
#  SYNOPSIS
#     sge_macro { macro_name } 
#
#  FUNCTION
#     This procedure returns the string defined by the macro. 
#
#  INPUTS
#     macro_name - sge source code macro
#
#  RESULT
#     string
#
#  EXAMPLE
#     set string [sge_macro MSG_OBJ_SHARETREE] 
#
#  SEE ALSO
#     ???/???
#*******************************************************************************
proc sge_macro { macro_name } {
   global CHECK_OUTPUT
 
   set value ""
   switch -exact $macro_name {
      "MSG_OBJ_QUEUE" -
      "MSG_SGETEXT_ALREADYEXISTS_SS" -
      "MSG_QCONF_NOXDEFINED_S" -
      "MSG_QUEUE_XISNOTAQUEUENAME_S" -
      "MSG_SGETEXT_MODIFIEDINLIST_SSSS" -
      "MSG_SGETEXT_ADDEDTOLIST_SSSS" -
      "MSG_SGETEXT_REMOVEDFROMLIST_SSSS" -
      "MSG_SGETEXT_UNKNOWNQUEUE_SSSS"  -
      "MSG_EXEC_HOSTENTRYOFXCHANGEDINEXECLIST_S" -
      "MSG_SGETEXT_CONFIG_MODIFIEDINLIST_SSS" -
      "MSG_SGETEXT_CONFIG_ADDEDTOLIST_SSS" -
      "MSG_PARSE_EDITFAILED" -
      "MSG_MULTIPLY_MODIFIEDIN"   -
      "MSG_MULTIPLY_ADDEDTO"   -
      "MSG_SCHEDD_CHANGEDSCHEDULERCONFIGURATION"  -
      "MSG_EXEC_ADDEDHOSTXTOEXECHOSTLIST_S"  -
      "MSG_SGETEXT_UNKNOWNUSERSET_SSSS" -
      "MSG_SCHEDD_INFO_NOMESSAGE" -
      "MSG_GDI_ADDTOACL_SS" -
      "MSG_QUEUE_DISABLEQ_SSS" -
      "MSG_QUEUE_ENABLEQ_SSS"  -
      "MSG_PROJECT" -
      "MSG_SGETEXT_DOESNOTEXIST_S" -
      "MSG_OBJ_SHARETREE" -
      "MSG_SGETEXT_REMOVEDLIST_SSS" -
      "MSG_JOB_SUBMITJOB_USS" -
      "MSG_QSUB_YOURQSUBREQUESTCOULDNOTBESCHEDULEDDTRYLATER" -
      "MSG_QSUB_YOURIMMEDIATEJOBXHASBEENSUCCESSFULLYSCHEDULED_U" -
      "MSG_SGETEXT_MOD_JOBS_SU" -
      "MSG_GDI_USAGE_USAGESTRING" -
      "MSG_JOB_SUBMITJOBARRAY_UUUUSS" -
      "MSG_JOB_MOD_JOBNETPREDECESSAMBIGUOUS_SUU" -
      "MSG_ANSWER_UNKOWNOPTIONX_S"  -
      "MSG_SGETEXT_NO_ACCESS2PRJ4USER_SS" -
      "MSG_STREE_USERTNOACCESS2PRJ_SS"  -
      "MSG_JOB_NOPERMS_SS" -
      "MSG_JOB_PRJNOSUBMITPERMS_S" -
      "MSG_SGETEXT_RESOURCE_NOT_REQUESTABLE_S" -
      "MSG_SGETEXT_CANTRESOLVEHOST_S" -
      "MSG_SGETEXT_UNKNOWN_RESOURCE_S" -
      "MSG_SCHEDD_JOBREQUESTSUNKOWNRESOURCE" -
      "MSG_JOB_MORETASKSTHAN_U" -
      "MSG_JOB_REGDELTASK_SUU" -
      "MSG_JOB_REGDELJOB_SU" -
      "MSG_JOB_DELETETASK_SUU" -
      "MSG_JOB_DELETEJOB_SU" -
      "MSG_QUEUE_SUSPENDQ_SSS" -
      "MSG_QUEUE_UNSUSPENDQ_SSS" -
      "MSG_JOB_SUSPENDTASK_SUU" -
      "MSG_JOB_SUSPENDJOB_SU" -
      "MSG_JOB_UNSUSPENDTASK_SUU" -
      "MSG_JOB_UNSUSPENDJOB_SU" -
      "MSG_SGETEXT_DOESNOTEXIST_SU" -
      "MSG_SGETEXT_DOESNOTEXISTTASK_UU" -
      "MSG_SGETEXT_DOESNOTEXISTTASKRANGE_UUUU" -
      "MSG_QSH_INTERACTIVEJOBHASBEENSCHEDULED_D" -
      "MSG_GDI_USING_SS" -
      "MSG_QSH_REQUESTCANTBESCHEDULEDTRYLATER_S" -
      "MSG_SGETEXT_NOSUBMITORADMINHOST_S" -
      "MSG_SCHEDD_SCHEDULINGINFO" -
      "MSG_SCHEDD_INFO_JOBHOLD_" -
      "MSG_SGETEXT_MOD_JATASK_SUU" -
      "MSG_SCHEDD_INFO_JOBINERROR_" -
      "MSG_SCHEDD_INFO_EXECTIME_" -
      "MSG_SCHEDD_INFO_JOBDEPEND_" -
      "MSG_SCHEDD_INFO_MAX_AJ_INSTANCES_" -
      "MSG_OBJ_USER"  -
      "MSG_OBJ_USERSET" -
      "MSG_SCHEDD_INFO_HASNOPERMISSION_SS" -
      "MSG_SCHEDD_INFO_NOACCESSTOPE" -
      "MSG_SCHEDD_INFO_NOACCESSTOPE_S" -
      "MSG_JOB_NOTINANYQ_S" -
      "MSG_PARSE_XOPTIONALREADYSETOVERWRITINGSETING_S" -
      "MSG_TREE_CHANGEDSHARETREE" -
      "MSG_QCONF_CANTREADSHARETREEX_S" -
      "MSG_JOB_PRIOSET_SSUI" { set value "" }

      "DISTINST_HIT_RETURN_TO_CONTINUE" { set value "\nHit <RETURN> to continue >> " } 
      "DISTINST_NOT_COMPILED_IN_SECURE_MODE" { set value "\n>sge_qmaster< binary is not compiled with >-secure< option!\n" }
      "DISTINST_ENTER_HOSTS" { set value "Host(s): " }
      "DISTINST_MASTER_INSTALLATION_COMPLETE" { set value "\nYour Grid Engine qmaster installation is now completed" }
      "DISTINST_ENTER_A_RANGE" { set value "Please enter a range >> " }
      "DISTINST_PREVIOUS_SCREEN" { set value "Do you want to see previous screen about using Grid Engine again (y/n) \[n\] >> " }
      "DISTINST_FILE_FOR_HOSTLIST" { set value "Do you want to use a file which contains the list of hosts (y/n) \[n\] >> " }
      "DISTINST_FINISHED_ADDING_HOSTS" { set value "Finished adding hosts. Hit <RETURN> to continue >> " }
      "DISTINST_FILENAME_FOR_HOSTLIST" { set value "\nPlease enter the file name which contains the host list: " }
      "DISTINST_CREATE_NEW_CONFIGURATION" { set value "Do you want to create a new configuration (y/n) \[y\] >> " }
      "DISTINST_INSTALL_SCRIPT" { set value "\nWe can install the startup script that\nGrid Engine is started at machine boot (y/n) \[y\] >> " }
      "DISTINST_ANSWER_YES" { set value "y" }
      "DISTINST_ANSWER_NO" { set value "n" }
      "DISTINST_ENTER_DEFAULT_DOMAIN" { set value "\nPlease enter your default domain >> " }
      "DISTINST_CONFIGURE_DEFAULT_DOMAIN" { set value "Do you want to configure a default domain (y/n) \[y\] >> " }
      "DISTINST_PKGADD_QUESTION" { set value "Did you install this version with >pkgadd< or did you already\nverify and set the file permissions of your distribution (y/n) \[y\] >> " }
      "DISTINST_MESSAGES_LOGGING" { set value "Hit <RETURN> to see where Grid Engine logs messages >> " }
      "DISTINST_OTHER_SPOOL_DIR" { set value "Do you want to select another qmaster spool directory (y/n) \[n\] >> " }
      "DISTINST_OTHER_USER_ID_THAN_ROOT" { set value "Do you want to install Grid Engine\nunder an user id other than >root< (y/n) \[y\] >> " }
      "DISTINST_INSTALL_AS_ADMIN_USER" { set value "Do you want to install Grid Engine as admin user >%s< (y/n) \[y\] >> " }
      "DISTINST_ADMIN_USER_ACCOUNT" { set value "      admin user account = %s" }
      "DISTINST_USE_CONFIGURATION_PARAMS" { set value "\nDo you want to use these configuration parameters (y/n) \[y\] >> " }
      "DISTINST_INSTALL_GE_NOT_AS_ROOT" { set value "Do you want to install Grid Engine\nunder an user id other than >root< (y/n) \[y\] >> " }
      "DISTINST_IF_NOT_OK_STOP_INSTALLATION" { set value "Hit <RETURN> if this is ok or stop the installation with Ctrl-C >> " }
      "DISTINST_DNS_DOMAIN_QUESTION" { set value "Are all hosts of your cluster in a single DNS domain (y/n) \[y\] >> " }
      "DISTINST_ENTER_SPOOL_DIR_OR_HIT_RET" { set value "If you will install shadow master hosts or if you want to be able to start\n the qmaster daemon on other hosts (see the corresponding sectionin the\n Grid Engine Installation and Administration Manual for details) the account\n on the shadow master hosts also needs read/write access to this directory.\n\n Enter spool directory or hit <RETURN> to use default\n \[%s\] >> " }
      "DISTINST_USING_GID_RANGE_HIT_RETURN" { set value "\nUsing >%s< as gid range. Hit <RETURN> to continue >> " }
      "DISTINST_EXECD_INSTALL_COMPLETE" { set value "Your execution daemon installation is now completed." }
      "DISTINST_LOCAL_CONFIG_FOR_HOST" { set value "Local configuration for host >%s< created." }
      "DISTINST_CELL_NAME_FOR_QMASTER" { set value "\nPlease enter cell name which you used for the qmaster\ninstallation or press <RETURN> to use default cell >default< >> " }
      "DISTINST_ADD_DEFAULT_QUEUE" { set value "Do you want to add a default queue for this host (y/n) \[y\] >> " }


   }
   if { $value == "" } {
      set value [get_macro_string_from_name $macro_name]
#      puts $CHECK_OUTPUT "value for $macro_name is \n\"$value\""
   }
   if { $value == -1 } {
      set macro_messages_file [get_macro_messages_file_name]
      add_proc_error "sge_macro" -3 "could not find macro \"$macro_name\" in source code!!!\ndeleting macro messages file:\n$macro_messages_file"
      if { [ file isfile $macro_messages_file] } {
         file delete $macro_messages_file
      }
      update_macro_messages_list
   }

   return $value
}

