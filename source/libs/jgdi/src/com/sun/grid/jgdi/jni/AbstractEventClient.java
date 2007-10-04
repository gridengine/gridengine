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
import java.util.ArrayList;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Abstract base clase for the event client
 *
 */
public abstract class AbstractEventClient implements Runnable {
    
    private static final Logger logger = Logger.getLogger("com.sun.grid.jgdi.event");
    private int evc_index = -1;
    private int id;
    private JGDI jgdi;
    private Thread thread;
    protected final Object syncObject = new Object();
    
    /**
     * Creates a new instance of EventClient
     * @param url  JGDI connection url in the form
     *             <code>bootstrap://&lt;SGE_ROOT&gt;@&lt;SGE_CELL&gt;:&lt;SGE_QMASTER_PORT&gt;</code>
     * @param regId  Registration id for the event client (0 => qmaster assignes a
     *               event client id dynamically)
     * @throws com.sun.grid.jgdi.JGDIException
     */
    public AbstractEventClient(String url, int regId) throws JGDIException {
        JGDI jgdi = JGDIFactory.newInstance(url);
        if (jgdi instanceof JGDIImpl) {
            this.jgdi = jgdi;
            evc_index = initNative(jgdi, regId);
        } else {
            throw new IllegalArgumentException("JGDI must be instanceof " + JGDIImpl.class.getName());
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
        synchronized (syncObject) {
            return evc_index;
        }
    }
    
    /**
     *  Close this event client
     */
    public void close() throws JGDIException, InterruptedException {
        
        logger.log(Level.FINE, "close event client");
        Thread aThread = null;
        int index = 0;
        JGDI aJgdi = null;
        synchronized (syncObject) {
            aThread = thread;
            index = evc_index;
            evc_index = -1;
            thread = null;
            aJgdi = this.jgdi;
            syncObject.notifyAll();
        }
        try {
            if (aThread != null) {
                while (aThread.isAlive()) {
                    logger.log(Level.FINE, "interrupting working thread " + aThread.getName());
                    aThread.interrupt();
                    aThread.join();
                }
                logger.log(Level.FINE, "working thread died");
            }
        } finally {
            try {
                if (index >= 0) {
                    logger.log(Level.FINE, "closing native event client");
                    closeNative(index);
                }
            } finally {
                if (aJgdi != null) {
                    logger.log(Level.FINE, "closing jgdi connection for event client");
                    aJgdi.close();
                }
            }
        }
    }
    
    /**
     *  Start the event client
     */
    public void start() throws InterruptedException {
        synchronized (syncObject) {
            if (thread == null) {
                thread = new Thread(this);
                thread.setPriority(Thread.NORM_PRIORITY - 1);
                thread.start();
                syncObject.wait();
            } else {
                throw new IllegalStateException("event client is already running");
            }
        }
    }
    
    /**
     *  Run method of the event client thread
     */
    public void run() {
        
        try {
            
            synchronized (syncObject) {
                try {
                    this.register();
                } finally {
                    syncObject.notify();
                }
            }
            
            List<Event> eventList = new ArrayList<Event>();
            logger.fine("event client registered at qmaster (id = " + getId() + ")");
            
            while (!Thread.currentThread().isInterrupted()) {
                boolean gotShutdownEvent = false;
                try {
                    synchronized (syncObject) {
                        if (thread == null) {
                            break;
                        }
                        if (evc_index < 0) {
                            break;
                        }
                        logger.log(Level.FINE, "calling native method fillEvents");
                        fillEvents(eventList);
                    }
                    if (logger.isLoggable(Level.FINE)) {
                        logger.log(Level.FINE, "got {0} events from qmaster", new Integer(eventList.size()));
                    }
                    for (Event event : eventList) {
                        try {
                            fireEventOccured(event);
                            if (event instanceof QmasterGoesDownEvent) {
                                gotShutdownEvent = true;
                            }
                        } catch (Exception e) {
                            logger.log(Level.WARNING, "error in fire event", e);
                        }
                    }
//give threads a different priority or use Thread.sleep() or queue events ?????
//               Thread.sleep(1);
                    Thread.yield();
                } catch (Exception e) {
                    logger.log(Level.WARNING, "error in event loop", e);
                }
                eventList.clear();
                if (gotShutdownEvent) {
                    logger.log(Level.FINE, "got shutdown event from master, exiting event thread");
                    break;
                }
            }
            // Deregister the event clients, no new message will be accepted
            deregister();
        } catch (JGDIException jgdie) {
            // TODO
            jgdie.printStackTrace();
        } finally {
            logger.exiting(getClass().getName(), "run");
            this.thread = null;
        }
    }
    
    /**
     *  Determine if the event client is running
     *  @return <code>true</code> if the event client is running
     */
    public boolean isRunning() {
        synchronized (syncObject) {
            return thread != null && thread.isAlive();
        }
    }
    
    protected void testConnected() throws JGDIException {
        if (jgdi == null) {
            throw new JGDIException("Not connected");
        }
    }
    
    /**
     *  Subscribe all events for this event client
     *  @throws JGDIException if the subscribtion is failed
     */
    public void subscribeAll() throws JGDIException {
        synchronized (syncObject) {
            subscribeAllNative();
        }
    }
    
    
    /**
     *  Unsubscribe all events for this event client
     *  @throws JGDIException if the unsubscribtion is failed
     */
    public void unsubscribeAll() {
        synchronized (syncObject) {
            try {
                unsubscribeAllNative();
            } catch (JGDIException jgdie) {
                // TODO
                jgdie.printStackTrace();
            }
        }
    }
    
    
    protected void fireEventOccured(Event evt) {
        
        logger.fine("fire event " + evt);
        EventListener[] lis = null;
        synchronized (eventListeners) {
            lis = new EventListener[eventListeners.size()];
            eventListeners.toArray(lis);
        }
        
        for (int i = 0; i < lis.length; i++) {
            lis[i].eventOccured(evt);
        }
    }
    
    private List<EventListener> eventListeners = new ArrayList<EventListener>();
    
    /**
     * Add an event listener to this event client
     * @param lis the event listener
     */
    public void addEventListener(EventListener lis) {
        synchronized (eventListeners) {
            eventListeners.add(lis);
        }
    }
    
    /**
     * Remove an event listener from this event client
     * @param lis the event listener
     */
    public void removeEventListener(EventListener lis) {
        synchronized (eventListeners) {
            eventListeners.remove(lis);
        }
    }
    
    public void commit() throws JGDIException {
        synchronized (syncObject) {
            logger.log(Level.FINE, "commit");
            testConnected();
            nativeCommit();
        }
    }
    
    private void register() throws JGDIException {
        synchronized (syncObject) {
            logger.log(Level.FINE, "register");
            testConnected();
            registerNative();
        }
    }
    
    private void deregister() throws JGDIException {
        synchronized (syncObject) {
            if (evc_index >= 0) {
                logger.log(Level.FINE, "deregistered");
                deregisterNative();
            } else {
                logger.log(Level.FINE, "can't deregister, already closed");
            }
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
