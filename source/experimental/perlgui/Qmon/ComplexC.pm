package Qmon::ComplexC;


use strict;

BEGIN {
   use Exporter   ();
   use vars       qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);

   $VERSION     = 1.00;

   @ISA         = qw(Exporter);
   @EXPORT      = qw(init_tkComplexC destroy_tkComplexC);
   @EXPORT_OK   = ();
   %EXPORT_TAGS = ( ALL => [@EXPORT, @EXPORT_OK]);
}

use SGE;
use Devel::Peek 'Dump';
use Tk;

use vars @EXPORT_OK;


sub init_tkComplexC {
}

sub destroy_tkComplexC {
}
1;
END { }       # module clean-up code here (global destructor)
__END__
