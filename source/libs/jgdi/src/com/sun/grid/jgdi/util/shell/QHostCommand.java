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
import com.sun.grid.jgdi.monitoring.HostInfo;
import com.sun.grid.jgdi.monitoring.QHostOptions;
import com.sun.grid.jgdi.monitoring.QHostResult;
import com.sun.grid.jgdi.monitoring.QueueInstanceSummaryPrinter;
import com.sun.grid.jgdi.monitoring.filter.HostFilter;
import com.sun.grid.jgdi.monitoring.filter.ResourceAttributeFilter;
import com.sun.grid.jgdi.monitoring.filter.ResourceFilter;
import com.sun.grid.jgdi.monitoring.filter.UserFilter;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.Iterator;
import java.util.LinkedList;

/**
 *
 */
public class QHostCommand extends AbstractCommand {
   
   /**
    * Creates a new instance of QHostCommand
    */
   public QHostCommand(Shell shell, String name) {
      super(shell, name);
   }
   
   public String getUsage() {
      StringWriter sw = new StringWriter();
      PrintWriter pw = new PrintWriter(sw);
      
      pw.println("usage: qhost [options]");
      pw.println("        [-help]                           print this help");
      pw.println("        [-h hostlist]                     display only selected hosts");
      pw.println("        [-q]                              display queues hosted by host");
      pw.println("        [-j]                              display jobs hosted by host");
      pw.println("        [-F [resource_attributes]]        full output and show (selected) resources of queue(s)");
      pw.println("        [-l attr=value[,...]]             request the given resources");
      pw.println("        [-u user[,...]]                   show only jobs for user(s)");
      pw.println("        [-xml]                            display the information in XML-Format");
      pw.close();
      return sw.getBuffer().toString();
   }
   
   public void run(String[] args) throws Exception {
      QHostOptions options = parse(args);
      
      JGDI jgdi = getShell().getConnection();
      
      if (jgdi == null) {
         throw new IllegalStateException("Not connected");
      }
      
      PrintWriter pw = new PrintWriter(System.out);
      QHostResult res = jgdi.execQHost(options);
      QueueInstanceSummaryPrinter.print(pw, res, options);
      pw.flush();
   }
   
   private QHostOptions parse(String [] args) throws Exception {
      ResourceAttributeFilter resourceAttributeFilter = null;
      ResourceFilter resourceFilter = null;
      boolean showQueues = false;
      boolean showJobs = false;
      UserFilter userFilter = null;
      HostFilter hostFilter = null;
      
      LinkedList argList = new LinkedList();
      for (int i = 0; i < args.length; i++) {
         argList.add(args[i]);
      }
      
      while (!argList.isEmpty()) {
         String arg = (String)argList.removeFirst();
         
         if (arg.equals("-help")) {
            System.out.print(getUsage());
         } else if (arg.equals("-h")) {
            if (argList.isEmpty()) {
               throw new IllegalArgumentException("missing host_list");
            }
            arg = (String)argList.removeFirst();
            hostFilter = HostFilter.parse(arg);
         } else if (arg.equals("-F")) {
            if (!argList.isEmpty()) {
               arg = (String)argList.getFirst();
               // we allow only a comma separated arg string
               // qhost CLI allows also whitespace separated arguments
               if (!arg.startsWith("-")) {
                  arg = (String)argList.removeFirst();
                  resourceAttributeFilter = ResourceAttributeFilter.parse(arg);
               } else {
                  resourceAttributeFilter = new ResourceAttributeFilter();
               }
            } else {
               resourceAttributeFilter = new ResourceAttributeFilter();
            }
         }  else if (arg.equals("-j")) {
            showJobs = true;
         } else if (arg.equals("-l")) {
            if (argList.isEmpty()) {
               throw new IllegalArgumentException("missing resource_list");
            }
            resourceFilter = new ResourceFilter();
            arg = (String)argList.removeFirst();
            resourceFilter = ResourceFilter.parse(arg);
         } else if (arg.equals("-q")) {
            showQueues = true;
         } else if (arg.equals("-u")) {
            if (argList.isEmpty()) {
               throw new IllegalArgumentException("missing user_list");
            }
            arg = (String)argList.removeFirst();
            userFilter = UserFilter.parse(arg);
         } else {
            throw new IllegalStateException("Unknown argument" + arg);
         }
      }
      
      QHostOptions options = new QHostOptions();
      
      options.setIncludeJobs(showJobs);
      options.setIncludeQueue(showQueues);
      if (hostFilter != null) {
         options.setHostFilter(hostFilter);
      }
      if (userFilter != null) {
         options.setUserFilter(userFilter);
      }
      if (resourceFilter != null) {
         options.setResourceFilter(resourceFilter);
      }
      if (resourceAttributeFilter != null) {
         options.setResourceAttributeFilter(resourceAttributeFilter);
      }
      
      return options;
   }
}
