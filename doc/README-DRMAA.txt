                            What is DRMAA?
                            --------------

Content
-------
1. Introduction
2. Getting Started
3. Getting Support
4. Copyright

1. Introduction
---------------

   DRMAA is a specification developed by a working group in the Global Grid
   Forum (GGF).  The best way to describe DRMAA is to cite the DRMAA-WG
   Charter:

      "Develop an API specification for the submission and control of jobs
      to one or more Distributed Resource Management (DRM) systems. The
      scope of this specification is all the high level functionality which
      is necessary for an application to consign a job to a DRM system
      including common operations on jobs like termination or suspension. 
      The objective is to facilitate the direct interfacing of applications
      to today's DRM systems by application's builders, portal builders, and
      Independent Software Vendors (ISVs)."

   Simply put, DRMAA is an API for submitting and controling jobs.  DRMAA
   has been implemented in several lanuages and for several DRMs.  Grid
   Engine since release 6.0 includes a C implementation, or C binding.

   For more information about DRMAA and the various bindings that are
   available, visit the DRMAA website at:

      http://www.drmaa.org/

   There you will find links to the DRMAA specification and mailing list
   archives detailing the thought process that went into DRMAA.

   Information about grid computing standards in general can be found at the
   GGF website:

      http://www.gridforum.org/

   The Perl language binding module for DRMAA can be found at:

      http://search.cpan.org/src/THARSCH/Schedule-DRMAAc-0.81/

   For information on the C language binding included with the Condor 6.7
   release, see:

      http://www.cs.wisc.edu/condor/manual/v6.7/4_4Application_Program.html#SECTION00542000000000000000


2. Getting Started
-------------------

   To develop applications that utilize the C binding, you will need two files.
   The first is the DRMAA header file, drmaa.h.  You will find this file under
   the $SGE_ROOT/include directory in the distribution.  You will need to
   include this file from any source files that are to use the DRMAA
   library.  You will also need the DRMAA shared library.  You will find
   this file under the $SGE_ROOT/lib/$ARCH directory in the distribution. 
   This file must be linked with your source files during compilation (make
   ... -ldrmaa ...) and must be accessible from the LD_LIBRARY_PATH in order
   for your application to link or run.

   The first thing to do is to look through the header file.  This file
   lists the functions available to you as a DRMAA developer.  For more
   information on any function you can read the man page for that function. 
   For example:

      % man -M $SGE_ROOT/man drmaa_set_attribute

   The next step is to look at the example program included in the
   $SGE_ROOT/examples/drmaa directory of the distribution.  The example
   program demonstrates a simple usage of the DRMAA library to submit
   several bulk jobs and several single jobs, wait for the jobs to finish,
   and then output the results.

   Also in the $SGE_ROOT/examples/drmaa directory you will find the example
   programs from the online tutorial at:

      http://gridengine.sunsource.net/project/gridengine/howto/drmaa.html

   Once you're familiar with the DRMAA API, you're ready to begin
   development of your application.  Every source file which will call
   functions from the DRMAA library will need to include the line:

      #include "drmaa.h"

   in order for the compiler to find the function definitions.  You may need
   to tell the compiler where to find the DRMAA header file, e.g. by passing
   the
   -I$SGE_ROOT/include to the compiler.

   When compiling your file, you will need to have $SGE_ROOT/lib/$ARCH
   included in your LD_LIBRARY_PATH, and you will need to indicate to the
   compiler/linker that you want to link in this library, e.g. by passing
   the "-ldrmaa" option to the compiler/linker.

3. Getting Support
------------------

   For commercial deployments SGE DRMAA is recommended.

   If you're having trouble with the DRMAA library delivered with Sun Grid
   Engine, please visit:

      http://gridengine.sunsource.net/servlets/ProjectIssues

   to see if the problem your having is a know problem.  Alternately or for
   additional help you can send email to the dev@gridengine.sunsource.net
   mailing list.  You can also browse archives of this mailing list at:

      http://gridengine.sunsource.net/project/gridengine/maillist.html


4. Copyright
------------
___INFO__MARK_BEGIN__
The Contents of this file are made available subject to the terms of the Sun
Industry Standards Source License Version 1.2

Sun Microsystems Inc., March, 2001

Sun Industry Standards Source License Version 1.2
=================================================

The contents of this file are subject to the Sun Industry Standards Source
License Version 1.2 (the "License"); You may not use this file except in
compliance with the License. You may obtain a copy of the License at
http://gridengine.sunsource.net/Gridengine_SISSL_license.html

Software provided under this License is provided on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
WITHOUT LIMITATION, WARRANTIES THAT THE SOFTWARE IS FREE OF DEFECTS,
MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE, OR NON-INFRINGING.

See the License for the specific provisions governing your rights and
obligations concerning the Software.

The Initial Developer of the Original Code is: Sun Microsystems, Inc.

Copyright: 2001 by Sun Microsystems, Inc.

All Rights Reserved.
___INFO__MARK_END__
