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
/*
 * DRMAASession.java
 *
 * Created on June 17, 2003, 10:33 AM
 */

package org.ggf.drmaa;

import java.util.*;

/** This class represents a session for interaction with the DRM.  The service
 * provider should extend this class to implement functionality specific to a
 * given DRM.
 * @author dan.templeton@sun.com
 */
public abstract class DRMAASession {
   /* String to return from getDRMAAImplementation() */
   /** The name of this DRMAA implementation. */   
   private static final String IMPLEMENTATION_STRING = "DRMAA 1.0 Java Binding 0.3";
   
	/** stop the job */
	public static final int SUSPEND = 0;
	/** (re)start the job */
	public static final int RESUME = 1;
	/** put the job on-hold */
	public static final int HOLD = 2;
	/** release the hold on the job */
	public static final int RELEASE = 3;
	/** kill the job */
	public static final int TERMINATE = 4;
	/** all jobs <b>submitted</b> during this DRMAA session */
	public static final List JOB_IDS_SESSION_ALL = Arrays.asList (new String[] {"DRMAA_JOB_IDS_SESSION_ALL"});
	/** any job from the session */
	public static final String JOB_IDS_SESSION_ANY = "DRMAA_JOB_IDS_SESSION_ANY";
	/** wait indefinitely for a result */
	public static final long TIMEOUT_WAIT_FOREVER = -1L;
	/** return immediately if no result is available */
	public static final long TIMEOUT_NO_WAIT = 0L;
	/** process status cannot be determined */
	public static final int UNDETERMINED = 0x00;
	/** job is queued and active */
	public static final int QUEUED_ACTIVE = 0x10;
	/** job is queued and in system hold */
	public static final int SYSTEM_ON_HOLD = 0x11;
	/** job is queued and in user hold */
	public static final int USER_ON_HOLD = 0x12;
	/** job is queued and in user and system hold */
	public static final int USER_SYSTEM_ON_HOLD = 0x13;
	/** job is running */
	public static final int RUNNING = 0x20;
	/** job is system suspended */
	public static final int SYSTEM_SUSPENDED = 0x21;
	/** job is user suspended */
	public static final int USER_SUSPENDED = 0x22;
	/** job finished normally */
	public static final int DONE = 0x30;
	/** job finished, but failed */
	public static final int FAILED = 0x40;
	
	/** Creates a new instance of DRMAASession */
	public DRMAASession () {
	}
	
   /** Initialize DRMAA API library and create a new DRMAA session.
	 * 'Contact' is an implementation-dependent string that may be used to specify
	 * which DRM system to use.  This routine must be called before any
	 * other DRMAA calls, except for getVersion(), getDRMSystem(),
    * getDRMAAImplementation(), or getContact().<BR>
	 * If 'contact' is <CODE>null</CODE>, the default DRM system SHALL be used
    * provided there is only one DRMAA implementation in the provided binary
    * module.  When there is more than one DRMAA implementation in the provided
    * binary module, init() SHALL throw a
    * NoDefaultContactStringSelectedException.
	 * init() SHOULD be called by only one of the threads. The main thread is
    * RECOMMENDED.  A call by another thread SHALL throw a
    * SessionAlreadyActiveException.
	 * @param contact implementation-dependent string that may be used to specify
	 * which DRM system to use.  If null, will select the default DRM if there
    * is only one DRM implementation available.
	 * @throws DRMAAException Maybe be one of the following:
	 * <UL>
	 * <LI>InvalidContactStringException</LI>
	 * <LI>AlreadyActiveSessionException</LI>
	 * <LI>DefaultContactStringException</LI>
    * <LI>NoDefaultContactStringSelectedException</LI>
	 * </UL>
	 */
	public abstract void init (String contact) throws DRMAAException;
	
   /** Disengage from DRMAA library and allow the DRMAA library to perform
	 * any necessary internal cleanup.
	 * This routine SHALL end the current DRMAA session but SHALL NOT affect any
    * jobs (e.g., queued and running jobs SHALL remain queued and running).
    * exit() SHOULD be called by only one of the threads.  Other thread calls to
    * exit() MAY fail since there is no active session.
	 * @throws DRMAAException May be one of the following:
	 * <UL>
	 * <LI>DRMSExitException</LI>
	 * <LI>NoActiveSessionException</LI>
	 * </UL>
	 */
	public abstract void exit () throws DRMAAException;
	
	/** Get a new job template.  The job template is used to set the
	 * environment for submitted jobs.
	 * @throws DRMAAException May be one of the following:
	 * <UL>
	 * <LI>DRMCommunicationException</LI>
	 * </UL>
	 * @return a blank JobTemplate object
	 */
	public abstract JobTemplate createJobTemplate () throws DRMAAException;
   
   /** Submit a job with attributes defined in the job template 'jt'.
	 * The returned job identifier SHALL be a String, identical to that returned
	 * by the underlying DRM system.
	 * @param jt the job template to be used to create the job
	 * @throws DRMAAException May be one of the following:
	 * <UL>
	 * <LI>TryLaterException</LI>
	 * <LI>DeniedByDRMException</LI>
	 * <LI>DRMCommunicationException</LI>
	 * <LI>AuthorizationException</LI>
	 * </UL>
	 * @return job identifier String identical to that returned by the
	 * underlying DRM system
	 */
	public abstract String runJob (JobTemplate jt) throws DRMAAException;

   /** Submit a set of parametric jobs, dependent on the implied loop index,
	 * each with attributes defined in the job template 'jt'.
	 * The returned job identifiers SHALL be Strings identical to those returned
	 * by the underlying DRM system.  Nonnegative loop bounds SHALL NOT use
	 * file names that start with a minus sign, like command line options.<BR>
	 * The special index placeholder is a DRMAA defined string:<BR>
	 * <CODE>
	 * drmaa_incr_ph // == $incr_pl$
	 * </CODE><BR>
	 * this is used to construct parametric job templates.  For example:<BR>
	 * <CODE>
	 * drmaa_set_attribute (pjt, "stderr", drmaa_incr_ph + ".err" );<BR>
	 * </CODE>
	 * @return job identifier Strings identical to that returned by the
	 * underlying DRM system
	 * @param start the starting value for the loop index
	 * @param end the terminating value for the loop index
	 * @param incr the value by which to increment the loop index each iteration
	 * @param jt the job template to be used to create the job
	 * @throws DRMAAException May be one of the following:
	 * <UL>
	 * <LI>TryLaterException</LI>
	 * <LI>DeniedByDRMException</LI>
	 * <LI>DRMCommunicationException</LI>
	 * <LI>AuthorizationException</LI>
	 * </UL>
	 */
	public abstract List runBulkJobs (JobTemplate jt, int start, int end, int incr) throws DRMAAException;
	
   /** Start, stop, restart, or kill the job identified by 'jobId'.
	 * If 'jobId' is JOB_IDS_SESSION_ALL, then this routine acts on all jobs
	 * <B>submitted</B> during this DRMAA session up to the moment control() is
    * called.  To avoid thread races in multithreaded applications, the DRMAA
    * implementation user should explicitly synchronize this call with any other
    * job submission calls or control calls that may change the number of remote
    * jobs.<BR>
	 * The legal values for 'action' and their meanings are:
	 * <UL>
	 * <LI>SUSPEND: stop the job,</LI>
	 * <LI>RESUME: (re)start the job,</LI>
	 * <LI>HOLD: put the job on-hold,</LI>
	 * <LI>RELEASE: release the hold on the job, and</LI>
	 * <LI>TERMINATE: kill the job.</LI>
	 * </UL>
	 * This routine SHALL return once the action has been acknowledged by
	 * the DRM system, but does not necessarily wait until the action
	 * has been completed.
	 * @param jobId The id of the job to control
	 * @param action the control action to be taken
	 * @throws DRMAAException May be one of the following:
	 * <UL>
	 * <LI>DRMCommunicationException</LI>
	 * <LI>AuthorizationException</LI>
	 * <LI>ResumeInconsistentStateException</LI>
	 * <LI>SuspendInconsistentStateException</LI>
	 * <LI>HoldInconsistentStateException</LI>
	 * <LI>ReleaseInconsistentStateException</LI>
	 * <LI>InvalidJobException</LI>
	 * </UL>
	 */
	public abstract void control (String jobId, int action) throws DRMAAException;
	
	/** Wait until all jobs specified by 'job_ids' have finished execution.
	 * If 'jobIds' is JOB_IDS_SESSION_ALL, then this routine waits for all
	 * jobs <B>submitted</B> during this DRMAA session up to the moment
    * synchronize() is called.  To avoid thread races in multithreaded
    * applications, the DRMAA implementation user should explicitly synchronize
    * this call with any other job submission calls or control calls that may
    * change the number of remote jobs.<BR>
    * To prevent blocking indefinitely in this call, the caller MAY use timeout
	 * specifying after how many seconds to time out in this call.  The value
    * TIMEOUT_WAIT_FOREVER MAY be specified to wait indefinitely for a result.
    * The value TIMEOUT_NO_WAIT MAY be specified to return immediately if no
    * result is available. If the call exits before timeout, all the jobs have
    * been waited on or there was an interrupt.<BR>
	 * If the invocation exits on timeout, an ExitTimeException SHALL be thrown.
    * The caller SHOULD check system time before and after this call in order to
    * check how much time has passed.<BR>
	 * Dispose parameter specifies how to treat reaping the remote job
    * consumption of system resources and other statistical information:
	 * <DL>
	 * <DT><CODE>true</CODE></DT><DD>"fake reap", i.e. dispose of the
	 * resource usage data</DD>
	 * <DT><CODE>false</CODE></DT><DD>do not reap</DD>
	 * </DL>
	 * @param jobIds the ids of the jobs to synchronize
	 * @param timeout the maximum number of seconds to wait
	 * @param dispose specifies how to treat reaping information
	 * @throws DRMAAException May be one of the following:
	 * <UL>
	 * <LI>DRMCommunicationException</LI>
	 * <LI>AuthorizationException</LI>
	 * <LI>ExitTimeoutException</LI>
	 * <LI>InvalidJobException</LI>
	 * </UL>
	 */
	public abstract void synchronize (List jobIds, long timeout, boolean dispose) throws DRMAAException;
	
   /** This routine SHALL wait for a job with jobId to fail or finish execution.
	 * If the special string JOB_IDS_SESSION_ANY is provided as the jobId,
	 * this routine SHALL wait for any job from the session. This routine is
	 * modeled on the wait3 POSIX routine.<BR>
	 * The timeout value is used to specify the desired behavior when a result
	 * is not immediately available.  The value TIMEOUT_WAIT_FOREVER MAY be
	 * specified to wait indefinitely for a result.  The value TIMEOUT_NO_WAIT
	 * may be specified to return immediately if no result is available.
	 * Alternatively, a number of seconds MAY be specified to indicate how
	 * long to wait for a result to become available.<BR>
	 * If the call exits before timeout, either the job has been waited on
	 * successfully or there was an interrupt.<BR>
	 * If the invocation exits on timeout, an ExitTimeoutException is thrown.
	 * The caller SHOULD check system time before and after this call
	 * in order to check how much time has passed.<BR>
	 * The routine reaps jobs on a successful call, so any subsequent calls
	 * to wait() SHOULD fail throwing an InvalidJobException, meaning
	 * that the job has been already reaped. This exception is the same as
	 * if the job was unknown.  Failing due to an elapsed timeout has an effect
	 * that it is possible to issue wait() multiple times for the same job_id.
	 * When successful, the resource usage information SHALL be provided as a
	 * Map of usage parameter names and thier values.
	 * The values contain the amount of resources consumed by the job and is
    * implementation defined.
	 * @param jobId the id of the job for which to wait
	 * @param timeout the maximum number of seconds to wait
	 * @return the resource usage and status information
	 * @throws DRMAAException May be one of the following:
	 * <UL>
	 * <LI>DRMCommunicationException</LI>
	 * <LI>AuthorizationException</LI>
	 * <LI>NoResourceUsageDataException</LI>
	 * <LI>ExitTimeoutException</LI>
	 * <LI>InvalidJobException</LI>
	 * </UL>
	 */
	public abstract JobInfo wait (String jobId, long timeout) throws DRMAAException;
	
	/** Get the program status of the job identified by 'jobId'.
	 * The possible values returned in 'remote_ps' and their meanings SHALL be:
	 * <UL>
	 * <LI>UNDETERMINED: process status cannot be determined</LI>
	 * <LI>QUEUED_ACTIVE: job is queued and active</LI>
	 * <LI>SYSTEM_ON_HOLD: job is queued and in system hold</LI>
	 * <LI>USER_ON_HOLD: job is queued and in user hold</LI>
	 * <LI>USER_SYSTEM_ON_HOLD: job is queued and in user and system hold</LI>
	 * <LI>RUNNING: job is running</LI>
	 * <LI>SYSTEM_SUSPENDED: job is system suspended</LI>
	 * <LI>USER_SUSPENDED: job is user suspended</LI>
	 * <LI>DONE: job finished normally</LI>
	 * <LI>FAILED: job finished, but failed.</LI>
	 * </UL>
	 * DRMAA SHOULD always get the status of jobId from DRM system,
	 * unless the previous status has been FAILED or DONE and the
	 * status has been successfully cached. Terminated jobs get FAILED status.
	 * @return the program status
	 * @param jobId the id of the job whose status is to be retrieved
	 * @throws DRMAAException May be one of the following:
	 * <UL>
	 * <LI>DRMCommunicationException</LI>
	 * <LI>AuthorizationException</LI>
	 * <LI>InvalidJobException</LI>
	 * </UL>
	 */
	public abstract int getJobProgramStatus (String jobId) throws DRMAAException;
	
   /** If called before init(), it SHALL return a comma delimited default DRMAA
    * implementation contact Strings, one per DRM system implementation
    * provided. If called after init(), it SHALL return the selected contact
    * String. The returned String is implementation dependent.
	 * @return current contact information for DRM system or a comma delimited
    * list of possible contact Strings
	 */	
	public abstract String getContact ();
	
	/** SHALL Return the major and minor version numbers of the DRMAA library;
	 * for DRMAA 1.0, 'major' is 1 and 'minor' is 0.
	 * @return the version number as a Version object
	 */	
	public abstract Version getVersion ();
	
   /** If called before init(), it SHALL return a comma delimited DRM system
    * Strings, one per DRM system implementation provided. If called after
    * init(), it SHALL return the selected DRM system. The returned String is
    * implementation dependent.
	 * @return DRM system implementation information
	 */	
	public abstract String getDRMSystem ();
	
   /** If called before init(), it SHALL return a comma delimited DRMAA
    * implementations String, one per each DRMAA implementation provided.  If
    * called after init(), it SHALL return the selected DRMAA implementation.
    * The returned String is implementation dependent and COULD contain the DRM
    * system as its part.
    * @return DRMAA implementation information
    */
   public String getDRMAAImplementation () {
      return IMPLEMENTATION_STRING;
   }
   
	/** Class used to represent the DRM version info */	
	public static class Version implements java.io.Serializable {
		/** The major version number */		
		private int major;
		/** The minor version number */		
		private int minor;
		
		/** Create a new Version object
		 * @param major major version number (non-negative integer)
		 * @param minor minor version number (non-negative integer)
		 */		
		public Version (int major, int minor) {
			if (major < 0) {
				throw new IllegalArgumentException ("Major version number must be non-negative");
			}
			else if (minor < 0) {
				throw new IllegalArgumentException ("Minor version number must be non-negative");
			}
			
			this.major = major;
			this.minor = minor;
		}
		
		/** Get the major version number.
		 * @return major version number (non-negative integer)
		 */		
		public int getMajor () {
			return major;
		}
		
		/** Get the minor version number.
		 * @return minor version number (non-negative integer)
		 */		
		public int getMinor () {
			return minor;
		}
		
      /** Converts this Version object into a printable String of the format
       * &lt;major&gt;.&lt;minor&gt;
       * @return a printable String of the format &lt;major&gt;.&lt;minor&gt;
       */      
		public String toString () {
			return Integer.toString (major) + "." + Integer.toString (minor);
		}
	}
}
