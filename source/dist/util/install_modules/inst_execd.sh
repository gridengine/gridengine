#! /bin/sh
#
# SGE/SGEEE configuration script (Installation/Uninstallation/Upgrade/Downgrade)
# Scriptname: inst_sgeee_execd.sh
# Module: execd installation functions
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

#set -x


#--------------------------------------------------------------------------
# WelcomeTheUserExecHost
#
WelcomeTheUserExecHost()
{
   if [ $AUTO = true ]; then
      return
   fi
   $INFOTEXT -u "\n\n\n\n  STARTING WITH EXECHOST INSTALLATION!!!"
   sleep 1
   $CLEAR
   $INFOTEXT -u "\n\n\n\n  STARTING WITH EXECHOST INSTALLATION!!!"
   sleep 1
   $CLEAR
 
   $INFOTEXT -u "\nWelcome to the Grid Engine execution host installation"
   $INFOTEXT "\nIf you haven't installed the Grid Engine qmaster host yet, you must execute\n" \
             "this step (with >install_qmaster<) prior the execution host installation.\n\n" \
             "For a sucessfull installation you need a running Grid Engine qmaster. It is\n" \
             "also neccesary that this host is an administrative host.\n\n" \
             "You can verify your current list of administrative hosts with\n" \
             "the command:\n\n" \
             "   # qconf -sh\n\n" \
             "You can add an administrative host with the command:\n\n" \
             "   # qconf -ah <hostname>\n\n" \
             "The execution host installation will take approximately 5 minutes.\n"

   $INFOTEXT -wait -auto $AUTO -n "Hit <RETURN> to continue >> "
   $CLEAR
}


#-------------------------------------------------------------------------
# CheckQmasterInstallation
#
CheckQmasterInstallation()
{
   if [ $AUTO = true ]; then
    SGE_CELL=$CELL_NAME
   else
    $CLEAR
    $INFOTEXT -u "\nGrid Engine cells"
    $INFOTEXT -n "\nPlease enter cell name which you used for the qmaster\n" \
                 "installation or press <RETURN> to use default cell >default< >> "
    INP=`Enter ""`
    if [ "$INP" = "" ]; then
       SGE_CELL=default
    else
       SGE_CELL=$INP
    fi
   fi
    SGE_CELL_VAL=$SGE_CELL

   SetCellDependentVariables

   if [ ! -f $COMMONDIR/act_qmaster -o ! -f $COMMONDIR/bootstrap ]; then
      $INFOTEXT "\nObviously there was no qmaster installation yet!\nCall >install_qmaster<\n" \
                  "on the machine which shall run the Grid Engine qmaster\n"
      exit 1
   else
      $INFOTEXT "\nUsing cell: >%s<\n" $SGE_CELL_VAL
   fi

   $INFOTEXT -wait -auto $AUTO -n "Hit <RETURN> to continue >> "
   $CLEAR

   user=`grep admin_user $COMMONDIR/bootstrap | awk '{ print $2 }'`

   if [ "$user" != "" ]; then
      if [ `echo "$user" |tr "A-Z" "a-z"` = "none" ]; then
         user=default
      fi
   fi

   if [ "$ADMINUSER" != "$user" ]; then
      if [ "$user" = default ]; then
         $INFOTEXT "The admin user >%s< is different than the default user >root<\n" \
                   "in the global cluster configuration.\n" $ADMINUSER
      else
         $INFOTEXT "The admin user >%s< doesn't match the admin username >%s<\n" \
                   "in the global cluster configuration.\n" $ADMINUSER $user
      fi
      $INFOTEXT "Installation failed. Exit."
      exit 1
   fi
}


#-------------------------------------------------------------------------
# Check whether the qmaster is installed and the file systems is shared
#
CheckCellDirectory()
{
   $CLEAR
   check_cell_error=0
   error_text=0
   if [ ! -d $COMMONDIR ]; then
      error_text=1
      check_cell_error=1
   fi

#   if [ ! -f $COMMONDIR/configuration ]; then
#      if [ $check_cell_error = 0 ]; then
#         error_text=2
#      fi
#      check_cell_error=1
#   fi

   if [ ! -f $COMMONDIR/act_qmaster ]; then
      if [ $check_cell_error = 0 ]; then
         error_text=3
      fi
      check_cell_error=1
   fi

   if [ $check_cell_error != 0 ]; then
      $INFOTEXT -u "\nChecking cell directory"
      $INFOTEXT "\nCannot find required files. The following error was returned:\n\n"

      if [ $error_text = 1 ]; then
         $INFOTEXT ">common directory not found: %s<\n" $COMMONDIR
      fi

      if [ $error_text = 2 ]; then
         $INFOTEXT ">configuration file not found: %s<\n" $COMMONDIR/configuration
      fi

      if [ $error_text = 3 ]; then
         $INFOTEXT ">%s file not found: %s<\n" act_qmaster $COMMONDIR/act_qmaster
      fi

      $INFOTEXT "\nPlease make sure that you have installed the qmaster host before\n" \
                "installing an execution host.\n\n" \
                "The installation procedure will only work if you use a shared\n" \
                "directory (e.g. shared with NFS) for your installation.\n\n" \
                "The installation of the execution daemon will abort now.\n"

      exit 1
   fi
}


#--------------------------------------------------------------------------
# CheckCSP check that there are no old certs/keys
#
CheckCSP()
{

   if [ $CSP = false ]; then
      return
   fi

   if [ "$SGE_QMASTER_PORT" != "" ]; then
      CA_PORT=port$SGE_QMASTER_PORT
   else
      CA_PORT=sge_qmaster
   fi

   $SGE_UTILBIN/adminrun $ADMINUSER test -f $HOME/.sge/$CA_PORT/$SGE_CELL
   if [ ! $? ]; then
      $INFOTEXT -e "Please remove the old CSP security directory \$HOME/.sge/\$CA_PORT/\$SGE_CELL. Exit."
      exit 1
   fi
}


#-------------------------------------------------------------------------
# Check the hostnames and put out warning message on probably wrong
# hostname resolving
#
CheckHostNameResolving()
{

   resolve_get_configuration=`ExecuteAsAdmin $SGE_BIN/qconf -sconf global | grep "^ignore_fqdn" `
   resolve_qmaster_params=`echo $resolve_get_configuration | egrep -i "true|1"`
   if [ "x$resolve_qmaster_params" = "x" ]; then
      $ECHO ""
      set IGNORE_FQDN_DEFAULT=false
   else
#     don't need this check when IGNORE_FQDN=true in qmaster_params
      set IGNORE_FQDN_DEFAULT=true
      return
   fi

   done=false
   loop_counter=0
   while [ $done = false ]; do
       done=false
       $CLEAR
       $INFOTEXT -u "\nChecking hostname resolving"

       resolve_admin_hosts=`ExecuteAsAdmin $SGE_BIN/qconf -sh`
       resolve_this_hostname=`ExecuteAsAdmin $SGE_UTILBIN/gethostname -aname`
       resolve_default_domain=`cat $SGE_ROOT/$SGE_CELL/common/bootstrap | grep "^default_domain" | awk '{print $2}'`

       if [ "$resolve_default_domain" = "" ]; then
           resolve_default_domain="none"
       fi
       $INFOTEXT "\nThis host has the local hostname >%s<.\n" $resolve_this_hostname

       resolve_default_domain_upper=`echo $resolve_default_domain | tr "[a-z]" "[A-Z]"`
       if [ "$resolve_default_domain_upper" != "NONE" ]; then
            resolve_tmp_name=`echo $resolve_this_hostname | cut -f 1 -d "."`
            if [ "$resolve_tmp_name" = "$resolve_this_hostname" ]; then
                resolve_this_hostname="$resolve_this_hostname.$resolve_default_domain"

                $INFOTEXT "The default_domain parameter is set in the global configuration\n" \
                          "and added to the unqualified hostname. As a result the\n" \
                          "execd on this host would return the following hostname: >%s<\n" \
                          $resolve_this_hostname
            fi
       fi

       resolve_upper_this_hostname=`echo $resolve_this_hostname | tr "[a-z]" "[A-Z]"`
       for i in $resolve_admin_hosts; do
           resolve_upper_admin_hostname=`echo $i | tr "[a-z]" "[A-Z]"`
           if [ "$resolve_upper_admin_hostname" = "$resolve_upper_this_hostname" ]; then
              $INFOTEXT "This host can be resolved correctly.\n"
              done=true
              break
           fi
       done

       if [ $done = false ]; then
           $INFOTEXT "This host is unknown on the qmaster host.\n\n" \
                     "Please make sure that you added this host as administrative host!\n" \
                     "If you did not, please add this host now with the command\n\n" \
                     "   # qconf -ah HOSTNAME\n\n" \
                     "on your qmaster host.\n"
           if [ $loop_counter != 0 ]; then
               $INFOTEXT "If this host is already added as administrative host on your qmaster host\n" \
                         "there may be a hostname resolving problem on this machine.\n\n" \
                         "Please check your >/etc/hosts< file and >/etc/nsswitch.conf< file.\n\n" \
                         "Hostname resolving problems will cause the problem that the\n" \
                         "execution host will not be accepted by qmaster. Qmaster will\n" \
                         "receive no load report values and show a load value\n" \
                         "(>load_avg<) of 99.99 for this host.\n"
               if [ $AUTO = true ]; then
                   exit 1
               fi
           fi

           $INFOTEXT -auto $AUTO -ask "y" "n" -def "y" -n "Check again (y/n) [y] >> "
           if [ $? = 0 ]; then
              done=false
           else
              done=true
           fi
       else
           $INFOTEXT -wait -auto $AUTO -n "Hit <RETURN> to continue >> "
           $CLEAR
           return
       fi
       loop_counter=`expr $loop_counter + 1`
   done
}


#-------------------------------------------------------------------------
# AddLocalConfiguration_With_Qconf
#
AddLocalConfiguration_With_Qconf()
{

   $CLEAR
   $INFOTEXT -u "\nCreating local configuration"

   TMPL=/tmp/${HOST}
   rm -f $TMPL
   if [ -f $TMPL ]; then
      $INFOTEXT "\nCan't create local configuration. Can't delete file >%s<" "$TMPL"
   else
      $INFOTEXT "\nCreating local configuration for host >%s<" $HOST
      PrintLocalConf 0 > /tmp/$HOST
      Execute $SGE_BIN/qconf -Aconf /tmp/$HOST
      rm -f /tmp/$HOST
      $INFOTEXT "Local configuration for host >%s< created." $HOST
   fi
   $INFOTEXT -wait -auto $AUTO -n "\nHit <RETURN> to continue >> "
}


#--------------------------------------------------------------------------
# StartExecd
#
StartExecd()
{
   $INFOTEXT -u "\nGrid Engine execution daemon startup"
   $INFOTEXT "\nStarting execution daemon daemon. Please wait ..."
   $SGE_STARTUP_FILE 
   $INFOTEXT -wait -auto $AUTO -n "\nHit <RETURN> to continue >> "
   $CLEAR
}


#-------------------------------------------------------------------------
# AddQueue
#
AddQueue()
{
   #if [ $addqueue = false ]; then
   #   return
   #fi

   exechost=`$SGE_UTILBIN/gethostname -name | cut -f1 -d.`

   slots=`$SGE_UTILBIN/loadcheck -loadval num_proc 2>/dev/null | sed "s/num_proc *//"`

   $INFOTEXT -u "\nAdding a queue for this host"
   $INFOTEXT "\nWe can now add a queue instance for this host:\n\n" \
             "   - it is added to the >allhosts< hostgroup\n" \
             "   - the queue provides %s slot(s) for jobs in all queues\n" \
             "     referencing the >allhosts< hostgroup\n\n" \
             "You do not need to add this host now, but before running jobs on this host\n" \
             "it must be added to at least one queue.\n" $slots

   $INFOTEXT -auto $AUTO -ask "y" "n" -def "y" -n \
             "Do you want to add a default queue instance for this host (y/n) [y] >> "

   if [ $? = 0 ]; then
      $SGE_BIN/qconf -aattr hostgroup hostlist $exechost @allhosts
      $SGE_BIN/qconf -aattr queue slots "[$exechost=$slots]" all.q
      $INFOTEXT -wait -auto $AUTO -n "\nHit <RETURN> to continue >> "
      $CLEAR
   fi
}

GetLocalExecdSpoolDir()
{
   $INFOTEXT -u "\nLocal execd spool directory configuration"
   $INFOTEXT "During the qmaster installation you've already entered " \
             "a global\nexecd spool directory. This is used, if no local " \
             "spool directory is configured.\n\n Now you can enter a local spool " \
             "directory for this host.\n"
   $INFOTEXT -n -auto $AUTO -ask "y" "n" -def "n" "Do you want to configure a local spool directory\n for this host (y/n) [n] >> "

   if [ $? = 0 ]; then
      $INFOTEXT -n "Please enter the local spool directory now! >> " 
      LOCAL_EXECD_SPOOL=`Enter`
      $INFOTEXT "Using local execd spool directory [%s]" $LOCAL_EXECD_SPOOL
      $INFOTEXT -wait -auto $AUTO "Hit <RETURN> to continue >> "
   else
      $CLEAR
   fi

   if [ $AUTO = "true" ]; then
      if [ $EXECD_SPOOL_DIR_LOCAL != "" ]; then
         LOCAL_EXECD_SPOOL=$EXECD_SPOOL_DIR_LOCAL
      fi
   fi

}
