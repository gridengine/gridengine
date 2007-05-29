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

import com.sun.grid.jgdi.JGDIException;
import com.sun.grid.jgdi.monitoring.*;
import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 *
 */
abstract public class JGDIBase implements com.sun.grid.jgdi.JGDIBase {
   
   private int ctxIndex;
   
   public JGDIBase() {
   }
   
   public void init(String url) throws JGDIException {
      ctxIndex = initNative(url);
   }
   
   int getCtxIndex() {
      return ctxIndex;
   }
   
   private native int initNative(String url) throws JGDIException;
   
   public synchronized void close() throws com.sun.grid.jgdi.JGDIException {
      if( ctxIndex >= 0 ) {
         close(ctxIndex);
         ctxIndex = -1;
      }
   }
   
   private native void close(int ctxIndex);
   
   
   private File sgeRoot;
   
   /**
    * Get the root directory of the grid engine installation
    * @throws com.sun.grid.jgdi.JGDIException 
    * @return root directory of the gridengine installation
    */
   public File getSGERoot() throws JGDIException {
      if(sgeRoot == null) {
         sgeRoot = new File(getNativeSGERoot());
      } 
      return sgeRoot;
   }
   
   private native String getNativeSGERoot() throws JGDIException;

   /**
    * Get the cell of the grid engine
    * @return the cell of the grid engine
    * @throws com.sun.grid.jgdi.JGDIException 
    */
   public native String getSGECell() throws JGDIException;
   
   
   public native String getAdminUser() throws JGDIException;
   public native String getActQMaster() throws JGDIException;
   
   
   public QHostResult execQHost(QHostOptions options) throws JGDIException {
      
      QHostResultImpl ret  = new QHostResultImpl();
      
      execQHost(options, ret);
      
      return ret;
   }
   
   private native void execQHost(QHostOptions options, QHostResultImpl result);
   
   
   public List getClusterQueueSummary(ClusterQueueSummaryOptions options) throws JGDIException {
      List ret = new ArrayList();
      fillClusterQueueSummary(options, ret);
      return ret;
   }
   
   private native void fillClusterQueueSummary(ClusterQueueSummaryOptions options, List list) throws JGDIException;
   
   public QueueInstanceSummaryResult getQueueInstanceSummary(QueueInstanceSummaryOptions options) throws JGDIException {
      QueueInstanceSummaryResultImpl ret = new QueueInstanceSummaryResultImpl();
      fillQueueInstanceSummary(options, ret);
      return ret;
   }
   
   private native void fillQueueInstanceSummary(QueueInstanceSummaryOptions options, QueueInstanceSummaryResultImpl result) throws JGDIException;
   
   /**
    *  Get the limit information.
    *
    *  The CLI equivialent of this method is <code>qquota</code> (see man qquota(1)).
    *  
    *  @param options the options for the qquota algorithm
    *  @return  the {@link com.sun.grid.jgdi.monitoring.QQuotaResult}
    */
   public QQuotaResult getQQuota(QQuotaOptions options) throws JGDIException {
      QQuotaResultImpl ret  = new QQuotaResultImpl();
      
      getQQuota(options, ret);
      
      return ret;
   }
   
   /*
    * @inherited
    */
   public native void cleanQueues(String[] queues) throws JGDIException;
   
   
   /*
    * @inherited
    */
   public native void unsuspendQueues(String[] queues, boolean force) throws JGDIException;
   
   /*
    * @inherited
    */
   public native void unsuspendJobs(String[] jobs, boolean force) throws JGDIException;
   
   /*
    * @inherited
    */
   public native void suspendQueues(String[] queues, boolean force) throws JGDIException;
   
   /*
    * @inherited
    */
   public native void suspendJobs(String[] jobs, boolean force) throws JGDIException;
   
   /*
    * @inherited
    */
   public native void rescheduleJobs(String[] jobs, boolean force) throws JGDIException;
   
   /*
    * @inherited
    */
   public native void rescheduleQueues(String[] queues, boolean force) throws JGDIException;
   
   /*
    * @inherited
    */
   public native void clearJobs(String[] jobs, boolean force) throws JGDIException;
   
   /*
    * @inherited
    */
   public native void clearQueues(String[] queues, boolean force) throws JGDIException;
   
   /*
    * @inherited
    */
   public native void disableQueues(String[] queues, boolean force) throws JGDIException;
   
   /*
    * @inherited
    */
   public native void enableQueues(String[] queues, boolean force) throws JGDIException;
   
   
   /*
    * @inherited
    */
   public native void killAllExecds(boolean terminateJobs) throws JGDIException;
   
   /*
    * @inherited
    */
   public native void killExecd(String[] hosts, boolean terminateJobs) throws JGDIException;
   
   /*
    * @inherited
    */
   public native void killEventClients(int[] ids) throws JGDIException;
   
   /*
    * @inherited
    */
   public native void triggerSchedulerMonitoring() throws JGDIException;
   
   /*
    * @inherited
    */
   public native void clearShareTreeUsage() throws JGDIException;
   
   /*
    * @inherited
    */
   public native void killAllEventClients() throws JGDIException;
   
   /*
    * @inherited
    */
   public native void killMaster() throws JGDIException;
   
   /*
    * @inherited
    */
   public native void killScheduler() throws JGDIException;
   
   
   /*
    * @inherited
    */
   public native String getSchedulerHost() throws JGDIException;

   /*
    * @inherited
    */
   public native String showDetachedSettings(String[] queues) throws JGDIException;

   /*
    * @inherited
    */
   public native String showDetachedSettingsAll() throws JGDIException;
   
   
   private native void getQQuota(QQuotaOptions options, QQuotaResultImpl ret) throws JGDIException;
}
