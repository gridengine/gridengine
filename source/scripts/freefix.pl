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
# This script processes all .c files in the selected directory and makes
# sure that all calls to lFreeElem, lFreeList, lFreeWhat, lFreeWhere, and
# lFreeSortOrder assign the call result to the freed variable.  When the
# script finds a file that does not assign the result to the freed variable,
# it creates a new version of the file, called fix.file_name (where file_name
# is the name of the file being processed), in which these errors are corrected.
# if -commit is passed as an argument, instead of creating a new file to
# contain the changes, the script will overwrite the original.
#
# Syntax:
#    freefix.pl directory [-commit]
#
# Example:
#    freefix.pl gridengine/source -commit
#
#########################################################################

# Process args
if ((@ARGV < 1) || (@ARGV > 2) || ($ARGV[0] eq "") ||
    ((@ARGV == 2) && ($ARGV[1] ne "-commit"))) {
   print "Usage: freefix.pl directory_name [-commit]\n";
   exit 1;
}
elsif (@ARGV == 2) {
   $commit = 1;
}

# Global to tell if any changes were made
$changed = 0;

# Kick off the process
processDir ($ARGV[0]);

# If no changes were made, say so
if (!$changed) {
   print "No changes.\n";
}

# This routine recursively processes directories looking for .c files
sub processDir {
   my $dir = $_[0];
   my @files;

   opendir (DIR, $dir) || die ("Can't open directory $dir\n");
   (@files = readdir (DIR)) || die ("Can't read directory $dir\n");
   closedir (DIR) || die ("Can't close directory $dir\n");

   foreach $file (@files) {
      if ($file !~ /^\.\.?$/) {
         if ($file =~ /.c$/) {
            processFile ("$dir/$file");
         }
         elsif (-d "$dir/$file") {
            processDir ("$dir/$file");
         }
      }
   }
}

# This routine processes .c files to replace bad calls to lFree*
sub processFile {
   my $file = $_[0];
   # Flag to tell if we need to change the file or not
   my $printed = 0;

   open (FILE, "<$file");
   open (TMP, ">fix.$file");

   # First make sure there's something that needs to be fixed.
   while (<FILE>) {
      if ((/^\s*([a-zA-Z0-9]+\s*=\s*)?lFree(Elem|List|What|Where|SortOrder)\s*\(\s*[a-zA-Z0-9\->[\].*&]+\s*\)\s*;.*$/) &&
          ($1 eq "")) {
         print "Fixing $file";
         print " => $file.fix" if ($commit == 0);
         print "\n";
         $printed = 1;
         last;
      }
   }

   close (FILE);

   # Reopen the file, and this time make changes
   if ($printed) {
      open (FILE, "<$file");
      open (TMP, ">$file.fix");

      while (<FILE>) {
         if ((/^(\s*)(([a-zA-Z0-9]+\s*=\s*)?)(lFree(Elem|List|What|Where|SortOrder)(\s*\(\s*([a-zA-Z0-9\->[\].*&]+)\s*\))\s*;.*)$/) &&
             ($2 eq "")) {
            print TMP "$1$7 = $4\n";
         }
         else {
            print TMP;
         }
      }

      close (FILE);
      close (TMP);

      # if -commit was passed, replace the original with the tmp copy
      if ($commit) {
         unlink ("$file");
         rename ("$file.fix", "$file");
      }

      # Note that we made a change
      $changed = 1;
   }
}
