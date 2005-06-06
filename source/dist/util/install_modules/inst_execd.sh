#! /bin/sh
#
# SGE configuration script (Installation/Uninstallation/Upgrade/Downgrade)
# Scriptname: inst_execd.sh
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
# GetAdminUser
#
GetAdminUser()
{
   ADMINUSER=`cat $SGE_ROOT/$SGE_CELL/common/bootstrap | grep "admin_user" | awk '{ print $2 }'`
   euid=`$SGE_UTILBIN/uidgid -euid`

      if [ `echo "$ADMINUSER" |tr "A-Z" "a-z"` = "none" -a $euid = 0 ]; then
         ADMINUSER=default
      fi
   
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
    if [ "$SGE_CELL" = "" ]; then
       SGE_CELL="default"
    fi
    $INFOTEXT -n "\nPlease enter cell name which you used for the qmaster\n" \
                 "installation or press <RETURN> to use [%s] >> " $SGE_CELL
    INP=`Enter $SGE_CELL`
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
      $INFOTEXT -log "\nObviously there was no qmaster installation yet!\nCall >install_qmaster<\n" \
                  "on the machine which shall run the Grid Engine qmaster\n"

      if [ $AUTO = true ]; then
         MoveLog
      fi

      exit 1
   else
      $INFOTEXT "\nUsing cell: >%s<\n" $SGE_CELL_VAL
      $INFOTEXT -log "\nUsing cell: >%s<\n" $SGE_CELL_VAL
   fi

   $INFOTEXT -wait -auto $AUTO -n "Hit <RETURN> to continue >> "
   $CLEAR

   GetAdminUser

   user=`grep admin_user $COMMONDIR/bootstrap | awk '{ print $2 }'`

   if [ "$user" != "" ]; then
      if [ `echo "$user" |tr "A-Z" "a-z"` = "none" -a $euid = 0 ]; then
         user=default
      fi
   fi

   if [ "$ADMINUSER" != "$user" ]; then
      if [ "$user" = default ]; then
         $INFOTEXT "The admin user >%s< is different than the default user >root<\n" \
                   "in the global cluster configuration.\n" $ADMINUSER
         $INFOTEXT -log "The admin user >%s< is different than the default user >root<\n" \
                   "in the global cluster configuration.\n" $ADMINUSER

      else
         $INFOTEXT "The admin user >%s< doesn't match the admin username >%s<\n" \
                   "in the global cluster configuration.\n" $ADMINUSER $user
         $INFOTEXT -log "The admin user >%s< doesn't match the admin username >%s<\n" \
                   "in the global cluster configuration.\n" $ADMINUSER $user
      fi
      $INFOTEXT "Installation failed. Exit."
      $INFOTEXT -log "Installation failed. Exit."

      if [ $AUTO = true ]; then
         MoveLog
      fi

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
         $INFOTEXT -log ">common directory not found: %s<\n" $COMMONDIR
      fi

      if [ $error_text = 2 ]; then
         $INFOTEXT ">configuration file not found: %s<\n" $COMMONDIR/configuration
         $INFOTEXT -log ">configuration file not found: %s<\n" $COMMONDIR/configuration
      fi

      if [ $error_text = 3 ]; then
         $INFOTEXT ">%s file not found: %s<\n" act_qmaster $COMMONDIR/act_qmaster
         $INFOTEXT -log ">%s file not found: %s<\n" act_qmaster $COMMONDIR/act_qmaster
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
# CheckHostNameResolving
#
# Check the hostnames and put out warning message on probably wrong hostname
# resolving
#
# If this host is not an admin host give user a chance to correct the
# resolving, host_aliases file and bootstrap settings
#
# Try at most $loop_max times 
#
CheckHostNameResolving()
{
   myrealname=`$SGE_UTILBIN/gethostname -name`

   loop_counter=0
   loop_max=10
   done=false
   while [ $done = false ]; do
      $CLEAR
      $INFOTEXT -u "\nChecking hostname resolving"

      errmsg=""
      $SGE_BIN/qconf -sh > /dev/null 2>&1
      if [ $? = 1 ]; then
         errmsg=`$SGE_BIN/qconf -sh 2>&1`
      else
         errmsg=`$SGE_BIN/qconf -sh 2>&1 |  grep denied:`
      fi
      
      if [ "$errmsg" != "" ]; then
         $INFOTEXT "\nCannot contact qmaster. The command failed:\n\n" \
                     "   %s\n\n" \
                     "The error message was:\n\n" \
                     "   %s\n\n" \
                     "$SGE_BIN/qconf -sh" "$errmsg"
         $INFOTEXT -log "\nCannot contact qmaster. The command failed:\n\n" \
                     "   %s\n\n" \
                     "The error message was:\n\n" \
                     "   %s\n\n" \
                     "$SGE_BIN/qconf -sh" "$errmsg"

         $INFOTEXT "You can fix the problem now or abort the installation procedure.\n" \
                   "The problem can be:\n\n" \
                   "   - the qmaster is not running\n" \
                   "   - the qmaster host is down\n" \
                   "   - an active firewall blocks your request\n"
         $INFOTEXT -auto $AUTO -ask "y" "n" -def "y" -n "Contact qmaster again (y/n) ('n' will abort) [y] >> " 
         if [ $? != 0 ]; then
            $INFOTEXT "Installation failed"
            $INFOTEXT -log "Cannot contact qmaster! Installation failed"

            if [ $AUTO = true ]; then
               MoveLog
            fi

            exit 1
         fi
      else
         ignore_fqdn=`cat $SGE_ROOT/$SGE_CELL/common/bootstrap | grep "^ignore_fqdn" | awk '{print $2}'| egrep -i "true|1"`

         if [ "$ignore_fqdn" != "" ]; then
            ignore_fqdn=true
         else
            ignore_fqdn=false
         fi

         default_domain=`cat $SGE_ROOT/$SGE_CELL/common/bootstrap | grep "^default_domain" | awk '{print $2}' | tr "[A-Z]" "[a-z]"`
         if [ "$default_domain" = NONE ]; then
            default_domain=none
         fi

         myaname=`ExecuteAsAdmin $SGE_UTILBIN/gethostname -aname`
         #myname=`echo $myaname | cut -f1 -d. | tr "[A-Z]" "[a-z]"`
         
         if [ $ignore_fqdn = true ]; then
            admin_host_list=`ExecuteAsAdmin $SGE_BIN/qconf -sh | cut -f1 -d. | tr "[A-Z]" "[a-z]"`
            myname=`echo $myaname | cut -f1 -d. | tr "[A-Z]" "[a-z]"`
         else
            admin_host_list=`ExecuteAsAdmin $SGE_BIN/qconf -sh | tr "[A-Z]" "[a-z]"`
            myname=$myaname
            if [ "$default_domain" != none ]; then
               hasdot=`echo $myname|grep '\.'`
               if [ "$hasdot" = "" ]; then
                  myname=$myname.$default_domain
               fi
               new_admin_host_list=""
               for h in $admin_host_list; do
                  hasdot=`echo $h|grep '\.'`
                  if [ hasdot = "" ]; then
                     h=$h.$default_domain 
                  fi
                  new_admin_host_list="$new_admin_host_list $h"
               done
               admin_host_list="$new_admin_host_list"
            fi 
         fi

         found=false
         for h in $admin_host_list; do
            if [ $myname = $h ]; then
               found=true
               break
            fi
         done

         if [ $found = false ]; then
            $INFOTEXT "\nThis hostname is not known at qmaster as an administrative host.\n\n" \
                        "Real hostname of this machine:                     %s\n" \
                        "Aliased hostname (if \"host_aliases\" file is used): %s\n" \
                        "Default domain (\"none\" means ignore):              %s\n" \
                        "Ignore domain names:                               %s\n\n" \
                        "The resulting hostname is:              =========> %s\n\n" \
                        "If you think that this host has already been added as an administrative\n" \
                        "host, the hostname resolving  on this host will most likely differ from\n" \
                        "the hostname resolving method on the qmaster machine\n" \
                        $myrealname $myaname $default_domain $ignore_fqdn $myname

            if [ $AUTO = true ]; then
               $INFOTEXT -log "Installation failed!\nThis hostname is not known at qmaster as an administrative host."
               MoveLog
               exit 1
            fi
           
            $INFOTEXT "Please check and correct your >/etc/hosts< file and >/etc/nsswitch.conf<\n" \
                      "file on this host and on the qmaster machine.\n\n" \
                      "You can now add this host as an administrative host in a seperate\n" \
                      "terminal window and then continue with the installation procedure.\n" 
                         
            $INFOTEXT -auto $AUTO -ask "y" "n" -def "y" -n "Check again (y/n) ('n' will abort) [y] >> "
            if [ $? != 0 ]; then
               $INFOTEXT "Installation failed"
               exit 1
            fi
         else
            $INFOTEXT -wait -auto $AUTO -n "\nThis hostname is known at qmaster as an administrative host.\n\n" \
                            "Hit <RETURN> to continue >>"
            $CLEAR
            done=true
         fi
      fi

      loop_counter=`expr $loop_counter + 1`
      if [ $loop_counter -ge $loop_max ]; then
         $INFOTEXT -e "Installation failed after %s retries" $loop_max
         exit
      fi
   done
}

#-------------------------------------------------------------------------
# AddLocalConfiguration_With_Qconf
#
AddLocalConfiguration_With_Qconf()
{
   $CLEAR
   $INFOTEXT -u "\nCreating local configuration"

   mkdir /tmp/$$
   TMPL=/tmp/$$/${HOST}
   rm -f $TMPL
   if [ -f $TMPL ]; then
      $INFOTEXT "\nCan't create local configuration. Can't delete file >%s<" "$TMPL"
      $INFOTEXT -log "\nCan't create local configuration. Can't delete file >%s<" "$TMPL"
   else
      $INFOTEXT -log "\nCreating local configuration for host >%s<" $HOST
      PrintLocalConf 0 > $TMPL
      ExecuteAsAdmin $SGE_BIN/qconf -Aconf $TMPL
      rm -rf /tmp/$$
      $INFOTEXT "Local configuration for host >%s< created." $HOST
      $INFOTEXT -log "Local configuration for host >%s< created." $HOST
   fi
   $INFOTEXT -wait -auto $AUTO -n "\nHit <RETURN> to continue >> "
}


#--------------------------------------------------------------------------
# StartExecd
#
StartExecd()
{
   $INFOTEXT -u "\nGrid Engine execution daemon startup"
   $INFOTEXT "\nStarting execution daemon. Please wait ..."
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

   #exechost=`$SGE_UTILBIN/gethostname -aname | cut -f1 -d.`

   ignore_fqdn=`cat $SGE_ROOT/$SGE_CELL/common/bootstrap | grep "^ignore_fqdn" | awk '{print $2}'| egrep -i "true|1"`
   if [ "$ignore_fqdn" != "" ]; then
      ignore_fqdn=true
   else
      ignore_fqdn=false
   fi

   default_domain=`cat $SGE_ROOT/$SGE_CELL/common/bootstrap | grep "^default_domain" | awk '{print $2}' | tr "[A-Z]" "[a-z]"`
   if [ "$default_domain" = NONE ]; then
      default_domain=none
   fi

   myaname=`$SGE_UTILBIN/gethostname -aname`
   if [ $ignore_fqdn = true ]; then
      exechost=`echo $myaname | cut -f1 -d. | tr "[A-Z]" "[a-z]"`
   else
      exechost=$myaname
      if [ "$default_domain" != none ]; then
         hasdot=`echo $exechost|grep '\.'`
         if [ "$hasdot" = "" ]; then
            exechost=$exechost.$default_domain
         fi
      fi
   fi

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
   $INFOTEXT "\nDuring the qmaster installation you've already entered " \
             "a global\nexecd spool directory. This is used, if no local " \
             "spool directory is configured.\n\n Now you can enter a local spool " \
             "directory for this host.\n"
   $INFOTEXT -n -auto $AUTO -ask "y" "n" -def "n" "Do you want to configure a local spool directory\n for this host (y/n) [n] >> "
   ret=$?

   while [ $ret = 0 ]; do 
      $INFOTEXT -n "Please enter the local spool directory now! >> " 
      LOCAL_EXECD_SPOOL=`Enter`
      if [ "$LOCAL_EXECD_SPOOL" = "" ]; then
         $INFOTEXT -n -auto $AUTO -ask "y" "n" -def "n" "Do you want to configure a local spool directory\n for this host (y/n) [n] >> "
         ret=$?
         LOCAL_EXECD_SPOOL="undef"
      else
         $INFOTEXT "Using local execd spool directory [%s]" $LOCAL_EXECD_SPOOL
         $INFOTEXT -wait -auto $AUTO -n "Hit <RETURN> to continue >> "
         MakeLocalSpoolDir
         ret=1
      fi
   done

   if [ "$ret" = 1 -a "$LOCAL_EXECD_SPOOL" = "undef" ]; then
      MakeHostSpoolDir
   fi

   if [ $AUTO = "true" ]; then
      if [ "$EXECD_SPOOL_DIR_LOCAL" != "" ]; then
         LOCAL_EXECD_SPOOL=$EXECD_SPOOL_DIR_LOCAL
         $INFOTEXT -log "Using local execd spool directory [%s]" $LOCAL_EXECD_SPOOL
         MakeLocalSpoolDir
      fi
   fi
}

MakeHostSpoolDir()
{
   spool_dir=`qconf -sconf | grep "execd_spool_dir" | awk '{ print $2 }'`
   host_dir=`$SGE_UTILBIN/gethostname -aname | cut -d"." -f1`

   mkdir -p $spool_dir/$host_dir
   ret=$?

   if [ $ret = 0 ]; then
      group=`$SGE_UTILBIN/checkuser -gid $ADMINUSER`
      chown -R $ADMINUSER:$group $spool_dir/$host_dir 
   else
      MKDIR="mkdir -p"
      ExecuteAsAdmin $MKDIR $spool_dir/$host_dir
   fi
}



MakeLocalSpoolDir()
{
   tmp_dir=$LOCAL_EXECD_SPOOL
   end_loop=0

   while [ "$end_loop" = "0" ]; do
      base_name=`basename $tmp_dir`
      tmp_dir=`dirname $tmp_dir`

      if [ -d $tmp_dir ]; then
         dir_exists=$tmp_dir
         end_loop=1
      fi 
   done

   #try as root
   mkdir -p $LOCAL_EXECD_SPOOL
   
   if [ $? = 0 ]; then
      group=`$SGE_UTILBIN/checkuser -gid $ADMINUSER`
      chown -R $ADMINUSER:$group $dir_exists/$base_name
   else
      #try as adminuser
      ExecuteAsAdmin mkdir -p $LOCAL_EXECD_SPOOL
   fi
}

#-------------------------------------------------------------------------
# AddHostsAuto
#
AddHostsAuto()
{
   if [ "$AUTO" = "true" ]; then
      for h in $ADMIN_HOST_LIST; do
        if [ -f $h ]; then
           $INFOTEXT -log "Adding ADMIN_HOSTS from file %s" $h
           for tmp in `cat $h`; do
             $INFOTEXT -log "Adding ADMIN_HOST %s" $tmp
             $SGE_BIN/qconf -ah $tmp
           done
        else
             $INFOTEXT -log "Adding ADMIN_HOST %s" $h
             $SGE_BIN/qconf -ah $h
        fi
      done
      
      for h in $SUBMIT_HOST_LIST; do
        if [ -f $h ]; then
           $INFOTEXT -log "Adding SUBMIT_HOSTS from file %s" $h
           for tmp in `cat $h`; do
             $INFOTEXT -log "Adding SUBMIT_HOST %s" $tmp
             $SGE_BIN/qconf -as $tmp
           done
        else
             $INFOTEXT -log "Adding SUBMIT_HOST %s" $h
             $SGE_BIN/qconf -as $h
        fi
      done  

      for h in $SHADOW_HOST; do
        if [ -f $h ]; then
           $INFOTEXT -log "Adding SHADOW_HOSTS from file %s" $h
           for tmp in `cat $h`; do
             $INFOTEXT -log "Adding SHADOW_HOST %s" $tmp
             $SGE_BIN/qconf -ah $tmp
           done
        else
             $INFOTEXT -log "Adding SHADOW_HOST %s" $h
             $SGE_BIN/qconf -ah $h
        fi
      done
   fi
}



ExecdAlreadyInstalled()
{
   hostname=$1
   ret="undef"

   ret=`qconf -sconf $hostname | grep execd_spool_dir | awk '{ print $2 }'`
   if [ ! -d $ret/$hostname ]; then
      ret=`qconf -sconf | grep execd_spool_dir | awk '{ print $2 }'`
   fi

   if [ -d $ret/$hostname ]; then
      $INFOTEXT -log "Found execd spool directory: $s!\nExecution host %s is already installed!" $ret/$hostname $hostname
      return 1 
   else
      return 0 
   fi
}


CheckWinAdminUser()
{
   if [ "$SGE_ARCH" != "win32-x86" ]; then
      return
   fi

   if [ -f $SGE_ROOT/$SGE_CELL/common/bootstrap ]; then
      win_admin_user=`cat $SGE_ROOT/$SGE_CELL/common/bootstrap | grep admin_user | awk '{ print $2 }'`
      WIN_HOST_NAME=`hostname | tr [a-z] [A-Z]`
      ADMINUSER=$WIN_HOST_NAME"+$win_admin_user"
   else
      $INFOTEXT "bootstrap file could not be found, please check your installation! Exiting now ..."
      exit 1
   fi

   if [ "$win_admin_user" != "default" -a "$win_admin_user" != "root" ]; then
      tmp_path=$PATH
      PATH=/usr/contrib/win32/bin:/common:$SAVED_PATH
      export PATH
      eval net user $win_admin_user > /dev/null 2>&1
      ret=$?
      if [ "$ret" = 127 ]; then
         /usr/contrib/win32/bin/net user $win_adimn_user > /dev/null 2>&1
         ret=$?
         if [ "$ret" = 127 ]; then
	         $INFOTEXT "The net binary could not be found!\nPlease, check your $PATH variable or your installation!"
            exit 1
         fi
      fi
      if [ "$ret" != 0 ]; then
         while [ "$ret" != 0 ]; do
            $INFOTEXT -u "Local Admin User"
            $INFOTEXT "\nThe local admin user %s, does not exist!\n The script tries to create the admin user." $win_admin_user
            $INFOTEXT "Please enter a password for your admin user >> "
            stty_orig=`stty -g`
            stty -echo
            read SECRET
            stty $stty_orig
            $INFOTEXT "Creating admin user %s, now ...\n" $win_admin_user
            eval net user $win_admin_user $SECRET /add
            ret=$?
            unset SECRET
         done
         ExecuteAsAdmin regpwd
         $INFOTEXT -wait "Admin user created, hit <ENTER> to continue!"
         is_nis=`mapadmin.exe | grep "UNIX Users and Groups Source" | cut -d ":" -f2 | cut -d " " -f2`
         if [ "$is_nis" = "NIS" ]; then
            $INFOTEXT "Your system is using NIS Domain setup. To setup the right Windows Domain\n" \
                      "Unix Domain mapping we need your Unix Nis Domain.\n\n"
            $INFOTEXT -n "Please enter your Unix Nis Domain now >> "
            NIS_DOMAIN=`Enter`
         else
            NIS_DOMAIN="PCNFS"
         fi
         
         mapadmin.exe ADD -wu "$WIN_HOST_NAME\\$win_admin_user" -uu "$NIS_DOMAIN\\$win_admin_user" -setprimary
         
         $SGE_BIN/qconf -am $WIN_HOST_NAME"+"$win_admin_user
      fi
      PATH=$tmp_path
      export PATH 
   fi

}
