package Qmon::Help;

# Qmon/Help.pm, 2001-Mar-11, Markus Grabert
#
# Copyright (c) 1997 Sun Microsystems, Inc. All Rights Reserved.
#
# This software is the confidential and proprietary information of Sun
# Microsystems, Inc. ("Confidential Information").  You shall not
# disclose such Confidential Information and shall use it only in
# accordance with the terms of the license agreement you entered into
# with Sun.
#
# SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF THE
# SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
# PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR ANY DAMAGES
# SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR DISTRIBUTING
# THIS SOFTWARE OR ITS DERIVATIVES.

use strict;

BEGIN {
   use Exporter   ();
   use vars       qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);

   $VERSION     = 1.00;

   @ISA         = qw(Exporter);
   @EXPORT      = qw(init_tkHelp destroy_tkHelp);
   @EXPORT_OK   = ();
   %EXPORT_TAGS = ( ALL => [@EXPORT, @EXPORT_OK]);
}

use Tk;

use vars @EXPORT_OK;

my $Window;
my $Frame;

sub init_tkHelp {
   my %hash;
   if (defined ($Window)) {
      %hash = %$Window;
   }
   if (! exists $hash{'_names_'}) {

      $Window = MainWindow->new();
      $Window->title("Help");
      $Window->minsize(qw(250 250));
  
      $Frame = $Window->Frame(
         -borderwidth => 3,
#      -background => 'grey',
         -relief => 'groove');
   }
   pack_tkHelp();
}

sub pack_tkHelp {
   $Frame->pack(
      -side => 'left',
      -anchor => 's',
      -fill => 'y');
}

sub destroy_tkHelp {
   if (defined $Window) {
      my %hash = %$Window;
      if (exists $hash{'_names_'}) {
         $Frame->destroy();
         $Window->destroy();
      }
   }
}
1;
END { }       # module clean-up code here (global destructor)
__END__
