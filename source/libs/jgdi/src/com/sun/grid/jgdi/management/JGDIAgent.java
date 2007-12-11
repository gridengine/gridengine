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
import java.io.FileInputStream;
import java.io.IOException;
import java.util.Properties;
import java.util.concurrent.locks.ReentrantLock;
import java.util.logging.Level;
import java.util.logging.LogManager;
import javax.management.ObjectName;
import javax.management.MBeanServer;
import java.lang.management.ManagementFactory;

import com.sun.grid.jgdi.management.mbeans.JGDIJMX;
import java.io.File;
import java.io.FileOutputStream;
import java.io.PrintStream;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.logging.Logger;
import javax.management.remote.JMXConnectorServer;
import sun.management.jmxremote.ConnectorBootstrap;

/**
 * JGDI JMX agent class.
 */
public class JGDIAgent {

    private JMXConnectorServer mbeanServerConnector;
    // Platform MBeanServer used to register your MBeans
    private MBeanServer mbs;
    private static final JGDIAgent agent = new JGDIAgent();

    /**
     * Instantiate and register your MBeans.
     */
    private void JGDIAgent() throws Exception {
    }

    public void startMBeanServer() throws Exception {

        File managementFile = new File(getJmxDir(), "management.properties");

        logger.log(Level.FINE, "loading mbean server configuration from {0}", managementFile);

        final Properties props = new Properties();
        props.load(new FileInputStream(managementFile));

        final String portStr = props.getProperty(ConnectorBootstrap.PropertyNames.PORT);

        mbeanServerConnector = ConnectorBootstrap.initialize(portStr, props);
        mbs = mbeanServerConnector.getMBeanServer();
        logger.log(Level.FINE, "starting mbean server");
        mbeanServerConnector.start();
        logger.log(Level.INFO, "mbean server started (port={0})", portStr);
    }

    private void stopMBeanServer() {
        if (mbeanServerConnector != null) {
            try {
                logger.log(Level.FINE, "stopping mbean server");
                mbeanServerConnector.stop();
            } catch (Exception ex) {
                logger.log(Level.WARNING, "cannot stop mbean server", ex);
            }
        }
    }

    private void registerDefaultMBean() throws Exception {
        // Instantiate and register JGDIJMX MBean
        JGDIJMX mbean = new JGDIJMX();
        ObjectName mbeanName = new ObjectName("gridengine:type=JGDI");
        getMBeanServer().registerMBean(mbean, mbeanName);

        logger.log(Level.INFO, "mbean {0} registered (jgdi_url={1} ", new Object[]{mbeanName, getUrl()});
    }

    public static String getUrl() {
        if (url == null) {
            throw new IllegalStateException("JGDIAgent.url is not initialized");
        }
        return url;
    }
    private static File sgeRoot;

    public static File getSgeRoot() {
        if (sgeRoot == null) {
            String sgeRootStr = System.getProperty("com.sun.grid.jgdi.sgeRoot");
            if (sgeRootStr == null) {
                throw new IllegalStateException("system properties com.sun.grid.jgdi.sgeRoot not found");
            }
            sgeRoot = new File(sgeRootStr);
        }
        return sgeRoot;
    }

    public static String getSgeCell() {
        String ret = System.getProperty("com.sun.grid.jgdi.sgeCell");
        if (ret == null) {
            throw new IllegalStateException("system properties com.sun.grid.jgdi.sgeCell not found");
        }
        return ret;
    }
    private static File jmxDir;

    public static File getJmxDir() {
        if (jmxDir == null) {
            File sgeRoot = getSgeRoot();
            jmxDir = new File(sgeRoot, getSgeCell() + File.separatorChar + "common" + File.separatorChar + "jmx");
        }
        return jmxDir;
    }

    private MBeanServer getMBeanServer() throws IOException {
        if (mbs == null) {
            throw new IllegalStateException("mbean server is not started");
        }
        return mbs;
    }
    // JGDI url string
    private static String url;
    private final static Logger logger = Logger.getLogger(JGDIAgent.class.getName());

    public static void main(String[] args) {
        try {

            if (args.length != 1) {
                logger.log(Level.SEVERE, "invalid arguments for JGDIAgent: JGDIAgent <jgdi connect url>");
                return;
            }
            url = args[0];

            try {
                FileOutputStream stdout = new FileOutputStream("jgdi.stdout", true);
                System.setOut(new PrintStream(stdout, true));
                logger.fine("stdout redirected to jgdi.stdout");
            } catch (Exception ex) {
                logger.log(Level.WARNING, "cannot redirect stdout to file jgdi.stdout", ex);
            }
            try {
                FileOutputStream stderr = new FileOutputStream("jgdi.stderr", true);
                System.setErr(new PrintStream(stderr, true));
                logger.fine("stderr redirected to jgdi.stderr");
            } catch (Exception ex) {
                logger.log(Level.WARNING, "cannot redirect stderr to file jgdi.stderr", ex);
            }

            try {
                agent.startMBeanServer();
            } catch (Exception ex) {
                logger.log(Level.SEVERE, "startup of mbean server failed", ex);
                return;
            }
            try {

                try {
                    agent.registerDefaultMBean();
                } catch (Exception ex) {
                    logger.log(Level.SEVERE, "cannot register default mbean", ex);
                    return;
                }

                // The following code blocks until the shutdownMain method is called
                waitForShutdown();
            } catch (InterruptedException ex) {
                logger.log(Level.FINE, "JGDIAgent has been interrupted");
            } finally {
                logger.log(Level.INFO, "JGDIAgent is going down");
                try {
                    AbstractEventClient.closeAll();
                    JGDIBaseImpl.closeAllConnections();
                } finally {
                    agent.stopMBeanServer();
                }
            }

        } catch (Throwable ex) {
            logger.log(Level.SEVERE, "unexpected error in JGDIAgent", ex);
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
            while (isRunning) {
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

