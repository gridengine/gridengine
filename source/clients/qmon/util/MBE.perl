#!/usr/bin/perl -w

open(FILE,"<$ARGV[0]") or die "unable to open file $ARGV[0].\n";

( -T $ARGV[0] ) or die "File $ARGV[0] is not a text-file.";



while ( $Line = <FILE> )
  {
    chomp $Line;
    if ($Line !~ /^\!/)
    {
      $Line =~ /^(.*):/;
      $orig = $string = $1;
      if ($string =~ /(\@fB|\@fI|\@f\[BIG\])(.*)/) {
         $format = $1;
         $result = `echo "$2" |entombe`;
      }
      else {
         $format = "";
         $result = `echo "$string" |entombe`;
      }   

      print "$orig: ${format}$result";
    }  
  }

