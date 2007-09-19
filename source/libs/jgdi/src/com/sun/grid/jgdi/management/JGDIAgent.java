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

package com.sun.grid.jgdi.management;

import java.util.logging.Level;
import javax.management.ObjectName;
import javax.management.MBeanServer;
import java.lang.management.ManagementFactory;

import com.sun.grid.jgdi.management.mbeans.JGDIJMX;
import java.util.logging.Logger;

/**
 * JGDI JMX agent class.
 */
public class JGDIAgent {
    
   private final static Logger log = Logger.getLogger(JGDIAgent.class.getName());
   private JGDIJMX mbean;
//   private CalendarBean cal;
   
   public JGDIJMX getJGDIMBean() {
      return mbean;
   }
   
   /**
    * Instantiate and register your MBeans.
    */
   public void init(String url) throws Exception {
      
      //TODO Add your MBean registration code here
      
      log.log(Level.INFO,"init: " + url + "-----------------------");
      
      // Instantiate JGDIJMX MBean
      mbean = new JGDIJMX(url);
      ObjectName mbeanName = new ObjectName("gridengine:type=JGDI");
      //Register the JGDI MBean
      getMBeanServer().registerMBean(mbean, mbeanName);
      
      log.log(Level.INFO,"mbean " + mbeanName + " registered");
      
      
//      cal = new CalendarBean();
//      mbeanName = new ObjectName("jgdi.grid.sun.com:type=configuration.CalendarBean");
//      //Register the JGDI MBean
//      getMBeanServer().registerMBean(cal, mbeanName);
      
      
   }
   
   /**
    * Returns an agent singleton.
    */
   public synchronized static JGDIAgent getDefault(String url) throws Exception {
      if(singleton == null) {
         singleton = new JGDIAgent();
         singleton.init(url);
      }
      return singleton;
   }
   
   public MBeanServer getMBeanServer() {
      return mbs;
   }
   
   // Platform MBeanServer used to register your MBeans
   private final MBeanServer mbs = ManagementFactory.getPlatformMBeanServer();
   
   // Singleton instance
   private static JGDIAgent singleton;
   
   private final static Logger logger = Logger.getLogger(JGDIAgent.class.getName());
   
   public static void main(String [] args) {
       
       try {
           if(args.length != 1) {
               System.err.println("JGDIAgent <jgdi connect url>");
               System.exit(1);
           }
           String sge_url = args[0];

           // start JGDIAgent
           JGDIAgent agent = JGDIAgent.getDefault(sge_url);

           ShutdownHook shutdownHook = new ShutdownHook();
           Runtime.getRuntime().addShutdownHook(shutdownHook);
           shutdownHook.waitForShutdown();
       } catch(Exception ex) {
           log.log(Level.SEVERE, "Unexpected error", ex);
       }
   }
   
   private static class ShutdownHook extends Thread {
      
      public void run() {
        synchronized(this) {
            notifyAll();
        }
      }
      
      public synchronized void waitForShutdown() throws InterruptedException {
         wait();
      }
   }
   
}



