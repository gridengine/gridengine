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
   
   public int getExitStatus () {
      return ((status & EXIT_STATUS_BITS) >> EXIT_STATUS_OFFSET);
   }
   
   public String getTerminatingSignal () {
      return signal;
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
   
   private static Map nameValuesToMap (String[] nameValuePairs) {
      Map map = new HashMap ();
      
      for (int count = 0; count < nameValuePairs.length; count++) {
         int equals = nameValuePairs[count].indexOf ('=');
         map.put (nameValuePairs[count].substring (0, equals), nameValuePairs[count].substring (equals + 1));
      }
      
      return map;
   }
}
