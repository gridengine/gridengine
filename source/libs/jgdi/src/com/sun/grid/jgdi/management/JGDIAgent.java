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

import com.sun.grid.jgdi.jni.AbstractEventClient;
import com.sun.grid.jgdi.jni.JGDIBaseImpl;
import java.util.concurrent.locks.ReentrantLock;
import java.util.logging.Level;
import java.util.logging.LogManager;
import javax.management.ObjectName;
import javax.management.MBeanServer;
import java.lang.management.ManagementFactory;

import com.sun.grid.jgdi.management.mbeans.JGDIJMX;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.logging.Logger;

/**
 * JGDI JMX agent class.
 */
public class JGDIAgent {

    /**
     * Instantiate and register your MBeans.
     */
    public void init(String url) throws Exception {

        //TODO Add your MBean registration code here
        this.url = url;
        logger.log(Level.FINE, "init: " + JGDIAgent.getUrl());

        // Instantiate and register JGDIJMX MBean
        JGDIJMX mbean = new JGDIJMX();
        ObjectName mbeanName = new ObjectName("gridengine:type=JGDI");
        getMBeanServer().registerMBean(mbean, mbeanName);
        logger.log(Level.INFO, "mbean " + mbeanName + " registered");

    }
    
    /**
     * Returns an agent singleton.
     */
    public synchronized static JGDIAgent getDefault(String url) throws Exception {
        if (singleton == null) {
            singleton = new JGDIAgent();
            singleton.init(url);
        }
        return singleton;
    }

    public static String getUrl() {
        return url;
    }

    public MBeanServer getMBeanServer() {
        return mbs;
    }
    
    // Platform MBeanServer used to register your MBeans
    private final MBeanServer mbs = ManagementFactory.getPlatformMBeanServer();
    // Singleton instance

    private static JGDIAgent singleton;
    // JGDI url string

    private static String url;
    private final static Logger logger = Logger.getLogger(JGDIAgent.class.getName());

    public static void main(String[] args) {
        try {
            if (args.length != 1) {
                logger.log(Level.SEVERE, "invalid arguments for JGDIAgent: JGDIAgent <jgdi connect url>");
                return;
            }
            String sge_url = args[0];
            JGDIAgent agent = JGDIAgent.getDefault(sge_url);

            logger.log(Level.INFO, "JGDIAgent '" + sge_url + "' started");       
            
            waitForShutdown();

            logger.log(Level.INFO, "JGDIAgent shuts down");
            
            AbstractEventClient.closeAll();
            JGDIBaseImpl.closeAllConnections();
            
            
        } catch (Exception ex) {
            logger.log(Level.SEVERE, "Unexpected error", ex);
        } finally {
            logger.log(Level.INFO, "JGDIAgent is down");
            LogManager.getLogManager().reset();
        }


    }

    private static final Lock shutdownLock = new ReentrantLock();
    private static final Condition shutdownCondition = shutdownLock.newCondition();
    private static boolean isRunning = true;
    
    
    
    private static void waitForShutdown() throws InterruptedException {
        shutdownLock.lock();
        try {
            while(isRunning) {
                shutdownCondition.await();
            }
        } finally {
            shutdownLock.unlock();
        }
    }
    
    public static void shutdown() {
        shutdownLock.lock();
        try {
            isRunning = false;
            shutdownCondition.signalAll();
        } finally {
            shutdownLock.unlock();
        }
    }
    
    
}

