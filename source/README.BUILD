			Grid Engine Build Page
			----------------------

Content
-------

 1) Overview and files referenced in this document
 2) Prerequisites
 3) Building the dependency tool 'sge_depend'
 4) Building the Berkeley DB database
 5) Creating dependencies
 6) Compiling Grid Engine
 7) Creating man pages and qmon help file
 8) Installing Grid Engine
    8.1) Creating a local distribution
    8.2) Creating a distribution repository
 9) Creating a distribution from a distribution repository
10) Installing Grid Engine
11) Copyright


1) Overview and files referenced in this document
-------------------------------------------------

   This document gives you a brief overview about the steps how to compile,
   install and start Grid Engine. It is highly recommend to read this file
   and the files referenced here, if you are going to compile and run Grid
   Engine the first time.

   Files and URL's referenced in this document
   -------------------------------------------

   Description             File
   ------------------------------------   
   this file               README.BUILD    
   build wrapper           README.aimk
   architecture mapping    dist/README.arch
   distribution install    scripts/README.distinst
   tar.gz distribution     scripts/README.mk_dist
   Berkeley DB build       libs/spool/berkeleydb/README
   Dependency tool         http://gridengine.sunsource.net/unbranded-source/browse/~checkout~/gridengine/source/3rdparty
   access to CVS           http://gridengine.sunsource.net/servlets/ProjectSource
   getting source snapshot http://gridengine.sunsource.net/servlets/ProjectDownloadList
    

2) Prerequisites
----------------

   You first need to checkout the Grid Engine sources. Please refer to

      http://gridengine.sunsource.net/servlets/ProjectSource

   for more information about downloading the source with CVS. If you are
   unpacking a tar.gz Tarbell of the sources, please look at

      http://gridengine.sunsource.net/servlets/ProjectDownloadList

   Snapshots of the source code usually have a name in the form
  
       sge-<CVSTAG>-src.tar.gz

    e.g.

      sge-V53p5_TAG-src.tar.gz

   which corresponds to the source code of SGE 5.3p5.

   To compile and install Grid Engine, the following steps need to be
   carried out:

      - read requirements about Berkeley DB (see below)
      - get and compile openSSL library (optional)
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


3) Building the Berkeley DB database
------------------------------------

   Beginning with Grid Engine 6.0 sge_qmaster will use a Berkeley DB database
   for spooling of its configuration and state information.  The Berkeley DB
   include files and libraries have to be installed in order to build Grid
   Engine.

   See the file 

      libs/spool/berkeleydb/README

   for details about installing and using Berkeley DB.


4) Building the dependency tool 'sge_depend'
--------------------------------------------

   The Grid Engine project uses a tool 'sge_depend' to create header
   dependencies. The header dependencies are only created for local header
   files which are included with double quotes ("), e.g.:

       #include "header.h"

   See 

      http://gridengine.sunsource.net/unbranded-source/browse/~checkout~/gridengine/source/3rdparty/sge_depend/sge_depend.html?content-type=text/html

   for more information about 'sge_depend'.

   To build 'sge_depend' enter:

      % aimk -only-depend

   The result is the binary

      3rdparty/sge_depend/<ARCH>/sge_depend

   See the file 'README.aimk' for more information how 'aimk' works.


5) Creating dependencies
------------------------

   The header dependency files are created in every source directory.
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


6) Compiling Grid Engine
------------------------
  
   By default the script aimk compiles 'everything'. This may cause problems
   for certain modules, because one or more tools or libraries might not be
   available (yet) on your system. Therefore aimk provides a couple of
   useful switches to select only parts of Grid Engine for compilation:

   Enter

      % aimk -help' 

   to see a list of all aimk options. Not all options actually may work,
   esp. not in all combinations. Some options like the security related
   options enable compilation of code which is under development and is
   likely to be untested.

   Useful aimk options:

      -no-qmon  	don't compile qmon
      -no-qmake         don't compile qmake
      -no-qtcsh         don't compile qtcsh
      -no-remote        don't compile remote modules rsh, rshd, rlogin

   To compile the core system (daemons, command line clients, no interactive
   commands (qsh only), no qmon) you'll enter:

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
   binaries. Shared libraries are also supported, but their installation is
   not yet supported. The DRMAA library is always created and installed as
   a shared library. Some third party libraries (e.g. 'libXltree' used by
   qmon) are created as shared libraries due to copyright requirements).

   See 'README.aimk' for more information on all compilation issues.


7) Creating man pages and qmon help file
----------------------------------------

   Man pages in nroff format are created with

      % aimk -man

   or

      % aimk -mankv    (the man pages are checked out with the CVS "-kv"
                        flag" - needs access to the CVS repository)

   To create man pages in the "catman" format (e.g. used on SGI systems)
   after creating the nroff man pages enter

      % aimk -catman

8) Staging for Installation
---------------------------

   Once Grid Engine is compiled it can be prepared for installation by
   staging it to an appropriate place. Two types are supported. The script
   'scripts/distinst' is responsible for this purpose. It either copies the
   files directly to a local installation in the directory in $SGE_ROOT or
   it copies the files to a distribution directory where you can create
   customized distributions, e.g. in tar format for further redistribution.

   See the file

       scripts/README.distinst

   for more details about options of the 'distinst' script.

   8.1) Creating a local distribution
   ----------------------------------

      You can copy Grid Engine binaries and other parts of the distribution
      directly to a local installation. The purpose of this shortcut is to
      quickly install and run Grid Engine after compilation or other changes of
      the distribution.

      Create your <sge_root> root directory and set the environment variable
      $SGE_ROOT to <sge_root> and call

         scripts/distinst -local -noexit -allall <arch1> <arch2> ...

      Since some of binaries need to be installed SUID-root you need to login
      as user root to install a fully operable Grid Engine distribution. 

      The "-local" switch requires that the variable $SGE_ROOT is set, the
      "-noexit" switch causes the script to print warnings instead of exiting
      of some install targets (e.g. binaries or man pages) do not exist. The
      "-allall" target copies all files including man pages and binaries of
      the targets <arch1>, <arch2> to $SGE_ROOT.

   8.2) Creating a distribution repository
   ---------------------------------------

      If you are planning to create a distribution which later should be
      used to create further distributions (currently tar.gz is supported),
      the files are first copied to a distribution repository. From that
      distribution repository you later can create distributions. The base
      directory for the distribution repository is defined in the 'distinst'
      script and can be overridden with command line parameters.
      
      By default distinst in this mode will have a "strict" behavior. It
      will exit, if one of the installation targets cannot be installed
      successfully.  This is done to ensure that a distribution will only
      contain a valid set of files.


9) Creating a distribution from a distribution repository
---------------------------------------------------------

   If you need to create a Grid Engine distribution for further
   redistribution (like putting it on a FTP server), the script

      scripts/mk_dist

   carries out the necessary steps. You might want to copy the script to
   the base directory of your distribution repository. 

   See the file

     scripts/README.mk_dist

   how to create a Grid Engine distribution.


10) Installing Grid Engine
--------------------------

   After installing the distribution you need to run the installation script
   "inst_sge" (or a wrapper) on your master host and on all execution hosts
   for a first time configuration and startup of your Grid engine daemons.


11) Copyright
-------------

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
