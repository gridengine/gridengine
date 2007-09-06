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
import java.util.LinkedList;
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
      List ret = new LinkedList();
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
   public void cleanQueues(String[] queues) throws JGDIException {
       cleanQueuesWithAnswer(queues, null);
   }
   
   /*
    * @inherited
    */
   public native void cleanQueuesWithAnswer(String[] queues, java.util.List answers) throws JGDIException;

   /*
    * @inherited
    */
   public void unsuspend(String[] queues, boolean force) throws JGDIException {
      unsuspendWithAnswer(queues, force, null); 
   }

   /*
    * @inherited
    */
   public native void unsuspendWithAnswer(String[] queues, boolean force, java.util.List answers) throws JGDIException;
      
      
   /*
    * @inherited
    */
   public void unsuspendQueues(String[] queues, boolean force) throws JGDIException {
      unsuspendQueuesWithAnswer(queues, force, null); 
   }

   /*
    * @inherited
    */
   public native void unsuspendQueuesWithAnswer(String[] queues, boolean force, java.util.List answers) throws JGDIException;
      
   /*
    * @inherited
    */
   public void unsuspendJobs(String[] jobs, boolean force) throws JGDIException {
       unsuspendJobsWithAnswer(jobs, force, null);
   }

   /*
    * @inherited
    */
   public native void unsuspendJobsWithAnswer(String[] jobs, boolean force, java.util.List answers) throws JGDIException;
   
   /*
    * @inherited
    */
   public void suspend(String[] queues, boolean force) throws JGDIException {
       suspendWithAnswer(queues, force, null);
   }

   /*
    * @inherited
    */
   public native void suspendWithAnswer(String[] queues, boolean force, java.util.List answers) throws JGDIException;
   
   
   /*
    * @inherited
    */
   public void suspendQueues(String[] queues, boolean force) throws JGDIException {
       suspendQueuesWithAnswer(queues, force, null);
   }

   /*
    * @inherited
    */
   public native void suspendQueuesWithAnswer(String[] queues, boolean force, java.util.List answers) throws JGDIException;
   
   
   /*
    * @inherited
    */
   public void suspendJobs(String[] jobs, boolean force) throws JGDIException {
      suspendJobsWithAnswer(jobs, force, null); 
   }

   /*
    * @inherited
    */
   public native void suspendJobsWithAnswer(String[] jobs, boolean force, java.util.List answers) throws JGDIException;
   
   
   /*
    * @inherited
    */
   public void rescheduleJobs(String[] jobs, boolean force) throws JGDIException {
       rescheduleJobsWithAnswer(jobs, force, null);
   }

   /*
    * @inherited
    */
   public native void rescheduleJobsWithAnswer(String[] jobs, boolean force, java.util.List answers) throws JGDIException;
   
   
   /*
    * @inherited
    */
   public void rescheduleQueues(String[] queues, boolean force) throws JGDIException {
      rescheduleQueuesWithAnswer(queues, force, null); 
   }

   /*
    * @inherited
    */
   public native void rescheduleQueuesWithAnswer(String[] queues, boolean force, java.util.List answers) throws JGDIException;

   /*
    * @inherited
    */
   public void reschedule(String[] queue_or_job, boolean force) throws JGDIException {
      rescheduleWithAnswer(queue_or_job, force, null); 
   }

   /*
    * @inherited
    */
   public native void rescheduleWithAnswer(String[] queue_or_job, boolean force, java.util.List answers) throws JGDIException;
   
   
   /*
    * @inherited
    */
   public void clearJobs(String[] jobs, boolean force) throws JGDIException {
       clearJobsWithAnswer(jobs, force, null);
   }

   /*
    * @inherited
    */
   public native void clearJobsWithAnswer(String[] jobs, boolean force, java.util.List answers) throws JGDIException;
   
   
   /*
    * @inherited
    */
   public void clearQueues(String[] queues, boolean force) throws JGDIException {
       clearQueuesWithAnswer(queues, force, null);
   }

   /*
    * @inherited
    */
   public native void clearQueuesWithAnswer(String[] queues, boolean force, java.util.List answers) throws JGDIException;
   
   
   /*
    * @inherited
    */
   public void disableQueues(String[] queues, boolean force) throws JGDIException {
       disableQueuesWithAnswer(queues, force, null);
   }

   /*
    * @inherited
    */
   public native void disableQueuesWithAnswer(String[] queues, boolean force, java.util.List answers) throws JGDIException;
   
   
   /*
    * @inherited
    */
   public void enableQueues(String[] queues, boolean force) throws JGDIException {
     enableQueuesWithAnswer(queues, force, null);  
   }

   /*
    * @inherited
    */
   public native void enableQueuesWithAnswer(String[] queues, boolean force, java.util.List answers) throws JGDIException;
   
   
   /*
    * @inherited
    */
   public void killAllExecds(boolean terminateJobs) throws JGDIException {
       killAllExecdsWithAnswer(terminateJobs, null);
   }
   
   /*
    * @inherited
    */
   public native void killAllExecdsWithAnswer(boolean terminateJobs, List answers) throws JGDIException;

   /*
    * @inherited
    */
   public void killExecd(String[] hosts, boolean terminateJobs) throws JGDIException {
       killExecdWithAnswer(hosts, terminateJobs, null);
   }
   
   /*
    * @inherited
    */
   public native void killExecdWithAnswer(String[] hosts, boolean terminateJobs, List answers) throws JGDIException;

   /*
    * @inherited
    */
   public void killEventClients(int[] ids) throws JGDIException {
       killEventClientsWithAnswer(ids, null);
   }

   /*
    * @inherited
    */
   public native void killEventClientsWithAnswer(int[] ids, List answers) throws JGDIException;

   /*
    * @inherited
    */
   public void triggerSchedulerMonitoring() throws JGDIException {
       triggerSchedulerMonitoringWithAnswer(null);
   }
           
   /*
    * @inherited
    */
   public native void triggerSchedulerMonitoringWithAnswer(java.util.List answer) throws JGDIException;
   
   /*
    * @inherited
    */
   public void clearShareTreeUsage() throws JGDIException {
      clearShareTreeUsageWithAnswer(null);
   }

   /*
    * @inherited
    */
   public native void clearShareTreeUsageWithAnswer(java.util.List answers) throws JGDIException;
   
   
   /*
    * @inherited
    */
   public void killAllEventClients() throws JGDIException {
       killAllEventClientsWithAnswer(null);
   }

   /*
    * @inherited
    */
   public native void killAllEventClientsWithAnswer(List answers) throws JGDIException;
   
   /*
    * @inherited
    */
   public void killMaster() throws JGDIException {
      killMasterWithAnswer(null); 
   }
   
   /*
    * @inherited
    */
   public native void killMasterWithAnswer(List answers) throws JGDIException;
   
   /*
    * @inherited
    */
   public void killScheduler() throws JGDIException {
       killSchedulerWithAnswer(null);
   }
   
   /*
    * @inherited
    */
   public native void killSchedulerWithAnswer(List answers) throws JGDIException;
   
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
