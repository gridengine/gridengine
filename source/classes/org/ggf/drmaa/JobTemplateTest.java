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
 * JobTemplateTest.java
 * JUnit based test
 *
 * Created on November 13, 2004, 5:28 PM
 */

package org.ggf.drmaa;

import java.util.*;
import junit.framework.*;

/**
 *
 * @author dan.templeton@sun.com
 */
public class JobTemplateTest extends TestCase {
   private JobTemplate jt = null;
   
   public JobTemplateTest (java.lang.String testName) {
      super (testName);
   }
   
   public static Test suite () {
      TestSuite suite = new TestSuite (JobTemplateTest.class);
      return suite;
   }
   
   protected void setUp () {
      jt = new JobTemplateImpl ();
   }
   
   protected void tearDown () {
      jt = null;
   }
   
   /** Test of g|setRemoteCommand method, of class org.ggf.drmaa.JobTemplate. */
   public void testRemoteCommand () throws DrmaaException {
      System.out.println ("testRemoteCommand");
      
      jt.setRemoteCommand ("MyRemoteCommand");
      assertEquals ("MyRemoteCommand", jt.getRemoteCommand ());
   }
   
   /** Test of g|setArgs method, of class org.ggf.drmaa.JobTemplate. */
   public void testArgs () throws DrmaaException {
      System.out.println ("testArgs");
      
      String[] args = {"arg1", "arg2", "arg3"};
      
      jt.setArgs (args);
      
      String[] retArgs = jt.getArgs ();
      
      assertNotSame (args, retArgs);
      
      for (int count = 0; count < Math.min (args.length, retArgs.length); count++) {
         assertEquals (args[count], retArgs[count]);
      }
   }
   
   /** Test of g|setJobSubmissionState method, of class org.ggf.drmaa.JobTemplate. */
   public void testJobSubmissionState () throws DrmaaException {
      System.out.println ("testJobSubmissionState");

      jt.setJobSubmissionState (jt.HOLD);
      assertEquals (jt.HOLD, jt.getJobSubmissionState ());
      jt.setJobSubmissionState (jt.ACTIVE);
      assertEquals (jt.ACTIVE, jt.getJobSubmissionState ());
   }
   
   /** Test of g|setJobEnvironment method, of class org.ggf.drmaa.JobTemplate. */
   public void testJobEnvironment () throws DrmaaException {
      System.out.println ("testJobEnvironment");
      
      Properties env = new Properties ();
      env.put ("PATH", "/usr/bin");
      env.put ("LD_LIBRARY_PATH", "/usr/lib");

      jt.setJobEnvironment (env);
      
      Properties retEnv = jt.getJobEnvironment ();
      
      assertNotSame (env, retEnv);
      assertEquals (env, retEnv);
   }
   
   /** Test of g|setWorkingDirectory method, of class org.ggf.drmaa.JobTemplate. */
   public void testWorkingDirectory () throws DrmaaException {
      System.out.println ("testWorkingDirectory");

      jt.setWorkingDirectory ("/home/me");
      assertEquals ("/home/me", jt.getWorkingDirectory ());
   }
   
   /** Test of g|setJobCategory method, of class org.ggf.drmaa.JobTemplate. */
   public void testJobCategory () throws DrmaaException {
      System.out.println ("testJobCategory");
      
      jt.setJobCategory ("mycat");
      assertEquals ("mycat", jt.getJobCategory ());
   }
   
   /** Test of g|setNativeSpecification method, of class org.ggf.drmaa.JobTemplate. */
   public void testNativeSpecification () throws DrmaaException {
      System.out.println ("testNativeSpecification");

      jt.setNativeSpecification ("-shell yes");
      assertEquals ("-shell yes", jt.getNativeSpecification ());
   }
   
   /** Test of g|setEmail method, of class org.ggf.drmaa.JobTemplate. */
   public void testEmail () throws DrmaaException {
      System.out.println ("testEmail");
      
      String[] email = new String[] {"dant@germany", "admin"};
      
      jt.setEmail (email);
      
      String[] retEmail = jt.getEmail ();
      
      assertNotSame (email, retEmail);
      
      for (int count = 0; count < Math.min (email.length, retEmail.length); count++) {
         assertEquals (email[count], retEmail[count]);
      }
   }
   
   /** Test of g|setBlockEmail method, of class org.ggf.drmaa.JobTemplate. */
   public void testBlockEmail () throws DrmaaException {
      System.out.println ("testBlockEmail");

      jt.setBlockEmail (true);
      assertTrue (jt.getBlockEmail ());
      jt.setBlockEmail (false);
      assertFalse (jt.getBlockEmail ());
   }
   
   /** Test of g|setStartTime method, of class org.ggf.drmaa.JobTemplate. */
   public void testStartTime () throws DrmaaException {
      System.out.println ("testStartTime");

      PartialTimestamp pt = new PartialTimestamp ();
      Calendar cal = Calendar.getInstance ();
      
      pt.set (pt.HOUR_OF_DAY, cal.get (cal.HOUR_OF_DAY) + 1);
      pt.set (pt.MINUTE, 0);
      
      jt.setStartTime (pt);
      
      PartialTimestamp retPt = jt.getStartTime ();
      
      assertNotSame (pt, retPt);
      assertEquals (pt, retPt);
      
      pt.set (pt.HOUR_OF_DAY, cal.get (cal.HOUR_OF_DAY) - 1);
      
      jt.setStartTime (pt);
      
      retPt = jt.getStartTime ();
      
      assertNotSame (pt, retPt);
      assertEquals (pt, retPt);
            
      pt.set (pt.CENTURY, 19);
      
      try {
         jt.setStartTime (pt);
         fail ("Allowed start time in the past");
      }
      catch (IllegalArgumentException e) {
         /* Don't care. */
      }
   }
   
   /** Test of g|setJobName method, of class org.ggf.drmaa.JobTemplate. */
   public void testJobName () throws DrmaaException {
      System.out.println ("testJobName");

      jt.setJobName ("MyJob");
      assertEquals ("MyJob", jt.getJobName ());
   }
   
   /** Test of g|setInputPath method, of class org.ggf.drmaa.JobTemplate. */
   public void testInputPath () throws DrmaaException {
      System.out.println ("testInputPath");

      jt.setInputPath ("/tmp");
      assertEquals ("/tmp", jt.getInputPath ());
   }
   
   /** Test of g|setOutputPath method, of class org.ggf.drmaa.JobTemplate. */
   public void testOutputPath () throws DrmaaException {
      System.out.println ("testOutputPath");
      
      jt.setOutputPath ("/tmp");
      assertEquals ("/tmp", jt.getOutputPath ());
   }
   
   /** Test of g|setErrorPath method, of class org.ggf.drmaa.JobTemplate. */
   public void testErrorPath () throws DrmaaException {
      System.out.println ("testErrorPath");
      
      jt.setErrorPath ("/tmp");
      assertEquals ("/tmp", jt.getErrorPath ());
   }
   
   /** Test of g|setJoinFiles method, of class org.ggf.drmaa.JobTemplate. */
   public void testJoinFiles () throws DrmaaException {
      System.out.println ("testJoinFiles");

      jt.setJoinFiles (true);
      assertTrue (jt.getJoinFiles ());
      jt.setJoinFiles (false);
      assertFalse (jt.getJoinFiles ());
   }
   
   /** Test of setTransferFiles method, of class org.ggf.drmaa.JobTemplate. */
   public void testTransferFiles () throws DrmaaException {
      System.out.println ("testTransferFiles");
      
      FileTransferMode mode = new FileTransferMode (true, false, true);
      
      jt.setTransferFiles (mode);
      
      FileTransferMode retMode = jt.getTransferFiles ();
      
      assertNotSame (mode, retMode);
      assertEquals (mode, retMode);
   }
   
   /** Test of g|setDeadlineTime method, of class org.ggf.drmaa.JobTemplate. */
   public void testDeadlineTime () throws DrmaaException {
      System.out.println ("testDeadlineTime");

      PartialTimestamp pt = new PartialTimestamp ();
      
      jt.setDeadlineTime (pt);
      
      PartialTimestamp retPt = jt.getDeadlineTime ();
      
      assertNotSame (pt, retPt);
      assertEquals (pt, retPt);
   }
   
   /** Test of g|setHardWallclockTimeLimit method, of class org.ggf.drmaa.JobTemplate. */
   public void testHardWallclockTimeLimit () throws DrmaaException {
      System.out.println ("testHardWallclockTimeLimit");

      jt.setHardWallclockTimeLimit (101L);
      assertEquals (101L, jt.getHardWallclockTimeLimit ());
   }
   
   /** Test of g|setSoftWallclockTimeLimit method, of class org.ggf.drmaa.JobTemplate. */
   public void testSoftWallclockTimeLimit () throws DrmaaException {
      System.out.println ("testSoftWallclockTimeLimit");
      
      jt.setSoftWallclockTimeLimit (101L);
      assertEquals (101L, jt.getSoftWallclockTimeLimit ());
   }
   
   /** Test of g|setHardRunDurationLimit method, of class org.ggf.drmaa.JobTemplate. */
   public void testHardRunDurationLimit () throws DrmaaException {
      System.out.println ("testHardRunDurationLimit");
      
      jt.setHardRunDurationLimit (101L);
      assertEquals (101L, jt.getHardRunDurationLimit ());
   }
   
   /** Test of g|setSoftRunDurationLimit method, of class org.ggf.drmaa.JobTemplate. */
   public void testSoftRunDurationLimit () throws DrmaaException {
      System.out.println ("testSoftRunDurationLimit");
      
      jt.setSoftRunDurationLimit (101L);
      assertEquals (101L, jt.getSoftRunDurationLimit ());
   }
   
   /** Test of getAttributeNames method, of class org.ggf.drmaa.JobTemplate. */
   public void testGetAttributeNames () {
      System.out.println ("testGetAttributeNames");
      
      ArrayList names = new ArrayList (21);
      
      names.add ("args");
      names.add ("blockEmail");
      names.add ("email");
      names.add ("errorPath");
      names.add ("inputPath");
      names.add ("jobCategory");
      names.add ("jobEnvironment");
      names.add ("jobName");
      names.add ("jobSubmissionState");
      names.add ("joinFiles");
      names.add ("nativeSpecification");
      names.add ("outputPath");
      names.add ("remoteCommand");
      names.add ("startTime");
      names.add ("workingDirectory");
      names.add ("transferFiles");
      names.add ("deadlineTime");
      names.add ("hardWallclockTimeLimit");
      names.add ("softWallclockTimeLimit");
      names.add ("hardRunDurationLimit");
      names.add ("softRunDurationLimit");

      List retNames = jt.getAttributeNames ();
      
      Collections.sort (names);
      Collections.sort (retNames);
      
      assertEquals (names, retNames);
   }
   
   /** Test of getOptionalAttributeNames method, of class org.ggf.drmaa.JobTemplate. */
   public void testGetOptionalAttributeNames () {
      System.out.println ("testGetOptionalAttributeNames");
      
      ArrayList names = new ArrayList (6);
      
      names.add ("transferFiles");
      names.add ("deadlineTime");
      names.add ("hardWallclockTimeLimit");
      names.add ("softWallclockTimeLimit");
      names.add ("hardRunDurationLimit");
      names.add ("softRunDurationLimit");

      List retNames = jt.getOptionalAttributeNames ();
      
      Collections.sort (names);
      Collections.sort (retNames);
      
      assertEquals (names, retNames);
   }
   
   /** Test of toString method, of class org.ggf.drmaa.JobTemplate. */
   public void testToString () throws DrmaaException {
      System.out.println ("testToString");
      
      assertEquals ("{blockEmail = false} {jobSubmissionState = ACTIVE} {joinFiles = false}",
                    jt.toString ());
      
      jt.setJobCategory ("myCategory");
      
      assertEquals ("{blockEmail = false} {jobCategory = myCategory} {jobSubmissionState = ACTIVE} {joinFiles = false}",
                    jt.toString ());
      
      jt.setArgs (new String[] {"arg1", "arg2"});
      
      assertEquals ("{args = \"arg1\", \"arg2\"} {blockEmail = false} {jobCategory = myCategory} {jobSubmissionState = ACTIVE} {joinFiles = false}",
                    jt.toString ());
      
      jt.setJoinFiles (true);
      
      assertEquals ("{args = \"arg1\", \"arg2\"} {blockEmail = false} {jobCategory = myCategory} {jobSubmissionState = ACTIVE} {joinFiles = true}",
                    jt.toString ());
      
      jt.setJobSubmissionState (jt.HOLD);
      
      assertEquals ("{args = \"arg1\", \"arg2\"} {blockEmail = false} {jobCategory = myCategory} {jobSubmissionState = HOLD} {joinFiles = true}",
                    jt.toString ());
      
      jt.setNativeSpecification ("-l arch=sol-sparc64");
      
      assertEquals ("{args = \"arg1\", \"arg2\"} {blockEmail = false} {jobCategory = myCategory} {jobSubmissionState = HOLD} {joinFiles = true} {nativeSpecification = \"-l arch=sol-sparc64\"}",
                    jt.toString ());
      
      Properties env = new Properties ();
      env.setProperty ("PATH", "/tmp:/usr/bin");
      env.setProperty ("SHELL", "/usr/bin/csh");
      jt.setJobEnvironment (env);
      
      assertEquals ("{args = \"arg1\", \"arg2\"} {blockEmail = false} {jobCategory = myCategory} {jobEnvironment = [\"PATH\" = \"/tmp:/usr/bin\"], [\"SHELL\" = \"/usr/bin/csh\"]} {jobSubmissionState = HOLD} {joinFiles = true} {nativeSpecification = \"-l arch=sol-sparc64\"}",
                    jt.toString ());
      
      jt.setTransferFiles (new FileTransferMode (true, false, true));
      
      assertEquals ("{args = \"arg1\", \"arg2\"} {blockEmail = false} {jobCategory = myCategory} {jobEnvironment = [\"PATH\" = \"/tmp:/usr/bin\"], [\"SHELL\" = \"/usr/bin/csh\"]} {jobSubmissionState = HOLD} {joinFiles = true} {nativeSpecification = \"-l arch=sol-sparc64\"} {transferFiles = \"input, error\"}",
                    jt.toString ());
      
      jt.setHardWallclockTimeLimit (100L);
      
      assertEquals ("{args = \"arg1\", \"arg2\"} {blockEmail = false} {hardWallclockTimeLimit = 100ms} {jobCategory = myCategory} {jobEnvironment = [\"PATH\" = \"/tmp:/usr/bin\"], [\"SHELL\" = \"/usr/bin/csh\"]} {jobSubmissionState = HOLD} {joinFiles = true} {nativeSpecification = \"-l arch=sol-sparc64\"} {transferFiles = \"input, error\"}",
                    jt.toString ());
      
      jt.setStartTime (new PartialTimestamp (19, 10, 49));
      
      assertEquals ("{args = \"arg1\", \"arg2\"} {blockEmail = false} {hardWallclockTimeLimit = 100ms} {jobCategory = myCategory} {jobEnvironment = [\"PATH\" = \"/tmp:/usr/bin\"], [\"SHELL\" = \"/usr/bin/csh\"]} {jobSubmissionState = HOLD} {joinFiles = true} {nativeSpecification = \"-l arch=sol-sparc64\"} {startTime = \"19:10:49\"} {transferFiles = \"input, error\"}",
                    jt.toString ());
   }

   /** Test of equals method, of class org.ggf.drmaa.JobTemplate. */
   public void testEquals () throws DrmaaException {
      System.out.println ("testEquals");

      JobTemplate jt2 = new JobTemplateImpl ();
      
      assertTrue (jt.equals (jt2));
      assertTrue (jt2.equals (jt));
      jt.setBlockEmail (true);
      assertFalse (jt.equals (jt2));
      assertFalse (jt2.equals (jt));
      jt2.setBlockEmail (true);
      assertTrue (jt.equals (jt2));
      assertTrue (jt2.equals (jt));
      jt2.setDeadlineTime (new PartialTimestamp (10, 21, 01));
      assertFalse (jt.equals (jt2));
      assertFalse (jt2.equals (jt));
      jt.setDeadlineTime (new PartialTimestamp (10, 21, 01));
      assertTrue (jt.equals (jt2));
      assertTrue (jt2.equals (jt));
   }

   /** Test of hashCode method, of class org.ggf.drmaa.JobTemplate. */
   public void testHashCode () throws DrmaaException {
      System.out.println ("testHashCode");

      JobTemplate jt2 = new JobTemplateImpl ();
      
      assertEquals (jt.hashCode (), jt2.hashCode ());
      jt.setBlockEmail (true);
      assertFalse (jt.hashCode () == jt2.hashCode ());
      jt2.setBlockEmail (true);
      assertEquals (jt.hashCode (), jt2.hashCode ());
      jt2.setDeadlineTime (new PartialTimestamp (10, 21, 01));
      assertFalse (jt.hashCode () == jt2.hashCode ());
      jt.setDeadlineTime (new PartialTimestamp (10, 21, 01));
      assertEquals (jt.hashCode (), jt2.hashCode ());
   }
   
   /** Generated implementation of abstract class org.ggf.drmaa.JobTemplate. Please fill dummy bodies of generated methods. */
   private class JobTemplateImpl extends JobTemplate {
      private FileTransferMode mode = null;
      private PartialTimestamp deadline = null;
      private long wch = 0L;
      private long wcs = 0L;
      private long rdh = 0L;
      private long rds = 0L;
      
      protected JobTemplateImpl () {
         super ();
      }
      
      protected List getOptionalAttributeNames () {
         ArrayList names = new ArrayList (6);
         
         names.add ("transferFiles");
         names.add ("deadlineTime");
         names.add ("hardWallclockTimeLimit");
         names.add ("softWallclockTimeLimit");
         names.add ("hardRunDurationLimit");
         names.add ("softRunDurationLimit");
         
         return names;
      }
      
      public void setTransferFiles (FileTransferMode mode) throws DrmaaException {
         if (mode != null) {
            this.mode = (FileTransferMode)mode.clone ();
         }
         else {
            this.mode = null;
         }
      }

      public FileTransferMode getTransferFiles () {
         if (mode != null) {
            return (FileTransferMode)mode.clone ();
         }
         else {
            return null;
         }
      }

      public void setDeadlineTime (PartialTimestamp deadline) throws DrmaaException {
         if (deadline != null) {
            this.deadline = (PartialTimestamp)deadline.clone ();
         }
         else {
            this.deadline = null;
         }
      }

      public PartialTimestamp getDeadlineTime () {
         if (deadline != null) {
            return (PartialTimestamp)deadline.clone ();
         }
         else {
            return null;
         }
      }

      public void setHardWallclockTimeLimit (long hardWallclockLimit) throws DrmaaException {
         wch = hardWallclockLimit;
      }

      public long getHardWallclockTimeLimit () {
         return wch;
      }

      public void setSoftWallclockTimeLimit (long softWallclockLimit) throws DrmaaException {
         wcs = softWallclockLimit;
      }

      public long getSoftWallclockTimeLimit () {
         return wcs;
      }

      public void setHardRunDurationLimit (long hardRunLimit) throws DrmaaException {
         rdh = hardRunLimit;
      }

      public long getHardRunDurationLimit () {
         return rdh;
      }

      public void setSoftRunDurationLimit (long softRunLimit) throws DrmaaException {
         rds = softRunLimit;
      }

      public long getSoftRunDurationLimit () {
         return rds;
      }
   }
}
