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
# Usage: 
# ctacct1.pl -j <sge_jid> -h <comma-separated-hostnames> -n <executable_name>
#         
#
use Time::Local;
use Time::localtime;
$[ = 1;			# set array base to 1

$DEBUG = 0;
$count = 0;
$Set_sge_jid=0;
$Set_command_name=0;
$Set_nodes=0;

foreach $arg (@ARGV) {
  $count++;
  if ($arg eq "-j") { 
    $SGE_JID = $ARGV[$count + 1]; 
    $Set_sge_jid=1;
  }
  elsif ($arg eq "-n") { 
    $Command_name = $ARGV[$count + 1]; 
    $Set_command_name=1;
    #
    # if it's longer than 8 chars, chop the rest
    #
    @charcmds=split(//,$Command_name);
    if ($#charcmds > 8) {
      $Command_name ="";
      foreach (1..8) {$Command_name .= $charcmds[$_];}
    }
  }
  elsif ($arg eq "-h") {
    @uniq_nodes = split (',',$ARGV[$count + 1]);
    $Set_nodes=1;
    $Total_nodes=$#uniq_nodes;
  }
  elsif ($arg eq "-d") {
    $DEBUG = 1;
  }
}
#
# Check if there are all the input arguments needed
#
if ( ($Set_sge_jid==0) | ($Set_command_name==0) | ($Set_nodes==0) ) {
  print "\n";
  print "Need more arguments...\n";
  print "Usage: \n";
  print "ctacct1.pl -j <sge_jid> -h <comma-separated-hostnames> -n <executable_name> -d\n";
  print "\n";
  die "Exited...\n";
}

#
# Get the accounting info
#
@sge_acct_info = `qacct -j $SGE_JID`;
#
# Process the accounting info
#
foreach (@sge_acct_info) {
  # print $_;
  chomp ($_);
  @fields = split (/\s+/, $_);
  if ( $fields[1] eq "owner" ) {
    $User_name = $fields[2];
  }
  elsif ( $fields[1] eq "start_time" ) {
    $Start_day    = $fields[4];
    $Start_time   = $fields[5];
  }
  elsif ( $fields[1] eq "end_time" ) {
    $End_day    = $fields[4];
    $End_time   = $fields[5];
  }
}

print "SGE Job ID: ",$SGE_JID,"\n";
print "User name: ",$User_name,"\n";
print "Executable name: ",$Command_name,"\n";
print "Total number of nodes: ",$Total_nodes,"\n";

if ($Total_nodes > 1) {
# Adjust start and end time to compensate minor time sync diff among machines
  ($hour, $min, $sec)=split(/:/,$Start_time);
# Convert to GMT - 10 sec
  $tm = localtime;
  $time = timelocal($sec, $min, $hour, $tm->mday, $tm->mon, $tm->year) - 10;
# 
# Get hh:mm:ss format back
#
  $tm = localtime($time);
  $Start_time=sprintf("%02d:%02d:%02d",$tm->hour, $tm->min, $tm->sec);

  ($hour, $min, $sec)=split(/:/,$End_time);
  $tm = localtime;
  $time = timelocal($sec, $min, $hour, $tm->mday, $tm->mon, $tm->year) + 10;
  $tm = localtime($time);
  $End_time=sprintf("%02d:%02d:%02d",$tm->hour, $tm->min, $tm->sec);
}

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
  print "Start time: ",$Start_time,"\n";
  print "End   time: ",$End_time,"\n";
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

# -s: pick account info for processes "START BEFORE" the given time
# -e: pick account info for processes "END AFTER" the given time
# -S: pick account info for processes "START AFTER" the given time
# -E: pick account info for processes "END BEFORE" the given time
#
# For CPU & mean core memory usages
#
if ( $DEBUG == 1) { 
  print "Actual command to retrieve accounting info on each node:\n";
}
foreach $Node (@uniq_nodes) {
  if ($Start_day eq $End_day) {
    $cmdstr="rsh -n $Node 'acctcom -t -m -u $User_name -S $Start_time -E $End_time -n $Command_name /var/adm/pacct*'";
  }
  else {
    $cmdstr="rsh -n $Node 'acctcom -t -m -u $User_name -s $Start_time -E $End_time -n $Command_name /var/adm/pacct*'";
  }
  @acct_out = `$cmdstr`;
  if ($DEBUG == 1) {
    print  $cmdstr,"\n";
    print  @acct_out;
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
    $cmdstr="rsh -n $Node 'acctcom -i -u $User_name -S $Start_time -E $End_time -n $Command_name /var/adm/pacct*'";
  }
  else {
    $cmdstr="rsh -n $Node 'acctcom -i -u $User_name -s $Start_time -E $End_time -n $Command_name /var/adm/pacct*'";
  }
  @acct_out = `$cmdstr`;
  if ($DEBUG == 1) {
    print  $cmdstr,"\n";
    print  @acct_out;
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
print  "Usage Summary:\n";
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

