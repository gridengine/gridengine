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
 * SGESession.java
 *
 * Created on March 3, 2004, 3:04 PM
 */

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
	
	public void control (String jobId, int action) throws DRMAAException {
		this.nativeControl (jobId, action);
	}
	
	private native void nativeControl (String jobId, int action) throws DRMAAException;
	//   private void nativeControl (String jobId, int action) throws DRMAAException {
	//      System.out.println("Call to drmaa_control");
	//   }
	
	public void exit () throws DRMAAException {
		this.nativeExit ();
	}
	
	private native void nativeExit () throws DRMAAException;
	//   private void nativeExit () throws DRMAAException {
	//      System.out.println("Call to drmaa_exit");
	//   }
	
	public String getContact () {
		return this.nativeGetContact ();
	}
	
	private native String nativeGetContact ();
	//   private String nativeGetContact () {
	//      System.out.println("Call to drmaa_get_contact");
	//      return "CONTACT";
	//   }
	
	public String getDRMSystem () {
		return this.nativeGetDRMSInfo ();
	}
	
	private native String nativeGetDRMSInfo ();
	//   private String nativeGetDRMSInfo () {
	//      System.out.println("Call to drmaa_get_DRM_system");
	//      return "DRMS";
	//   }
	
	public int getJobProgramStatus (String jobId) throws DRMAAException {
		return this.nativeGetJobProgramStatus (jobId);
	}
	
	private native int nativeGetJobProgramStatus (String jobId) throws DRMAAException;
	//   private int nativeGetJobProgramStatus (String jobId) throws DRMAAException {
	//      System.out.println("Call to drmaa_job_ps");
	//      return RUNNING;
	//   }
	
	public JobTemplate createJobTemplate () throws DRMAAException {
		int id = nativeAllocateJobTemplate ();
		
		return new SGEJobTemplate (this, id);
	}
	
	public DRMAASession.Version getVersion () {
		return new DRMAASession.Version (1, 0);
	}
	
	public void init (String contact) throws DRMAAException {
		this.nativeInit (contact);
	}
	
	private native void nativeInit (String contact) throws DRMAAException;
	//   private void nativeInit (String contact) throws DRMAAException {
	//      System.out.println("Call to drmaa_init");
	//   }
	
	public List runBulkJobs (JobTemplate jt, int start, int end, int incr) throws DRMAAException {
		String[] jobIds = this.nativeRunBulkJobs (((SGEJobTemplate)jt).getId (), start, end, incr);
		
		return Arrays.asList (jobIds);
	}
	
	private native String[] nativeRunBulkJobs (int jtId, int start, int end, int incr) throws DRMAAException;
	//   private String[] nativeRunBulkJobs (JobTemplate jt, int start, int end, int incr) throws DRMAAException {
	//      System.out.println("Call to drmaa_run_bulk_jobs");
	//      return new String[] {"123.1", "123.2"};
	//   }
	
	public String runJob (JobTemplate jt) throws DRMAAException {
		return this.nativeRunJob (((SGEJobTemplate)jt).getId ());
	}
	
	private native String nativeRunJob (int jtId) throws DRMAAException;
	//   private String nativeRunJob (JobTemplate jt) throws DRMAAException {
	//      System.out.println("Call to drmaa_run_job");
	//      return "321";
	//   }
	
	public void synchronize (List jobIds, long timeout, boolean dispose) throws DRMAAException {
		this.nativeSynchronize ((String[])jobIds.toArray (new String[jobIds.size ()]), timeout, dispose);
	}
	
	private native void nativeSynchronize (String[] jobIds, long timeout, boolean dispose) throws DRMAAException;
	//   private void nativeSynchronize (List jobIds, long timeout, boolean dispose) throws DRMAAException {
	//      System.out.println("Call to drmaa_synchronize");
	//   }
	
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
