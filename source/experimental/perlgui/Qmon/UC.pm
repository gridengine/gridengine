package Qmon::UC;


use strict;

BEGIN {
   use Exporter   ();
   use vars       qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);

   $VERSION     = 1.00;

   @ISA         = qw(Exporter);
   @EXPORT      = qw(init_tkUC destroy_tkUC);
   @EXPORT_OK   = ();
   %EXPORT_TAGS = ( ALL => [@EXPORT, @EXPORT_OK]);
}

use SGE;
use Devel::Peek 'Dump';
use Tk;

use vars @EXPORT_OK;


sub init_tkUC {
}

sub destroy_tkUC {
}
1;
END { }       # module clean-up code here (global destructor)
__END__
