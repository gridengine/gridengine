			Grid Engine Build Page
			----------------------


Content
-------

0) Overview and files referenced in this document
1) Prerequisites
2) Building the dependency tool 'sge_depend'
3) Creating dependencies
4) Compiling Grid Engine
5) Creating man pages and qmon help file
6) Installing Grid Engine
   6.1) Creating a local distribution
   6.2) Creating a distribution repository
7) Creating a distribution from a distribution repository
8) Installing Grid Engine
9) Copyright


0) Overview and files referenced in this document
-------------------------------------------------

   This document gives you a brief overview about the steps how to compile,
   install and start Grid Engine. It is highly recommend to read this file
   and the files referenced here, if you are going to compile and run Grid
   Engine the first time.

   Files referenced in this document:

      README.BUILD      (this file)
      README.aimk
      dist/README.arch
      scripts/README.distinst
      scripts/README.mk_dist

  
1) Prerequisites
----------------

   You first need to checkout the Grid Engine sources. Please refer to

      http://gridengine.sunsource.net/servlets/ProjectSource

   for more information about downloading the source with CVS. If you are
   unpacking a tar.gz tarball of the sources, please look at

      http://gridengine.sunsource.net/servlets/ProjectDownloadList

   Snapshots of the source code usually have a name in the form
  
       gridengine_<CVSTAG>.tar.gz

   To compile and install Grid Engine, the following steps need to be
   carried out:

      - create dependency tool and dependencies with 'aimk'
      - compile binaries with 'aimk'
      - create a distribution repository with 'distinst'
      - create a distribution from distribution repository with 'mk_dist'
      - unpack and install distribution

   This document describes the process up to the creation of a distribution. 
   The result is a tar file (or virtually any other OS specific distribution
   format) which can be used to install Grid Engine on a cluster of machines.

   See the file 'dist/README.arch' in this directory for more information
   how the architecture is determined and how it is used in the process of
   compiling and installing Grid engine.

   The following commands assume that your current working directory is

       gridengine/source


2) Building the dependency tool 'sge_depend'
--------------------------------------------

   The Grid Engine project uses a tool 'sge_depend' to create header
   dependencies. The header dependencies are only created for local header
   files which are included with double quotes ("), e.g.:

       #include "header.h"

   See <<here_link_to_andre's_doc>> for more information about
   'sge_depend'.

   To build 'sge_depend' enter:

      % aimk -only-depend

   The result is the binary

      3rdparty/sge_depend/<ARCH>/sge_depend

   See the file 'README.aimk' for how 'aimk' works.


3) Creating dependencies
------------------------

   The header dependency files are created in every directory.
   They are included by the makefiles and therefore they must exist. The
   dependencies will be created for every .c Files in the source code
   repository.

   Create zero length dependency files with the command

      % scripts/zerodepend

   Now create your dependencies:

      % aimk depend

   Depending on actual compiler flags for a specific OS 'sge_depend' may
   print out some warnings about unrecognized command line options. These
   warnings can be ignored.

   The dependencies are not recreated automatically if your header
   dependencies change. The dependencies need to be created only on one
   platform. They are shared for all target architectures.


4) Compiling Grid Engine
------------------------
  
   By default the script aimk compiles 'everything'. This may cause problems
   for certain modules, because one or more tools or libraries might not be
   available (yet) on your system. Therefore aimk provides a couple of
   useful switches to select only parts of Grid Engine for compilation:

   Try to call 

      % aimk -help' 

   to see a list of all options. Not all options actually may work, esp. not
   in all combinations. Some options like the security related options
   enable compilation of code which is under development and is likely to be
   untested.

   Useful options:

      -no-mt  	         don't compile multi-threading targets
      -no-qmon  	      don't compile qmon
      -no-qmake         don't compile qmake
      -no-qtcsh         don't compile qtcsh
      -no-remote        don't compile remote modules rsh, rshd, rlogin

   To compile the core system (daemons, command line clients, no interactive
   commands (qsh only)) you'll enter:

      % aimk -only-core

   When compilation begins a subdirectory named as an uppercase architecture
   string will be created and the system is compiled.

   At least on the supported and tested platforms problems with compilation
   most likely will be related to the required compiler (often the operating
   system compiler is used and other compilers like gcc are not supported or
   not well tested) or compiler version, partially to the default memory
   model on your machine (32 or 64bit). Usually these problems can be solved
   by tuning aimk.

   By default the Grid Engine libraries are linked statically to the
   binaries. Shared libraries are also supported, but installation is a bit
   more complicated.

   See 'README.aimk' for more information on all compilation issues.


5) Creating man pages and qmon help file
----------------------------------------

   Man pages in nroff format are created with

      % aimk -man

   or

      % aimk -mankv    (the man pages are checked out with the CVS "-kv"
                        flag" - needs access to the CVS repository)

   To create man pages in the "catman" format (e.g. used on SGI systems)
   after creating the nroff man pages enter

      % aimk -catman

   The qmon help file is created with the command

      % aimk -only-qmon qmon_help


6) Staging for Installation
---------------------------

   Once Grid Engine is compiled it can be prepared for installation by
   staging it to an appropriate place. Two types are supported. The script
   'scripts/distinst' is responsible for this purpose. If called as
   'myinst' it will copy the files directly to a local installation in the
   directory $SGE_ROOT. This option is useful for quickly testing the
   results of your compilation. If the script is called as 'distinst' it
   will copy the files to a distribution directory where you can create
   customized distributions in tar format etc.

   See the file

       scripts/README.distinst

   for more details about options of the 'distinst' script.


6.1) Creating a local distribution
----------------------------------

   You can copy Grid Engine binaries and other parts of the distribution
   directly to a local installation. The purpose of this shortcut is to
   quickly install and run Grid Engine after compilation or other changes of
   the distribution.

   If 'scripts/distinst' is invoked as 'myinst' (create a symlink:
   "ln -s scripts/distinst myinst") it will check for the variable SGE_ROOT
   and take this directory as staging target.

   By default 'myinst' in this mode will issue only warnings if one or more 
   targets cannot be installed successfully.


6.2) Creating a distribution repository
---------------------------------------

   If you are planning to create a distribution which later should be used
   to create further distributions (currently tar.gz is supported), the
   files are first copied to a distribution repository. From that
   distribution repository you later can create distributions. The base
   directory for the distribution repository is defined in the 'distinst'
   script or can be overridden with command line parameters.
   
   By default distinst in this mode will have a "strict" behavior. It will
   exit, if one of the installation targets cannot be installed
   successfully.  This is done to ensure that a distribution will only
   contain a valid set of files.


7) Creating a distribution from a distribution repository
---------------------------------------------------------

   If you need to create a Grid Engine distribution for further
   redistribution (like putting it on a FTP server), the script

      scripts/mk_dist

   carries out the necessary steps. You might want to copy the script to
   the base directory of your distribution repository. 

   See the file

     scripts/README.mk_dist

   how to create a Grid Engine distribution


8) Installing Grid Engine
-------------------------

   After installing the distribution (either after you installed it with
   "myinst" or after unpacking a distribution in tar format or in any other
   format which is supported by "mk_dist"), you need to run the installation
   script "inst_sge" (or a wrapper) on your master host and on all execution
   hosts for a first time configuration and startup of your Grid engine
   daemons.

   See the file

      dist/README.inst_sge

   for an overview on what "inst_sge" does.


9) Copyright
------------

   The Contents of this file are made available subject to the terms of
   the Sun Industry Standards Source License Version 1.2
 
   Sun Microsystems Inc., March, 2001

   Sun Industry Standards Source License Version 1.2  
   =================================================
   The contents of this file are subject to the Sun Industry Standards
   Source License Version 1.2 (the "License"); You may not use this file
   except in compliance with the License. You may obtain a copy of the
   License at http://gridengine.sunsource.net/Gridengine_SISSL_license.html
 
   Software provided under this License is provided on an "AS IS" basis,
   WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,  
   WITHOUT LIMITATION, WARRANTIES THAT THE SOFTWARE IS FREE OF DEFECTS,  
   MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE, OR NON-INFRINGING.
   See the License for the specific provisions governing your rights and
   obligations concerning the Software.
 
   The Initial Developer of the Original Code is: Sun Microsystems, Inc.

   Copyright: 2001 by Sun Microsystems, Inc.
 
   All Rights Reserved.
