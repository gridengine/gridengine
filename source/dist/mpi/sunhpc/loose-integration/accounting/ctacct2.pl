#!/usr/bin/perl
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
# This is a script to collect accounting information for 
# MPI application executed on Cluster Runtime Environment (CRE)
# of Sun ClusterTools software through Sun Grid Engine. 
# The Solaris System Accounting (man acct(1M)) is used to record
# the accounting information.
#
# This script should run within the stopsunmpi.sh
#
# $JOB_ID -> SGE job id
# $USER   -> user name                          (done)
# $TMPDIR/machines  -> list of hosts            (done)
# $TMPDIR/mprun_arg -> mprun command arguments  (done)
# $SGE_JOB_SPOOL_DIR/usage -> start & end times (done)
#
# Usage: 
# ctacct2.pl $JOB_ID $USER $TMPDIR $SGE_JOB_SPOOL_DIR $DEBUG
#
# Input files:
# $TMPDIR/machines
# $TMPDIR/mprun_arg
# $SGE_JOB_SPOOL_DIR/usage
#
use Time::Local;
use Time::localtime;
$[ = 1;			# set array base to 1

$JOB_ID      = $ARGV[1];
$User_name   = $ARGV[2];
$TMPDIR      = $ARGV[3];
$SGE_JOB_SPOOL_DIR = $ARGV[4];
$DEBUG       = $ARGV[5];
#
if ($DEBUG == 1){
  open(DEBUGOUT, "> /tmp/debugout");
}
#
# processing $TMPDIR/machines file to create the node list
#
open(RANKMAP, "< $TMPDIR/machines");
$count = 0;
while(<RANKMAP>) {
  @fields = split (/\s+/, $_);
  $count++;
  $nodes[$count] = $fields[1];
}
close (RANKMAP);
#
# Remove repeated nodes
#
$count = 0;
foreach $Node (@nodes) {
  $count++;
  if ( $count == 1 ) { # Record the first node
    $uniq_nodes[1] = $Node;
    $uniq_number = 1;
  } 
  else {
    $temp = $uniq_number;
    $duplicated = 0;
    while ($temp > 0) { # Loop over the unique nodes
      if ( $Node ne $uniq_nodes[$temp] ) {
	$temp--;
      } 
      else { # Found the node in the unique node list
	$duplicated = 1;
	$temp--;
      }
    }
    if ($duplicated == 0) { # If it's a unique node, add it.
      $uniq_number++;
      $uniq_nodes[$uniq_number] = $Node;
    }
  }
}
$Total_nodes=$uniq_number;
#
# processing $TMPDIR/mprun_arg file to get the first executable 
# (what happens if there are spawned processes?)
#
open(ARGMAP, "< $TMPDIR/mprun_arg");
while (defined($line = <ARGMAP>) ) {
  chomp $line;
  if ($line =~ s/\\$/ /) { # Replace a back slash with a blank space
    $line .= <ARGMAP>;
    redo unless eof(ARGMAP); # Create a single line if there's back slash
  }
  @ARG_LIST=split (/\s+/,$line);
  $Found = 0;
  $count = 0;
  $Args_to_skip = 0;
  while (!$Found) { # Do until find the executable name
    $count++;
    $item = $ARG_LIST[$count];
    # DO THIS ONLY IF $Args_to_skip is set to zero.
    #
    if ($Args_to_skip == 0) {
      # For mprun options without any following arg
      # HPC CT 4
      if ( $item eq "-h" || $item eq "-V"  || $item eq "-J" ||
	   $item eq "-W" || $item eq "-S"  || $item eq "-i" ||
	   $item eq "-o" || $item eq "-D"  || $item eq "-N" ||
	   $item eq "-n" || $item eq "-Ns" || $item eq "-Ys" ||
	   $item eq "-u" ) {
	# check if the following element doesn't start with "-"
	if ( substr ($ARG_LIST[$count+1],1,1) ne "-" ) {
	  # possible binary name
	  $Found = 1;
	  $Command_name = $ARG_LIST[$count+1];
	}
      }
      # For "-R" or "-l" option with mprun
      elsif( $item eq "-R" || $item eq "-l" ) {
	# $Args_to_skip = 1; # How many args?
	$End_reached = 0;
	$mycount = $count+1;
	while (!$End_reached) {
	  $Args_to_skip++;
	  # Check the last char of the curr word is quatation mark
	  if ( substr ($ARG_LIST[$mycount],-1) eq "\"" ) {
	    $End_reached = 1;
	  }
	  $mycount++;
	}
      }
      # For all other mprun options requiring another arg after that.
      else {
	$tempstr=substr ($item,1,1);
	if ($Found == 0 && ($tempstr eq "-")) {
	  $Args_to_skip = 1;
	}
	else {
	  $Found = 1;
	  $Command_name = $item;
	}
      }
    }
    # For $Args_to_skip > 0
    else {
      # Now decrease $Args_to_skip by one
      if ($Args_to_skip > 0) {
	$Args_to_skip--;
      }
    }
  }
}
#
# Get the basename of the binary (remove any paths)
#
$Command_name =~ s#.*/##s; 
#
# if longer than 8 chars, chop the rest (acctcom limit)
#
@charcmds=split(//,$Command_name);
if ($#charcmds > 8) {
  $Command_name ="";
  foreach (1..8) {$Command_name .= $charcmds[$_];}
}

if ($DEBUG == 1) {
  print DEBUGOUT "SGE Job ID: ",$JOB_ID,"\n";
  print DEBUGOUT "Executable name: ",$Command_name,"\n";
  print DEBUGOUT "Total number of nodes: ",$Total_nodes,"\n";
}
#
# Process $SGE_JOB_SPOOL_DIR/usage file to get start and end time
#
$sge_usage="$SGE_JOB_SPOOL_DIR/usage";
open(USAGE, "< $sge_usage")    or die "Opening: $!";
while (<USAGE>) {
  @fields = split (/\=/, $_);
  if ( $fields[1] eq "start_time" ) {
    $time = $fields[2];
    if ($Total_nodes > 1) { $time -= 10; } # In case, nodes are not in sync w/ time
    $tm = localtime($time);
    $Start_day =sprintf("%d",$tm->mday);
    $Start_time=sprintf("%02d:%02d:%02d",$tm->hour, $tm->min, $tm->sec);
  }
  elsif ( $fields[1] eq "end_time" ) {
    $time = $fields[2];
    if ($Total_nodes > 1) { $time = $time + 10; } # In case, nodes are not in sync w/ time
    $tm = localtime($time);
    $End_day =sprintf("%d",$tm->mday);
    $End_time=sprintf("%02d:%02d:%02d",$tm->hour, $tm->min, $tm->sec);
  }
}
close(USAGE);

if ($Start_day ne $End_day) {
# For a long job, only looking for jobs finished 60 sec window before the end time.
# $Start_time is 60 sec before $End_time
  $Start_time=$End_time;
  ($hour, $min, $sec)=split(/:/,$Start_time);
  $tm = localtime;
  $time = timelocal($sec, $min, $hour, $tm->mday, $tm->mon, $tm->year) - 60;
  $tm = localtime($time);
  $Start_time=sprintf("%02d:%02d:%02d",$tm->hour, $tm->min, $tm->sec);
}

if ($DEBUG == 1) {
  print DEBUGOUT "User  name: ",$User_name,"\n";
  print DEBUGOUT "Start time: ",$Start_time,"\n";
  print DEBUGOUT "End   time: ",$End_time,"\n";
}

# 
# Run remote shell command to get the accounting info
# 
$Total_real   = 0;
$Total_sys    = 0;
$Total_user   = 0;
$Total_chars  = 0;
$Total_blocks = 0;
$Total_memory = 0;
%Summary;

#
# Save the result in $ACCTOUT file
#
if ($DEBUG == 1) {
  open (ACCTOUT,"> /tmp/acctsummary");
}
else {
  open (ACCTOUT,"> $TMPDIR/acctsummary");
}
print ACCTOUT "SGE Job ID: ",$JOB_ID,"\n";
print ACCTOUT "User name: ",$User_name,"\n";
print ACCTOUT "Executable name: ",$Command_name,"\n";
print ACCTOUT "Total number of nodes: ",$Total_nodes,"\n";
if ( $Total_nodes > 1 && $DEBUG == 1) { 
  print DEBUGOUT "NOTE: Run on multiple nodes, start and end time adjusted.\n";
}
# print ACCTOUT "Start time: ",$Start_time,"\n";
# print ACCTOUT "End   time: ",$End_time,"\n";
if ( $DEBUG == 1) { 
  print DEBUGOUT "Actual command to retrieve accounting info on each node:\n";
}
print ACCTOUT "Usage Summary:\n";

# -s: pick account info for processes "START BEFORE" the given time
# -e: pick account info for processes "END AFTER" the given time
# -S: pick account info for processes "START AFTER" the given time
# -E: pick account info for processes "END BEFORE" the given time

#
# For CPU & mean core memory usages
#
foreach $Node (@uniq_nodes) {
  if ($Start_day eq $End_day) {
    $cmdstr="rsh -n $Node 'acctcom -v -t -m -u $User_name -S $Start_time -E $End_time -n $Command_name /var/adm/pacct*'";
  }
  else {
    $cmdstr="rsh -n $Node 'acctcom -v -t -m -u $User_name -s $Start_time -E $End_time -n $Command_name /var/adm/pacct*'";
  }
  @acct_out = `$cmdstr`;
  if ($DEBUG == 1) {
    print DEBUGOUT $cmdstr,"\n";
    print DEBUGOUT @acct_out;
  }
  foreach $line (@acct_out) {
    chomp($line);
    if ($line =~ /$User_name/) {
      @items=split(/\s+/,$line);
      if ( $#items == 9 ) {
	$Fld1   = $items[6];  # Real  time (secs)
	$Fld2   = $items[7];  # System CPU (secs)
	$Fld3   = $items[8];  # User   CPU (secs)
	$Fld6   = $items[9];  # Mean Memory Size (KB)
      }
      elsif ( $#items == 8 ) {
	# Can't split CPU usage from acctcom output
	# Assume items[8] & items[9] are together
	$Fld1   = $items[6];  # Real  time (secs)
	$Fld2   = $items[7];  # System CPU (secs)
	# split the last string in two providing that each has two significant digits
	@subnums=split(/\./,$items[8]);
	$Fld3 = $subnums[1]; $Fld3 .= "."; $Fld3 .= substr($subnums[2],1,2); # User   CPU (secs)
	$Fld4 = substr($subnums[2],3); $Fld4 .= "."; $Fld4 .= $subnums[3];   # Mean Memory Size (KB)
      }
      else {
	# Can't split CPU usage from acctcom output
	$Fld1   = 0;    # Real  time (secs)
	$Fld2   = 0;    # System CPU (secs)
	$Fld3   = 0;    # User   CPU (secs)
	$Fld6   = 0;    # Mean Memory Size (KB)
      }
      $Total_real   = ($Fld1 > $Total_real) ? $Fld1 : $Total_real;
      $Total_sys    += $Fld2;
      $Total_user   += $Fld3;
      $Total_memory += $Fld6;
      $Total_procs  += 1;
      $Summary{$Node}[1] =$Total_real;
      $Summary{$Node}[2]+=$Fld2;
      $Summary{$Node}[3]+=$Fld3;
      $Summary{$Node}[6]+=$Fld6;
      $Summary{$Node}[7]+=1;
    }
  }
}
#
# For other usages
#
foreach $Node (@uniq_nodes) {
  if ($Start_day eq $End_day) {
    $cmdstr="rsh -n $Node 'acctcom -v -i -u $User_name -S $Start_time -E $End_time -n $Command_name /var/adm/pacct*'";
  }
  else {
    $cmdstr="rsh -n $Node 'acctcom -v -i -u $User_name -s $Start_time -E $End_time -n $Command_name /var/adm/pacct*'";
  }
  @acct_out = `$cmdstr`;
  if ($DEBUG == 1) {
    print DEBUGOUT $cmdstr,"\n";
    print DEBUGOUT @acct_out;
  }
  foreach $line (@acct_out) {
    chomp($line);
    if ($line =~ /$User_name/) {
      @items=split(/\s+/,$line);
      if ( $#items == 9) {
	$Fld4   = $items[8];  # Chars Transferred
	$Fld5   = $items[9];  # Blocks Read
      }
      elsif ( $#items == 8) {
	# Can't split other usage from acctcom output
	# Assume $items[7] & $items[8] are together
	# split the last string in two providing that each has two significant digits
	@subnums=split(/\./,$items[7]);
	$Fld4 = substr($subnums[2],3); # Chars Transferred
 	$Fld5 = $items[8];                # Blocks Read/Write
      }
      else {
	# Can't split chars transferred usage from acctcom output
	# use substring command for blocks r/w
 	$Fld4   = 0;                 # Chars Transferred
 	$Fld5   = substr($line, -8); # Blocks Read/Write
      }
      if ($DEBUG == 1) {
	print DEBUGOUT "Print chars transferred, $Fld4...\n";
      }
      $Total_chars  += $Fld4;
      $Total_blocks += $Fld5;
      $Summary{$Node}[4]+=$Fld4;
      $Summary{$Node}[5]+=$Fld5;
    }
  }
}

#
# Redirect STDOUT to ACCTOUT
#
*STDOUT=*ACCTOUT;
format FINAL_NODE_TOP=
                      Wallclock       User     System
Node           Procs  time(sec)      (sec)      (sec) Blocks     KChars    Mem(KB)
----------------------------------------------------------------------------------
.
format FINAL_NODE =
@<<<<<<<<<<< @###### @######.## @######.## @######.## @##### @######### @#########
$User,   $Jobs, $Realtime, $Usertime, $Systime, $Blocks, $Chars, $Mems
.

$^=FINAL_NODE_TOP;
$~=FINAL_NODE;
$-=0;
foreach $Node ( sort (keys %Summary ) ) {
  $User=$Node;
  $Jobs=$Summary{$Node}[7];
  $Realtime=$Summary{$Node}[1];
  $Usertime=$Summary{$Node}[3];
  $Systime =$Summary{$Node}[2];
  $Blocks  =$Summary{$Node}[5];
  $Chars   =(($Summary{$Node}[4])+500)/1000;
  $Mems=$Summary{$Node}[6];
  write;
}
$User="TOTALS:";
$Jobs=$Total_procs;
$Realtime=$Total_real;
$Usertime=$Total_user;
$Systime=$Total_sys;
$Blocks=$Total_blocks;
$Chars=($Total_chars+500)/1000;
$Mems=$Total_memory;
write;

print ACCTOUT "\n";
print ACCTOUT "Note:The accounting information may not be correct in some cases. \n";
print ACCTOUT "Please be aware of its limitations.\n";
print ACCTOUT "You can use the interactive script, ctacct1.pl, to debug the accounting data.\n";
print ACCTOUT "Usage: ctacct1.pl -j <sge_jid> -h <comma-separated-hostnames> -n <executable_name> -d\n";
print ACCTOUT "\n";

close(ACCTOUT);
