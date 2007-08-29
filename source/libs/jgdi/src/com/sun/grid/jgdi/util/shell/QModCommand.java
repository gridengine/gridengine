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
package com.sun.grid.jgdi.util.shell;

import com.sun.grid.jgdi.JGDI;
import java.io.PrintWriter;
import java.io.StringWriter;

import static com.sun.grid.jgdi.util.JGDIShell.getResourceString;
/**
 *
 */
public class QModCommand extends AbstractCommand {
   
   
   /** Creates a new instance of QModCommand */
   public QModCommand(Shell shell, String name) {
      super(shell, name);
   }

   public String getUsage() {
      return getResourceString("sge.version.string")+"\n"+
             getResourceString("usage.qmod");
   }
   

   public void run(String[] args) throws Exception {  
      JGDI jgdi = getShell().getConnection();
      
      if (jgdi == null) {
         throw new IllegalStateException("Not connected");
      }
      if(args.length == 0) {
         throw new IllegalArgumentException("Invalid number of arguments");
      }
      
      PrintWriter pw = shell.getPrintWriter();
      
      boolean force = false;
      
      for(int i = 0; i < args.length; i++) {
         
         if (args[i].equals("-help")) {
            pw.println(getUsage());
            return;
         } else if(args[i].equals("-cj")) {
            i++;
            if(i>=args.length) {
               throw new IllegalArgumentException("job_wc_queue_list is missing");
            }
            String jobs [] = parseJobWCQueueList(args[i]);
            jgdi.clearJobs(jobs, force);
            break;
         } else if (args[i].equals("-c")) {
            i++;
            if(i>=args.length) {
               throw new IllegalArgumentException("job_wc_queue_list is missing");
            }
            String queues [] = parseJobWCQueueList(args[i]);
            jgdi.clearQueues(queues, force);
         } else if (args[i].equals("-s")) {
            i++;
            if(i>=args.length) {
               throw new IllegalArgumentException("job_wc_queue_list is missing");
            }
            String queues [] = parseJobWCQueueList(args[i]);
            jgdi.suspendQueues(queues, force);
         } else if (args[i].equals("-us")) {
            i++;
            if(i>=args.length) {
               throw new IllegalArgumentException("job_wc_queue_list is missing");
            }
            String queues [] = parseJobWCQueueList(args[i]);
            jgdi.unsuspendQueues(queues, force);
         } else if (args[i].equals("-sj")) {
            i++;
            if(i>=args.length) {
               throw new IllegalArgumentException("job_list is missing");
            }
            String jobs [] = parseJobList(args[i]);
            jgdi.suspendJobs(jobs, force);
         } else if (args[i].equals("-usj")) {
            i++;
            if(i>=args.length) {
               throw new IllegalArgumentException("job_list is missing");
            }
            String jobs [] = parseJobList(args[i]);
            jgdi.unsuspendJobs(jobs, force);
            
         } else if (args[i].equals("-sq")) {
            i++;
            if(i>=args.length) {
               throw new IllegalArgumentException("job_wc_queue_list is missing");
            }
            String queues [] = parseWCQueueList(args[i]);
            jgdi.suspendQueues(queues, force);
         } else if (args[i].equals("-uq")) {
            i++;
            if(i>=args.length) {
               throw new IllegalArgumentException("job_wc_queue_list is missing");
            }
            String queues [] = parseWCQueueList(args[i]);
            jgdi.unsuspendQueues(queues, force);
         } else if (args[i].equals("-r")) {
            i++;
            if(i>=args.length) {
               throw new IllegalArgumentException("job_wc_queue_list is missing");
            }
            String jobs [] = parseJobWCQueueList(args[i]);
            jgdi.rescheduleJobs(jobs, force);
         } else if (args[i].equals("-rj")) {
            i++;
            if(i>=args.length) {
               throw new IllegalArgumentException("job_wc_queue_list is missing");
            }
            String jobs [] = parseJobList(args[i]);
            jgdi.rescheduleJobs(jobs, force);
            
         } else if (args[i].equals("-cq")) {
            i++;
            if(i>=args.length) {
               throw new IllegalArgumentException("wc_queue_list is missing");
            }
            String queues [] = parseWCQueueList(args[i]);
            jgdi.clearQueues(queues, force);
            break;
         } else if (args[i].equals("-rq")) {
            i++;
            if(i>=args.length) {
               throw new IllegalArgumentException("wc_queue_list is missing");
            }
            String queues [] = parseWCQueueList(args[i]);
            jgdi.rescheduleQueues(queues, force);
            break;
         } else if (args[i].equals("-d")) {
            
            i++;
            if(i>=args.length) {
               throw new IllegalArgumentException("wc_queue_list is missing");
            }
            String queues [] = parseWCQueueList(args[i]);
            jgdi.disableQueues(queues, force);
            break;
         } else if (args[i].equals("-e")) {
            
            i++;
            if(i>=args.length) {
               throw new IllegalArgumentException("wc_queue_list is missing");
            }
            String queues [] = parseWCQueueList(args[i]);
            jgdi.enableQueues(queues, force);
            break;
         } else if ( args[i].equals("-f") ) {
            force = true;
         } else {
            throw new IllegalArgumentException("Unknown or not implemented option " + args[i]);
         }
      }
   }
   
   
}
