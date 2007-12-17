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
package com.sun.grid.jgdi.jni;

import com.sun.grid.jgdi.JGDI;
import com.sun.grid.jgdi.JGDIException;
import com.sun.grid.jgdi.JGDIFactory;
import com.sun.grid.jgdi.event.Event;
import com.sun.grid.jgdi.event.EventListener;
import com.sun.grid.jgdi.event.QmasterGoesDownEvent;
import com.sun.grid.jgdi.event.ShutdownEvent;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;
import java.util.logging.Level;
import java.util.logging.LogRecord;
import java.util.logging.Logger;

/**
 * Abstract base clase for the event client
 *
 */
public abstract class AbstractEventClient implements Runnable {

    private static final Logger logger = Logger.getLogger("com.sun.grid.jgdi.event");
    
    private static List<AbstractEventClient> instances = new LinkedList<AbstractEventClient>();
    private static boolean shutdown = false;
    
    private int evc_index = -1;
    private int id;
    private JGDI jgdi;
    private Thread thread;
    
    protected final Lock fairLock = new ReentrantLock(true);
    private final Condition shutdownCondition = fairLock.newCondition();
    private final Condition startupCondition = fairLock.newCondition();
    
    private Set<EventListener> eventListeners = Collections.<EventListener>emptySet();
    private final Lock listenerLock = new ReentrantLock();
    

    /**
     * Creates a new instance of EventClient
     * @param url  JGDI connection url in the form
     *             <code>bootstrap://&lt;SGE_ROOT&gt;@&lt;SGE_CELL&gt;:&lt;SGE_QMASTER_PORT&gt;</code>
     * @param regId  Registration id for the event client (0 => qmaster assignes a
     *               event client id dynamically)
     * @throws com.sun.grid.jgdi.JGDIException
     */
    public AbstractEventClient(String url, int regId) throws JGDIException {
        synchronized (instances) {
            if (shutdown) {
                throw new JGDIException("qmaster is going down");
            }
            instances.add(this);
        }
        JGDI aJgdi = JGDIFactory.newInstance(url);
        if (aJgdi instanceof JGDIImpl) {
            this.jgdi = aJgdi;
            evc_index = initNative(aJgdi, regId);
        } else {
            throw new IllegalArgumentException("JGDI must be instanceof " + JGDIImpl.class.getName());
        }
    }

    public static void closeAll() {
        List<AbstractEventClient> currentInstances = null;
        synchronized (instances) {
            currentInstances = new ArrayList<AbstractEventClient>(instances);
            shutdown = true;
        }

        for (AbstractEventClient evt : currentInstances) {
            try {
                evt.close();
            } catch (Exception ex) {
            // Ignore
            }
        }

    }

    /**
     * Get the id of this event client
     * @return the event client id
     */
    public int getId() {
        return id;
    }

    /**
     * Set the event client id of this event client
     * <p><strong>This method is not intended to be call by the
     *         user. </strong>
     * The native part of the <code>AbstractEventClient</code> uses this method
     * to set a dynamic event client id into the java object.</p>
     *
     * @param id the event client id
     *
     */
    public void setId(int id) {
        this.id = id;
    }

    /**
     *  Get the index of this event client in the native event client
     *  list.
     *  @return index of this event client in the native event client list
     */
    public int getEVCIndex() {
        fairLock.lock();
        try {
            return evc_index;
        } finally {
            fairLock.unlock();
        }
    }
    
    /**
     *  Close this event client
     * @throws com.sun.grid.jgdi.JGDIException if the connection could not be closed
     * @throws java.lang.InterruptedException if the close has been interrupted
     */
    public void close() throws JGDIException, InterruptedException {

        Thread aThread = null;
        int index = 0;
        JGDI aJgdi = null;
        int evcId;
        fairLock.lock();
        try {
            aThread = thread;
            thread = null;
            index = evc_index;
            evc_index = -1;
            evcId = id;
        } finally {
            fairLock.unlock();
        }

        try {
            if (aThread != null) {
                logger.log(Level.FINE, "interrupting working thread for event client {0}", evcId);
                aThread.interrupt();
            }
            if (index >= 0) {
                logger.log(Level.FINE, "closing native event client with id {0}", evcId);
                closeNative(index);
                logger.log(Level.FINE, "native event client with id {0} closed", evcId);
            }
            if (aThread != null) {
                if (aThread.isAlive()) {
                    logger.log(Level.FINE, "waiting for end working thread of event client {0}", evcId);
                    aThread.join();
                }
                logger.log(Level.FINE, "working thread for event client {0} died", evcId);
            }
        } finally {
            try {
                fairLock.lock();
                try {
                    aJgdi = this.jgdi;
                    this.jgdi = null;
                    shutdownCondition.signalAll();
                } finally {
                    fairLock.unlock();
                }

                if (aJgdi != null) {
                    logger.log(Level.FINE, "closing jgdi connection for event client {0}", evcId);
                    aJgdi.close();
                    logger.log(Level.FINE, "jgdi connection for event client {0} closed", evcId);
                }
            } finally {
                synchronized (instances) {
                    instances.remove(this);
                }
            }
        }
    }

    /**
     *  Start the event client
     * @throws java.lang.InterruptedException if the startup has been interrupted
     */
    public void start() throws InterruptedException {
        fairLock.lock();
        try {
            if (thread == null) {
                thread = new Thread(this);
                thread.setPriority(Thread.NORM_PRIORITY - 1);
                thread.start();
                startupCondition.await();
            } else {
                throw new IllegalStateException("event client is already running");
            }
        } finally {
            fairLock.unlock();
        }
    }

    /**
     *  Run method of the event client thread
     */
    public void run() {

        int evcId = 0;
        try {
            fairLock.lock();
            try {
                try {
                    this.register();
                } finally {
                    startupCondition.signal();
                }
            } finally {
                fairLock.unlock();
            }

            List<Event> eventList = new ArrayList<Event>();
            evcId = getId();
            logger.log(Level.FINE, "event client registered at qmaster (id = {0})", evcId);

            while (!Thread.currentThread().isInterrupted()) {
                boolean gotShutdownEvent = false;
                try {
                    fairLock.lock();
                    try {

                        if (thread == null) {
                            break;
                        }
                        if (evc_index < 0) {
                            break;
                        }
                        logger.log(Level.FINER, "calling native method fillEvents for event client {0}", evcId);
                        fillEvents(eventList);
                    } finally {
                        fairLock.unlock();
                    }
                    if (logger.isLoggable(Level.FINER)) {
                        logger.log(Level.FINE, "got {0} events from qmaster for event client {1}",
                                new Object[]{eventList.size(), evcId});
                    }
                    if (Thread.currentThread().isInterrupted()) {
                        break;
                    }
                    for (Event event : eventList) {
                        try {
                            fireEventOccured(event);
                            if (event instanceof QmasterGoesDownEvent || event instanceof ShutdownEvent) {
                                gotShutdownEvent = true;
                            }
                        } catch (Exception e) {
                            logger.log(Level.WARNING, "error in fire event", e);
                        }
                    }
                } catch (Exception e) {
                    LogRecord lr = new LogRecord(Level.WARNING, "error in event loop of event client {0}");
                    lr.setParameters(new Object[]{evcId});
                    lr.setThrown(e);
                    logger.log(lr);
                }
                eventList.clear();
                if (gotShutdownEvent) {
                    logger.log(Level.FINE, "got shutdown event from master, exiting working thread of event client {0}", evcId);
                    break;
                }
            }
            close();
        } catch (Exception ex) {
            LogRecord lr = new LogRecord(Level.WARNING, "Unexpected error in event loop of event client {0}");
            lr.setParameters(new Object[]{evcId});
            lr.setThrown(ex);
            logger.log(lr);
        } finally {
            logger.exiting(getClass().getName(), "run");
            fairLock.lock();
            try {
                this.thread = null;
            } finally {
                fairLock.unlock();
            }
        }
    }

    /**
     *  Determine if the event client has been closed
     *  @return <code>true</code> if the event client has been closed
     */
    public boolean isClosed() {
        fairLock.lock();
        try {
            return evc_index < 0 || !isRunning();
        } finally {
            fairLock.unlock();
        }
    }
    
    /**
     * Determine if the event client is running
     * @return <code>true</code> if the event client is running
     */
    public boolean isRunning() {
        fairLock.lock();
        try {
            return thread != null && thread.isAlive() && !thread.isInterrupted();
        } finally {
            fairLock.unlock();
        }
    }

    protected void testConnected() throws JGDIException {
        fairLock.lock();
        try {
            if (jgdi == null) {
                throw new JGDIException("Not connected");
            }
        } finally {
            fairLock.unlock();
        }
    }

    /**
     *  Subscribe all events for this event client
     *  @throws JGDIException if subscribe failed
     */
    public void subscribeAll() throws JGDIException {
        fairLock.lock();
        try {
            subscribeAllNative();
        } finally {
            fairLock.unlock();
        }
    }

    /**
     *  Unsubscribe all events for this event client
     *  @throws JGDIException if unsubscribe failed
     */
    public void unsubscribeAll() throws JGDIException {
        fairLock.lock();
        try {
            unsubscribeAllNative();
        } finally {
            fairLock.unlock();
        }
    }

    protected void fireEventOccured(Event evt) {

        logger.log(Level.FINER, "fire event {0}", evt);
        Set<EventListener> tmp = null;
        listenerLock.lock();
        try {
            tmp = eventListeners;
        } finally {
            listenerLock.unlock();
        }

        for (EventListener lis : tmp) {
            lis.eventOccured(evt);
        }
    }

    /**
     * Add an event listener to this event client
     * @param lis the event listener
     */
    public void addEventListener(EventListener lis) {
        listenerLock.lock();
        try {
            Set<EventListener> tmp = eventListeners;
            eventListeners = new HashSet<EventListener>(tmp.size() + 1);
            eventListeners.addAll(tmp);
            eventListeners.add(lis);
        } finally {
            listenerLock.unlock();
        }
    }

    /**
     * Remove an event listener from this event client
     * @param lis the event listener
     */
    public void removeEventListener(EventListener lis) {
        listenerLock.lock();
        try {
            eventListeners = new HashSet<EventListener>(eventListeners);
            eventListeners.remove(lis);
        } finally {
            listenerLock.unlock();
        }
    }

    public void commit() throws JGDIException {
        fairLock.lock();
        try {
            logger.log(Level.FINE, "commit");
            testConnected();
            nativeCommit();
        } finally {
            fairLock.unlock();
        }
    }

    private void register() throws JGDIException {
        fairLock.lock();
        try {
            logger.log(Level.FINE, "register");
            testConnected();
            registerNative();
        } finally {
            fairLock.unlock();
        }
    }

    private void deregister() throws JGDIException {
        fairLock.lock();
        try {
            if (evc_index >= 0) {
                logger.log(Level.FINE, "deregistered");
                deregisterNative();
            } else {
                logger.log(Level.FINE, "can't deregister, already closed");
            }
        } finally {
            fairLock.unlock();
        }
    }

    private native void nativeCommit() throws JGDIException;

    private native void subscribeNative(int type) throws JGDIException;

    private native void unsubscribeNative(int type) throws JGDIException;

    private native void subscribeAllNative() throws JGDIException;

    private native void unsubscribeAllNative() throws JGDIException;

    private native void closeNative(int evcIndex) throws JGDIException;

    private native int initNative(JGDI jgdi, int regId) throws JGDIException;

    private native void registerNative() throws JGDIException;

    private native void deregisterNative() throws JGDIException;

    private native void fillEvents(List eventList) throws JGDIException;
}
