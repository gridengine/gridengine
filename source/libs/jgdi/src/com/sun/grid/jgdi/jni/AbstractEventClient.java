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
import com.sun.grid.jgdi.event.Event;
import com.sun.grid.jgdi.event.EventListener;
import com.sun.grid.jgdi.event.EventType;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.logging.Logger;

/**
 * Abstract base clase for the event client 
 *
 * @author  richard.hierlmeier@sun.com
 */
public abstract class AbstractEventClient implements Runnable {

   private Logger logger = Logger.getLogger("com.sun.grid.jgdi.event");
   int evc_index;
   int id;
   private Thread thread;
   protected Object syncObject = new Object();
   
   /**
    * Creates a new instance of EventClient
    * @param jgdi   the jgdi connection which is used for communication
    * @param regId  Registration id for the event client (0 => qmaster assignes a 
    *               event client id dynamically)
    * @throws com.sun.grid.jgdi.JGDIException 
    */
   public AbstractEventClient(com.sun.grid.jgdi.JGDI jgdi, int regId) throws JGDIException {
      if (jgdi instanceof JGDIImpl) {
         evc_index = initNative(jgdi, regId);
      } else {
         throw new IllegalArgumentException("JGDI must be instanceof com.sun.grid.jgdi.jni.JGDIImpl");
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
      return evc_index;
   }
   
   /**
    *  Close this event client
    */
   public void close() throws JGDIException, InterruptedException {
      
      Thread aThread = thread;
      thread = null;
      if( aThread != null ) {
         aThread.interrupt();
         if( aThread != Thread.currentThread() ) {
            aThread.join();
         }
      }
      
      int index = this.evc_index;
      this.evc_index = 0;
      if( index != 0) {
         closeNative(index);
      }
      
   }
   
   /**
    *  Start the event client
    */
   public void start() throws InterruptedException {
      synchronized(syncObject) {
         if( thread == null ) {
            thread = new Thread(this);
            thread.start();
            syncObject.wait();
         }
      }
   }
   
   /**
    *  Run method of the event client thread
    */
   public void run() {
      
      try {
         
         synchronized(syncObject) {
            try {
               this.register();
            } finally {
               syncObject.notify();
            }
         }
         
         ArrayList eventList = new ArrayList();
         try {
            logger.info("event client registered at qmaster (id = " + getId() + ")");

            
            while( !Thread.currentThread().isInterrupted() ) {
               try {
                  synchronized(syncObject) {
                     fillEvents(eventList);
                  }
                  Iterator iter = eventList.iterator();
                  while( iter.hasNext() ) {
                     try {
                        fireEventOccured((Event)iter.next());
                     } catch(Exception e) {
                        e.printStackTrace();
                     }
                  }
               } catch( Exception e ) {
                  e.printStackTrace();
               }
               eventList.clear();
               Thread.sleep(1000); // TODO define a sleep time, may be fillEvents should block
            }
         } finally {
            synchronized(syncObject) {
               fillEvents(eventList);
               this.deregister();
            }
         }
         
      } catch( JGDIException jgdie ) {
         // TODO
         jgdie.printStackTrace();
      } catch( InterruptedException ire ) {
         
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
      return thread != null && thread.isAlive();
   }
   
   /**
    *  Subscribe all events for this event client
    *  @throws JGDIException if the subscribtion is failed
    */
   public void subscribeAll() throws JGDIException {
      subscribeAllNative();
   }
   
   
   /**
    *  Unsubscribe all events for this event client
    *  @throws JGDIException if the unsubscribtion is failed
    */
   public void unsubscribeAll() {
      try {
         unsubscribeAllNative();
      } catch (JGDIException jgdie) {
         // TODO
         jgdie.printStackTrace();
      }
      
   }
   
   
   protected void fireEventOccured(Event evt) {

      logger.fine("fire event " + evt);
      EventListener [] lis = null;
      synchronized(eventListeners) {
         lis = new EventListener[eventListeners.size()];
         eventListeners.toArray(lis);
      }
      
      for(int i = 0; i < lis.length; i++) {
         lis[i].eventOccured(evt);
      }
   }
   
   private List eventListeners = new ArrayList();
   
   /**
    * Add an event listener to this event client
    * @param lis the event listener
    */
   public void addEventListener(EventListener lis) {
      synchronized(eventListeners) {
         eventListeners.add(lis);
      }
   }
   
   /**
    * Remove an event listener from this event client
    * @param lis the event listener
    */
   public void removeEventListener(EventListener lis) {
      synchronized(eventListeners) {
         eventListeners.remove(lis);
      }
   }
   
   public void commit() throws JGDIException {
      synchronized(syncObject) {
         nativeCommit();
      }
   }
   
   private native void nativeCommit() throws JGDIException;
   
   private native void subscribeNative(int type) throws JGDIException;
   private native void unsubscribeNative(int type) throws JGDIException;
   private native void subscribeAllNative() throws JGDIException;
   private native void unsubscribeAllNative() throws JGDIException;
   private native void closeNative(int evcIndex) throws JGDIException;
   private native int initNative(JGDI jgdi, int regId) throws JGDIException;
   private native void register() throws JGDIException;
   
   private native void deregister() throws JGDIException;
   
   private native void fillEvents(List eventList) throws JGDIException;
   
}
