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

/** <p>The class represents the status and usage information for a finished job.  It
 * contains the job's id, the job's exit status, and a table indicating the
 * amount of resources used during the execution of the job.  The resource table
 * contents are dependent on the DRM.</p>
 * <p>Example</p>
 * <pre>public static void main (String[] args) {
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
 *      JobInfo info = session.wait (id, Session.TIMEOUT_WAIT_FOREVER);
 *
 *      // Interrogate job exit status
 *      if (info.wasAborted ()) {
 *         System.out.println("Job " + info.getJobId () + " never ran");
 *      }
 *      else if (info.hasExited ()) {
 *         System.out.println("Job " + info.getJobId () +
 *                            " finished regularly with exit status " +
 *                            info.getExitStatus ());
 *      }
 *      else if (info.hasSignaled ()) {
 *         System.out.println("Job " + info.getJobId () +
 *                            " finished due to signal " +
 *                            info.getTerminatingSignal ());
 *
 *         if (info.hasCoreDump()) {
 *            System.out.println("A core dump is available.");
 *         }
 *      }
 *      else {
 *         System.out.println("Job " + info.getJobId () +
 *                            " finished with unclear conditions");
 *      }
 *
 *      System.out.println ("\nJob Usage:");
 *
 *      Map rmap = info.getResourceUsage ();
 *      Iterator i = rmap.keySet ().iterator ();
 *
 *      while (i.hasNext ()) {
 *         String name = (String)i.next ();
 *         String value = (String)rmap.get (name);
 *
 *         System.out.println("  " + name + "=" + value);
 *      }
 *
 *      session.exit ();
 *   }
 *   catch (DrmaaException e) {
 *      System.out.println ("Error: " + e.getMessage ());
 *   }
 * }
 * </pre>
 * @author dan.templeton@sun.com
 * @since 0.4.2
 */
public abstract class JobInfo implements java.io.Serializable {
	/** the id of the job this class describes */	
	protected String jobId = null;
	/** the exit status code for the job */	
	protected int status = 0;
	/** a Map of resource usage data */	
	protected Map resourceUsage = null;
	
	/** Creates a new instance of JobInfo
	 * @param jobId the id of the job
	 * @param status the status code of the job
	 * @param resourceUsage the resource usage data for the job
	 */
	protected JobInfo (String jobId, int status, Map resourceUsage) {
		this.jobId = jobId;
		this.status = status;
		this.resourceUsage = new HashMap ();
      
      if (resourceUsage != null) {
         this.resourceUsage.putAll (resourceUsage);
      }
	}
	
	/** Get the job id.
	 * @return the job id
	 */	
	public String getJobId () {
		return jobId;
	}

	/** Get the resource usage data.
	 * @return the resource usage data
	 */	
	public Map getResourceUsage () {
		return Collections.unmodifiableMap (resourceUsage);
	}
	
	/** Returns <CODE>true</CODE> if the job terminated normally.
    * <CODE>False</CODE> can also indicate that
    * although the job has terminated normally an exit status is not available
    * or that it is not known whether the job terminated normally. In both
    * cases getExitStatus() doesn't provide exit status information.
    * <CODE>True</CODE> indicates more detailed diagnosis can be provided
    * by means of getExitStatus().
    * @return if the job has exited
    */	
	public abstract boolean hasExited ();
	
	/** If hasExited() returns true,  this function returns the exit code
    * that the job passed to _exit() (see exit(2)) or exit(3C)), or the value
    * that the child process returned from main.
    * @return the exit code for the job
    */	
	public abstract int getExitStatus ();
	
	/** Returns <CODE>true</CODE> if the job terminated due to the receipt
    * of a signal. <CODE>False</CODE> can also indicate that although the
    * job has terminated due to the receipt of a signal the signal is not
    * available or that it is not known whether the job terminated due to
    * the receipt of a signal. In both cases getTerminatingSignal() does not
    * provide signal information.
    * @return if the job exited on a signal
    */	
	public abstract boolean hasSignaled ();
	
	/** If hasSignaled() returns <CODE>true</CODE>, this returns a representation of the
    * signal that caused the termination of the job. For signals declared by
    * POSIX, the symbolic names are be returned (e.g., SIGABRT, SIGALRM).<BR>
    * For signals not declared by POSIX, a DRM dependent string is returned.
    * @return the name of the terminating signal
    */	
	public abstract String getTerminatingSignal ();
	
	/** If hasSignaled() returns <CODE>true</CODE>, this function returns <CODE>true</CODE>
    * if a core image of the terminated job was created.
    * @return whether a core dump image was created
    */	
	public abstract boolean hasCoreDump ();
	
	/** Returns <CODE>true</CODE> if the job ended before entering the running state.
    * @return whether the job ended before entering the running state
    */	
	public abstract boolean wasAborted ();
}
