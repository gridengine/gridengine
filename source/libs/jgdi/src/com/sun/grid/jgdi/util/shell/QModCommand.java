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

/**
 *
 * @author rh150277
 */
public class QModCommand extends AbstractCommand {
   
   
   /** Creates a new instance of QModCommand */
   public QModCommand(Shell shell, String name) {
      super(shell, name);
   }

   public String getUsage() {
      StringWriter sw = new StringWriter();
      PrintWriter pw = new PrintWriter(sw);
      pw.println("usage: qmod [options]");
      pw.println("   [-c job_wc_queue_list]  clear error state");
      pw.println("   [-cj job_list]          clear job error state");
      pw.println("   [-cq wc_queue_list]     clear queue error state");
      pw.println("   [-d wc_queue_list]      disable");
      pw.println("   [-e wc_queue_list]      enable");
      pw.println("   [-f]                    force action");
      pw.println("   [-help]                 print this help");
      pw.println("   [-r job_wc_queue_list]  reschedule jobs (running in queue)");
      pw.println("   [-rj job_list]          reschedule jobs");
      pw.println("   [-rq wc_queue_list]     reschedule all jobs in a queue");
      pw.println("   [-s job_wc_queue_list]  suspend");
      pw.println("   [-sj job_list]          suspend jobs");
      pw.println("   [-sq wc_queue_list]     suspend queues");
      pw.println("   [-us job_wc_queue_list] unsuspend");
      pw.println("   [-usj job_list]         unsuspend jobs");
      pw.println("   [-usq wc_queue_list]    unsuspend queues");
      pw.println();
      pw.println("   job_wc_queue_list          {job_tasks|wc_queue}[{','|' '}{job_tasks|wc_queue}[{','|' '}...]]");
      pw.println("   job_list                   {job_tasks}[{','|' '}job_tasks[{','|' '}...]]");
      pw.println("   job_tasks                  {{job_id'.'task_id_range}|job_name|pattern}[' -t 'task_id_range]");
      pw.println("   task_id_range              task_id['-'task_id[':'step]]");
      pw.println("   wc_cqueue                  wildcard expression matching a cluster queue");
      pw.println("   wc_host                    wildcard expression matching a host");
      pw.println("   wc_hostgroup               wildcard expression matching a hostgroup");
      pw.println("   wc_qinstance               wc_cqueue@wc_host");
      pw.println("   wc_qdomain                 wc_cqueue@wc_hostgroup");
      pw.println("   wc_queue                   wc_cqueue|wc_qdomain|wc_qinstance");
      pw.println("   wc_queue_list              wc_queue[','wc_queue[','...]]");
      pw.flush();
      return sw.getBuffer().toString();
   }
   

   public void run(String[] args) throws Exception {
      
      JGDI jgdi = getShell().getConnection();
      
      if (jgdi == null) {
         throw new IllegalStateException("Not connected");
      }
      if(args.length == 0) {
         throw new IllegalArgumentException("Invalid number of arguments");
      }
      
      
      boolean force = false;
      
      for(int i = 0; i < args.length; i++) {
         
         if(args[i].equals("-cj")) {
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
