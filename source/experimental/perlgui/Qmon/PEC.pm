package Qmon::PEC;


use strict;

BEGIN {
   use Exporter   ();
   use vars       qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);

   $VERSION     = 1.00;

   @ISA         = qw(Exporter);
   @EXPORT      = qw(init_tkPEC destroy_tkPEC);
   @EXPORT_OK   = ();
   %EXPORT_TAGS = ( ALL => [@EXPORT, @EXPORT_OK]);
}

use SGE;
use Devel::Peek 'Dump';
use Tk;

use vars @EXPORT_OK;


sub init_tkPEC {
}

sub destroy_tkPEC {
}
1;
END { }       # module clean-up code here (global destructor)
__END__
