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
import com.sun.grid.jgdi.JGDIException;
import com.sun.grid.jgdi.configuration.ExecHost;
import com.sun.grid.jgdi.util.OutputTable;
import java.beans.IntrospectionException;
import java.io.PrintWriter;
import java.net.UnknownHostException;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

/**
 * OtherQConfOptions class
 * Special handling methods for all other special options
 * @see {@link QConfOption}
 */
public class OtherQConfOptions extends QConfOption {
   
   //-tsm
   void triggerSchedulerMonitoring(final JGDI jgdi, final PrintWriter pw) {
      try {
         jgdi.triggerSchedulerMonitoring();
         pw.println(jgdi.getAdminUser()+"@"+java.net.InetAddress.getLocalHost().getHostName()+" triggers scheduler monitoring");
      } catch (JGDIException ex) {
         pw.println(ex.getMessage());
      } catch (UnknownHostException ex) {
         ex.getMessage();
      } finally {
         pw.flush();
      }
      //TODO LP: Message should be propagated from Qmaster
   }
   
   //-clearusage
   void clearUsage(final JGDI jgdi, final PrintWriter pw) {
      try {
         jgdi.clearShareTreeUsage();
      } catch (JGDIException ex) {
         pw.println(ex.getMessage());
         pw.flush();
      }
      //TODO LP: Message should be propagated from Qmaster, we have no way of knowing what was modified
   }
   
   //-cq
   void cleanQueue(final JGDI jgdi, final List args, final PrintWriter pw) {
      try {
         jgdi.cleanQueues((String[])args.toArray());
      } catch (JGDIException ex) {
         pw.println(ex.getMessage());
      } finally {
         pw.flush();
      }
      //TODO LP: Message should be propagated from Qmaster, we have no way of knowing what was modified
   }
   
   //-kec
   void killEventClient(final JGDI jgdi, final List args, final PrintWriter pw) {
      String arg;
      int [] ids = new int[args.size()];
      try {
         for (int i=0; i< args.size(); i++) {
            arg = (String) args.get(i);
            if(arg.equals("all")) {
               jgdi.killAllEventClients();
               return;
            } else {
               try {
                  ids[i] = Integer.parseInt(arg);
               } catch(NumberFormatException nfe) {
                  throw new IllegalArgumentException(arg + " is not a valid event client id");
               }
            }
         }
         jgdi.killEventClients(ids);
      } catch (JGDIException ex) {
         pw.println(ex.getMessage());
         pw.flush();
      }
   }
   
   //-km
   void killMaster(final JGDI jgdi, final PrintWriter pw) {
      try {
         jgdi.killMaster();
      } catch (JGDIException ex) {
         pw.println(ex.getMessage());
         pw.flush();
      }
   }
   
   //-ks
   void killScheduler(final JGDI jgdi, final PrintWriter pw) {
      try {
         jgdi.killScheduler();
         pw.println("sent shutdown notification to scheduler on host \""+jgdi.getActQMaster()+"\"");
      } catch (JGDIException ex) {
         pw.println(ex.getMessage());
      } finally {
         pw.flush();
      }
      //TODO LP: Message should be from qmaster
   }
   
   //-sds
   void showDetachedSettings(final JGDI jgdi, final PrintWriter pw) {
      String sds;
      try {
         sds = jgdi.showDetachedSettingsAll();
         if (sds != null) {
            pw.println(sds);
         }
      } catch (JGDIException ex) {
         pw.println(ex.getMessage());
      } finally {
         pw.flush();
      }
   }
   
   //-secl
   void showEventClientList(final JGDI jgdi, final PrintWriter pw) {
      List evcl;
      try {
         evcl = jgdi.getEventClientList();
      } catch (JGDIException ex) {
         pw.println(ex.getMessage());
         pw.flush();
         return;
      }
      if (evcl.size() > 0) {
         OutputTable table = new OutputTable(com.sun.grid.jgdi.configuration.EventClient.class);
         try {
            table.addCol("id", "ID", 8, OutputTable.Column.RIGHT);
            table.addCol("name", "NAME", 15, OutputTable.Column.LEFT);
            table.addCol("host", "HOST", 24, OutputTable.Column.LEFT);
         } catch (IntrospectionException ex) {
            ex.printStackTrace();
         }
         table.printHeader(pw);
         //TODO LP client cleanup: Delimiter in client is one char longer
         //table.printDelimiter(pw, '-');
         pw.write("--------------------------------------------------\n");
         Iterator iter = evcl.iterator();
         while (iter.hasNext()) {
            com.sun.grid.jgdi.configuration.EventClient evc = (com.sun.grid.jgdi.configuration.EventClient)iter.next();
            table.printRow(pw, evc);
         }
      } else {
         pw.println("no event clients registered");
      }
      pw.flush();
   }
   
   //-sep
   void showProcessors(final JGDI jgdi, final PrintWriter pw) {
      List hosts;
      try {
         hosts = jgdi.getExecHostList();
      } catch (JGDIException ex) {
         pw.println(ex.getMessage());
         pw.flush();
         return;
      }
      String name, arch;
      int cpu, totalCpu = 0;
      Set set;
      pw.println("HOST                      PROCESSOR        ARCH");
      pw.println("===============================================");
      for (Iterator iter = hosts.iterator(); iter.hasNext(); ) {
         ExecHost eh = (ExecHost) iter.next();
         name = eh.getName();
         if (name.equals("global") || name.equals("template")) {
            continue;
         }
         cpu = eh.getProcessors();
         totalCpu += cpu;
         if (eh.isSetLoad()) {
            arch = " " + eh.getLoad("arch");
         } else
            arch = "";
         pw.println(name+Format.right(String.valueOf(cpu), 35 - name.length())+arch);
      }
      pw.println("===============================================");
      pw.println("SUM"+Format.right(String.valueOf(totalCpu), 32));
      pw.flush();
   }
   
   //-sss
   void showSchedulerState(final JGDI jgdi, final PrintWriter pw) {
      try {
         pw.println(jgdi.getSchedulerHost());
      } catch (JGDIException ex) {
         pw.println(ex.getMessage());
      } finally {
         pw.flush();
      }
   }
   
   //-ke
   void killExecd(final JGDI jgdi, final List args, final PrintWriter pw) {
      killExecd(jgdi, false, args, pw);
   }
   
   //-kej
   void killExecdWithJobs(final JGDI jgdi, final List args, final PrintWriter pw) {
      killExecd(jgdi, true, args, pw);
   }
   
   
   private void killExecd(final JGDI jgdi, final boolean terminateJobs, final List args, final PrintWriter pw) {
      String host;
      try {
         for (int i=0; i<args.size(); i++) {
            host = (String) args.get(i);
            if(host.equals("all")) {
               jgdi.killAllExecds(terminateJobs);
               return;
            }
         }
         jgdi.killExecd((String[])args.toArray(), terminateJobs);
      } catch (JGDIException ex) {
         pw.println(ex.getMessage());
         pw.flush();
      }
   }
}
