import java.util.*;

import com.sun.grid.drmaa.*;

public class DRMAAExample {
	private static int NBULKS = 3;
	private static int JOB_CHUNK = 8;
	private static DRMAASession session = null;
	
	public static void main (String[] args) throws Exception {
		String jobPath = args[0];
      
		DRMAASessionFactory factory = DRMAASessionFactory.getFactory ();
      
      session = factory.getSession ();
		session.init (null);
		
		JobTemplate jt = createJobTemplate (jobPath, 5, true);
		
		List allJobIds = new LinkedList ();
		Set jobIds = null;
		boolean retry = true;
		
		for (int count = 0; count < NBULKS; count++) {
			do {
				try {
					jobIds = session.runBulkJobs (jt, 1, JOB_CHUNK, 1);
					retry = false;
				}
				catch (DRMCommunicationException e) {
					System.err.println ("runBulkJobs() failed - retry: " + e.getMessage ());
					
					Thread.sleep (1000);
				}
			}
			while (retry);
			
			allJobIds.addAll (jobIds);
			
			System.out.println ("submitted bulk job with jobids:");
			
			Iterator i = jobIds.iterator ();
			
			while (i.hasNext ()) {
				System.out.println ("\t \"" + i.next () + "\"");
			}
		}
		
      jt.delete ();
      
		/* submit some sequential jobs */
		jt = createJobTemplate (jobPath, 5, false);
		
		String jobId = null;
		retry = true;
		
		for (int count = 0; count < JOB_CHUNK; count++) {
			do {
				try {
					jobId = session.runJob (jt);
					retry = false;
				}
				catch (DRMCommunicationException e) {
					System.err.println ("runBulkJobs() failed - retry: " + e.getMessage ());
					
					Thread.sleep (1000);
				}
			}
			while (retry);
			
			System.out.println ("\t \"" + jobId + "\"");
			allJobIds.add (jobId);
		}
		
      jt.delete ();
      
		/* synchronize with all jobs */
		session.synchronize (allJobIds, DRMAASession.TIMEOUT_WAIT_FOREVER, false);
		System.out.println ("synchronized with all jobs");
		
		/* wait all those jobs */
		Iterator i = allJobIds.iterator ();
		
		while (i.hasNext ()) {
			JobInfo status = null;
         jobId = (String)i.next ();
			
			status = session.wait (jobId, DRMAASession.TIMEOUT_WAIT_FOREVER);
			
			/* report how job finished */
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
		}
	}
	
	private static JobTemplate createJobTemplate (String jobPath, int seconds, boolean isBulkJob) throws DRMAAException {
		JobTemplate jt = session.allocateJobTemplate ();
		
		jt.setAttribute (JobTemplate.WORKING_DIRECTORY, "$drmaa_hd_pd$");
		jt.setAttribute (JobTemplate.REMOTE_COMMAND, jobPath);
		jt.setAttribute (JobTemplate.INPUT_PARAMETERS, Arrays.asList (new String[] {Integer.toString (seconds)}));
		jt.setAttribute (JobTemplate.JOIN_FILES, "y");
		
		if (!isBulkJob) {
			jt.setAttribute (JobTemplate.OUTPUT_PATH, "$drmaa_hd_pd$/DRMAA_JOB");
		}
		else {
			jt.setAttribute (JobTemplate.OUTPUT_PATH, "$drmaa_hd_pd$/DRMAA_JOB$drmaa_incr_ph$");
		}
		
		return jt;
	}
}