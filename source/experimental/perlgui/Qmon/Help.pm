package Qmon::Help;


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
