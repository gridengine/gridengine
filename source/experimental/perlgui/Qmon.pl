#!/usr/bin/perl -w


use SGE;
use Cull;

use Devel::Peek 'Dump';
use Qmon::Main;
use Tk;

#####################################
# You finally found the important part:
# Here you are: the main loop

#   SGE::sge_gdi_setup("perl-sge");
#   my $result=SGE::perl_gdi_get($SGE::SGE_ZOMBIE_LIST, undef, undef);
#   SGE::perl_lWriteListTo($result);
#   SGE::sge_gdi_shutdown();


init_tkMainWindow();
init_tkMainMenu();

MainLoop();
