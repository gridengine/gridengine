/*
 * SGEJobInfo.java
 *
 * Created on March 4, 2004, 10:17 AM
 */

package sun.sge.drmaa;

import java.util.HashMap;
import java.util.Map;

import com.sun.grid.drmaa.*;

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
   SGEJobInfo (String jobId, int status, Map resourceUsage) {
      super (jobId, status, resourceUsage);
   }
   
   public int getExitStatus () {
      return ((status & EXIT_STATUS_BITS) >> EXIT_STATUS_OFFSET);
   }
   
   public String getTerminatingSignal () {
      return signal;
   }

   void setTerminatingSignal (String signal) {
      this.signal = signal;
   }
   
   int getSignalNumber () {
      return ((status & SIGNAL_BITS) >> SIGNAL_OFFSET);
   }
   
   public boolean hasCoreDump () {
      return ((status & COREDUMP_BIT) != 0);
   }
   
   public boolean hasExited () {
      return ((status & EXITED_BIT) != 0);
   }
   
   public boolean hasSignaled () {
      return ((status & SIGNALED_BIT) != 0);
   }
   
   public boolean wasAborted () {
      return ((status & NEVERRAN_BIT) != 0);
   }   
}
