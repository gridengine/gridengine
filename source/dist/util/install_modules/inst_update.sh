#!/bin/sh
#
# Grid Engine configuration script (Installation/Uninstallation/Upgrade/Downgrade)
# Scriptname: inst_update.sh
# Module: gridengine update functions
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
# set -x


UpdateConfiguration()
{
   pid=$$
   GetOldDirs

   GetOldConfiguration
   GetOldLocalConf
   GetOldComplexes
   GetOldAdminHosts
   GetOldExecHosts
   GetOldSubmitHosts
   GetOldManagers
   GetOldOperators
   GetOldUsers
   GetOldUsersets
   GetOldCalendars
   GetOldCkpt
   GetOldProjects
   GetOldPe
   GetOldShareTree
   GetOldFiles
    
}

GetOldDirs()
{
   OLD_COMMON_DIR=$OLD_SGE_ROOT/$OLD_SGE_CELL/common
   OLD_QMASTER_SPOOL=`cat $OLD_COMMON_DIR/configuration | grep qmaster_spool_dir | awk '{ print $2 }'`
   OLD_EXECD_SPOOL=`cat $OLD_COMMON_DIR/configuration | grep execd_spool_dir | awk '{ print $2 }'`
   OLD_BINARY_PATH=`cat $OLD_COMMON_DIR/configuration | grep binary_path | awk '{ print $2 }'`
}

GetOldConfiguration()
{
   pid=$$
   cat $OLD_COMMON_DIR/configuration | grep -v "#" | grep -v "execd_spool_dir" | grep -v "gid_range" | grep -v "qmaster_spool_dir" | grep -v "binary_path" \
      | grep -v "enforce_user" | grep -v "schedd_params" | grep -v "admin_user" | grep -v "default_domain" | grep -v "ignore_fqdn" \
      | grep -v "administrator_mail" > /tmp/configuration.$pid

   OLD_SCHEDD_PARAMS=`cat $OLD_COMMON_DIR/configuration | grep "schedd_params" | awk '{ print $2 }'`  

   echo "administrator_mail        $CFG_MAIL_ADDR" >> /tmp/configuration.$pid
   echo "execd_spool_dir           $CFG_EXE_SPOOL" >> /tmp/configuration.$pid
   echo "gid_range                 $CFG_GID_RANGE" >> /tmp/configuration.$pid
   echo "enforce_user              auto" >> /tmp/configuration.$pid
   echo "reporting_params          accounting=true reporting=false flush_time=00:00:15 joblog=false sharelog=00:00:00" >> /tmp/configuration.$pid
   echo "max_jobs                  0" >> /tmp/configuration.$pid
   echo "auto_user_oticket         0" >> /tmp/configuration.$pid
   echo "auto_user_fshare          0" >> /tmp/configuration.$pid
   echo "auto_user_default_project none" >> /tmp/configuration.$pid
   echo "auto_user_delete_time     86400" >> /tmp/configuration.$pid
   echo "delegated_file_staging    false" >> /tmp/configuration.$pid

   ExecuteAsAdmin $SPOOLDEFAULTS configuration /tmp/configuration.$pid
   rm /tmp/configuration.$pid    
}

GetOldScheddConfig()
{
   pid=$$
   cat $OLD_COMMON_DIR/sched_configuration | grep -v "#" | grep -v "user_sort" | grep -v "sgeee_schedule_interval" | grep -v "weight_jobclass" \
      | grep -v "weight_tickets_deadline" > /tmp/schedd_conf.$pid

   t=`cat $OLD_COMMON_DIR/sched_configuration | grep "halftime"`
   if [ "$t" = "" ]; then
      echo "halftime 168" >>  /tmp/schedd_conf.$pid
      echo "usage_weight_list          cpu=1,mem=0,io=0" >>  /tmp/schedd_conf.$pid
      echo "compensation_factor        5" >>  /tmp/schedd_conf.$pid
      echo "weight_user                0.25" >>  /tmp/schedd_conf.$pid
      echo "weight_project             0.25" >>  /tmp/schedd_conf.$pid
      echo "weight_department          0.25" >>  /tmp/schedd_conf.$pid
      echo "weight_job                 0.25" >>  /tmp/schedd_conf.$pid
      echo "weight_tickets_functional  10000" >>  /tmp/schedd_conf.$pid
      echo "weight_tickets_share       0" >>  /tmp/schedd_conf.$pid
   fi

   echo "flush_submit_sec                 0" >>  /tmp/schedd_conf.$pid
   echo "flush_finish_sec                 0" >>  /tmp/schedd_conf.$pid
   echo "params                           none" >>  /tmp/schedd_conf.$pid
   echo "reprioritize_interval            0:2:0" >> /tmp/schedd_conf.$pid
   echo "share_override_tickets           true" >> /tmp/schedd_conf.$pid
   echo "share_functional_shares          true" >> /tmp/schedd_conf.$pid
   echo "max_functional_jobs_to_schedule  200" >> /tmp/schedd_conf.$pid 
   echo "report_pjob_tickets              true" >> /tmp/schedd_conf.$pid
   echo "max_pending_tasks_per_job        50" >> /tmp/schedd_conf.$pid
   echo "halflife_decay_list              none" >> /tmp/schedd_conf.$pid
   echo "policy_hierarchy                 OFS" >> /tmp/schedd_conf.$pid
   echo "weight_ticket                    0.01" >> /tmp/schedd_conf.$pid
   echo "weight_waiting_time              0" >> /tmp/schedd_conf.$pid
   echo "weight_deadline                  3600000" >> /tmp/schedd_conf.$pid
   echo "weight_urgency                   0.1" >> /tmp/schedd_conf.$pid
   echo "weight_priority                  1" >> /tmp/schedd_conf.$pid
   echo "max_reservation                  0" >> /tmp/schedd_conf.$pid
   echo "default_duration                 0:10:0" >> /tmp/schedd_conf.$pid

   $SGE_BIN/qconf -Msconf /tmp/schedd_conf.$pid 
   rm  /tmp/schedd_conf.$pid

   $CLEAR
}

GetOldLocalConf()
{
   for l in `ls $OLD_COMMON_DIR/local_conf`; do
      ExecuteAsAdmin $SPOOLDEFAULTS local_conf $OLD_COMMON_DIR/local_conf/$l $l
   done
}


GetOldAdminHosts()
{
   ExecuteAsAdmin $SPOOLDEFAULTS adminhosts $OLD_QMASTER_SPOOL/admin_hosts/
}

GetOldCalendars()
{
   ExecuteAsAdmin $SPOOLDEFAULTS calendars $OLD_QMASTER_SPOOL/calendars/
}


GetOldCkpt()
{
   pid=$$
   Makedir /tmp/ckpt_dir.$pid >> /dev/null

   for ckpt in `ls $OLD_QMASTER_SPOOL/ckpt`; do
      ExecuteAsAdmin $TOUCH /tmp/ckpt_dir.$pid/$ckpt      
      cat $OLD_QMASTER_SPOOL/ckpt/$ckpt | grep -v "#" | grep -v queue_list > /tmp/ckpt_dir.$pid/$ckpt
   done
      ExecuteAsAdmin $SPOOLDEFAULTS ckpts /tmp/ckpt_dir.$pid/ 
      ExecuteAsAdmin $RM /tmp/ckpt_dir.$pid/*
      rm -fR /tmp/ckpt_dir.$pid
}



GetOldComplexes()
{
   Makedir /tmp/centry
   loop=1

   for c in `ls $OLD_QMASTER_SPOOL/complexes`; do
   OLD_IFS=$IFS
  IFS="
"
      for ce in `cat $OLD_QMASTER_SPOOL/complexes/$c | grep -v "#"`; do
         ce_name=`echo $ce | awk '{ print $1 }'`
         echo $ce | tr " " "\n" | grep "[a-z A-Z 0-9 ==]" >>  /tmp/centry/$ce_name"_tmp"
  IFS="$OLD_IFS"

         for e in `cat /tmp/centry/$ce_name"_tmp"`; do
            case $loop in

               1)
                echo $e | sed 's/^/name /' >> /tmp/centry/$ce_name"_tmp2"
               ;;
               2)
                echo $e | sed 's/^/shortcut   /' >> /tmp/centry/$ce_name"_tmp2"
               ;;
               3)
                echo $e | sed 's/^/type /' >> /tmp/centry/$ce_name"_tmp2"
               ;;
               4)
                echo $e | sed 's/^/value   /' >> /tmp/centry/$ce_name"_tmp2"
               ;;
               5)
                echo $e | sed 's/^/relop   /' >> /tmp/centry/$ce_name"_tmp2"
               ;;
               6)
                echo $e | sed 's/^/requestable   /' >> /tmp/centry/$ce_name"_tmp2"
               ;;
               7)
                echo $e | sed 's/^/consumable /' >> /tmp/centry/$ce_name"_tmp2"
               ;;
               8)
                echo $e | sed 's/^/default /' >> /tmp/centry/$ce_name"_tmp2"
               ;;
               
            esac

            loop=`expr $loop + 1`
         done

         echo "urgency     0" >> /tmp/centry/$ce_name"_tmp2"
         cat /tmp/centry/$ce_name"_tmp2" | grep -v "^value" >> /tmp/centry/$ce_name"_tmp3"

         rm -f /tmp/centry/$ce_name"_tmp"
         rm -f /tmp/centry/$ce_name"_tmp2"
         loop=1

         CE_NAME=`cat /tmp/centry/$ce_name"_tmp3" | grep "name" | awk '{ print $2 }'`
         CE_SHORTCUT=`cat /tmp/centry/$ce_name"_tmp3" | grep "shortcut" | awk '{ print $2 }'`
         CE_TYPE=`cat /tmp/centry/$ce_name"_tmp3" | grep "type" | awk '{ print $2 }' | tr [a-z] [A-Z]`
         CE_RELOP=`cat /tmp/centry/$ce_name"_tmp3" | grep "relop" | awk '{ print $2 }'`
         CE_REQUESTABLE=`cat /tmp/centry/$ce_name"_tmp3" | grep "requestable" | awk '{ print $2 }' | tr [a-z] [A-Z]`
         CE_CONSUMABLE=`cat /tmp/centry/$ce_name"_tmp3" | grep "consumable" | awk '{ print $2 }' | tr [a-z] [A-Z]`
         CE_DEFAULT=`cat /tmp/centry/$ce_name"_tmp3" | grep "default" | awk '{ print $2 }'`
         CE_URGENCY=`cat /tmp/centry/$ce_name"_tmp3" | grep "urgency" | awk '{ print $2 }'`

         rm -f /tmp/centry/$ce_name"_tmp3"

         TOUCH=touch
         UPDATE_LOG=/tmp/update.$pid
         ExecuteAsAdmin $TOUCH $UPDATE_LOG 
         if [ $CE_TYPE = "INT" -o $CE_TYPE = "DOUBLE" -o $CE_TYPE = "MEMORY" -o $CE_TYPE = "TIME" ]; then
            if [ $CE_CONSUMABLE = "YES" -a $CE_REQUESTABLE = "NO" ]; then
               if [ $CE_TYPE = "INT" -o $CE_TYPE = "MEMORY" -o $CE_TYPE = "DOUBLE" ]; then
                  if [ $CE_DEFAULT != "0" ]; then
                     :
                  else
                     CE_DEFAULT=1
                     echo "Changed complex:  " >> $UPDATE_LOG 
                     echo "Name:       $CE_NAME" >> $UPDATE_LOG 
                     echo "Shortcut:   $CE_SHORTCUT" >> $UPDATE_LOG
                     echo "Type:       $CE_TYPE" >> $UPDATE_LOG
                     echo "Relop:      $CE_RELOP" >> $UPDATE_LOG
                     echo "Requestable:$CE_REQUESTABLE" >> $UPDATE_LOG
                     echo "Consumable: $CE_CONSUMABLE" >> $UPDATE_LOG
                     echo "Changed entry:" >> $UPDATE_LOG
                     echo "Default:    $CE_DEFAULT" >> $UPDATE_LOG
                     echo "Urgency:    $CE_URGENCY" >> $UPDATE_LOG 
                     echo "----------------------------" >> $UPDATE_LOG
                     echo >> $UPDATE_LOG
                  fi
               elif [ $CE_TYPE = "TIME" ]; then
                    if [ $CE_DEFAULT != "0:0:0" ]; then
                       :
                    else
                       CE_DEFAULT="0:0:1"
                     echo "Changed complex:  " >> $UPDATE_LOG
                     echo "Name:       $CE_NAME" >> $UPDATE_LOG
                     echo "Shortcut:   $CE_SHORTCUT" >> $UPDATE_LOG
                     echo "Type:       $CE_TYPE" >> $UPDATE_LOG
                     echo "Relop:      $CE_RELOP" >> $UPDATE_LOG
                     echo "Requestable:$CE_REQUESTABLE" >> $UPDATE_LOG
                     echo "Consumable: $CE_CONSUMABLE" >> $UPDATE_LOG
                     echo "Changed entry:" >> $UPDATE_LOG
                     echo "Default:    $CE_DEFAULT" >> $UPDATE_LOG
                     echo "Urgency:    $CE_URGENCY" >> $UPDATE_LOG 
                     echo "----------------------------" >> $UPDATE_LOG
                     echo >> $UPDATE_LOG

                    fi
               fi
               CE_RELOP="<="
               echo "Changed complex:  " >> $UPDATE_LOG
               echo "Name:       $CE_NAME" >> $UPDATE_LOG
               echo "Shortcut:   $CE_SHORTCUT" >> $UPDATE_LOG
               echo "Type:       $CE_TYPE" >> $UPDATE_LOG
               echo "Changed entry:" >> $UPDATE_LOG
               echo "Relop:      $CE_RELOP" >> $UPDATE_LOG
               echo "Requestable:$CE_REQUESTABLE" >> $UPDATE_LOG
               echo "Consumable: $CE_CONSUMABLE" >> $UPDATE_LOG
               echo "Default:    $CE_DEFAULT" >> $UPDATE_LOG
               echo "Urgency:    $CE_URGENCY" >> $UPDATE_LOG 
               echo "----------------------------" >> $UPDATE_LOG
               echo >> $UPDATE_LOG
            fi
         else
            CE_CONSUMABLE="NO"
            echo "Changed complex:  " >> $UPDATE_LOG
            echo "Name:       $CE_NAME" >> $UPDATE_LOG
            echo "Shortcut:   $CE_SHORTCUT" >> $UPDATE_LOG
            echo "Type:       $CE_TYPE" >> $UPDATE_LOG
            echo "Relop:      $CE_RELOP" >> $UPDATE_LOG
            echo "Requestable:$CE_REQUESTABLE" >> $UPDATE_LOG
            echo "Changed entry:" >> $UPDATE_LOG
            echo "Consumable: $CE_CONSUMABLE" >> $UPDATE_LOG
            echo "Default:    $CE_DEFAULT" >> $UPDATE_LOG
            echo "Urgency:    $CE_URGENCY" >> $UPDATE_LOG 
            echo "----------------------------" >> $UPDATE_LOG
            echo >> $UPDATE_LOG

            if [ $CE_TYPE = "BOOL" ]; then
               CE_DEFAULT="0"
               echo "Changed complex:  " >> $UPDATE_LOG
               echo "Name:       $CE_NAME" >> $UPDATE_LOG
               echo "Shortcut:   $CE_SHORTCUT" >> $UPDATE_LOG
               echo "Type:       $CE_TYPE" >> $UPDATE_LOG
               echo "Relop:      $CE_RELOP" >> $UPDATE_LOG
               echo "Requestable:$CE_REQUESTABLE" >> $UPDATE_LOG
               echo "Consumable: $CE_CONSUMABLE" >> $UPDATE_LOG
               echo "Changed entry:" >> $UPDATE_LOG
               echo "Default:    $CE_DEFAULT" >> $UPDATE_LOG
               echo "Urgency:    $CE_URGENCY" >> $UPDATE_LOG 
               echo "----------------------------" >> $UPDATE_LOG
               echo >> $UPDATE_LOG
            elif [ $CE_TYPE = "STRING" -o $CE_TYPE = "CSTRING" -o $CE_TYPE = "RESTRING" -o $CE_TYPE = "HOST" ]; then
               CE_DEFAULT="NONE"
               echo "Changed complex:  " >> $UPDATE_LOG
               echo "Name:       $CE_NAME" >> $UPDATE_LOG
               echo "Shortcut:   $CE_SHORTCUT" >> $UPDATE_LOG
               echo "Type:       $CE_TYPE" >> $UPDATE_LOG
               echo "Relop:      $CE_RELOP" >> $UPDATE_LOG
               echo "Requestable:$CE_REQUESTABLE" >> $UPDATE_LOG
               echo "Consumable: $CE_CONSUMABLE" >> $UPDATE_LOG
               echo "Changed entry:" >> $UPDATE_LOG
               echo "Default:    $CE_DEFAULT" >> $UPDATE_LOG
               echo "Urgency:    $CE_URGENCY" >> $UPDATE_LOG 
               echo "----------------------------" >> $UPDATE_LOG
               echo >> $UPDATE_LOG
            fi
         fi 
         
         if [ $CE_CONSUMABLE = "NO" ]; then
            if [ $CE_TYPE = "STRING" -o $CE_TYPE = "HOST" -o $CE_TYPE = "CSTRING" ]; then
               if [ $CE_RELOP = "==" -o  $CE_RELOP = "!=" ]; then
                  :
               else
                  CE_RELOP="=="
                  echo "Changed complex:  " >> $UPDATE_LOG
                  echo "Name:       $CE_NAME" >> $UPDATE_LOG
                  echo "Shortcut:   $CE_SHORTCUT" >> $UPDATE_LOG
                  echo "Type:       $CE_TYPE" >> $UPDATE_LOG
                  echo "Changed entry:" >> $UPDATE_LOG
                  echo "Relop:      $CE_RELOP" >> $UPDATE_LOG
                  echo "Requestable:$CE_REQUESTABLE" >> $UPDATE_LOG
                  echo "Consumable: $CE_CONSUMABLE" >> $UPDATE_LOG
                  echo "Default:    $CE_DEFAULT" >> $UPDATE_LOG
                  echo "Urgency:    $CE_URGENCY" >> $UPDATE_LOG 
                  echo "----------------------------" >> $UPDATE_LOG
                  echo >> $UPDATE_LOG
               fi
            fi

            if [ $CE_TYPE = "BOOL" -a $CE_RELOP = "==" ]; then
               :
            else
               CE_RELOP="=="
               echo "Changed complex:  " >> $UPDATE_LOG
               echo "Name:       $CE_NAME" >> $UPDATE_LOG
               echo "Shortcut:   $CE_SHORTCUT" >> $UPDATE_LOG
               echo "Type:       $CE_TYPE" >> $UPDATE_LOG
               echo "Changed entry:" >> $UPDATE_LOG
               echo "Relop:      $CE_RELOP" >> $UPDATE_LOG
               echo "Requestable:$CE_REQUESTABLE" >> $UPDATE_LOG
               echo "Consumable: $CE_CONSUMABLE" >> $UPDATE_LOG
               echo "Default:    $CE_DEFAULT" >> $UPDATE_LOG
               echo "Urgency:    $CE_URGENCY" >> $UPDATE_LOG 
               echo "----------------------------" >> $UPDATE_LOG
               echo >> $UPDATE_LOG

            fi   
         fi

         echo "name  $CE_NAME" >> /tmp/centry/$ce_name
         echo "shortcut $CE_SHORTCUT" >> /tmp/centry/$ce_name
         echo "type  $CE_TYPE" >> /tmp/centry/$ce_name
         echo "relop $CE_RELOP" >> /tmp/centry/$ce_name
         echo "requestable $CE_REQUESTABLE" >> /tmp/centry/$ce_name
         echo "consumable  $CE_CONSUMABLE" >> /tmp/centry/$ce_name
         echo "default  $CE_DEFAULT" >> /tmp/centry/$ce_name
         echo "urgency  $CE_URGENCY" >> /tmp/centry/$ce_name
  
      done
   done

#   export SGE_ROOT
#   export SGE_CELL
   CPR="cp -fR"
   RM="rm -fR"
   MV="mv"
   ExecuteAsAdmin $CPR $SGE_ROOT/util/resources/centry/* /tmp/centry
   ExecuteAsAdmin $SPOOLDEFAULTS complexes /tmp/centry
   ExecuteAsAdmin $MV $UPDATE_LOG $QMDIR
   ExecuteAsAdmin $RM /tmp/centry/*
   ExecuteAsAdmin $RM /tmp/centry
    
}



GetOldExecHosts()
{
   pid=$$
   Makedir /tmp/exec_dir.$pid >> /dev/null

   for eh in `ls $OLD_QMASTER_SPOOL/exec_hosts`; do
      ExecuteAsAdmin $TOUCH /tmp/exec_dir.$pid/$eh      
      cat $OLD_QMASTER_SPOOL/exec_hosts/$eh | grep -v "#" | grep -v resource_capability_factor | grep -v complex_list > /tmp/exec_dir.$pid/$eh
      scale=`cat /tmp/exec_dir.$pid/$eh | grep "usage_scaling"` > /dev/null
      if [ "$scale" != "" ]; then
         echo "report_variables           NONE" >> /tmp/exec_dir.$pid/$eh
      else
         echo "report_variables           NONE" >> /tmp/exec_dir.$pid/$eh
         echo "usage_scaling              NONE" >> /tmp/exec_dir.$pid/$eh
      fi
   done
      ExecuteAsAdmin $SPOOLDEFAULTS exechosts /tmp/exec_dir.$pid/ 
      ExecuteAsAdmin $RM /tmp/exec_dir.$pid/*
      rm -fR /tmp/exec_dir.$pid
}



GetOldManagers()
{
   OLD_MANAGERS=`cat $OLD_QMASTER_SPOOL/managers | grep -v "#" | tr "\n" " "`    
   ExecuteAsAdmin $SPOOLDEFAULTS managers $OLD_MANAGERS
}



GetOldOperators()
{
   OLD_OPERATORS=`cat $OLD_QMASTER_SPOOL/operators | grep -v "#" | tr "\n" " "`    
   ExecuteAsAdmin $SPOOLDEFAULTS operators $OLD_MANAGERS
}



GetOldPe()
{
   pid=$$
   Makedir /tmp/pe_dir.$pid >> /dev/null

   if [ -d $OLD_QMASTER_SPOOL/pe ]; then
      for pe in `ls $OLD_QMASTER_SPOOL/pe`; do
         ExecuteAsAdmin $TOUCH /tmp/pe_dir.$pid/$pe      
         cat $OLD_QMASTER_SPOOL/pe/$pe | grep -v "#" | grep -v queue_list > /tmp/pe_dir.$pid/$pe
         echo "urgency_slots     min" >> /tmp/pe_dir.$pid/$pe
      done
         ExecuteAsAdmin $SPOOLDEFAULTS pes /tmp/pe_dir.$pid
         ExecuteAsAdmin $RM /tmp/pe_dir.$pid/*
         rm -fR /tmp/pe_dir.$pid
   fi
}



GetOldProjects()
{
   pid=$$
   Makedir /tmp/prj_dir.$pid >> /dev/null

   if [ -d $OLD_QMASTER_SPOOL/projects ]; then 
      for prj in `ls $OLD_QMASTER_SPOOL/projects`; do
         ExecuteAsAdmin $TOUCH /tmp/prj_dir.$pid/$prj      
         cat $OLD_QMASTER_SPOOL/projects/$prj > /tmp/prj_dir.$pid/$prj
         #echo "default_project NONE" >> /tmp/prj_dir.$pid/$prj
      done
         ExecuteAsAdmin $SPOOLDEFAULTS projects /tmp/prj_dir.$pid
         ExecuteAsAdmin $RM /tmp/prj_dir.$pid/*
         rm -fR /tmp/prj_dir.$pid
   fi
}



AddDefaultQueue()
{
   $INFOTEXT -u "\nCreating the default <all.q> queue and <allhosts> hostgroup"
   echo
   $INFOTEXT -log "Creating the default <all.q> queue and <allhosts> hostgroup"
   TMPL=/tmp/hostqueue$$
   TMPL2=${TMPL}.q
   rm -f $TMPL $TMPL2
   if [ -f $TMPL -o -f $TMPL2 ]; then
      $INFOTEXT "\nCan't delete template files >%s< or >%s<" "$TMPL" "$TMPL2"
   else
      PrintHostGroup @allhosts > $TMPL
      Execute $SGE_BIN/qconf -Ahgrp $TMPL
      Execute $SGE_BIN/qconf -sq > $TMPL
      Execute sed -e "/qname/s/template/all.q/" \
                  -e "/hostlist/s/NONE/@allhosts/" \
                  -e "/pe_list/s/NONE/make/" $TMPL > $TMPL2
      Execute $SGE_BIN/qconf -Aq $TMPL2
      rm -f $TMPL $TMPL2        
   fi

   $INFOTEXT -wait -n "\nHit, <RETURN> to continue!"
   $CLEAR
}



GetOldSubmitHosts()
{
   ExecuteAsAdmin $SPOOLDEFAULTS submithosts $OLD_QMASTER_SPOOL/submit_hosts/
}



GetOldUsers()
{
   pid=$$
   Makedir /tmp/us_dir.$pid >> /dev/null

   if [ -d $OLD_QMASTER_SPOOL/users ]; then
      for us in `ls $OLD_QMASTER_SPOOL/users`; do
         ExecuteAsAdmin $TOUCH /tmp/us_dir.$pid/$us      
         cat $OLD_QMASTER_SPOOL/users/$us > /tmp/us_dir.$pid/$us
         echo "delete_time 0" >> /tmp/us_dir.$pid/$us
      done
         ExecuteAsAdmin $SPOOLDEFAULTS users /tmp/us_dir.$pid
         ExecuteAsAdmin $RM /tmp/us_dir.$pid/*
         rm -fR /tmp/us_dir.$pid
   fi
}



GetOldUsersets()
{  
   pid=$$
   if [ -d $OLD_QMASTER_SPOOL/usersets ]; then
      for uss in `ls $OLD_QMASTER_SPOOL/usersets`; do
         tmp_type=`cat $OLD_QMASTER_SPOOL/usersets/$uss | grep type` > /dev/null
         if [ "$tmp_type" = "" ]; then
            if [ -d /tmp/usersets.$pid ]; then
               :
            else
               mkdir /tmp/usersets.$pid
            fi
            cat $OLD_QMASTER_SPOOL/usersets/$uss > /tmp/usersets.$pid/$uss
            echo "type       ACL" >> /tmp/usersets.$pid/$uss
            echo "oticket    0" >> /tmp/usersets.$pid/$uss
            echo "fshare     0" >> /tmp/usersets.$pid/$uss
         fi    
      done
      if [ "$tmp_type" = "" ]; then
         ExecuteAsAdmin $SPOOLDEFAULTS usersets /tmp/usersets.$pid
         rm -fR /tmp/usersets.$pid
      else
         ExecuteAsAdmin $SPOOLDEFAULTS usersets $OLD_QMASTER_SPOOL/usersets
      fi
   fi
}


GetOldShareTree()
{
   if [ -d $OLD_QMASTER_SPOOL/sharetree ]; then
      ExecuteAsAdmin $SPOOLDEFAULTS sharetree $OLD_QMASTER_SPOOL/sharetree
   fi
}


GetOldFiles()
{
   CP="cp -fR"
   if [ -f $OLD_COMMON_DIR/accounting ]; then
      ExecuteAsAdmin $CP $OLD_COMMON_DIR/accounting $SGE_ROOT/$SGE_CELL/common
   fi

   if [ -f $OLD_COMMON_DIR/qtask ]; then
      ExecuteAsAdmin $CP $OLD_COMMON_DIR/qtask $SGE_ROOT/$SGE_CELL/common
   fi

   if [ -f $OLD_COMMON_DIR/host_aliases ]; then
      ExecuteAsAdmin $CP $OLD_COMMON_DIR/host_aliases $SGE_ROOT/$SGE_CELL/common
   fi
}

UpdateHints()
{
   $INFOTEXT -u "Congratulation, your upgrade was successful!"
   $INFOTEXT -n "\nPlease check your setup, to make sure that everything is\n" \
                "configured well!\n" \
                "Due to new features of our product we added some new configuration\n" \
                "options and we were forced to change some values of complexes.\n" \
                "So, please check your complexes. You will find a updatelog file\n" \
                "within the qmaster spool directory. This file shows you the name and\n" \
                "of cemplexes and the values which have been concerned of the upgrade.\n" \
                "Please log in to your executions hosts now and install the execution daemon\n" \
                "with inst_sge -x -upd. If you're using loadsensor scripts for your execution hosts,\n" \
                "please copy these scripts to the new location and adapt the path \nto the new location!\n" \
                "Thank you very much for using SGE 6.0 and have fun!\n\n" 
}
