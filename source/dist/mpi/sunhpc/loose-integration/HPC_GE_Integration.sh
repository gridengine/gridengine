#!/bin/sh -f
#set -x
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
#  License at http://www.gridengine.sunsource.net/license.html
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
# Loose Integration of Grid Engine (including Enterprise Edition)
# with Sun HPC ClusterTools

#-------------------------------------------------------------------------
# CheckRunEnv  
#
CheckRunEnv()
{
RESPONSE=`$QCONF 2>&1| grep "critical error" | grep -v grep`
if [ "$RESPONSE" != "" ]; then
    echo " "
    echo "Grid Engine Environment is not set up correctly."
    echo "Please set up Grid Engine Environment before running the loose integration script."
    echo " "
    exit 1
fi
RESPONSE=`ps -ef | grep execd | grep -v grep`
if [ "$RESPONSE" = "" ]; then
    echo " "
    echo "Grid Engine execution daemon is not running on this node."
    echo "Please make sure that this node is part of Sun HPC cluster."
    echo "If the answer is yes, this node should be running SGE daemon for execution host."
    echo " "
    exit 1
fi
RESPONSE=`ls -l /opt/SUNWhpc/bin/mpinfo`
if [ "$RESPONSE" = "" ]; then
    echo " "
    echo "Sun ClusterTools' command, mpinfo, is not available."
    echo "Please check if this node is part of Sun HPC Cluster."
    echo " "
    exit 1
else
    RESPONSE2=`/opt/SUNWhpc/bin/mpinfo -N 2>&1 | grep mpinfo | grep -v grep`
    if [ "$RESPONSE2" != "" ]; then
	echo " "
	echo "Sun ClusterTools SW is not setup correctly."
        echo "Please check if this node is part of Sun HPC Cluster."
	echo "Also check if Sun HPC ClusterTools daemons are running."
	echo " "
        exit 1
    fi
fi
}

#-------------------------------------------------------------------------
# ModExecHost 
#
ModExecHost()
{
   ScreenExecHost
#
#  create a temporary exec host definition file
#
   $QCONF -se template > /tmp/template.$$
#
#  create a shell script to put variables in the exec host definition file
#  and replace them with appropriate values later on
#
   eh_template=/tmp/eh_template
   cat > $eh_template.$$.sh <<EOF
#!/bin/sh -f
#set -x
#
# Loose Integration of Grid Engine (including Enterprise Edition)
# with Sun HPC ClusterTools
#
EOF
   $ECHO "cat > \$1 << EOF_QUEUE" >> $eh_template.$$.sh
#
#  Replace some of queue template names with variables
#  Add it to $q_template.$$.sh script
#
   cat /tmp/template.$$ | nawk '
   { line[NR] = $0 }
   END { for (i = 1; i <= NR; i++)
         { n = split(line[i],temp)
           if ( temp[1] == "hostname" ) {
              sub(/template/, "$myeh", line[i]); print line[i] }
           else if  ( temp[1] == "complex_values" ) {
              sub(/NONE/, "slots=$nslots", line[i]); print line[i] }
           else if  ( temp[1] == "load_values" ) { }
           else if  ( temp[1] == "processors" ) { }
           else if  ( temp[1] == "reschedule_unknown_list" ) { } # SGE 3.2.3 fix
           else
               print line[i] 
         } 
       }'  >> $eh_template.$$.sh
   $ECHO EOF_QUEUE >> $eh_template.$$.sh
   chmod u+x $eh_template.$$.sh
#
#
#  Loop over the selected exec hosts
#
   for myeh in `$ECHO $MQLIST_VAL`
   do {
      nslots=`$QCONF -se $myeh | grep processors | nawk '{print $2}'`
#
#     Export variables to be transfered
#
      export myeh
      export nslots
#
#     Translate variables in the shell script for exec host customization 
#     Create /tmp/template.$myeh to customize the exec host
#
      $eh_template.$$.sh /tmp/template.$myeh
#
#     Overwrite the execution host configuration
#
      $QCONF -Me /tmp/template.$myeh
#
#     Clean up temporary files
#
#     /bin/rm /tmp/template.$myeh
   }
   done
#
#  Clean up temporary files
#
#  /bin/rm $eh_template.$$.sh /tmp/template.$$
}

#-------------------------------------------------------------------------
# ScreenExecHost 
#
ScreenExecHost()
{
   clear
   $ECHO ""
   $ECHO "To synchronize Grid Engine queues on an execution host" 
   $ECHO "$LINES------"
   $ECHO "" 
   $ECHO "This process will customize execution hosts with multiple queues:"
   $ECHO "    complex_values             slots=<available_CPUs> " 
   $ECHO ""
   $ECHO "Searching for execution hosts with multiple queues on a Sun HPC cluster"
   $ECHO " ...... " 
#
#  1. Find the list of all queues & exec hosts. (qconf -sql, qconf -sel)
#  2. Find the exec host name of all the queues. 
#  3. Create list of exec host w/ multiple queues
#
    QLIST=`$QCONF -sql`
#
#   This includes too many exec hosts if we have multiple CRE queues
#   Let's use "mpinfo -N" to find exec host on HPC Cluster
#
#     EHLIST=`$QCONF -sel`
    EHLIST=`/opt/SUNWhpc/bin/mpinfo -N | nawk '{print $1}' | grep -v NAME`
    MQLIST=""
#
#   loop over the listed exection hosts
#
    for myeh in `$ECHO $EHLIST`
    do {
       NAPPEAR=""
#
#      loop over the listed queues
#
       for myq in `$ECHO $QLIST`
       do {
          myehname=`qconf -sq $myq | grep hostname | nawk '{print $2}'`
#
#         check if my queue uses the exection host in review
#
          if [ "$myehname" = "$myeh" ]; then
	     if [ "$NAPPEAR" = "" ]; then
		NAPPEAR="YES"
	     elif [ "$NAPPEAR" = "YES" ]; then
		NAPPEAR="MULTIPLE"
                MQLIST="$MQLIST $myeh"
	     fi
          fi
       }
       done
    }
    done
#
#  4. Modify exec hosts with multiple queues 
#
   $ECHO "The list of execution hosts with multiple queues:" 
   $ECHO "$MQLIST"
   $ECHO "" 
   $ECHO "<RETURN> if the current list will be used for the customization. "
   $ECHO "Otherwise, "
   $ECHO "Enter a list of space separated execution host names. >> \c"
   INP=`Enter ""`
   if [ "$INP" = "" ]; then
      MQLIST_VAL=""
      for item in `$ECHO $MQLIST` 
      do { 
         MQLIST_VAL="$MQLIST_VAL $item"
      }
      done
   else
      MQLIST_VAL=$INP
   fi
   $ECHO " " 
   $ECHO "The list of selected execution hosts:"
   $ECHO " " 
   $ECHO "$MQLIST_VAL"     
   $ECHO " " 
}

#-------------------------------------------------------------------------
# GetEHList 
#
GetEHList()
{
   clear
   $ECHO ""
   $ECHO "To set up a Parallel Environment (PE) for Sun HPC ClusterTools" 
   $ECHO "$LINES------"
   $ECHO "" 
   $ECHO "List of execution hosts running Sun HPC ClusterTools "
   $ECHO "should be provided to set up the PE, >$HPC_VAL<" 
   $ECHO "" 
#
#  Get execution host for Sun HPC ClusterTools installation
   EXHOST_LIST=`/opt/SUNWhpc/bin/mpinfo -N | nawk '{print $1}' | grep -v NAME`
   $ECHO "The list of currently available execution hosts: " 
   $ECHO " " 
   $ECHO "$EXHOST_LIST" 
   $ECHO " " 

   $ECHO "<RETURN> if the current list will be used for the PE. "
   $ECHO "Otherwise, "
   $ECHO "Enter a list of space separated execution host names. >> \c"
   INP=`Enter ""`
   if [ "$INP" = "" ]; then
      EXHOST_LIST_VAL=""
      for item in `$ECHO $EXHOST_LIST` 
      do { 
         EXHOST_LIST_VAL=`$ECHO $EXHOST_LIST_VAL $item` 
      }
      done
   else
      EXHOST_LIST_VAL=$INP
   fi
   $ECHO " " 
   $ECHO "The list of selected execution hosts:"
   $ECHO " " 
   $ECHO "$EXHOST_LIST_VAL"
     
   $ECHO " " 
   WaitClear clear
}

#-------------------------------------------------------------------------
# GenUserGroup
#
GenUserGroup()
{
   clear
   hpc_group="HPCusers"
   $ECHO ""
   $ECHO "To do customization for HPC ClusterTools Users Group:"
   $ECHO "$LINES------"
   $ECHO ""
   $ECHO "Create an exclusive user access list (ACL) and its members." 
   $ECHO "Or choose one from the existing ACL lists" 
   $ECHO "" 
   $ECHO "The list of currently available user group lists:" 
   $ECHO " " 
   USER_GROUP_LISTS=`$QCONF -sul`
   if [ "$USER_GROUP_LISTS" != "" ]; then
      $ECHO $USER_GROUP_LISTS
   fi  
   $ECHO "" 
   $ECHO "Enter the name of the group list "
   $ECHO "or hit <RETURN> to use the default name, >$hpc_group< >> \c"
   INP=`Enter ""`

   if [ "$INP" != "" ]; then
      hpc_group=$INP
   fi
   if [ "`$QCONF -sul 2>/dev/null | grep $hpc_group`" != "" ]; then
      $ECHO "" 
      $ECHO "The user group list, >$hpc_group< exists. "
      $ECHO "Use it for HPC ClusterTools user group"
     $ECHO "" 
   else
      $ECHO "" 
      $ECHO "The user group list, >$hpc_group< does not exist."
#
#     List user names of the user group
#
      done=false
      while [ $done = false ]; do
         $ECHO "Enter comma separated user names for the group >> \c"
         INP=`Enter ""`
         if [ "$INP" = "" ]; then
            $ECHO "" 
            $ECHO "You should provide the comma separated user list." 
            $ECHO "" 
         else
            hpc_members=$INP
     	    done=true
         fi
      done    
#
#     Generate users information file
#     Use Grid Engine qconf -Au command
#
      if [ $QSYST = "SGE" ]; then
         $ECHO "name       $hpc_group" > /tmp/$hpc_group
         $ECHO "entries    $hpc_members" >> /tmp/$hpc_group
      elif [ $QSYST = "SGEEE" ]; then
         $ECHO "name       $hpc_group" > /tmp/$hpc_group
         $ECHO "type       ACL" >> /tmp/$hpc_group
         $ECHO "oticket    0" >> /tmp/$hpc_group
         $ECHO "fshare     0" >> /tmp/$hpc_group
         $ECHO "entries    $hpc_members" >> /tmp/$hpc_group
       fi
       $QCONF -Au /tmp/$hpc_group
       /bin/rm /tmp/$hpc_group
   fi
#
   WaitClear clear
}

#-------------------------------------------------------------------------
# GetQList
#
GetQList()
{
   clear
   $ECHO ""
   $ECHO "To set up a Parallel Environment (PE) for Sun HPC ClusterTools" 
   $ECHO "$LINES------"
   $ECHO "" 
   $ECHO "List of queues for execution hosts running HPC ClusterTools " 
   $ECHO "will be automatically generated by the script." 
   $ECHO "" 
   $ECHO "To avoid any problems with queues for non-parallel jobs, " 
   $ECHO "the queue name will be <hostname>.cre for each execution host." 
   $ECHO "" 
   $ECHO "To change the naming scheme, look for GetQList() shell function." 
   $ECHO "" 

   q_ext=cre
#
#  Get queue list from the installation 
#
   QUEUE_LIST_VAL=""
   for item in `$ECHO $EXHOST_LIST_VAL` 
   do { 
      QUEUE_LIST_VAL=`$ECHO $QUEUE_LIST_VAL $item.$q_ext` 
      }
   done
# 
   $ECHO "The list of available queues for Sun HPC ClusterTools:"
   $ECHO " " 
   $ECHO "$QUEUE_LIST_VAL"
     
   $ECHO " " 
   WaitClear clear
}

#-------------------------------------------------------------------------
# GenQueues
#
# Things added to default queue for HPC ClusterTools:
# - Put "PARALLEL" to qtype in queue definitions of the listed queue of the PE.
# - Define "suspend_method" and "terminate_method".
#
GenQueues()
{
   clear
   $ECHO ""
   $ECHO "To set up a Parallel Environment (PE) for Sun HPC ClusterTools" 
   $ECHO "$LINES------"
   $ECHO "" 
   $ECHO "Create exclusive queues for Sun HPC ClusterTools" 
   $ECHO "" 
#
#  Get queue list from the installation 
#
   for queue in `$ECHO $QUEUE_LIST_VAL` 
   do { 
      q_template=/tmp/$queue
      exhost=`basename $queue .$q_ext`
      nslots=`/opt/SUNWhpc/bin/mpinfo -N | grep $exhost | nawk '{print $6}'`
#
#     Without the given qname, qconf -sq will show the default values
#
      $QCONF -sq > $q_template
#
#     Creat a shell script to convert all the shell variables into the absolute value
#
      cat > $q_template.$$.sh <<EOF
#!/bin/sh -f
#set -x
#
# Loose Integration of Grid Engine (including Enterprise Edition)
# with Sun HPC ClusterTools
#
EOF
echo "cat > \$1 << EOF_QUEUE" >> $q_template.$$.sh
#
#     Replace some of queue template names with variables
#     Add it to $q_template.$$.sh script
#
      cat $q_template | nawk '
         { line[NR] = $0 }
         END { for (i = 1; i <= NR; i++)
	       { n = split(line[i],temp)
                 if ( temp[1] == "qname" ) {
                    sub(/template/, "$queue", line[i]); print line[i] }
                 else if  ( temp[1] == "hostname" ) {
                    sub(/unknown/, "$exhost", line[i]); print line[i] }
                 else if  ( temp[1] == "qtype" ) {
                    print line[i] " PARALLEL" }
                 else if  ( temp[1] == "slots" ) {
                    sub(/1/, "$nslots", line[i]); print line[i] }
                 else if  ( temp[1] == "suspend_method" ) {
                    sub(/NONE/, "$QSYST_ROOT_VAL/mpi/sunhpc/loose-integration/suspend_sunmpi.sh", line[i]); print line[i] }
                 else if  ( temp[1] == "resume_method" ) {
                    sub(/NONE/, "$QSYST_ROOT_VAL/mpi/sunhpc/loose-integration/resume_sunmpi.sh", line[i]); print line[i] }
                 else if  ( temp[1] == "terminate_method" ) {
                    sub(/NONE/, "$QSYST_ROOT_VAL/mpi/sunhpc/loose-integration/terminate_sunmpi.sh", line[i]); print line[i] }
                 else if  ( temp[1] == "complex_list" ) {
                    sub(/NONE/, "$DEFAULT_COMPLEX", line[i]); print line[i] }
                 else if  ( temp[1] == "complex_values" ) {
                    sub(/NONE/, "$DEFAULT_SHORTCUT=true", line[i]); print line[i] }
                 else
                     print line[i] 
               } 
             }'  >> $q_template.$$.sh
echo EOF_QUEUE >> $q_template.$$.sh
#
#     Executable shell script to create exclusive queue for HPC ClusterTools
#
      /bin/rm $q_template
      chmod u+x $q_template.$$.sh
#
#     Export variables to be transfered
#
      export queue
      export exhost
      export nslots
      export QSYST_ROOT_VAL
      export DEFAULT_COMPLEX
      export DEFAULT_SHORTCUT
#
#     Translate variables in the queue template 
#     Recreate $q_template to customize the queue
#
      $q_template.$$.sh $q_template 
#
#     Add the queue
#
      qconf -Aq $q_template
      /bin/rm $q_template  $q_template.$$.sh
      }
   done
# 
   $ECHO " " 
     
   WaitClear clear
}

#-------------------------------------------------------------------------
# GetPESlots
#
GetPESlots()
{
   clear
   $ECHO ""
   $ECHO "To set up a Parallel Environment (PE) for Sun HPC ClusterTools" 
   $ECHO "$LINES------"
   $ECHO "" 
   $ECHO ""
   $ECHO "Parallel Environment (PE), >$HPC_VAL< needs to set"
   $ECHO "the number of available slots (CPUs)." 
   $ECHO "" 

   NCPUS=`/opt/SUNWhpc/bin/mpinfo -N | nawk '{ line[NR] = $0 }
                                             END { sum = 0
				                 for (i = 2; i <= NR; i++)
				                 { n = split(line[i],temp)
                                                   sum = sum + temp[6]
                                                 }
                                                 print sum
                                             }'`
   $ECHO "Based on the current HPC ClusterTools installation,"
   $ECHO "The total number of available CPUs is $NCPUS "
   $ECHO "" 
   $ECHO "Hit <RETURN> to use all the available CPUs for the PE, >$HPC_VAL<"
   $ECHO "Or enter the number less than or equal to $NCPUS  >> \c"
   INP=`Enter ""`
   if [ "$INP" = "" ]; then
      eval HPC_SLOTS=$NCPUS
   else
      eval HPC_SLOTS=$INP
   fi

   $ECHO "" 
   $ECHO "The number of CPUs for the PE, >$HPC_VAL< is $HPC_SLOTS."
   $ECHO "" 
   WaitClear clear
}

#-------------------------------------------------------------------------
# GenComplex
#
GenComplex()
{
   clear
   DEFAULT_COMPLEX=cre_exclusive
   DEFAULT_SHORTCUT=cre
   $ECHO ""
   $ECHO "To set up a exclusive complex for Sun HPC ClusterTools" 
   $ECHO "$LINES------"
   $ECHO "" 
   $ECHO "Default complex name: >$DEFAULT_COMPLEX<"
   $ECHO "Default complex shortcut name: >$DEFAULT_SHORTCUT<"
   $ECHO ""  
   $ECHO "Enter a complex name or "
   $ECHO "<RETURN> to use the default complex name, >$DEFAULT_COMPLEX< >> \c"   
   INP=`Enter ""`
   if [ "$INP" = "" ]; then
      DEFAULT_COMPLEX=cre_exclusive
   else
      DEFAULT_COMPLEX=$INP
   fi
   $ECHO ""  
   $ECHO "Enter a complex shortcut name or"
   $ECHO "<RETURN> to use the default, >$DEFAULT_SHORTCUT< >> \c"   
   INP=`Enter ""`
   if [ "$INP" = "" ]; then
      DEFAULT_SHORTCUT=cre 
   else
      DEFAULT_SHORTCUT=$INP
   fi
#
#  Create a template file for the exclusive complex
#
   cre_complex=/tmp/cre_complex
   cat > $cre_complex<<EOF   
#name            shortcut   type   value           relop requestable consumable default
#--------------------------------------------------------------------------------------
$DEFAULT_SHORTCUT              $DEFAULT_SHORTCUT        BOOL   true            ==    FORCED      NO         false
#--- # starts a comment but comments are not saved across edits -----------------------
EOF
   $ECHO ""  
#
#  Create the exclusive complex
#
   qconf -Ac $DEFAULT_COMPLEX $cre_complex
   /bin/rm $cre_complex

   $ECHO ""  
   WaitClear clear
}

#-------------------------------------------------------------------------
# GetPEName
#
GetPEName()
{
   clear
   $ECHO ""
   $ECHO "To set up a Parallel Environment (PE) for Sun HPC ClusterTools" 
   $ECHO "$LINES------"
   $ECHO "" 
   $ECHO "Default Parallel Environment (PE) name: >$HPC<"
   $ECHO "" 
   $ECHO "To set up a different PE name,"  
   $ECHO "Enter a PE name or <RETURN> to use the default PE name, >$HPC< >> \c"
   
   INP=`Enter ""`
   if [ "$INP" = "" ]; then
      HPC=hpc
   else
      HPC=$INP
   fi
#
#  Check if the PE, $HPC already exists?
#
   PE_OVERWRITE=NO
   if [ "`$QCONF -spl 2>/dev/null | grep $HPC`" = $HPC ]; then
      $ECHO "" 
      $ECHO "The PE, >$HPC< already exists."
      $ECHO "Do you want to overwrite it? \c "
      YesNo " " y
      if [ $? = 0 ]; then
#        Overwrite it
         PE_OVERWRITE=YES
      else
#        Name a new one
         $ECHO "" 
         $ECHO "Enter a new PE name >> \c"
         INP=`Enter ""`
         if [ "$INP" = "" ]; then
            $ECHO "You MUST give a new PE name."
            $ECHO "" 
      	    WaitClear clear
            GetPEName
         else
            HPC=$INP
         fi
      fi
   fi
   HPC_VAL=$HPC
   $ECHO "" 
   if [ $HPC_VAL = hpc ]; then
      $ECHO "Using default parallel environment: >hpc<"
   else
      $ECHO "Using a parallel environment: >$HPC_VAL<"
   fi
     
   $ECHO "" 
   WaitClear clear
}

#-------------------------------------------------------------------------
# GenPE
#
GenPE()
{
PE_FILE=/tmp/pefile.$$
cat > $PE_FILE <<EOF
pe_name           $HPC_VAL
queue_list        $QUEUE_LIST_VAL
slots             $HPC_SLOTS
user_lists        $hpc_group
xuser_lists       NONE
start_proc_args   $QSYST_ROOT_VAL/mpi/sunhpc/loose-integration/startsunmpi.sh \$pe_hostfile /opt/SUNWhpc/bin
stop_proc_args    $QSYST_ROOT_VAL/mpi/sunhpc/loose-integration/stopsunmpi.sh
allocation_rule   \$fill_up
control_slaves    FALSE
job_is_first_task FALSE
EOF
#
# Use Sge command to add a PE
#
if [ $PE_OVERWRITE = "YES" ]; then
    $QCONF -dp $HPC_VAL
fi
$QCONF -Ap $PE_FILE
/bin/rm $PE_FILE

$ECHO " "
$ECHO "Parallel Environment definition has been generated. "
$ECHO " "
$ECHO "The PE configuration file is as follows: "
$ECHO "   $HPC_VAL"
$ECHO " "
$ECHO "Contents of $HPC_VAL:"
$ECHO " "
$QCONF -sp $HPC_VAL
$ECHO " "
$ECHO " "
$ECHO "To create PEs for other allocation rules (\$round_robin, \$pe_slots, 1),"
$ECHO "use \"qconf -ap <pe_name>\" to open an edit window with a default PE"
$ECHO "configuration.Then, customize it according to a site-specific "
$ECHO "Grid Engine installation and an allocation rule."
$ECHO " "

WaitClear clear
}

#-------------------------------------------------------------------------
# GetRootDir
#
GetRootDir()
{
#      clear
   $ECHO ""
   $ECHO "$QSYST installation root directory"
   $ECHO "$LINES------"
   $ECHO "" 
   $ECHO "To set up a loose integration between Sun HPC ClusterTools"
   $ECHO "and >$QSYST<, the installation root directory path should be known in advance."
   $ECHO "" 
   $ECHO "   $QSYST_ROOT=your_install_path" 
   $ECHO "" 
      
   $ECHO "Enter root path or <RETURN> to use default root path >$QSYST_ROOT_DEFAULT< >> \c"
   INP=`Enter ""`
   if [ "$INP" = "" ]; then
      eval $QSYST_ROOT=$QSYST_ROOT_DEFAULT
   else
      eval $QSYST_ROOT=$INP
   fi

   QSYST_ROOT_VAL=`eval echo '$'$QSYST_ROOT`   

   $ECHO "" 
   if [ $QSYST_ROOT_VAL = /gridware/sge ]; then
      $ECHO "Using default root path: >/gridware/sge<"
   else
      $ECHO "Using root path: >$QSYST_ROOT_VAL<"
   fi
     
   $ECHO "" 
   WaitClear clear
}

#-------------------------------------------------------------------------
# GetCell
#
GetCell()
{
   if [ $fast = true ]; then
      eval $QSYST_CELL=default
      QSYST_CELL_VAL=default
      return
   fi

   clear
   $ECHO ""
   $ECHO "$QSYST cells"  
   $ECHO "$LINES------"
   $ECHO "" 
   $ECHO  "$QSYST supports multiple cells." 
   $ECHO "" 
   $ECHO "If you are not planning to run multiple $QSYST clusters or if you don't" 
   $ECHO "know yet what is a $QSYST cell it is safe to keep the default cell name" 
   $ECHO "" 
   $ECHO "   \"default\"" 
   $ECHO "" 
   $ECHO "If you want to run multiple cells you can enter a cell name now."  
   $ECHO "Note that you have to set the environment variable" 
   $ECHO "" 
   $ECHO "   $QSYST_CELL=your_cell_name" 
   $ECHO "" 
   $ECHO "for all further $QSYST commands." 
   $ECHO ""

   $ECHO "Enter cell name or <RETURN> to use default cell >default< >> \c"
   INP=`Enter ""`
   if [ "$INP" = "" ]; then
      eval $QSYST_CELL=default
   else
      eval $QSYST_CELL=$INP
   fi

   QSYST_CELL_VAL=`eval echo '$'$QSYST_CELL`   

   $ECHO ""
   if [ $QSYST_CELL_VAL = default ]; then
      $ECHO "Using default cell: >default<"
   else
      $ECHO "Using cell: >$QSYST_CELL_VAL<"
   fi
     
   $ECHO ""
   WaitClear clear
}

#-------------------------------------------------------------------------
# GetProdType
#
GetProdType()
{
 clear
 $ECHO ""
 $ECHO "Loose Integration of Grid Engine (including Enterprise Edition)"
 $ECHO " with Sun HPC ClusterTools" 
 $ECHO "$LINES------"
 $ECHO "" 
 $ECHO "The integration script uses Grid Engine and Sun HPC ClusterTools commands."
 $ECHO "Both software should be installed before running this script."
 $ECHO ""
 $ECHO "To install a loose integration package for Sun HPC ClusterTools," 
 $ECHO "Administrator of Grid Engine MUST execute this script "
 $ECHO "on a execution host running Sun HPC ClusterTools." 
 $ECHO "" 
 $ECHO ""

 $ECHO "Ready?\c"
 YesNo " " y
 if [ $? = 0 ]; then
    PROD_NAME=SGE
    PROD_NAME2="Grid Engine"
#
#   Get the product name: SGE (Grid Engine) or SGEEE (Grid Engine, Enterprise Edition)
#
   $ECHO " "
   $ECHO "Enter the product ID [1 or 2]:"
   $ECHO "  1: Grid Engine "
   $ECHO "  2: Grid Engine, Enterprise Edition >> \c"
   INP=`Enter ""`
   if [ "$INP" = "1" ]; then
      PROD_NAME=SGE
      PROD_NAME2="Grid Engine"
   elif [ "$INP" = "2" ]; then
      PROD_NAME=SGEEE
      PROD_NAME2="Grid Engine, Enterprise Edition"
   else
      $ECHO " "
      $ECHO "Input should be the product ID (1: Grid Engine, 2: Grid Engine, Enterprise Edition)"
      $ECHO "Try it again. "
      $ECHO " "
      exit 1
   fi
#
    $ECHO " "
    $ECHO "Loose integration of >$PROD_NAME2< with Sun HPC ClusterTools"
    $ECHO " "
           WaitClear clear
           GetProd_done=true
 fi

if [ $PROD_NAME = SGE ]; then
   inst=sge
   QSYST=SGE
   QSYST_NAME=SGE
   LINES=------
   BLANKS=""
   QSYST_ROOT=SGE_ROOT
   QSYST_ROOT_DEFAULT="/gridware/sge"
   QSYST_CELL=SGE_CELL
   QSYST_MASTER_NAME=sge_qmaster
   QSYST_EXECD_NAME=sge_execd
   QSYST_SCHEDD_NAME=sge_schedd
   QSYST_SHEPHERD_NAME=sge_shepherd
   QSYST_QSTD_NAME=sge_qstd
   QSYST_SHADOWD_NAME=sge_shadowd
   QSYST_COMMD_NAME=sge_commd
   QSYST_COMMDCNTL_NAME=sgecommdcntl
   QSYST_SERVICE=sge_commd
   QSYST_PREFIX=sge
   BATCH_SCRIPT=sge.batch.script
elif [ $PROD_NAME = SGEEE ]; then
   inst=sge
   QSYST=SGEEE
   QSYST_NAME=SGEEE
   LINES=---
   BLANKS="   "
   QSYST_ROOT=SGE_ROOT
   QSYST_ROOT_DEFAULT="/gridware/sge"
   QSYST_CELL=SGE_CELL
   QSYST_MASTER_NAME=sge_qmaster
   QSYST_EXECD_NAME=sge_execd
   QSYST_SCHEDD_NAME=sge_schedd
   QSYST_SHEPHERD_NAME=sge_shepherd
   QSYST_QSTD_NAME=sge_qstd
   QSYST_SHADOWD_NAME=sge_shadowd
   QSYST_COMMD_NAME=sge_commd
   QSYST_COMMDCNTL_NAME=sgecommdcntl
   QSYST_SERVICE=sge_commd
   QSYST_PREFIX=sge
   BATCH_SCRIPT=sge.batch.script
else
   echo
   echo
   exit 1
fi

}

#-------------------------------------------------------------------------
# WaitClear
#
WaitClear()
{
   do_clear=$1
   text="$2"
 
   if [ "$text" != "" ]; then
      Write "$text"
   else
      $ECHO "Hit <RETURN> to continue >> \c"
   fi

   if [ $autoinst = true ]; then
      echo
   else      
      read INP_WaitClear

      if [ $do_clear = clear ]; then
         clear
      fi
   fi   
}

#-------------------------------------------------------------------------
# Write: print arguments, one per line
#
Write()
{
   ARGC=$#
   while [ $ARGC != 0 ]; do
      $ECHO "$1"
      shift
      ARGC=`expr $ARGC - 1`
   done
}

#-------------------------------------------------------------------------
# Enter: input returned to stdout. If input is empty echo $1
#
Enter()
{
   if [ $autoinst = true ]; then
      $ECHO $1
   else
      read INP
      if [ "$INP" = "" ]; then
         $ECHO $1
      else
         $ECHO $INP
      fi
   fi
}

#-------------------------------------------------------------------------
# YesNo: print argument and get YES/NO answer 
# $1 = string to print
# $2 = default value if user enters return. Values are y,n,x (for no default)
#
YesNo()
{
   text=$1
   text="$text (y/n)"
   default=$2 
   if [ "$default" = "" ]; then
      default=x
      text="$text >> \c"
   else
      text="$text [$default] >> \c" 
   fi

   if [ $autoinst = true -a $default != x -a $default != X ]; then
      Write "$text"
      echo $default
      if [ $default = y -o $default = Y ]; then
         return 0
      else
         return 1
      fi
   fi   
   
   YesNo_done=false
   while [ $YesNo_done = false ]; do
      Write "$text"
      read YesNo_INP

      if [ "$YesNo_INP" = "" ]; then
         if [ "$default" = y -o "$default" = Y ]; then
            YesNo_INP=y
         elif [ "$default" = n -o "$default" = N ]; then
            YesNo_INP=n
         fi
      fi

      case "$YesNo_INP" in
         y|y|j|J)
            YesNo_done=true
            ret=0
            ;;
         n|N)
            YesNo_done=true
            ret=1
            ;;
      esac
   done
   return $ret
}

#--------------------------------------------------------------------------
# Translate
#     $1 string to internationalize
#  if $2 is 1 - use $ECHO for output to stdout
#  if $2 is 2 - use $ECHO for output to stderr
#  if $2 is "" - don't use $ECHO, just save it in variable "transout"
#  if $3 is 1 - make new line
Translate()
{
   if [ "$translation" = "1" ]; then
      transout=`$TRANSLATE "$1"`
      if [ "$transout" = "" ]; then
         transout="$1"
      fi
   else
      transout="$1"
   fi

   if [ "$4" != "" ]; then
      expression="`eval echo $4`"
      transout=`echo "$transout" | sed -e s?%s?"$expression"?g`
   fi

   if [ "$2" = "1" ]; then
       $ECHO "$transout \c" 
   fi

   if [ "$2" = "2" ]; then
       $ECHO "$transout \c" >&2
   fi

   if [ "$3" = "1" ]; then
       $ECHO
   fi

}


#--------------------------------------------------------------------------
# THE MAIN PROCEDURE
#--------------------------------------------------------------------------

ECHO=/usr/5bin/echo
#-----------------------------
# CommandLine Argument Parsing
#
WHICH="undef"
strict=true
fast=false
autoinst=false
addqueue=true

#-----------------------------------
# FIND ARCH + ARCH SPECIFIC DEFAULTS
#
echo " " 
echo "Loose Integration between Grid Engine and Sun ClusterTools" 
echo " " 

PROD_NAME=UNKNOWN
GetProdType
#
# Obtain Root installation dir and cell name
#
GetRootDir
GetCell
#
# Need to find arch type
#
#if [ `isainfo -b` = 64 ]; then
# isainfo is not available on Solaris 2.6.
#
if [ "`isalist | grep sparcv9`" != "" ]; then
         ARCH=solaris64
else
         ARCH=solaris
fi
QCONF=$QSYST_ROOT_VAL/bin/$ARCH/qconf

#
# Check if Grid Engine or Sun ClusterTools are running
#
CheckRunEnv  
#
# Define PE name for Sun HPC ClusterTools
# Default PE is >hpc<
#
HPC=hpc
GetPEName
#
# Create exclusive complex for Sun HPC ClusterTools
#
GenComplex
#
# Get the list of execution hosts running Sun HPC ClusterTools
#
GetEHList
#
# Get the list of parallel queues available on execution hosts
#
GetQList
#
# Create a user group list for Sun HPC ClusterTools users
# 
$ECHO " "
$ECHO "Create Access List (ACL) for Sun HPC ClusterTools"
$ECHO "$LINES------"
$ECHO "" 
$ECHO "Only users in the ACL can use Sun HPC ClusterTools if you create one."
$ECHO " "
$ECHO "Do you want to create an ACL for Sun HPC ClusterTools ?\c"
YesNo " " n
if [ $? = 0 ]; then
   GenUserGroup
else
   hpc_group="NONE"
fi
#
# Get the total number of slots available for the PE
#
GetPESlots
#
# Create default queues for PE
#
GenQueues
#
# Generate Parallel Environment
#
GenPE 
#
# Distribute scripts
#
# /bin/cp startsunmpi.sh $QSYST_ROOT_VAL/mpi/sunhpc/loose-integration
# /bin/cp stopsunmpi.sh $QSYST_ROOT_VAL/mpi/sunhpc/loose-integration
# /bin/cp suspend_sunmpi.sh $QSYST_ROOT_VAL/mpi/sunhpc/loose-integration
# /bin/cp resume_sunmpi.sh $QSYST_ROOT_VAL/mpi/sunhpc/loose-integration
# /bin/cp terminate_sunmpi.sh $QSYST_ROOT_VAL/mpi/sunhpc/loose-integration
# /bin/cp sunmpi.pe.template $QSYST_ROOT_VAL/mpi/sunhpc/loose-integration
# /bin/cp batch.sunmpi.template $QSYST_ROOT_VAL/mpi/sunhpc/loose-integration
# /bin/cp MPRUN $QSYST_ROOT_VAL/mpi/sunhpc/loose-integration
# /bin/cp PRISM $QSYST_ROOT_VAL/mpi/sunhpc/loose-integration

$ECHO " "
$ECHO " "
$ECHO "Loose Integration between Sun HPC ClusterTools" 
$ECHO "and $PROD_NAME2 is completed." 
$ECHO "To submit a batch job, customize a batch script template file, "
$ECHO "batch.sunmpi.template located at $QSYST_ROOT_VAL/mpi directory"
$ECHO " "
$ECHO " "
$ECHO ">> WARNING <<"
$ECHO " "
$ECHO "If there are more than one queues for an execution host"
$ECHO "as a result of adding the exclusive CRE queue, "
$ECHO "synchronize the queues for an execution host to prevent "
$ECHO "overloading the execution host. "
$ECHO " "
$ECHO "This can be done by defining consumable slots for host objects."
$ECHO "Use qconf -me <hostname> and replace NONE on the line "
$ECHO "for complex_values with <slots=available_cpus>."
$ECHO " "
#
# Queue Synchronization on execution hosts having multiple queues
#
$ECHO "Warning: "
$ECHO "If you choose yes,"
$ECHO "All HPC nodes with multiple queues will be synchronized."
$ECHO " "
$ECHO "Do you want to do queue synchronizaiton now?\c"
YesNo " " y
if [ $? = 0 ]; then
   ModExecHost
else
   echo
   exit 1
fi
