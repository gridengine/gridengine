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
#     sge_macro { macro_name {raise_error 1} } 
#
#  FUNCTION
#     This procedure returns the string defined by the macro. 
#
#  INPUTS
#     macro_name  - sge source code macro
#     raise_error - if macro is not found, shall an error be raised and 
#                   reparsing of messages file be triggered?
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
proc sge_macro { macro_name {raise_error 1} } {
   global CHECK_OUTPUT
 
   set value ""

   # special handling for install macros
   switch -exact $macro_name {
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
      "DISTINST_CELL_NAME_FOR_QMASTER" { set value "\nPlease enter cell name which you used for the qmaster\ninstallation or press <RETURN> to use default cell >%s< >> " }
      "DISTINST_ADD_DEFAULT_QUEUE" { set value "Do you want to add a default queue for this host (y/n) \[y\] >> " }
   }

   # if it was no install macro, try to find it from messages files
   if { $value == "" } {
      set value [get_macro_string_from_name $macro_name]
#      puts $CHECK_OUTPUT "value for $macro_name is \n\"$value\""
   }

   # macro nowhere found
   if {$raise_error} {
      if { $value == -1 } {
         set macro_messages_file [get_macro_messages_file_name]
         add_proc_error "sge_macro" -3 "could not find macro \"$macro_name\" in source code!!!\ndeleting macro messages file:\n$macro_messages_file"
         if { [ file isfile $macro_messages_file] } {
            file delete $macro_messages_file
         }
         update_macro_messages_list
      }
   }

   return $value
}

