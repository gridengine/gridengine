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
package com.sun.grid.drmaa;

import java.util.*;

import org.ggf.drmaa.*;

/**
 *
 * @author  dan.templeton@sun.com
 */
public class SGESession extends DRMAASession {
	private static final String IMPLEMENTATION_STRING = " -- SGE 6.0";
	
	static {
		System.loadLibrary ("jdrmaa");
	}
	
	public static void main (String[] args) throws Exception {
		SGESession session = new SGESession ();
		
		System.out.println ("DRMS: " + session.getDRMSystem ());
		System.out.println ("Contact: " + session.getContact ());
		System.out.println ("Implementation: " + session.getDRMAAImplementation ());
		
		DRMAASession.Version version = session.getVersion ();
		
		System.out.println ("Version: " + Integer.toString (version.getMajor ()) + "." + Integer.toString (version.getMinor ()));
		
		session.init (session.getContact ());
		
		JobTemplate jt = session.createJobTemplate ();
		List names = jt.getAttributeNames ();
		Iterator i = names.iterator ();
		
		System.out.println ("Attributes:");
		
		while (i.hasNext ()) {
			System.out.println ("\t" + i.next ());
		}
		
		jt.setRemoteCommand ("/tmp/dant/examples/jobs/exit.sh");
		System.out.println ("Submitting job");
		
		String jobId = session.runJob (jt);
		
		System.out.println ("Job id is " + jobId);
		
		JobInfo status = session.wait (jobId, TIMEOUT_WAIT_FOREVER);
		
		System.out.println ("Job completed");
		
		if (status.wasAborted ()) {
			System.out.println ("job \"" + jobId + "\" never ran");
		}
		else if (status.hasExited ()) {
			System.out.println ("job \"" + jobId + "\" finished regularly with exit status " + status.getExitStatus ());
		}
		else if (status.hasSignaled ()) {
			System.out.println ("job \"" + jobId + "\" finished due to signal " + status.getTerminatingSignal ());
		}
		else {
			System.out.println ("job \"" + jobId + "\" finished with unclear conditions");
		}
		
		System.out.println ("Resource usage:");
		
		Map resources = status.getResourceUsage ();
		
		i = resources.keySet ().iterator ();
		
		while (i.hasNext ()) {
			String key = (String)i.next ();
			
			System.out.println("\t" + key + "=" + resources.get (key));
		}
		
		session.exit ();
	}
	
	/** Creates a new instance of SGESession */
	public SGESession () {
	}
	
   /** <p>Start, stop, restart, or kill the job identified by 'jobId'.
	 * If 'jobId' is JOB_IDS_SESSION_ALL, then this routine acts on all jobs
	 * <B>submitted</B> during this DRMAA session up to the moment control() is
    * called.  To avoid thread races in multithreaded applications, the DRMAA
    * implementation user should explicitly synchronize this call with any other
    * job submission calls or control calls that may change the number of remote
    * jobs.</p>
	 * <p>The legal values for 'action' and their meanings are:</p>
	 * <UL>
	 * <LI>SUSPEND: stop the job,</LI>
	 * <LI>RESUME: (re)start the job,</LI>
	 * <LI>HOLD: put the job on-hold,</LI>
	 * <LI>RELEASE: release the hold on the job, and</LI>
	 * <LI>TERMINATE: kill the job.</LI>
	 * </UL>
	 * <p>This routine returns once the action has been acknowledged by
	 * the DRM system, but does not necessarily wait until the action
	 * has been completed.</p>
    * <p>The DRMAA suspend/resume operations are equivalent to the use of
    * `-s <jobid>' and `-us <jobid>' options with qmod.  (See the qmod(1) man
    * page.)</p>
    * <p>The DRMAA hold/release operations are equivalent to the use of
    * qhold and qrls.  (See the qhold(1) and qrls(1) man pages.)</p>
    * <p>The DRMAA terminate operation is equivalent to the use of qdel.  (See
    * the qdel(1) man page.)</p>
    * <p>Only user hold and user suspend can be controled via control().  For
    * affecting system hold and system suspend states the appropriate DRM
    * interfaces must be used.</p>
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
	public void control (String jobId, int action) throws DRMAAException {
		this.nativeControl (jobId, action);
	}
	
	private native void nativeControl (String jobId, int action) throws DRMAAException;
	//   private void nativeControl (String jobId, int action) throws DRMAAException {
	//      System.out.println("Call to drmaa_control");
	//   }
	
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
	public void exit () throws DRMAAException {
		this.nativeExit ();
	}
	
	private native void nativeExit () throws DRMAAException;
	//   private void nativeExit () throws DRMAAException {
	//      System.out.println("Call to drmaa_exit");
	//   }
	
   /** <p>getContact() returns an opaque string containing contact information
    * related to the current DRMAA session to be used with the init() method.
    * In the current implemention, however, the getContact() function returns an
    * empty string, and the contact parameter has no effect on the init()
    * method.</p>
    * <p>The getContact() method returns the same value before and after init()
    * is called.</p>
	 * @return current contact information for DRM system or a comma delimited
    * list of possible contact Strings
	 */	
	public String getContact () {
		return this.nativeGetContact ();
	}
	
	private native String nativeGetContact ();
	//   private String nativeGetContact () {
	//      System.out.println("Call to drmaa_get_contact");
	//      return "CONTACT";
	//   }
	
   /** If called before init(), it SHALL return a comma delimited DRM system
    * Strings, one per DRM system implementation provided. If called after
    * init(), it SHALL return the selected DRM system. The returned String is
    * implementation dependent.
	 * @return DRM system implementation information
	 */	
	public String getDRMSystem () {
		return this.nativeGetDRMSInfo ();
	}
	
	private native String nativeGetDRMSInfo ();
	//   private String nativeGetDRMSInfo () {
	//      System.out.println("Call to drmaa_get_DRM_system");
	//      return "DRMS";
	//   }
	
	/** <p>Get the program status of the job identified by 'jobId'.
	 * The possible values returned in 'remote_ps' and their meanings SHALL
    * be:</p>
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
	 * <p>DRMAA SHOULD always get the status of jobId from DRM system, unless the
    * previous status has been FAILED or DONE and the status has been
    * successfully cached. Terminated jobs get FAILED status.</p>
    * <p>Jobs' user hold and user suspend states can be controled via control().
    * For affecting system hold and system suspend states the appropriate DRM
    * interfaces must be used.</p>
	 * @return the program status
	 * @param jobId the id of the job whose status is to be retrieved
	 * @throws DRMAAException May be one of the following:
	 * <UL>
	 * <LI>DRMCommunicationException</LI>
	 * <LI>AuthorizationException</LI>
	 * <LI>InvalidJobException</LI>
	 * </UL>
	 */
	public int getJobProgramStatus (String jobId) throws DRMAAException {
		return this.nativeGetJobProgramStatus (jobId);
	}
	
	private native int nativeGetJobProgramStatus (String jobId) throws DRMAAException;
	//   private int nativeGetJobProgramStatus (String jobId) throws DRMAAException {
	//      System.out.println("Call to drmaa_job_ps");
	//      return RUNNING;
	//   }
	
	/** Get a new job template.  The job template is used to set the
	 * environment for submitted jobs.
	 * @throws DRMAAException May be one of the following:
	 * <UL>
	 * <LI>DRMCommunicationException</LI>
	 * </UL>
	 * @return a blank JobTemplate object
	 */
	public JobTemplate createJobTemplate () throws DRMAAException {
		int id = nativeAllocateJobTemplate ();
		
		return new SGEJobTemplate (this, id);
	}
	
	/** SHALL Return the major and minor version numbers of the DRMAA library;
	 * for DRMAA 1.0, 'major' is 1 and 'minor' is 0.
	 * @return the version number as a Version object
	 */	
	public DRMAASession.Version getVersion () {
		return new DRMAASession.Version (1, 0);
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
	public void init (String contact) throws DRMAAException {
		this.nativeInit (contact);
	}
	
	private native void nativeInit (String contact) throws DRMAAException;
	//   private void nativeInit (String contact) throws DRMAAException {
	//      System.out.println("Call to drmaa_init");
	//   }
	
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
	public List runBulkJobs (JobTemplate jt, int start, int end, int incr) throws DRMAAException {
		String[] jobIds = this.nativeRunBulkJobs (((SGEJobTemplate)jt).getId (), start, end, incr);
		
		return Arrays.asList (jobIds);
	}
	
	private native String[] nativeRunBulkJobs (int jtId, int start, int end, int incr) throws DRMAAException;
	//   private String[] nativeRunBulkJobs (JobTemplate jt, int start, int end, int incr) throws DRMAAException {
	//      System.out.println("Call to drmaa_run_bulk_jobs");
	//      return new String[] {"123.1", "123.2"};
	//   }
	
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
	public String runJob (JobTemplate jt) throws DRMAAException {
		return this.nativeRunJob (((SGEJobTemplate)jt).getId ());
	}
	
	private native String nativeRunJob (int jtId) throws DRMAAException;
	//   private String nativeRunJob (JobTemplate jt) throws DRMAAException {
	//      System.out.println("Call to drmaa_run_job");
	//      return "321";
	//   }
	
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
	public void synchronize (List jobIds, long timeout, boolean dispose) throws DRMAAException {
		this.nativeSynchronize ((String[])jobIds.toArray (new String[jobIds.size ()]), timeout, dispose);
	}
	
	private native void nativeSynchronize (String[] jobIds, long timeout, boolean dispose) throws DRMAAException;
	//   private void nativeSynchronize (List jobIds, long timeout, boolean dispose) throws DRMAAException {
	//      System.out.println("Call to drmaa_synchronize");
	//   }
	
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
	public JobInfo wait (String jobId, long timeout) throws DRMAAException {
		SGEJobInfo jobInfo = this.nativeWait (jobId, timeout);
		
		return jobInfo;
	}
	
	private native SGEJobInfo nativeWait (String jobId, long timeout) throws DRMAAException;
	
	//   private SGEJobInfo nativeWait (String jobId, long timeout) throws DRMAAException {
	//      System.out.println("Call to drmaa_wait");
	//      return new SGEJobInfo (jobId, 1, Collections.singletonMap ("user", "100.00"));
	//   }
	
	//   private void allocateJobTemplate (JobTemplate jt) {
	//      Set names = jt.getAttributeNames ();
	//      Iterator i = names.iterator ();
	//
	//      /* This could have mutli-threading issues... */
	//      nativeAllocateJobTemplate ();
	//
	//      while (i.hasNext ()) {
	//         String name = (String)i.next ();
	//         List value = jt.getAttribute (name);
	//
	//         if (value.size () == 1) {
	//            nativeSetAttributeValue (name, (String)value.get (0));
	//         }
	//         else {
	//            nativeSetAttributeValues (name, (String[])value.toArray (new String[value.size ()]));
	//         }
	//      }
	//   }
	
	private native int nativeAllocateJobTemplate ();
	//   private int nativeAllocateJobTemplate () {
	//      System.out.println("Call to drmaa_allocate_job_template");
	//      return 0;
	//   }
	
	native void nativeSetAttributeValue (int jtId, String name, String value);
	//   void nativeSetAttributeValue (String name, String value) {
	//      System.out.println("Call to drmaa_set_attribute");
	//   }
	
	native void nativeSetAttributeValues (int jtId, String name, String[] values);
	//   void nativeSetAttributeValues (String name, String[] values) {
	//      System.out.println("Call to drmaa_set_vector_attribute");
	//   }
	
	native String[] nativeGetAttributeNames (int jtId);
	//   String[] nativeGetAttributeNames () {
	//      System.out.println("Call to drmaa_get_attribute_names");
	//      return new String[] {"DRMAA_WD", "DRMAA_REMOTE_COMMAND"};
	//   }
	
	native String[] nativeGetAttribute (int jtId, String name);
	//   String[] nativeGetAttribute (String name) {
	//      System.out.println("Call to drmaa_get_attribute & drmaa_get_vector_attribute");
	//      return new String[] {"/tmp", "/var/tmp"};
	//   }
	
	native void nativeDeleteJobTemplate (int jtId);
	//   void nativeDeleteJobTemplate (JobTemplate jt) {
	//      System.out.println("Call to drmaa_delete_job_template");
	//   }
	
   /** If called before init(), it SHALL return a comma delimited DRMAA
    * implementations String, one per each DRMAA implementation provided.  If
    * called after init(), it SHALL return the selected DRMAA implementation.
    * The returned String is implementation dependent and COULD contain the DRM
    * system as its part.
    * @return DRMAA implementation information
    */
	public String getDRMAAImplementation () {
		return super.getDRMAAImplementation () + IMPLEMENTATION_STRING;
	}
	
	private Map buildMap (String[] params) {
		HashMap map = new HashMap ();
		
		for (int count = 0; count < params.length; count++) {
			int index = params[count].indexOf ('=');
			
			if (index >= 0) {
				map.put (params[count].substring (0, index),
				params[count].substring (index + 1));
			}
			else {
				map.put (params[count], null);
			}
		}
		
		return map;
	}
}
