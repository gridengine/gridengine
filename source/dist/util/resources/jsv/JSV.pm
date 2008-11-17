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
#  Copyright: 2008 by Sun Microsystems, Inc.
#
#  All Rights Reserved.
#
##########################################################################
#___INFO__MARK_END__

package JSV;

require Exporter;
@ISA = qw(Exporter);

# always export the following
@EXPORT = qw(
   on_start on_verify 
   get_env_hash get_param_hash
   set_param del_param sub_add_param sub_del_param 
   job_accept job_correct job_reject job_reject_wait 
   main 
);

# symbols to export on request
@EXPORT_OK = qw(
   is_env get_env 
   is_param get_param 
   sub_is_param sub_get_param 
   show_params show_envs 
   send_command send_error send_env 
   script_log 
   log_info log_warning log_error
);

# use JSV qw/:ALL/; # to import everything
%EXPORT_TAGS = (ALL => [@EXPORT_OK, @EXPORT]);

use strict;
use warnings;
use FindBin;

##########################################################################
# global variables
my %config = (
   log_file        => '/tmp/jsv.log',
   logging_enabled => 0,
   head_incoming   => '>>>',
   head_outgoing   => '<<<',
);

my %handler = (
   on_start  => sub {},
   on_verify => sub { job_accept('Job is accepted (default handler)'); },
   ENV       => \&handle_command_env,
   PARAM     => \&handle_command_param,
   START     => \&handle_command_start,
   SHOW      => \&handle_command_show,
   BEGIN     => \&handle_command_begin,
);

# current state, allowed values: "initialized", "started" or "verifying"
my $state = 'initialized';

# values of parameters
my %param = ();

# values of the environment sent to this script
my %env = ();


##########################################################################
# functions

# get or set on_start handler
sub on_start {
   if (@_) {
      $handler{on_start} = $_[0];
   } else {
      return $handler{on_start};
   }
}

# get or set on_verify handler
sub on_verify {
   if (@_) {
      $handler{on_verify} = $_[0];
   } else {
      return $handler{on_verify};
   }
}

sub handle_command_start {
   if ($state eq 'initialized') {
      $handler{on_start}->();
      send_command('STARTED');
      $state = 'started';
   } else {
      send_error("JSV script got START command but is in state $state");
   }
}


sub handle_command_show {
   show_params();
   show_envs();
} 

sub handle_command_begin {
   if ($state eq 'started') {
      $state = 'verifying';
      $handler{on_verify}->();
      %env = ();
      %param = ();
   } else {
      send_error("JSV script got BEGIN command but is in state $state");
   }
}

{
   my %is_list_command = ();
   @is_list_command{
      qw(ac e hold_jid hold_jid_ad i l_hard l_soft M m masterq o pe q q_hard q_soft S u)
   } = ();

sub handle_command_param {
   my ($key, @val) = @_;
   my $val = join ' ', @val;

   if ($state eq 'started') {
      if (exists $is_list_command{$key}) {
         # parse $val into hashref
         #   $val contains something like 'foo=bar,baz=blub' or 'foo,bar,baz'
         my %h;
         for my $item (split /,/, $val) {
            my ($k,$v) = split /=/, $item;
            $h{$k} = $v;
         }
         $param{$key} = { %h };
      } else {
         $param{$key} = $val;
      }
   } else {
      send_error("JSV script got PARAM command but is in state $state");
   }
}

} # end of closure over %is_list_command


sub handle_command_env {
   my ($action, $key, @val) = @_;

   if ($state eq 'started') {
      if ($action eq 'ADD') {
         $env{$key} = join ' ', @val;
      }
   } else {
      send_error("JSV script got ENV command but is in state $state");
   }
}

# makes a tailored 'deep_copy' (deep enough for the %param and %env hash)
sub deep_copy {
   my ($from) = @_;
   my %h = ();

   for my $k (keys %$from) {
      my $v = $from->{$k};
      if (ref $v eq 'HASH') {
         # copy of hash in $v
         $h{$k} = { %$v };
      } else {
         $h{$k} = $v;
      }
   }

   return %h;
}


# returns a copy of the hash of the current environment (sent to this script)
sub get_env_hash {
   return deep_copy(\%env);
}

sub is_env {
   return exists $env{$_[0]};
}

sub get_env {
   my ($key) = @_;

   if (exists $env{$key}) {
      return $env{$key};
   }
   return;
}

# returns a copy of the hash of the current parameters
sub get_param_hash {
   return deep_copy(\%param);
}

sub is_param {
   return exists $param{$_[0]};
}

sub get_param {
   my ($key) = @_;

   if (exists $param{$key}) {
      return $param{$key};
   }
   return;
}

sub stringify {
   my ($val) = @_;

   if (ref $val eq 'HASH') {
      # stringify the hashref
      # { foo => 'bar', baz => 'boz' } gets stringified to 'foo=bar,baz=boz'
      # { foo => undef, baz => undef } gets stringified to 'foo,baz'
      # { foo => undef, baz => 'boz' } gets stringified to 'foo,baz=boz'
      $val = join(',', map { my $v = $val->{$_}; (defined $v) ? "$_=$v" : "$_"} keys %$val);
   }

   return $val;
}

sub set_param {
   my ($key, $val) = @_;
   $param{$key} = $val;

   $val = stringify($val);
   send_command("PARAM $key $val");
}

sub del_param {
   my ($key) = @_;
   delete $param{$key};
   send_command("PARAM $key");
}

sub sub_is_param {
   my ($key, $sub) = @_;

   # do not autovivify => two exists checks
   return unless exists $param{$key};

   if (exists $param{$key}{$sub}) {
      return 1;
   }
   return;
}

# returns undef or empty list if sub-parameter does not exist.
sub sub_get_param {
   my ($key, $sub) = @_;

   # do not autovivify => two exists checks
   return unless exists $param{$key};

   if (exists $param{$key}{$sub}) {
      return $param{$key}{$sub};
   }
   return;
}


sub sub_add_param {
   my ($key, $sub, $val) = @_;

   my $href = $param{$key};
   $href->{$sub} = $val;

   set_param($key, $href);
}

sub sub_del_param {
   my ($key, $sub) = @_;

   my $href = $param{$key};
   delete $href->{$sub};

   set_param($key, $href);
}

sub show_hash {
   my ($name, $href) = @_;
   my ($k,$v);

   while ( ($k,$v) = each %$href ) {
      $v = stringify($v);
      send_command("LOG INFO got $name: $k='$v'");
   }
}

sub show_params {
   show_hash('param', \%param);
} 

sub show_envs {
   show_hash('env', \%env);
}

sub send_command {
   print "@_\n";
   script_log($config{head_outgoing},@_);
}

sub send_error {
   send_command('ERROR', @_);
}

sub send_env {
   send_command("SEND ENV");
}

sub job_accept {
   job_handle('ACCEPT', @_);
}

sub job_correct {
   job_handle('CORRECT', @_);
}
sub job_reject {
   job_handle('REJECT', @_);
}

sub job_reject_wait {
   job_handle('REJECT_WAIT', @_);
}

sub job_handle {
   if ($state eq 'verifying') {
      send_command("RESULT STATE @_");
      $state = 'initialized';
   } else {
      send_error("JSV script will send 'RESULT STATE $_[0]' command but is in state $state");
   }
}

{
   my $fh; # logging file handle
   my $myself = "$FindBin::Bin/$FindBin::Script"; # full path of this script

   # open log file and write logging header
   sub script_log_open {
      return unless $config{logging_enabled};

      # don't do any error handling, if we can't log, it's not the end of the world
      open $fh, '>>', $config{log_file};

      # no buffering of logging file handle
      my $old_fh = select $fh;
      $| = 1;
      select $old_fh;

      my $date_time = localtime();
      script_log(<<"END_OF_LOG");
$myself started on $date_time

This file contains logging output from a GE JSV script. Lines beginning
with $config{head_incoming} contain the data which was send by a command line client or
sge_qmaster to the JSV script. Lines beginning with $config{head_outgoing} contain data
which is send from this JSV script to the client or sge_qmaster
END_OF_LOG
   }

   # close log file and write logging footer
   sub script_log_close {
      return unless $config{logging_enabled};

      my $date_time = localtime();
      script_log("$myself is terminating on $date_time");

      # don't do any error handling, if we can't log, it's not the end of the world
      close $fh;
      undef $fh;
   }

   sub script_log {
      return unless $config{logging_enabled} && $fh;

      # don't do any error handling, if we can't log this is not the end of the world
      print $fh "@_\n";
   }
} # end of closure over $fh and $myself

sub log_info {
   send_command('LOG INFO', @_);
}

sub log_warning {
   send_command('LOG WARNING', @_);
}

sub log_error {
   send_command('LOG ERROR', @_);
}


sub main {
   script_log_open();

   $| = 1; # no output buffering

   while (<>) {
      chomp;
      next if $_ eq '';
      script_log("$config{head_incoming} $_");

      # split on one space and do not throw any trailing fields away
      my @arg = split / /, $_, -1;
      my $cmd = shift @arg;

      last if $cmd eq 'QUIT';

      if (exists $handler{$cmd}) {
         $handler{$cmd}->(@arg);
      } else {
         send_error("JSV script got unknown command '$cmd'");
      }
   }

   script_log_close();
}


1; # return true
