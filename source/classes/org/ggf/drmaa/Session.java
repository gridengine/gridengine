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

package org.ggf.drmaa;

import java.util.*;

/** <P>This interface represents a the operations available for interacting with the
 * DRM.  In DRMAA, (almost) all DRM interaction occur within the context of a
 * session.  The spec also strongly recommends that a DRMAA implementation not
 * allow concurrent sessions.  Since DRMAA has no facility for user authentication
 * or authorization, most DRMAA implementations will likely only support one
 * session per implementation instance.</p>
 * <p>In order to use a Session, it must first be initialized.  Once initialized it
 * is the responsibility of the programmer to ensure that the session also be
 * explicitly terminated.  Otherwise, session artifacts may be left behind on
 * the client and/or server.  A handy way to make sure the Session is terminated is
 * to set up a shutdown hook to call the exit() method on the active session.</p>
 * <p>To get a Session implementation appropriate for the DRM in use, one uses the
 * SessionFactory.getSession() method.</p>
 * <p>Example:</p>
 * <pre>public static void main (String[] args) throws Exception {
 *   SessionFactory factory = SessionFactory.getFactory ();
 *   Session session = factory.getSession ();
 *
 *   try {
 *      session.init (null);
 *      JobTemplate jt = session.createJobTemplate ();
 *      jt.setRemoteCommand ("sleeper.sh");
 *      jt.setArgs (new String[] {"5"});
 *
 *      String id = session.runJob (jt);
 *
 *      session.deleteJobTemplate (jt);
 *
 *      while (session.getJobProgramStatus (id) != Session.RUNNING) {
 *         Thread.sleep (1000);
 *      }
 *
 *      System.out.println ("Job " + id + " is now running.");
 *
 *      session.control (id, Session.SUSPEND);
 *      Thread.sleep (1000);
 *      session.control (id, Session.RELEASE);
 *
 *      JobInfo info = session.wait (id, Session.TIMEOUT_WAIT_FOREVER);
 *
 *      System.out.println ("Job " + info.getJobId () + " exited with status: " +
 *                          info.getExitStatus ());
 *
 *      session.exit ();
 *   }
 *   catch (DrmaaException e) {
 *      System.out.println ("Error: " + e.getMessage ());
 *   }
 * }
 * </pre>
 * @author dan.templeton@sun.com
 * @see java.lang.Runtime#addShutdownHook
 * @see org.ggf.drmaa.SessionFactory
 * @see <a
 * href="http://gridengine.sunsource.net/project/gridengine/howto/drmaa_java.html">Grid
 * Engine DRMAA Java[TM] Language Binding HowTo</a>
 * @since 0.5
 */
public abstract interface Session {
	/** Suspend the job */
	public static final int SUSPEND = 0;
	/** Resume the job */
	public static final int RESUME = 1;
	/** Put the job on hold */
	public static final int HOLD = 2;
	/** Release the hold on the job */
	public static final int RELEASE = 3;
	/** Kill the job */
	public static final int TERMINATE = 4;
	/** All jobs submitted during this DRMAA session */
	public static final String JOB_IDS_SESSION_ALL = "DRMAA_JOB_IDS_SESSION_ALL";
	/** Any job from the session */
	public static final String JOB_IDS_SESSION_ANY = "DRMAA_JOB_IDS_SESSION_ANY";
	/** Wait indefinitely for a result */
	public static final long TIMEOUT_WAIT_FOREVER = -1L;
	/** Return immediately if no result is available */
	public static final long TIMEOUT_NO_WAIT = 0L;
	/** Job status cannot be determined */
	public static final int UNDETERMINED = 0x00;
	/** Job is queued and active */
	public static final int QUEUED_ACTIVE = 0x10;
	/** Job is queued and in system hold */
	public static final int SYSTEM_ON_HOLD = 0x11;
	/** Job is queued and in user hold */
	public static final int USER_ON_HOLD = 0x12;
	/** Job is queued and in user and system hold */
	public static final int USER_SYSTEM_ON_HOLD = 0x13;
	/** Job is running */
	public static final int RUNNING = 0x20;
	/** Job is system suspended */
	public static final int SYSTEM_SUSPENDED = 0x21;
	/** Job is user suspended */
	public static final int USER_SUSPENDED = 0x22;
	/** Job is user suspended */
	public static final int USER_SYSTEM_SUSPENDED = 0x23;
	/** Job has finished normally */
	public static final int DONE = 0x30;
	/** Job finished, but terminated abnormally */
	public static final int FAILED = 0x40;
	
   /** <p>Initialize the DRMAA implementation.
    * <i>contact</i> is an implementation-dependent string that may be used to specify
    * which DRM system to use.  This routine must be called before any
    * other DRMAA calls, except for getVersion(), getDRMSystem(),
    * getDRMAAImplementation(), or getContact().</p>
    * If contact is <CODE>null</CODE>, the default DRM system is used,
    * provided there is only one DRMAA implementation in the module.
    * If there is more than one DRMAA implementation in the module, init() throws a
    * NoDefaultContactStringSelectedException.
    * init() should be called only once, by only one of the threads. The main thread is
    * recommended.  A call to init() by another thread or additional calls to init()
    * by the same thread with throw a SessionAlreadyActiveException.
    * @param contact implementation-dependent string that may be used to specify
    * which DRM system to use.  If null, will select the default DRM if there
    * is only one DRMAA implementation available.
    * @throws DrmaaException May be be one of the following:
    * <UL>
    * <LI>InvalidContactStringException</LI>
    * <LI>AlreadyActiveSessionException</LI>
    * <LI>DefaultContactStringException</LI>
    * <LI>NoDefaultContactStringSelectedException</LI>
    * </UL>
    */
	public abstract void init (String contact) throws DrmaaException;
	
   /** Disengage from DRM and allow the DRMAA implementation to perform
    * any necessary internal cleanup.
    * This routine ends the current DRMAA session but doesn't affect any
    * jobs (e.g., queued and running jobs remain queued and running).
    * exit() should be called only once, by only one of the threads.  Additional
    * calls to exit() beyond the first will throw a NoActiveSessionException.
    * @throws DrmaaException May be one of the following:
    * <UL>
    * <LI>DrmsExitException</LI>
    * <LI>NoActiveSessionException</LI>
    * </UL>
    */
	public abstract void exit () throws DrmaaException;
	
	/** Get a new job template.  The job template is used to set the
    * environment for jobs to be submitted.  Once the job template has been created,
    * it should also be deleted (via deleteJobTemplate()) when no longer needed.
    * Failure to do so may result in a memory leak.
    * @throws DrmaaException May be one of the following:
    * <UL>
    * <LI>DrmCommunicationException</LI>
    * </UL>
    * @return a blank JobTemplate object
    * @see org.ggf.drmaa.JobTemplate
    */
	public abstract JobTemplate createJobTemplate () throws DrmaaException;
   
   /** Deallocate a job template. This routine has no effect on running jobs.
    * @param jt the JobTemplate to delete
    * @throws DrmaaException May be one of the following:
    * <UL>
    * <LI>DrmCommunicationException</LI>
    * </UL>
    */
   public abstract void deleteJobTemplate (JobTemplate jt) throws DrmaaException;

   /** Submit a job with attributes defined in the job template, <i>jt</i>.
    * The returned job identifier is a String identical to that returned
    * from the underlying DRM system.
    * @param jt the job template to be used to create the job
    * @throws DrmaaException May be one of the following:
    * <UL>
    * <LI>TryLaterException</LI>
    * <LI>DeniedByDrmException</LI>
    * <LI>DrmCommunicationException</LI>
    * <LI>AuthorizationException</LI>
    * </UL>
    * @return job identifier String identical to that returned from the
    * underlying DRM system
    */
	public abstract String runJob (JobTemplate jt) throws DrmaaException;

   /** <p>Submit a set of parametric jobs, dependent on the implied loop index,
    * each with attributes defined in the job template, <i>jt</i>.
    * The returned job identifiers are Strings identical to those returned
    * from the underlying DRM system.</p>
    * <p>The JobTemplate class defines a <code>PARAMETRIC_INDEX</code> placeholder for use in
    * specifying paths.  This placeholder is used to represent the individual
    * identifiers of the tasks submitted through this method.</p>
    * @return job identifier Strings identical to that returned by the
    * underlying DRM system
    * @param start the starting value for the loop index
    * @param end the terminating value for the loop index
    * @param incr the value by which to increment the loop index each iteration
    * @param jt the job template to be used to create the job
    * @throws DrmaaException May be one of the following:
    * <UL>
    * <LI>TryLaterException</LI>
    * <LI>DeniedByDrmException</LI>
    * <LI>DrmCommunicationException</LI>
    * <LI>AuthorizationException</LI>
    * </UL>
    */
	public abstract List runBulkJobs (JobTemplate jt, int start, int end, int incr) throws DrmaaException;
	
   /** <p>Hold, release, suspend, resume, or kill the job identified by <i>jobId</i>.
    * If <i>jobId</i> is <code>JOB_IDS_SESSION_ALL</code>, then this routine acts on all jobs
    * <B>submitted</B> during this DRMAA session up to the moment control() is
    * called.  To avoid thread races in multithreaded applications, the DRMAA
    * implementation user should explicitly synchronize this call with any other
    * job submission calls or control calls that may change the number of remote
    * jobs.</P>
    * <p>The legal values for <i>action</i> and their meanings are:
    * <UL>
    * <LI><code>SUSPEND</code>: stop the job,</LI>
    * <LI><code>RESUME</code>: (re)start the job,</LI>
    * <LI><code>HOLD</code>: put the job on-hold,</LI>
    * <LI><code>RELEASE</code>: release the hold on the job, and</LI>
    * <LI><code>TERMINATE</code>: kill the job.</LI>
    * </UL>
    * </p>
    * <p>This method returns once the action has been acknowledged by
    * the DRM system, but does not necessarily wait until the action
    * has been completed.</p>
    * <p>Some DRMAA implementations may allow this method to be used to control jobs
    * submitted external to the DRMAA session, such as jobs submitted by other DRMAA
    * session in other DRMAA implementations or jobs submitted via native
    * utilities.</p>
    * @param jobId The id of the job to control
    * @param action the control action to be taken
    * @throws DrmaaException May be one of the following:
    * <UL>
    * <LI>DrmCommunicationException</LI>
    * <LI>AuthorizationException</LI>
    * <LI>ResumeInconsistentStateException</LI>
    * <LI>SuspendInconsistentStateException</LI>
    * <LI>HoldInconsistentStateException</LI>
    * <LI>ReleaseInconsistentStateException</LI>
    * <LI>InvalidJobException</LI>
    * </UL>
    */
	public abstract void control (String jobId, int action) throws DrmaaException;
	
	/** <p>Wait until all jobs specified by <i>jobIds</i> have finished execution.
    * If <i>jobIds</i> contains <code>JOB_IDS_SESSION_ALL</code>, then this method waits for all
    * jobs <B>submitted</B> during this DRMAA session up to the moment
    * synchronize() is called.  To avoid thread race conditions in multithreaded
    * applications, the DRMAA implementation user should explicitly synchronize
    * this call with any other job submission calls or control calls that may
    * change the number of remote jobs.</p>
    * <p>To prevent blocking indefinitely in this call, the caller may use a timeout
    * specifying after how many seconds to block in this call.  The value
    * <code>TIMEOUT_WAIT_FOREVER MAY</code> be specified to wait indefinitely for a result.
    * The value <code>TIMEOUT_NO_WAIT MAY</code> be specified to return immediately if no
    * result is available. If the call exits before the timeout has elapsed, all the
    * jobs have been waited on or there was an interrupt.
    * If the invocation exits on timeout, an ExitTimeException is thrown.
    * The caller should check system time before and after this call in order to
    * be sure of how much time has passed.</p>
    * <p>The <i>dispose</i> parameter specifies how to treat the reaping of the remote job's
    * internal data record, which includes a record of the job's consumption of system
    * resources during its execution and other statistical information.  If set to
    * <code>true</code>, the DRM will dispose of the job's data record at the end of
    * the synchroniize() call.  If set to <code>false</code>, the data record will be
    * left for future access via the wait() method.</p>
    * @param jobIds the ids of the jobs to synchronize
    * @param timeout the maximum number of seconds to wait
    * @param dispose specifies how to treat reaping information
    * @throws DrmaaException May be one of the following:
    * <UL>
    * <LI>DrmCommunicationException</LI>
    * <LI>AuthorizationException</LI>
    * <LI>ExitTimeoutException</LI>
    * <LI>InvalidJobException</LI>
    * </UL>
    * @see #wait
    */
	public abstract void synchronize (List jobIds, long timeout, boolean dispose) throws DrmaaException;
	
   /** <P>This method will wait for a job with <i>jobId</i> to finish execution or fail.
    * If the special string, <code>JOB_IDS_SESSION_ANY</code>, is provided as the
    * <i>jobId</i>,
    * this routine will wait for any job from the session. This routine is
    * modeled on the wait3 POSIX routine.</p>
    * <P>The <i>timeout</i> value is used to specify the desired behavior when a result
    * is not immediately available.  The value, <code>TIMEOUT_WAIT_FOREVER<code>, may be
    * specified to wait indefinitely for a result.  The value, <code>TIMEOUT_NO_WAIT<code>,
    * may be specified to return immediately if no result is available.
    * Alternatively, a number of seconds may be specified to indicate how
    * long to wait for a result to become available.</p>
    * <p>If the call exits before timeout, either the job has been waited on
    * successfully or there was an interrupt.
    * If the invocation exits on timeout, an ExitTimeoutException is thrown.
    * The caller should check system time before and after this call
    * in order to be sure how much time has passed.</p>
    * <p>The routine reaps job data records on a successful call, so any subsequent
    * calls to wait() will fail, throwing an InvalidJobException, meaning
    * that the job's data record has been already reaped. This exception is the same as
    * if the job was unknown.  (The only case where wait() can be successfully called
    * on a single job more than once is when the previous call to wait() timed out
    * before the job finished.)</p>
    * <p>When successful, the resource usage information for the job is provided as a
    * Map of usage parameter names and their values.
    * The values contain the amount of resources consumed by the job and are
    * implementation defined.</p>
    * @param jobId the id of the job for which to wait
    * @param timeout the maximum number of seconds to wait
    * @return the resource usage and status information
    * @throws DrmaaException May be one of the following:
    * <UL>
    * <LI>DrmCommunicationException</LI>
    * <LI>AuthorizationException</LI>
    * <LI>NoResourceUsageDataException</LI>
    * <LI>ExitTimeoutException</LI>
    * <LI>InvalidJobException</LI>
    * </UL>
    */
	public abstract JobInfo wait (String jobId, long timeout) throws DrmaaException;
	
	/** Get the program status of the job identified by jobId.
    * The possible values returned from this method are:
    * <UL>
    * <LI><code>UNDETERMINED</code>: process status cannot be determined</LI>
    * <LI><code>QUEUED_ACTIVE</code>: job is queued and active</LI>
    * <LI><code>SYSTEM_ON_HOLD</code>: job is queued and in system hold</LI>
    * <LI><code>USER_ON_HOLD</code>: job is queued and in user hold</LI>
    * <LI><code>USER_SYSTEM_ON_HOLD</code>: job is queued and in user and system hold</LI>
    * <LI><code>RUNNING</code>: job is running</LI>
    * <LI><code>SYSTEM_SUSPENDED</code>: job is system suspended</LI>
    * <LI><code>USER_SUSPENDED</code>: job is user suspended</LI>
    * <LI><code>DONE</code>: job finished normally</LI>
    * <LI><code>FAILED</code>: job finished, but failed.</LI>
    * </UL>
    * The DRMAA implementation should always get the status of jobId from DRM system
    * unless the status has already been determined to be <code>FAILED</code> or <code>DONE</code> and the
    * status has been successfully cached. Terminated jobs return a <code>FAILED</code> status.
    * @return the program status
    * @param jobId the id of the job whose status is to be retrieved
    * @throws DrmaaException May be one of the following:
    * <UL>
    * <LI>DrmCommunicationException</LI>
    * <LI>AuthorizationException</LI>
    * <LI>InvalidJobException</LI>
    * </UL>
    */
	public abstract int getJobProgramStatus (String jobId) throws DrmaaException;
	
   /** If called before init(), this method returns a comma delimited String containing
    * the contact Strings available from the default DRMAA
    * implementation, one element per DRM system available. If called after init(),
    * this method returns the contact String for the DRM system to which the session
    * is attached. The returned String is implementation dependent.
    * @return current contact information for DRM system or a comma delimited
    * list of possible contact Strings
    */	
	public abstract String getContact ();
	
	/** Returns a Version object containing the major and minor version numbers of the
    * DRMAA library.  For DRMAA 1.0, major is 1 and minor is 0.
    * @return the version number as a Version object
    * @see Version
    */	
	public abstract Version getVersion ();
	
   /** If called before init(), this method returns a comma delimited list of DRM
    * systems, one element per DRM system implementation provided. If called after
    * init(), this method returns the selected DRM system. The returned String is
    * implementation dependent.
    * @return DRM system implementation information
    */	
	public abstract String getDrmSystem ();
	
   /** If called before init(), this method returns a comma delimited list of DRMAA
    * implementations, one element for each DRMAA implementation provided.  If
    * called after init(), this method returns the selected DRMAA implementation.
    * The returned String is implementation dependent and may contain the DRM
    * system as a component.
    * @return DRMAA implementation information
    */
   public abstract String getDrmaaImplementation ();
}
