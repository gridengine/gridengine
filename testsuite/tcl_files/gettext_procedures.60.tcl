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
      "DISTINST_HIT_RETURN_TO_CONTINUE" { set value "\nHit <RETURN> to continue >> " } 
      "DISTINST_HOSTNAME_KNOWN_AT_MASTER" { set value "\nThis hostname is known at qmaster as an administrative host.\n\nHit <RETURN> to continue >>" }
      "DISTINST_NOT_COMPILED_IN_SECURE_MODE" { set value "\n>sge_qmaster< binary is not compiled with >-secure< option!\n" }
      "DISTINST_ENTER_HOSTS" { set value "Host(s): " }
      "DISTINST_VERIFY_FILE_PERMISSIONS" { set value "\nWe may now verify and set the file permissions of your Grid Engine\ndistribution.\n\nThis may be useful since due to unpacking and copying of your distribution\nyour files may be unaccessible to other users.\n\nWe will set the permissions of directories and binaries to\n\n   755 - that means executable are accessible for the world\n\nand for ordinary files to\n\n   644 - that means readable for the world\n\nDo you want to verify and set your file permissions (y/n) \[y\] >> " }
      "DISTINST_MASTER_INSTALLATION_COMPLETE" { set value "\nYour Grid Engine qmaster installation is now completed" }
      "DISTINST_ENTER_A_RANGE" { set value "Please enter a range >> " }
      "DISTINST_PREVIOUS_SCREEN" { set value "Do you want to see previous screen about using Grid Engine again (y/n) \[n\] >> " }
      "DISTINST_FILE_FOR_HOSTLIST" { set value "Do you want to use a file which contains the list of hosts (y/n) \[n\] >> " }
      "DISTINST_FINISHED_ADDING_HOSTS" { set value "Finished adding hosts. Hit <RETURN> to continue >> " }
      "DISTINST_FILENAME_FOR_HOSTLIST" { set value "\nPlease enter the file name which contains the host list: " }
      "DISTINST_CREATE_NEW_CONFIGURATION" { set value "Do you want to create a new configuration (y/n) \[y\] >> " }
      "DISTINST_INSTALL_SCRIPT" { set value "\nWe can install the startup script that will\nstart %s at machine boot (y/n) \[y\] >> " }
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
      "DISTINST_CHOOSE_SPOOLING_METHOD" { set value "Your SGE binaries are compiled to link the spooling libraries\nduring runtime (dynamically). So you can choose between Berkeley DB \nspooling and Classic spooling method.\nPlease choose a spooling method (berkeleydb|classic) \[berkeleydb\] >> " }
      "DISTINST_ENTER_SPOOL_DIR_OR_HIT_RET" { set value "If you will install shadow master hosts or if you want to be able to start\n the qmaster daemon on other hosts (see the corresponding sectionin the\n Grid Engine Installation and Administration Manual for details) the account\n on the shadow master hosts also needs read/write access to this directory.\n\n Enter spool directory or hit <RETURN> to use default\n \[%s\] >> " }
      "DISTINST_USING_GID_RANGE_HIT_RETURN" { set value "\nUsing >%s< as gid range. Hit <RETURN> to continue >> " }
      "DISTINST_EXECD_INSTALL_COMPLETE" { set value "Your execution daemon installation is now completed." }
      "DISTINST_LOCAL_CONFIG_FOR_HOST" { set value "Local configuration for host >%s< created." }
      "DISTINST_CELL_NAME_FOR_QMASTER" { set value "\nGrid Engine supports multiple cells.\n\nIf you are not planning to run multiple Grid Engine clusters or if you don't\nknow yet what is a Grid Engine cell it is safe to keep the default cell name\n\n   default\n\nIf you want to install multiple cells you can enter a cell name now.\n\nThe environment variable\n\n   \\\$SGE_CELL=<your_cell_name>\n\nwill be set for all further Grid Engine commands.\n\nEnter cell name \[default\] >> " }
      "DISTINST_CELL_NAME_FOR_EXECD" { set value "\nPlease enter cell name which you used for the qmaster\ninstallation or press <RETURN> to use default cell >default< >> " }
      "DISTINST_CELL_NAME_EXISTS" { set value "Do you want to select another cell name? (y/n) \[y\] >> " }
      "DISTINST_CELL_NAME_OVERWRITE" { set value "Do you want to overwrite \[y\] or delete \[n\] the directory? (y/n) \[y\] >> " }
      "DISTINST_ADD_DEFAULT_QUEUE" { set value "Do you want to add a default queue for this host (y/n) \[y\] >> " }
      "DISTINST_ALL_QUEUE_HOSTGROUP" { set value "Creating the default <all.q> queue and <allhosts> hostgroup" }
      "DISTINST_ADD_DEFAULT_QUEUE_INSTANCE" { set value "Do you want to add a default queue instance for this host (y/n) \[y\] >> " }

      "DISTINST_ENTER_DATABASE_SERVER" { set value "Enter database server (none for local spooling)\nor hit <RETURN> to use default \[%s\] >> " }
      "DISTINST_ENTER_DATABASE_SERVER_LOCAL_SPOOLING" { set value "Please enter the name of your Berkeley DB Spooling Server!\nFor local spooling without Server, hit <RETURN> else enter the Servername! >>" }
      "DISTINST_ENTER_DATABASE_DIRECTORY" { set value "Enter the database directory\nor hit <RETURN> to use default \[%s\] >> " }
      "DISTINST_ENTER_DATABASE_DIRECTORY" { set value "Please enter the Database Directory now, even if you want to spool locally\nit is necessary to enter this Database Directory. \n\nDefault: \[] >> " }
      "DISTINST_ENTER_DATABASE_DIRECTORY_LOCAL_SPOOLING" { set value "\nPlease enter the Database Directory now, even if you want to spool locally\n it is necessary to enter this Database Directory. \n\nDefault: \[%s\] >> " }
      "DISTINST_DATABASE_DIR_NOT_ON_LOCAL_FS" { set value "The database directory >%s<\nis not on a local filesystem.\nPlease choose a local filesystem or configure the RPC Client/Server mechanism" }
      "DISTINST_STARTUP_RPC_SERVER" { set value "Please startup the rc script >%s< on the RPC server machine" }
      "DISTINST_DONT_KNOW_HOW_TO_TEST_FOR_LOCAL_FS" { set value "Don't know how to test for local filesystem. Exit." }
      "DISTINST_CURRENT_GRID_ROOT_DIRECTORY" { set value "The Grid Engine root directory is:\n\n   \\\$SGE_ROOT = %s\n\nIf this directory is not correct (e.g. it may contain an automounter\nprefix) enter the correct path to this directory or hit <RETURN>\nto use default \[%s\] >> " }
      "DISTINST_DATABASE_LOCAL_SPOOLING" { set value "Do you want to use a Berkeley DB Spooling Server? (y/n) \[n\] >> " }
      "DISTINST_EXECD_SPOOLING_DIR_NOROOT_NOADMINUSER" { set value "\nPlease give the basic configuration parameters of your Grid Engine\ninstallation:\n\n   <execd_spool_dir>\n\nThe pathname of the spool directory of the execution hosts. You\nmust have the right to create this directory and to write into it.\n" }
      "DISTINST_EXECD_SPOOLING_DIR_NOROOT" { set value "\nPlease give the basic configuration parameters of your Grid Engine\ninstallation:\n\n   <execd_spool_dir>\n\nThe pathname of the spool directory of the execution hosts. User >%s<\nmust have the right to create this directory and to write into it.\n" }
      "DISTINST_EXECD_SPOOLING_DIR_DEFAULT" { set value "Default: \[%s\] >> " }
      "DISTINST_ENTER_ADMIN_MAIL" { set value "\n<administrator_mail>\n\nThe email address of the administrator to whom problem reports are sent.\n\nIt's is recommended to configure this parameter. You may use >none<\nif you do not wish to receive administrator mail.\n\nPlease enter an email address in the form >user@foo.com<.\n\nDefault: \[none\] >> " }
      "DISTINST_SHOW_CONFIGURATION" { set value "\nThe following parameters for the cluster configuration were configured:\n\n   execd_spool_dir        %s\n   administrator_mail     %s\n" }
      "DISTINST_ACCEPT_CONFIGURATION" { set value "Do you want to change the configuration parameters (y/n) \[n\] >> " }
      "DISTINST_INSTALL_STARTUP_SCRIPT" { set value "\nWe can install the startup script that\nGrid Engine is started at machine boot (y/n) \[n\] >> " }
      "DISTINST_CHECK_ADMINUSER_ACCOUNT" { set value "\nThe current directory\n\n   %s\n\nis owned by user\n\n   %s\n\nIf user >root< does not have write permissions in this directory on *all*\nof the machines where Grid Engine will be installed (NFS partitions not\nexported for user >root< with read/write permissions) it is recommended to\ninstall Grid Engine that all spool files will be created under the user id\nof user >%s<.\n\nIMPORTANT NOTE: The daemons still have to be started by user >root<. \n" }
      "DISTINST_CHECK_ADMINUSER_ACCOUNT_ANSWER" { set value "Do you want to install Grid Engine as admin user" }
      "DISTINST_ENTER_LOCAL_EXECD_SPOOL_DIR" { set value "During the qmaster installation you've already entered a global\nexecd spool directory. This is used, if no local spool directory is configured.\n\n Now you can enter a local spool directory for this host.\n" }
      "DISTINST_ENTER_LOCAL_EXECD_SPOOL_DIR_ASK" { set value "Do you want to configure a local spool directory\n for this host (y/n) \[n\] >> " }
      "DISTINST_ENTER_LOCAL_EXECD_SPOOL_DIR_ENTER" { set value "Please enter the local spool directory now! >> " }
      "DISTINST_ENTER_SCHEDLUER_SETUP" { set value "Enter the number of your prefered configuration and hit <RETURN>! \nDefault configuration is \[1\] >> " }
      "DISTINST_DELETE_DB_SPOOL_DIR" { set value "The spooling directory already exists! Do you want to delete it? \[n\] >> " }


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

