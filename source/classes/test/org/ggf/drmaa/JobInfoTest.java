/*___INFO__MARK_BEGIN__*/
/*************************************************************************
 * 
 *  The Contents of this file are made available subject to the terms of
 *  the Sun Industry Standards Source License Version 1.2
 * 
 *  Sun Microsystems Inc., March, 2001
 * 
 * 
 *  Sun Industry Standards Source License Version 1.2
 *  =================================================
 *  The contents of this file are subject to the Sun Industry Standards
 *  Source License Version 1.2 (the "License"); You may not use this file
 *  except in compliance with the License. You may obtain a copy of the
 *  License at http://gridengine.sunsource.net/Gridengine_SISSL_license.html
 * 
 *  Software provided under this License is provided on an "AS IS" basis,
 *  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
 *  WITHOUT LIMITATION, WARRANTIES THAT THE SOFTWARE IS FREE OF DEFECTS,
 *  MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE, OR NON-INFRINGING.
 *  See the License for the specific provisions governing your rights and
 *  obligations concerning the Software.
 * 
 *   The Initial Developer of the Original Code is: Sun Microsystems, Inc.
 * 
 *   Copyright: 2001 by Sun Microsystems, Inc.
 * 
 *   All Rights Reserved.
 * 
 ************************************************************************/
/*___INFO__MARK_END__*/
/*
 * JobInfoTest.java
 * JUnit based test
 *
 * Created on November 13, 2004, 4:25 PM
 */

package org.ggf.drmaa;

import java.util.*;
import junit.framework.*;

/**
 *
 * @author dan.templeton@sun.com
 */
public class JobInfoTest extends TestCase {
   private JobInfo jiAll = null;
   private JobInfo jiNone = null;
   private HashMap resources = null;
   private static final String TERMSIG = "SIGMYSIG";
   
   public JobInfoTest (java.lang.String testName) {
      super (testName);
   }
   
   public static Test suite () {
      TestSuite suite = new TestSuite (JobInfoTest.class);
      return suite;
   }
   
   protected void setUp () {
      resources = new HashMap ();
      resources.put ("cpu_time", "123");
      resources.put ("memory", "1024");
      
      jiAll = new JobInfoImpl ("12345", 0x1f, resources);
      
      jiNone = new JobInfoImpl ("67890", 0x00, null);
   }
   
   protected void tearDown () {
      resources = null;
      jiAll = null;
      jiNone = null;
   }
   
   /** Test of getJobId method, of class org.ggf.drmaa.JobInfo. */
   public void testGetJobId () {
      System.out.println ("testGetJobId");
      
      assertEquals ("12345", jiAll.getJobId ());
      assertEquals ("67890", jiNone.getJobId ());
   }
   
   /** Test of getResourceUsage method, of class org.ggf.drmaa.JobInfo. */
   public void testGetResourceUsage () {
      System.out.println ("testGetResourceUsage");

      assertEquals (resources, jiAll.getResourceUsage ());
      assertEquals (new HashMap (resources), jiAll.getResourceUsage ());
      assertEquals (new HashMap (), jiNone.getResourceUsage ());
   }
   
   /** Generated implementation of abstract class org.ggf.drmaa.JobInfo. Please fill dummy bodies of generated methods. */
   private class JobInfoImpl extends JobInfo {
      protected JobInfoImpl (String jobId, int status, Map resourceUsage) {
         super (jobId, status, resourceUsage);
      }
      
      /** Returns <CODE>true</CODE> if the job terminated normally.
       * <CODE>False</CODE> can also indicate that
       * although the job has terminated normally an exit status is not available
       * or that it is not known whether the job terminated normally. In both
       * cases getExitStatus() SHALL NOT provide exit status information.
       * <CODE>True</CODE> indicates more detailed diagnosis can be provided
       * by means of hasSignaled(), getTerminatingSignal() and hasCoreDump().
       * @return if the job has exited
       *
       */
      public boolean hasExited () {// fill the body in order to provide useful implementation
         return ((status & 0x01) != 0);
      }
      
      /** If hasExited() returns true,  this function returns the exit code
       * that the job passed to _exit() (see exit(2)) or exit(3C), or the value
       * that the child process returned from main.
       * @return the exit code for the job
       *
       */
      public int getExitStatus () {// fill the body in order to provide useful implementation
         return (status >> 4);
      }
      
      /** Returns <CODE>true</CODE> if the job terminated due to the receipt
       * of a signal. <CODE>False</CODE> can also indicate that although the
       * job has terminated due to the receipt of a signal the signal is not
       * available or that it is not known whether the job terminated due to
       * the receipt of a signal. In both cases getTerminatingSignal() SHALL
       * NOT provide signal information.
       * @return if the job exited on a signal
       *
       */
      public boolean hasSignaled () {// fill the body in order to provide useful implementation
         return ((status & 0x02) != 0);
      }
      
      /** If hasSignaled() returns true, this returns a representation of the
       * signal that caused the termination of the job. For signals declared by
       * POSIX, the symbolic names SHALL be returned (e.g., SIGABRT, SIGALRM).<BR>
       * For signals not declared by POSIX, any other string may be returned.
       * @return the name of the terminating signal
       *
       */
      public String getTerminatingSignal () {// fill the body in order to provide useful implementation
         return TERMSIG;
      }
      
      /** If hasSignaled() returns true, this function returns true
       * if a core image of the terminated job was created.
       * @return whether a core dump image was created
       *
       */
      public boolean hasCoreDump () {// fill the body in order to provide useful implementation
         return ((status & 0x04) != 0);
      }
      
      /** Returns true if the job ended before entering the running state.
       * @return whether the job ended before entering the running state
       *
       */
      public boolean wasAborted () {// fill the body in order to provide useful implementation
         return ((status & 0x08) != 0);
      }      
   }

   public void testHasExited () {
      System.out.println ("testHasExited");
      
      assertTrue (jiAll.hasExited ());
      assertFalse (jiNone.hasExited ());
   }
   
   public void testGetExitStatus () {
      System.out.println ("testGetExitStatus");
      
      assertEquals (1, jiAll.getExitStatus ());
      assertEquals (0, jiNone.getExitStatus ());
   }
   
   public void testHasSignaled () {
      System.out.println ("testHasSignaled");
      
      assertTrue (jiAll.hasSignaled ());
      assertFalse (jiNone.hasSignaled ());
   }
   
   public void testGetTermSig () {
      System.out.println ("testGetTermSig");
      
      assertEquals (TERMSIG, jiAll.getTerminatingSignal ());
      assertEquals (TERMSIG, jiNone.getTerminatingSignal ());
   }
   
   public void testHasCoreDump () {
      System.out.println ("testHasCoreDump");
      
      assertTrue (jiAll.hasCoreDump ());
      assertFalse (jiNone.hasCoreDump ());
   }
   
   public void testWasAborted () {
      System.out.println ("testWasAborted");
      
      assertTrue (jiAll.wasAborted ());
      assertFalse (jiNone.wasAborted ());
   }
}
