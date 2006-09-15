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
package com.sun.grid.jgdi.qmod;

import com.sun.grid.jgdi.configuration.ClusterQueue;
import junit.framework.Test;
import junit.framework.TestSuite;
import com.sun.grid.jgdi.JGDI;
import com.sun.grid.jgdi.JGDIFactory;
import com.sun.grid.jgdi.configuration.ExecHost;
import com.sun.grid.jgdi.configuration.EventClient;
import java.util.List;
import java.util.Iterator;

/**
 *
 * @author andre.alefeld@sun.com
 */
public class TestQmod extends com.sun.grid.jgdi.BaseTestCase {
   
   /** Creates a new instance of TestQmod */
   public TestQmod(String testName) {
      super(testName);
   }
   
   protected void setUp() throws Exception {
      System.loadLibrary( "jgdi" );
      logger.fine("Version: " + JGDIFactory.getJGDIVersion());
      super.setUp();

   }
   
   public static Test suite() {
      TestSuite suite = new TestSuite( TestQmod.class);
      return suite;
   }
   
   public void testCleanQueues() throws Exception {
      JGDI jgdi = createJGDI();
      try {
         jgdi.cleanQueues(new String[] { "*" } );
      } finally {
         jgdi.close();
      }
   }

   public void testUnsuspendQueues() throws Exception {
      JGDI jgdi = createJGDI();
      try {
         jgdi.unsuspendQueues(new String[] { "*" }, false );
      } finally {
         jgdi.close();
      }
   }
   
   public void testKillAllExecd() throws Exception {
      JGDI jgdi = createJGDI();
      try {
         jgdi.killAllExecds(false);
      } finally {
         jgdi.close();
      }
   }
   
   public void testKillScheduler() throws Exception {
      JGDI jgdi = createJGDI();
      try {
         jgdi.killScheduler();
      } finally {
         jgdi.close();
      }
   }
   
   public void testKillExecd() throws Exception {
      logger.entering("TestQmod", "testKillExecd");
      JGDI jgdi = createJGDI();
      try {
         List ehList = jgdi.getExecHostList();
         Iterator iter = ehList.iterator();
         String[] hosts = new String[ehList.size()];
         int i=0;
         while(iter.hasNext()) {
            ExecHost eh = (ExecHost)iter.next();
            if(eh.getName().equals("template") ||
                    eh.getName().equals("global")) {
               continue;
            }
            hosts[i] = eh.getName();
            i++;
         }
         jgdi.killExecd(hosts, true);
      } finally {
         jgdi.close();
      }
   }
   
   private int[] testGetEventClients(JGDI jgdi) throws Exception {
      List evlist = jgdi.getEventClientList();
      Iterator iter = evlist.iterator();
      int[] ids = null;
      if (evlist.size()>0) {
         ids = new int[evlist.size()];
         
         int i = 0;
         while (iter.hasNext()) {
            EventClient ev = (EventClient)iter.next();
            ids[i++] = ev.getId();
            logger.fine("EventId:    " + ev.getId());
            logger.fine("EventName:  " + ev.getName());
            logger.fine("Event Host: " + ev.getHost());
         }
      }
      return ids;
   }
   
   public void testKillEventClients() throws Exception {
      JGDI jgdi = createJGDI();
      try {
         int[] ids = testGetEventClients(jgdi);
         jgdi.killEventClients(ids);
      } finally {
         jgdi.close();
      }
   }
   
   public void testClearShareTreeUsage() throws Exception {
      JGDI jgdi = createJGDI();
      try {
         jgdi.clearShareTreeUsage();
      } finally {
         jgdi.close();
      }
   }
   
   public void testGetSchedulerHost() throws Exception {
      JGDI jgdi = createJGDI();
      try {
         logger.fine("Scheduler Host: " + jgdi.getSchedulerHost());
      } finally {
         jgdi.close();
      }
   }
   
   public void testEnableQueues() throws Exception {
      JGDI jgdi = createJGDI();
      try {
         List cqs = jgdi.getClusterQueueList();
         Iterator iter = cqs.iterator();
         String[] queues = new String[cqs.size()];
         int i=0;
         while(iter.hasNext()) {
            ClusterQueue cq = (ClusterQueue)iter.next();
            if(cq.getName().equals("template")) {
               continue;
            }
            queues[i] = cq.getName();
            i++;
         }
         for (i=0; i < queues.length; i++) {
            logger.fine("Enable Queue: " + queues[i]);
         }
         try {
         jgdi.enableQueues(queues, false);
         } catch (IllegalArgumentException je) {
            je.printStackTrace();
         }
      } finally {
         jgdi.close();
      }
   }
   
   public void testDisableQueues() throws Exception {
      JGDI jgdi = createJGDI();
      try {
         List cqs = jgdi.getClusterQueueList();
         Iterator iter = cqs.iterator();
         String[] queues = new String[cqs.size()];
         int i=0;
         while(iter.hasNext()) {
            ClusterQueue cq = (ClusterQueue)iter.next();
            if(cq.getName().equals("template")) {
               continue;
            }
            queues[i] = cq.getName();
            i++;
         }
         for (i=0; i < queues.length; i++) {
           logger.fine("Disable Queue: " + queues[i]);
         }
         try {
            jgdi.disableQueues(queues, false);
         } catch (IllegalArgumentException je) {
            je.printStackTrace();
         }
      } finally {
         jgdi.close();
      }
   }
   
   
   
}
