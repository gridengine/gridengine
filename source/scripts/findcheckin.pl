#!/usr/bin/perl

#########################################################################
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
#   The Initial Developer of the Original Code is: Sun Microsystems, Inc.
# 
#   Copyright: 2001 by Sun Microsystems, Inc.
# 
#   All Rights Reserved.
# 
#########################################################################
#
# This script parses the output from `cvs log` and extracts the file names
# and version number for files containing a CVS log entry that begins with
# the given id.
#
# Syntax:
#    Usage: findcheckin.pl id [cvs_log_args]
#
# Example:
#    findcheckin.pl DT-2004-07-19-0 -wtempledf
#
#########################################################################

sub parselog {
   # Here we have to join stderr to stdout because otherwise we have no control
   # over it.  We will filter the CVS messages back out later and print them
   # directly to stderr so they are in a different stream.
   open (LOG, "cvs log $args 2>&1 |");

   while (<LOG>) {
      if (/^RCS file: (.*),v$/) {
         $filename = $1;
      }
      elsif (/^-+$/) {
         $inRev = 1;
      }
      elsif ($inRev && /^revision ([0-9.]+)$/) {
         $rev = $1;
      }
      elsif ($inRev && /^($tag[-0-9]*)/) {
         print "$1 - $filename: $rev\n";
      }
      elsif (/^=+$/) {
         $filename = "";
         $rev = "";
         $inRev = 0;
      }
      elsif (/^cvs server:/ && !$quiet) {
         #Propogate CVS messages to stderr
         print stderr;
      }
      elsif (/^cvs log:/ && !$quiet) {
         #Propogate CVS messages to stderr
         print stderr;
      }
   }
}

$tag = shift @ARGV;

if ($ARGV[0] eq "-q") {
   shift @ARGV;
   $quiet = 1;
}

$args = join " ", (@ARGV);

if ($tag eq "") {
   print "Usage: findcheckin.pl id [-q] [cvs_log_args]\n";
   print "Example: findcheckin.pl DT-2004-07-19-0 -wtempledf\n";
   exit;
}

parselog ();
print "Done.\n";
