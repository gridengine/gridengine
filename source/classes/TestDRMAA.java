/*
 * TestDRMAA.java
 *
 * Created on May 4, 2004, 6:16 PM
 */

import java.io.*;
import java.util.*;

import org.ggf.drmaa.*;

/**
 *
 * @author  dan.templeton@sun.com
 */
public class TestDRMAA {
   private static DRMAASession session = null;
   private static LinkedList jobIds = null;
   private static final Object lock = new Object ();
   
   /** Creates a new instance of TestDRMAA */
   public TestDRMAA () throws Exception {
      jobIds = new LinkedList ();
      
      DRMAASessionFactory factory = DRMAASessionFactory.getFactory ();
      
      session = factory.getSession ();
      session.init (null);
      
      System.out.println ("DRMS: " + session.getDRMSystem ());
      System.out.println ("Contact: " + session.getContact ());
      System.out.println ("Implementation: " + session.getDRMAAImplementation ());
      
      DRMAASession.Version version = session.getVersion ();
      
      System.out.println ("Version: " + Integer.toString (version.getMajor ()) + "." + Integer.toString (version.getMinor ()));
      System.out.println ("--------------------------------------------------");
      System.out.println ("");
      
      log ("0: Starting test threads...");
      
      new Thread (new SubmitWaitTester (1)).start ();
      new Thread (new BulkSubmitSyncTester (2)).start ();
//      new Thread (new SubmitDeleteTester (3)).start ();
//      new Thread (new SubmitHoldReleaseTester (4)).start ();
//      new Thread (new SubmitSuspendResumeTester (5)).start ();
//      new Thread (new SubmitTester (6, 5)).start ();
//      new Thread (new BulkSubmitTester (7, 5)).start ();
//      new Thread (new WaitTester (8)).start ();
//      new Thread (new SyncTester (9)).start ();      
      
      sleep (5);
      
      synchronized (lock) {         
         lock.notifyAll ();
      }
      
      log ("0: Sleeping");
      sleep (120);
      
      log ("0: Exiting");
      session.exit ();
   }
   
   /**
    * @param args the command line arguments
    */
   public static void main (String[] args) throws Exception {
      new TestDRMAA ();
      
      System.exit (0);
   }
   
   static synchronized void log (String message) {
      System.out.println (message);
   }
   
   private abstract class Tester implements Runnable {
      private int id = 0;
      protected JobTemplate jt = null;
      
      Tester (int id) {
         this.id = id;
      }
      
      void log (String message) {
         TestDRMAA.log (Integer.toString (id) + ": " + message);
      }
      
      void log (Throwable e) {
         StringWriter sout = new StringWriter ();
         PrintWriter pout = new PrintWriter (sout);
         e.printStackTrace (pout);
         pout.close ();
         TestDRMAA.log (Integer.toString (id) + ": " + sout.toString ());
      }
      
      public void run () {
         try {
            synchronized (lock) {
               lock.wait ();
            }
         }
         catch (InterruptedException e) {
            log (e);
            return;
         }
         
         log ("Starting up");
         
         while (true) {
            try {
               test ();
            }
            catch (NoActiveSessionException e) {
               log ("No active session. Shutting down.");
               break;
            }
            catch (Exception e) {
               log (e);
            }
         }
      }
      
      public abstract void test () throws DRMAAException;
      
      public void cleanup () {
         try {
            jt.delete ();
         }
         catch (DRMAAException e) {
            log (e);
         }
      }
   }
   
   private class SubmitWaitTester extends Tester {
      SubmitWaitTester (int id) throws DRMAAException {
         super (id);
         
         jt = createJobTemplate ("/tmp/dant/examples/jobs/sleeper.sh", 5, false);
      }
      
      public void test () throws DRMAAException {
         LinkedList ids = new LinkedList ();
         String jobId = null;
         
         for (int count = 0; count < 10; count++) {
            jobId = session.runJob (jt);
            
            log ("Submitted job " + jobId);
            
            ids.add (jobId);
         }
         
         Iterator i = ids.iterator ();
         
         while (i.hasNext ()) {
            JobInfo status = null;
            jobId = (String)i.next ();
            
            status = session.wait (jobId, DRMAASession.TIMEOUT_WAIT_FOREVER);
            
            /* report how job finished */
            if (status.wasAborted ()) {
               log ("job \"" + status.getJobId () + "\" never ran");
            }
            else if (status.hasExited ()) {
               log ("job \"" + status.getJobId () + "\" finished regularly with exit status " + status.getExitStatus ());
            }
            else if (status.hasSignaled ()) {
               log ("job \"" + status.getJobId () + "\" finished due to signal " + status.getTerminatingSignal ());
            }
            else {
               log ("job \"" + status.getJobId () + "\" finished with unclear conditions");
            }
         }
      }
   }
   
   private class BulkSubmitSyncTester extends Tester {
      BulkSubmitSyncTester (int id) throws DRMAAException {
         super (id);
         
         jt = createJobTemplate ("/tmp/dant/examples/jobs/sleeper.sh", 5, true);
      }
      
      public void test () throws DRMAAException {
         List jobIds = session.runBulkJobs (jt, 1, 20, 1);
         
         log ("Submitted " + jobIds.size () + " jobs");
         
         session.synchronize (jobIds, DRMAASession.TIMEOUT_WAIT_FOREVER, true);
         
         log ("All jobs have finished");
      }
   }
   
   private class BulkSubmitTester extends Tester {
      private int sleep = 0;
      
      BulkSubmitTester (int id, int sleep) throws DRMAAException {
         super (id);
         
         this.sleep = sleep;
         
         jt = createJobTemplate ("/tmp/dant/examples/jobs/sleeper.sh", sleep, true);
      }
      
      public void test () throws DRMAAException {
         List jobIds = session.runBulkJobs (jt, 1, 20, 1);
         
         log ("Submitted " + jobIds.size () + " jobs");
         
         if (sleep > 0) {
            sleep (sleep);
         }
      }
   }
   
   private class SubmitTester extends Tester {
      private int sleep = 0;
      
      SubmitTester (int id, int sleep) throws DRMAAException {
         super (id);
         
         this.sleep = sleep;
         
         jt = createJobTemplate ("/tmp/dant/examples/jobs/sleeper.sh", sleep, false);
         
         if (sleep > 0) {
            sleep (sleep);
         }
      }
      
      public void test () throws DRMAAException {
         String jobId = session.runJob (jt);
         
         log ("Submitted job " + jobId);
      }
   }
   
   private class WaitTester extends Tester {
      WaitTester (int id) throws DRMAAException {
         super (id);
      }
      
      public void test () throws DRMAAException {
         JobInfo info = null;
         
         try {
            info = session.wait (DRMAASession.JOB_IDS_SESSION_ANY,
            DRMAASession.TIMEOUT_WAIT_FOREVER);
            
            if (info.wasAborted ()) {
               log ("job \"" + info.getJobId () + "\" never ran");
            }
            else if (info.hasExited ()) {
               log ("job \"" + info.getJobId () + "\" finished regularly with exit status " + info.getExitStatus ());
            }
            else if (info.hasSignaled ()) {
               log ("job \"" + info.getJobId () + "\" finished due to signal " + info.getTerminatingSignal ());
            }
            else {
               log ("job \"" + info.getJobId () + "\" finished with unclear conditions");
            }
         }
         catch (NoResourceUsageDataException e) {
            log ("job \"" + info.getJobId () + "\" has already been reaped");
         }
      }
   }
   
   private class SyncTester extends Tester {
      SyncTester (int id) throws DRMAAException {
         super (id);
      }
      
      public void test () throws DRMAAException {
         session.synchronize (DRMAASession.JOB_IDS_SESSION_ALL,
         DRMAASession.TIMEOUT_WAIT_FOREVER, false);
         
         log ("All jobs have finished");
      }
   }
   
   private class SubmitDeleteTester extends Tester {
      SubmitDeleteTester (int id) throws DRMAAException {
         super (id);
         
         jt = createJobTemplate ("/tmp/dant/examples/jobs/sleeper.sh", 600, false);
      }
      
      public void test () throws DRMAAException {
         String jobId = null;
         
         jobId = session.runJob (jt);
         
         log ("Submitted job " + jobId);
         
         session.control (jobId, DRMAASession.TERMINATE);
         
         log ("Deleted job " + jobId);
      }
   }
   
   private class SubmitHoldReleaseTester extends Tester {
      SubmitHoldReleaseTester (int id) throws DRMAAException {
         super (id);
         
         jt = createJobTemplate ("/tmp/dant/examples/jobs/sleeper.sh", 60, false);
      }
      
      public void test () throws DRMAAException {
         String jobId = null;
         
         jobId = session.runJob (jt);
         
         log ("Submitted job " + jobId);
         
         session.control (jobId, DRMAASession.HOLD);
         
         log ("Job state is " + statusToString (session.getJobProgramStatus (jobId)));
         
         sleep (20);
         
         log ("Job state is " + statusToString (session.getJobProgramStatus (jobId)));
         
         session.control (jobId, DRMAASession.RELEASE);
         
         log ("Job state is " + statusToString (session.getJobProgramStatus (jobId)));
         
         sleep (20);
         
         log ("Job state is " + statusToString (session.getJobProgramStatus (jobId)));
      }
   }
   
   private class SubmitSuspendResumeTester extends Tester {
      SubmitSuspendResumeTester (int id) throws DRMAAException {
         super (id);
         
         jt = createJobTemplate ("/tmp/dant/examples/jobs/sleeper.sh", 60, false);
      }
      
      public void test () throws DRMAAException {
         String jobId = null;
         
         jobId = session.runJob (jt);
         
         log ("Submitted job " + jobId);
         
         session.control (jobId, DRMAASession.SUSPEND);
         
         log ("Job state is " + statusToString (session.getJobProgramStatus (jobId)));
         
         sleep (20);
         
         log ("Job state is " + statusToString (session.getJobProgramStatus (jobId)));
         
         session.control (jobId, DRMAASession.RESUME);
         
         log ("Job state is " + statusToString (session.getJobProgramStatus (jobId)));
         
         sleep (20);
         
         log ("Job state is " + statusToString (session.getJobProgramStatus (jobId)));
      }
   }
   
   private JobTemplate createJobTemplate (String jobPath, int seconds, boolean isBulkJob) throws DRMAAException {
      JobTemplate jt = session.createJobTemplate ();
      
      jt.setWorkingDirectory ("$drmaa_hd_ph$");
      jt.setRemoteCommand (jobPath);
      jt.setInputParameters (new String[] {Integer.toString (seconds)});
      jt.setJoinFiles (true);
      
      if (!isBulkJob) {
         jt.setOutputPath (":$drmaa_hd_ph$/DRMAA_JOB");
      }
      else {
         jt.setOutputPath (":$drmaa_hd_ph$/DRMAA_JOB$drmaa_incr_ph$");
      }
      
      return jt;
   }
   
   static String statusToString (int status) {
      switch (status) {
         case DRMAASession.QUEUED_ACTIVE:
            return "Queued";
         case DRMAASession.SYSTEM_ON_HOLD:
            return "System Hold";
         case DRMAASession.USER_ON_HOLD:
            return "User Hold";
         case DRMAASession.USER_SYSTEM_ON_HOLD:
            return "User & System Hold";
         case DRMAASession.RUNNING:
            return "Running";
         case DRMAASession.SYSTEM_SUSPENDED:
            return "System Suspended";
         case DRMAASession.USER_SUSPENDED:
            return "User Suspended";
         case DRMAASession.DONE:
            return "Finished";
         case DRMAASession.FAILED:
            return "Failed";
         default:
            return "Undetermined";
      }
   }
   
   static void sleep (int time) {
      try {
         Thread.sleep (time * 1000);
      }
      catch (InterruptedException e) {
         // Don't care
      }
   }
}
