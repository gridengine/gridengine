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

import java.util.HashMap;
import java.util.Map;

import org.ggf.drmaa.*;

/**
 * This class provides information about a completed Grid Engine job.
 * @see org.ggf.drmaa.JobInfo
 * @author  dan.templeton@sun.com
 * @since 0.5
 */
public class JobInfoImpl extends JobInfo {
   private static final int EXITED_BIT = 0x00000001;
   private static final int SIGNALED_BIT = 0x00000002;
   private static final int COREDUMP_BIT = 0x00000004;
   private static final int NEVERRAN_BIT = 0x00000008;
/* POSIX exit status has only 8 bit */
   private static final int EXIT_STATUS_BITS = 0x00000FF0;
   private static final int EXIT_STATUS_OFFSET = 4;
   private String signal = null;
   
   /** Creates a new instance of JobInfoImpl
    * @param jobId the job id string
    * @param status an opaque status code
    * @param resourceUsage an array of name=value resource usage pairs
    * @param signal the string description of the terminating signal
    */
   JobInfoImpl (String jobId, int status, String[] resourceUsage, String signal) {
      super (jobId, status, nameValuesToMap (resourceUsage));
      
      this.signal = signal;
   }
   
	/** If hasExited() returns true,  this function returns the exit code
	 * that the job passed to _exit() (see exit(2)) or exit(3C), or the value
	 * that the child process returned from main.
	 * @return the exit code for the job
	 */	
   public int getExitStatus () {
      return ((status & EXIT_STATUS_BITS) >> EXIT_STATUS_OFFSET);
   }
   
	/** If hasSignaled() returns true, this method returns a representation of
    * the signal that caused the termination of the job. For signals declared by
    * POSIX or otherwise known to Grid Engine, the symbolic names are returned (e.g.,
    * SIGABRT, SIGALRM).<BR>
    * For signals not known by Grid Engine, the string &quot;unknown signal&quot; is returned.
    * @return the name of the terminating signal
    */	
   public String getTerminatingSignal () {
      return signal;
   }

	/** If hasSignaled() returns true, this function returns true
	 * if a core image of the terminated job was created.
	 * @return whether a core dump image was created
	 */	
   public boolean hasCoreDump () {
      return ((status & COREDUMP_BIT) != 0);
   }
   
	/** Returns <CODE>true</CODE> if the job terminated normally.
    * <CODE>False</CODE> can also indicate that
    * although the job has terminated normally an exit status is not available
    * or that it is not known whether the job terminated normally. In both
    * cases getExitStatus() will not provide exit status information.
    * <CODE>True</CODE> indicates more detailed diagnosis can be provided
    * by means of getExitStatus().
    * @return if the job has exited
    */	
   public boolean hasExited () {
      return ((status & EXITED_BIT) != 0);
   }
   
	/** Returns <CODE>true</CODE> if the job terminated due to the receipt
	 * of a signal. <CODE>False</CODE> can also indicate that although the
	 * job has terminated due to the receipt of a signal the signal is not
	 * available or that it is not known whether the job terminated due to
	 * the receipt of a signal. In both cases getTerminatingSignal() will
	 * not provide signal information.
	 * @return if the job exited on a signal
	 */	
   public boolean hasSignaled () {
      return ((status & SIGNALED_BIT) != 0);
   }
   
	/** Returns <i>true</i> if the job ended before entering the running state.
	 * @return whether the job ended before entering the running state
	 */	
   public boolean wasAborted () {
      return ((status & NEVERRAN_BIT) != 0);
   }   
   
   private static Map nameValuesToMap (String[] nameValuePairs) {
      Map map = new HashMap ();
      
      for (int count = 0; count < nameValuePairs.length; count++) {
         int equals = nameValuePairs[count].indexOf ('=');
         map.put (nameValuePairs[count].substring (0, equals), nameValuePairs[count].substring (equals + 1));
      }
      
      return map;
   }
}
