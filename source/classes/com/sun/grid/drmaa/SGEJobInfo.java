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
 *
 * @author  dan.templeton@sun.com
 */
public class SGEJobInfo extends JobInfo {
   private static final int EXITED_BIT = 0x00000001;
   private static final int SIGNALED_BIT = 0x00000002;
   private static final int COREDUMP_BIT = 0x00000004;
   private static final int NEVERRAN_BIT = 0x00000008;
/* POSIX exit status has only 8 bit */
   private static final int EXIT_STATUS_BITS = 0x00000FF0;
   private static final int EXIT_STATUS_OFFSET = 4;
/* SGE signal numbers are high numbers so we use 16 bit */
   private static final int SIGNAL_BITS = 0x0FFFF000;
   private static final int SIGNAL_OFFSET = 12;
   private String signal = null;
   
   /** Creates a new instance of SGEJobInfo */
   SGEJobInfo (String jobId, int status, String[] resourceUsage, String signal) {
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
    * POSIX, the symbolic names are returned (e.g., SIGABRT, SIGALRM).<BR>
	 * For signals not declared by POSIX, any other string may be returned.
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
	 * by means of hasSignaled(), getTerminatingSignal() and hasCoreDump().
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
