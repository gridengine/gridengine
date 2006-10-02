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
import com.sun.grid.jgdi.monitoring.LimitRuleInfo;
import com.sun.grid.jgdi.monitoring.QLimitOptions;
import com.sun.grid.jgdi.monitoring.QLimitResult;
import com.sun.grid.jgdi.monitoring.ResourceLimit;
import com.sun.grid.jgdi.monitoring.filter.HostFilter;
import com.sun.grid.jgdi.monitoring.filter.ParallelEnvironmentFilter;
import com.sun.grid.jgdi.monitoring.filter.ProjectFilter;
import com.sun.grid.jgdi.monitoring.filter.QueueFilter;
import com.sun.grid.jgdi.monitoring.filter.ResourceAttributeFilter;
import com.sun.grid.jgdi.monitoring.filter.ResourceFilter;
import com.sun.grid.jgdi.monitoring.filter.UserFilter;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.Iterator;
import java.util.LinkedList;

/**
 *
 * @author rh150277
 */
public class QLimitCommand extends AbstractCommand {
   
   /** Creates a new instance of QModCommand */
   public QLimitCommand(Shell shell, String name) {
      super(shell, name);
   }
   
   public String getUsage() {
      StringWriter sw = new StringWriter();
      PrintWriter pw = new PrintWriter(sw);
      pw.println("usage: qlimit [options]");
      pw.println("[-help]                    print this help");
      pw.println("[-h host_list]             display only selected hosts");
      pw.println("[-l resource_attributes]   request the given resources");
      pw.println("[-u user_list]             display only selected users");
      pw.println("[-pe pe_list]              display only selected parallel environments");
      pw.println("[-P project_list]          display only selected projects");
      pw.println("[-q wc_queue_list]         display only selected queues");
      pw.println("[-xml]                     display the information in XML-Format");
      pw.flush();
      return sw.getBuffer().toString();
   }
   
   
   public void run(String[] args) throws Exception {
      QLimitOptions options = parse(args);
      
      JGDI jgdi = getShell().getConnection();
      
      if (jgdi == null) {
         throw new IllegalStateException("Not connected");
      }
  
      PrintWriter pw = new PrintWriter(System.out);
      QLimitResult res = jgdi.getQLimit(options);

      pw.println("limitation rule    limit                filter");
      pw.println("--------------------------------------------------------------------------------");
      Iterator iter = res.getLimitRules().iterator();
      while (iter.hasNext()) {
         LimitRuleInfo info = (LimitRuleInfo)iter.next();
         // need a Formatter here
         pw.print(info.getLimitRuleName());
         Iterator liter = info.getLimits().iterator();
         while (liter.hasNext()) {
            ResourceLimit relim = (ResourceLimit)liter.next();
            pw.print(" " + relim.getName() + "=" + relim.getUsageValue() + "/" + relim.getLimitValue());
         }
         if (!info.getUsers().isEmpty()) {
            pw.print(" users" + info.getUsers());
         }
         if (!info.getProjects().isEmpty()) {
            pw.print(" projects" + info.getProjects());
         }
         if (!info.getPes().isEmpty()) {
            pw.print(" pes" + info.getPes());
         }
         if (!info.getQueues().isEmpty()) {
            pw.print(" queues" + info.getQueues());
         }
         if (!info.getHosts().isEmpty()) {
            pw.print(" hosts" + info.getHosts());
         }
         
         pw.println();
      }
      pw.flush();
    
      
   }
   
   private QLimitOptions parse(String [] args) throws Exception {
      ResourceAttributeFilter resourceAttributeFilter = null;
      ResourceFilter resourceFilter = null;
      UserFilter userFilter = null;
      HostFilter hostFilter = null;
      ProjectFilter projectFilter = null;
      ParallelEnvironmentFilter peFilter = null;
      QueueFilter queueFilter = null;
      
      LinkedList argList = new LinkedList();
      for(int i = 0; i < args.length; i++) {
         argList.add(args[i]);
      }
      
      while(!argList.isEmpty()) {
         String arg = (String)argList.removeFirst();
         
         if(arg.equals("-help")) {
            System.out.print(getUsage());
         } else if (arg.equals("-h")) {
            if(argList.isEmpty()) {
               throw new IllegalArgumentException("missing host_list");
            }
            arg = (String)argList.removeFirst();
            hostFilter = HostFilter.parse(arg);
         } else if (arg.equals("-l")) {
            if(argList.isEmpty()) {
               throw new IllegalArgumentException("missing resource_list");
            }
            resourceFilter = new ResourceFilter();
            arg = (String)argList.removeFirst();
            resourceFilter = ResourceFilter.parse(arg);
         } else if (arg.equals("-u")) {
            if(argList.isEmpty()) {
               throw new IllegalArgumentException("missing user_list");
            }
            arg = (String)argList.removeFirst();
            userFilter = UserFilter.parse(arg);
         } else if (arg.equals("-pe")) {
            if(argList.isEmpty()) {
               throw new IllegalArgumentException("missing pe_list");
            }
            arg = (String)argList.removeFirst();
            peFilter = ParallelEnvironmentFilter.parse(arg);
         } else if (arg.equals("-P")) {
            if(argList.isEmpty()) {
               throw new IllegalArgumentException("missing project_list");
            }
            arg = (String)argList.removeFirst();
            projectFilter = ProjectFilter.parse(arg);
         } else if (arg.equals("-q")) {
            if(argList.isEmpty()) {
               throw new IllegalArgumentException("missing wc_queue_list");
            }
            arg = (String)argList.removeFirst();
            queueFilter = QueueFilter.parse(arg);
         } else {
            throw new IllegalStateException("Unknown argument: " + arg);
         }
      }
      
      QLimitOptions options = new QLimitOptions();
      
      if (hostFilter != null) {
         options.setHostFilter(hostFilter);
      }
      if (resourceFilter != null) {
         options.setResourceFilter(resourceFilter);
      }
      if (userFilter != null) {
         options.setUserFilter(userFilter);
      }
      if (peFilter != null) {
         options.setPeFilter(peFilter);
      }
      if (projectFilter != null) {
         options.setProjectFilter(projectFilter);
      }
      if (queueFilter != null) {
         options.setQueueFilter(queueFilter);
      }
      
      return options;
   }
}
