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
package com.sun.grid.drmaa.howto;

import java.util.*;

import org.ggf.drmaa.*;

public class Howto3_2 {
   public static void main (String[] args) {
      DRMAASessionFactory factory = DRMAASessionFactory.getFactory ();
      DRMAASession session = factory.getSession ();
      
      try {
         session.init (null);
         JobTemplate jt = session.createJobTemplate ();
         jt.setRemoteCommand ("sleeper.sh");
         jt.setInputParameters (new String[] {"5"});
         
         int start = 1;
         int end  = 30;
         int step = 2;
         
         List ids = session.runBulkJobs (jt, start, end, step);
         Iterator i = ids.iterator ();
         
         while (i.hasNext ()) {
            System.out.println ("Your job has been submitted with id " + i.next ());
         }
         
         jt.delete ();
         session.synchronize (Collections.singletonList (DRMAASession.JOB_IDS_SESSION_ALL),
         DRMAASession.TIMEOUT_WAIT_FOREVER, false);
         
         for (int count = start; count < end; count += step) {
            JobInfo info = session.wait (DRMAASession.JOB_IDS_SESSION_ANY,
            DRMAASession.TIMEOUT_WAIT_FOREVER);
            
            if (info.wasAborted ()) {
               System.out.println ("Job " + info.getJobId () + " never ran");
            }
            else if (info.hasExited ()) {
               System.out.println ("Job " + info.getJobId () +
               " finished regularly with exit status " +
               info.getExitStatus ());
            }
            else if (info.hasSignaled ()) {
               System.out.println ("Job " + info.getJobId () +
               " finished due to signal " +
               info.getTerminatingSignal ());
            }
            else {
               System.out.println ("Job " + info.getJobId () +
               " finished with unclear conditions");
            }
            
            System.out.println ("Job Usage:");
            
            Map rmap = info.getResourceUsage ();
            Iterator r = rmap.keySet ().iterator ();
            
            while (r.hasNext ()) {
               String name = (String)r.next ();
               String value = (String)rmap.get (name);
               
               System.out.println ("  " + name + "=" + value);
            }
         }
         
         session.exit ();
      }
      catch (DRMAAException e) {
         System.out.println ("Error: " + e.getMessage ());
      }
   }
}
