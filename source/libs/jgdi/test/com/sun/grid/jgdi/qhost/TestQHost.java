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
package com.sun.grid.jgdi.qhost;

import com.sun.grid.jgdi.JGDI;
import com.sun.grid.jgdi.monitoring.HostInfo;
import com.sun.grid.jgdi.monitoring.QHostOptions;
import com.sun.grid.jgdi.monitoring.QHostResult;
import com.sun.grid.jgdi.monitoring.filter.HostFilter;
import com.sun.grid.jgdi.monitoring.filter.ResourceAttributeFilter;
import com.sun.grid.jgdi.configuration.ClusterQueue;
import com.sun.grid.jgdi.configuration.QueueInstance;
import com.sun.grid.jgdi.configuration.ExecHost;
import com.sun.grid.jgdi.monitoring.QueueInfo;
import java.util.Iterator;
import java.util.List;
import java.util.Set;
import junit.framework.Test;
import junit.framework.TestSuite;

/**
 *
 */
public class TestQHost extends com.sun.grid.jgdi.BaseTestCase {
   
   /** Creates a new instance of TestQHost */
   public TestQHost(String testName) {
      super(testName);
   }
   
   protected void setUp() throws Exception {
      super.setUp();
   }
   
   public static Test suite() {
      TestSuite suite = new TestSuite(TestQHost.class);
      return suite;
   }
   
   private void printResult(QHostResult res) {
      Iterator hostIter = res.getHostNames().iterator();
      while(hostIter.hasNext()) {
         String hostname = (String)hostIter.next();
         
         HostInfo hostInfo = res.getHostInfo(hostname);
         
         logger.fine("Host " + hostname + " ---------------------");
         Iterator hostValueiter = hostInfo.getHostValueKeys().iterator();
         while(hostValueiter.hasNext()) {
            String hostValueNames = (String)hostValueiter.next();
            logger.fine("HostValue: " + hostValueNames + " = " + hostInfo.getHostValue(hostValueNames));
         }
         
         Iterator queueIter = hostInfo.getQueueList().iterator();
         while(queueIter.hasNext()) {
            QueueInfo qi = (QueueInfo)queueIter.next();
            logger.fine("  Queue: " + qi.getQname());
            logger.fine("  Qtype: " + qi.getQtype());
            logger.fine("  State: " + qi.getState());
            logger.fine("  total: " + qi.getTotalSlots());
            logger.fine("  used:  " + qi.getUsedSlots());
         }
         
         Iterator dominanceIter = hostInfo.getDominanceSet().iterator();
         while(dominanceIter.hasNext()) {
            String dom = (String)dominanceIter.next();
            Iterator resourceValueIter = hostInfo.getResourceValueNames(dom).iterator();
            while(resourceValueIter.hasNext()) {
               String resourceValueName = (String)resourceValueIter.next();
               logger.fine("Resource: " + dom + ": " + resourceValueName + " = " + hostInfo.getResourceValue(dom, resourceValueName));
            }
         }
      }
   }
   
   
   public void testAllHostValues() throws Exception {
      
      JGDI jgdi = createJGDI();
      try {
         QHostOptions qhostOptions = new QHostOptions();

         ResourceAttributeFilter resourceAttributeFilter = new ResourceAttributeFilter();

         qhostOptions.setResourceAttributeFilter(resourceAttributeFilter);

         QHostResult res = jgdi.execQHost(qhostOptions);

         printResult(res);      
      } finally {
         jgdi.close();
      }      
   }
   
   public void testQueueFilter() throws Exception {
      
      JGDI jgdi = createJGDI();
      try {
         QHostOptions qhostOptions = new QHostOptions();

         List ehList = jgdi.getExecHostList();

         List cqList = jgdi.getClusterQueueList();

         Iterator iter = ehList.iterator();

         qhostOptions.setIncludeQueue(true);

         while(iter.hasNext()) {
            ExecHost eh = (ExecHost)iter.next();

            if (eh.getName().equals("template") ||
                eh.getName().equals("global")) {
               continue;
            }
            HostFilter hf = new HostFilter();
            hf.addHost(eh.getName());
            qhostOptions.setHostFilter(hf);

            QHostResult res = jgdi.execQHost(qhostOptions);
            printResult(res);

            HostInfo hi = res.getHostInfo(eh.getName());

            assertNotNull(hi);            

            Iterator cqIter = cqList.iterator();
            while(cqIter.hasNext()) {
               ClusterQueue cq = (ClusterQueue)cqIter.next();

               Iterator cqiIter = cq.getQinstancesList().iterator();
               while(cqiIter.hasNext()) {
                  QueueInstance cqi = (QueueInstance)cqiIter.next();
                  if (cqi.getQhostname().equals(eh.getName()) ) {
                     boolean foundq = false;
                     Iterator hiter = hi.getQueueList().iterator();
                     while (hiter.hasNext()) {
                        String hqname = ((QueueInfo)hiter.next()).getQname();
                        // System.out.println("++ comparing cq.getName() " + cq.getName() +
                        //                      " and hqname " + hqname);
                        if (cq.getName().equals(hqname)) {
                        //   System.out.println("-- matching cq.getName() " + cq.getName() +
                        //                      " and hqname " + hqname);
                           foundq = true;
                           break;
                        }
                     }
                     assertTrue("queue " + cq.getName() + " not included for host " + eh.getName(), foundq);
                  }
               }
            }
         }      
      } finally {
         jgdi.close();
      }
   }
   
   public void testHostValueFilter() throws Exception {
      
      JGDI jgdi = createJGDI();
      try {
         QHostOptions qhostOptions = new QHostOptions();

         List ehList = jgdi.getExecHostList();


         Iterator iter = ehList.iterator();
         while(iter.hasNext()) {
            ExecHost eh = (ExecHost)iter.next();

            if(eh.getName().equals("template") ||
               eh.getName().equals("global")) {
               continue;
            }

            Iterator loadIter = eh.getLoadKeys().iterator();
            while(loadIter.hasNext()) {

               String loadValueName = (String)loadIter.next();
               String loadValue = eh.getLoad(loadValueName);

               ResourceAttributeFilter resourceAttributeFilter = new ResourceAttributeFilter();
               resourceAttributeFilter.addValueName(loadValueName);

               qhostOptions.setResourceAttributeFilter(resourceAttributeFilter);

               QHostResult res = jgdi.execQHost(qhostOptions);
               printResult(res);
               HostInfo hi = res.getHostInfo(eh.getName());
               assertNotNull(hi);            
               assertNotNull("Resource Value " + loadValueName + " not found", hi.getResourceValue("hl", loadValueName));
            }
         }      
      } finally {
         jgdi.close();
      }
   }
   
   public void testHostFilter() throws Exception {
      
      JGDI jgdi = createJGDI();
      try {
         QHostOptions qhostOptions = new QHostOptions();

         List ehList = jgdi.getExecHostList();


         Iterator iter = ehList.iterator();
         while(iter.hasNext()) {
            ExecHost eh = (ExecHost)iter.next();

            if(eh.getName().equals("template") ||
               eh.getName().equals("global")) {
               continue;
            }

            HostFilter hostFilter = new HostFilter();
            hostFilter.addHost(eh.getName());
            qhostOptions.setHostFilter(hostFilter);

            QHostResult res = jgdi.execQHost(qhostOptions);
            printResult(res);

            Set hostNames = res.getHostNames();

            assertTrue("host " + eh.getName() + " not found", hostNames.contains(eh.getName()));
            assertTrue("host global not found", hostNames.contains("global"));
         }
      } finally {
         jgdi.close();
      }      
   }
   
}
