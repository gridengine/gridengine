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
   has been implemented in several lanuages and for several DRMs.  The Grid
   Engine 6.0 release includes a C implementation, or C binding.

   For more information about DRMAA and the various bindings that are
   available, visit the DRMAA website at:

      http://www.drmaa.org/

   There you will find links to the DRMAA specification and mailing list
   archives detailing the thought process that went into DRMAA.

   Information about grid computing standards in general can be found at the
   GGF website:

      http://www.gridforum.org/

   The Perl language binding module for the Grid Engine 6.0 release can be
   found at:

      http://search.cpan.org/src/THARSCH/Schedule-DRMAAc-0.81/

   For information on the C language binding included with the Condor 6.7
   release, see:

      http://www.cs.wisc.edu/condor/manual/v6.7/4_4Application_Program.html#SECTION00542000000000000000


2. Getting Started
-------------------

   The Grid Engine 6.0 release supports a DRMAA Java language binding.  To
   develop applications that utilize the Java binding, you will need two files.
   The first is the jar file, drmaa.jar.  This file is need for both
   compiling and running applications utilizing the Java language binding.  The
   second file is the Java language binding shared library.  This file will need
   to be accessible from the LD_LIBRARY_PATH in order for your application to
   link properly and run.  If you built the binaries yourself (using "aimk
   -java"), you will find both of these files in the $SGE_ROOT/lib/$ARCH after
   using distinst -local to install the distribution.  Otherwise, you can
   download the binaries from the File Exchange at:

      http://gridengine.sunsource.net/servlets/ProjectDownloadList

   Look for the latest jdrmaa.tar file.  This file contains the drmaa.jar and
   the Java language binding shared libraries for all currently supported
   platforms.

   The first step is to look at the example program to be found at:

      http://gridengine.sunsource.net/source/browse/gridengine/source/classes/DRMAAExample.java

   The example program demonstrates a simple usage of the DRMAA library to
   submit several bulk jobs and several single jobs, wait for the jobs to
   finish, and then output the results.

   In the:
 
      http://gridengine.sunsource.net/source/browse/gridengine/source/classes/com/sun/grid/drmaa/howto/

   directory you will find the example programs from the online tutorial at:

      http://gridengine.sunsource.net/project/gridengine/howto/drmaa_java.html

   Once you're familiar with the DRMAA API, you're ready to begin
   development of your Java application.  When compiling your file, you will
   need to have $SGE_ROOT/lib/$ARCH/drmaa.jar included in your CLASSPATH.


3. Getting Support
------------------

   For commercial deployments N1GE6 DRMAA is recommended.

   If you're having trouble with the DRMAA library included with the Grid
   Engine 6 release, please visit:

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
