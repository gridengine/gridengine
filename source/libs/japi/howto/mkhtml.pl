#!/usr/bin/perl

if (@ARGV != 1) {
   usage ();
   exit;
}

$file = $ARGV[0];
$count = -1;

open (FILE, "<$file");

while (<FILE>) {
   if ($count < 0) {
      if (m#/\*___INFO__MARK_END__\*/#) {
         $count = 0;
      }

      next;
   }

   $count++;
}

close FILE;

$num_figs = 0;

do {
   $num_figs++;
   $count /= 10;
} while (int ($count) > 0);

$count = -1;

open (FILE, "<$file");

while (<FILE>) {
   if ($count < 0) {
      if (m#/\*___INFO__MARK_END__\*/#) {
         $count = 0;
      }

      next;
   }
   elsif ($count == 0) {
      print "<PRE>";
   }
   else {
      print "\n";
   }

   chomp;

   printCount ($count);
   print;
   $count++;
}

close FILE;

print "</PRE>\n";

sub usage {
   print "mkhtml.pl source.c";
}

sub printCount {
   $diff = $num_figs - length ($count + 1);

   for (;$diff > 0; $diff--) {
      print "0";
   }

   print $count + 1;
   print ": ";
}