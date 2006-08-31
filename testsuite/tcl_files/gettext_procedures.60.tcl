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
      "DISTINST_HIT_RETURN_TO_CONTINUE" { set value "\nHit <RETURN> to continue >>" } 
      "DISTINST_HOSTNAME_KNOWN_AT_MASTER" { set value "\nThis hostname is known at qmaster as an administrative host.\n\nHit <RETURN> to continue >>" }
      "DISTINST_CHECK_AGAIN" { set value "Check again (y/n) ('n' will abort) \[y\] >> " }
      "DISTINST_NOT_COMPILED_IN_SECURE_MODE" { set value "\n>sge_qmaster< binary is not compiled with >-secure< option!\n" }
      "DISTINST_ENTER_HOSTS" { set value "Host(s): " }
      "DISTINST_VERIFY_FILE_PERMISSIONS1" { set value "\nWe may now verify and set the file permissions of your Grid Engine\ndistribution.\n\nThis may be useful since due to unpacking and copying of your distribution\nyour files may be unaccessible to other users.\n\nWe will set the permissions of directories and binaries to\n\n   755 - that means executable are accessible for the world\n\nand for ordinary files to\n\n   644 - that means readable for the world\n\nDo you want to verify and set your file permissions (y/n) \[y\] >> " }
      "DISTINST_VERIFY_FILE_PERMISSIONS2" { set value "\nDid you install this version with >pkgadd< or did you already\nverify and set the file permissions of your distribution (enter: y)\n\nIn some cases, eg: the binaries are stored on a NTFS or on any other\nfilesystem, which provides additional file permissions, the UNIX file\npermissions can be wrong. In this case we would advise to verfiy and\nto set the file permissions (enter: n) (y/n) \[n\] >> " }
      "DISTINST_WILL_NOT_VERIFY_FILE_PERMISSIONS" { set value "We will not verify your file permissions. Hit <RETURN> to continue >>" }
      "DISTINST_DO_NOT_VERIFY_FILE_PERMISSIONS" { set value "We do not verify file permissions. Hit <RETURN> to continue >> " }
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
      "DISTINST_ENTER_SPOOL_DIR" { set value "Please enter a qmaster spool directory now! >>" }
      "DISTINST_USING_GID_RANGE_HIT_RETURN" { set value "\nUsing >%s< as gid range. Hit <RETURN> to continue >> " }
      "DISTINST_WINDOWS_SUPPORT" { set value "\nAre you going to install Windows Execution Hosts? (y/n) \[n\] >> " }
      "DISTINST_EXECD_INSTALL_COMPLETE" { set value "Your execution daemon installation is now completed." }
      "DISTINST_LOCAL_CONFIG_FOR_HOST" { set value "Local configuration for host >%s< created." }
      "DISTINST_CELL_NAME_FOR_QMASTER" { set value "\nGrid Engine supports multiple cells.\n\nIf you are not planning to run multiple Grid Engine clusters or if you don't\nknow yet what is a Grid Engine cell it is safe to keep the default cell name\n\n   default\n\nIf you want to install multiple cells you can enter a cell name now.\n\nThe environment variable\n\n   \\\$SGE_CELL=<your_cell_name>\n\nwill be set for all further Grid Engine commands.\n\nEnter cell name \[%s\] >> " }
      "DISTINST_CELL_NAME_FOR_EXECD" { set value "\nPlease enter cell name which you used for the qmaster\ninstallation or press <RETURN> to use \[%s\] >> " }
      "DISTINST_CELL_NAME_EXISTS" { set value "Do you want to select another cell name? (y/n) \[y\] >> " }
      "DISTINST_CELL_NAME_OVERWRITE" { set value "Do you want to overwrite \[y\] or delete \[n\] the directory? (y/n) \[y\] >> " }
      "DISTINST_ADD_DEFAULT_QUEUE" { set value "Do you want to add a default queue for this host (y/n) \[y\] >> " }
      "DISTINST_ALL_QUEUE_HOSTGROUP" { set value "Creating the default <all.q> queue and <allhosts> hostgroup" }
      "DISTINST_ADD_DEFAULT_QUEUE_INSTANCE" { set value "Do you want to add a default queue instance for this host (y/n) \[y\] >> " }
      "DISTINST_ENTER_DATABASE_SERVER" { set value "Please enter the name of your Berkeley DB Spooling Server! >> " }
      "DISTINST_ENTER_SERVER_DATABASE_DIRECTORY" { set value "Please enter the Database Directory now!\n\nDefault: \[%s\] >> " }
      "DISTINST_ENTER_DATABASE_DIRECTORY_LOCAL_SPOOLING" { set value "Please enter the Database Directory now, even if you want to spool locally,\nit is necessary to enter this Database Directory. \n\nDefault: \[%s\] >> " }
      "DISTINST_DATABASE_DIR_NOT_ON_LOCAL_FS" { set value "The database directory >%s<\nis not on a local filesystem.\nPlease choose a local filesystem or configure the RPC Client/Server mechanism" }
      "DISTINST_STARTUP_RPC_SERVER" { set value "*is completed, continue with <RETURN>" }
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
      "DISTINST_ADD_SHADOWHOST_HEADLINE" { set value "\nAdding Grid Engine shadow hosts" }
      "DISTINST_ADD_SHADOWHOST_INFO" { set value "\nIf you want to use a shadow host, it is recommended to add this host\n to the list of administrative hosts.\n\nIf you are not sure, it is also possible to add or remove hosts after the\ninstallation with <qconf -ah hostname> for adding and <qconf -dh hostname>\nfor removing this host\n\nAttention: This is not the shadow host installation procedure.\n You still have to install the shadow host separately\n\n" }
      "DISTINST_ADD_SHADOWHOST_INFO2" { set value "\nPlease now add the list of hosts, where you will later install your shadow\ndaemon.\n\nPlease enter a blank separated list of your execution hosts. You may\npress <RETURN> if the line is getting too long. Once you are finished\nsimply press <RETURN> without entering a name.\n\nYou also may prepare a file with the hostnames of the machines where you plan\nto install Grid Engine. This may be convenient if you are installing Grid\nEngine on many hosts.\n\n" }
      "DISTINST_ADD_SHADOWHOST_ASK" { set value "Do you want to add your shadow host(s) now? (y/n) \[y\] >> " }
      "DISTINST_ADD_SHADOWHOST_FROM_FILE_ASK" { set value "Do you want to use a file which contains the list of hosts (y/n) \[n\] >> " }
      "DISTINST_SHADOW_HEADLINE" { set value "\nShadow Master Host Setup" }
      "DISTINST_SHADOW_INFO" { set value "\nMake sure, that the host, you wish to configure as a shadow host,\n has read/write permissions to the qmaster spool and SGE_ROOT/<cell>/common \ndirectory! For using a shadow master it is recommended to set up a \nBerkeley DB Spooling Server\n\n Hit <RETURN> to continue >> " }
      "DISTINST_SHADOW_ROOT" { set value "Please enter your SGE_ROOT directory or use the default\n\[%s\] >> " }
      "DISTINST_SHADOW_CELL" { set value "Please enter your SGE_CELL directory or use the default \[default\] >> " }
      "DISTINST_SHADOWD_INSTALL_COMPLETE" { set value "Shadowhost installation completed!" }     
      "DISTINST_WE_CONFIGURE_WITH_X_SETTINGS" { set value "\nWe're configuring the scheduler with >%s< settings!\n Do you agree? (y/n) \[y\] >> " }     
      "DISTINST_RPC_WELCOME" { set value "Hit <RETURN> if this is ok or stop the installation with Ctrl-C >> " }     
      "DISTINST_RPC_INSTALL_AS_ADMIN" { set value "Do you want to install Grid Engine as admin user >%s< (y/n) \[y\] >> " }     
      "DISTINST_RPC_SGE_ROOT" { set value "If this directory is not correct (e.g. it may contain an automounter\nprefix) enter the correct path to this directory or hit <RETURN>\nto use default \[%s\] >> " }     
      "DISTINST_RPC_HIT_RETURN_TO_CONTINUE" { set value "Hit <RETURN> to continue >> " }     
      "DISTINST_RPC_SGE_CELL" { set value "Enter cell name \[%s\] >> " }     
      "DISTINST_RPC_SERVER" { set value "\nEnter database server name or \nhit <RETURN> to use default \[%s\] >> " }     
      "DISTINST_RPC_DIRECTORY" { set value "\nEnter the database directory\nor hit <RETURN> to use default \[%s\] >> " }     
      "DISTINST_RPC_DIRECTORY_EXISTS" { set value "The spooling directory already exists! Do you want to delete it? (y/n) \[n\] >> " }     
      "DISTINST_RPC_START_SERVER" { set value "Shall the installation script try to start the RPC server? (y/n) \[y\] >>" }     
      "DISTINST_RPC_SERVER_STARTED" { set value "Please remember these values, during Qmaster installation\n you will be asked for! Hit <RETURN> to continue!" }     
      "DISTINST_RPC_INSTALL_RC_SCRIPT" { set value "We can install the startup script that\nGrid Engine is started at machine boot (y/n) \[y\] >> " }     
      "DISTINST_RPC_SERVER_COMPLETE" { set value "e.g. * * * * * <full path to scripts> <sge-root dir> <sge-cell> <bdb-dir>\n" }
      "DISTINST_CSP_COPY_CERTS" { set value "Should the script try to copy the cert files, for you, to each\nexecution host? (y/n) \[y\] >>" }     
      "DISTINST_CSP_COPY_CMD" { set value "Do you want to use rsh/rcp instead of ssh/scp? (y/n) \[n\] >>" }
      "DISTINST_CSP_COPY_FAILED" { set value "The certificate copy failed!" }
      "DISTINST_CSP_COPY_RSH_FAILED" { set value "Certificates couldn't be copied!"}
      "DISTINST_EXECD_UNINST_NO_ADMIN" { set value "This host is not an admin host. Uninstallation is not allowed\nfrom this host!" }
      "DISTINST_EXECD_UNINST_ERROR_CASE" { set value "Disabling queues now!" }
      "DISTINST_QMASTER_WINDOWS_DOMAIN_USER" { set value "or are you going to use local Windows Users (answer: n) (y/n) \[y\] >> " }
      "DISTINST_QMASTER_WINDOWS_MANAGER" { set value "Please, enter the Windows Administrator name \[Default: Administrator\] >> " }
      "DISTINST_EXECD_WINDOWS_HELPER_SERVICE" { set value "Do you want to install the Windows Helper Service? (y/n) \[n\] >> " }
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

