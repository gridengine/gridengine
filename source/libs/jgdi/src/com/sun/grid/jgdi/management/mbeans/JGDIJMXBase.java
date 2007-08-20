/*___INFO__MARK_BEGIN__*/ /*************************************************************************
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

package com.sun.grid.jgdi.management.mbeans;

import com.sun.grid.jgdi.JGDI;
import com.sun.grid.jgdi.EventClient;
import com.sun.grid.jgdi.JGDIException;
import com.sun.grid.jgdi.JGDIFactory;
import com.sun.grid.jgdi.event.Event;
import com.sun.grid.jgdi.event.EventListener;
import com.sun.grid.jgdi.monitoring.ClusterQueueSummaryOptions;
import com.sun.grid.jgdi.monitoring.QHostOptions;
import com.sun.grid.jgdi.monitoring.QHostResult;
import com.sun.grid.jgdi.monitoring.QQuotaOptions;
import com.sun.grid.jgdi.monitoring.QQuotaResult;
import com.sun.grid.jgdi.monitoring.QueueInstanceSummaryOptions;
import com.sun.grid.jgdi.monitoring.QueueInstanceSummaryResult;
import java.io.File;
import java.util.List;
import java.util.logging.Logger;
import javax.management.*;

/**
 * Class JGDIJMXBase
 * JGDIJMXBase MBean
 */
public class JGDIJMXBase implements java.io.Serializable, JGDIJMXBaseMBean, NotificationEmitter, EventListener {
   
   protected Logger logger = Logger.getLogger("com.sun.grid.jgdi.management.mbeans");
   protected JGDI jgdi;
   private EventClient evc;
   private String url;
   
   /* Creates a new instance of JGDIJMXBase */
   public JGDIJMXBase(String url) throws JGDIException {
      // jgdi = JGDIFactory.newSynchronizedInstance(url);
      jgdi = JGDIFactory.newInstance(url);
      this.url = url;
   }
   
   public String getCurrentJGDIVersion() throws JGDIException {
      String version = "-NA-";
      version = JGDIFactory.getJGDIVersion();
      return version;
   }
   
   
   // JGDI Event Client -> MBean Notification 
   public synchronized void startEventClient() throws JGDIException {
      try {
         if (evc == null) {
            evc = JGDIFactory.createEventClient(url, 0);
            evc.addEventListener(this);
         }
//        evc.subscribeAll();
         evc.start();
      } catch (InterruptedException ex) {
         throw new JGDIException(ex.getMessage());
      }
   }

   public synchronized void stopEventClient() throws JGDIException {
      try {
         evc.close();
         evc = null;
      } catch (InterruptedException ex) {
         throw new JGDIException(ex.getMessage());
      }
   }
   
   public void unsubscribeAll() throws JGDIException {
      evc.unsubscribeAll();
   }
   
   public void subscribeAll() throws JGDIException {
      evc.subscribeAll();
   }
   
   public void subscribeAddJob() throws JGDIException {
      evc.subscribeAddJob(true);
   }
   
   public void unsubscribeAddJob() throws JGDIException {
      evc.subscribeAddJob(false);
   }
   
   public void removeNotificationListener(NotificationListener listener, NotificationFilter filter, Object handback) throws ListenerNotFoundException {
      broadcaster.removeNotificationListener(listener, filter, handback);
   }
   
   public void addNotificationListener(NotificationListener listener, NotificationFilter filter, Object handback) throws IllegalArgumentException {
      broadcaster.addNotificationListener(listener, filter, handback);
   }
   
   public void removeNotificationListener(NotificationListener listener) throws ListenerNotFoundException {
      broadcaster.removeNotificationListener(listener);
   }
   
   public MBeanNotificationInfo[] getNotificationInfo() {
      return broadcaster.getNotificationInfo();
   }
   
   private final NotificationBroadcasterSupport broadcaster = new NotificationBroadcasterSupport();
   
   public void eventOccured(Event evt) {
      System.out.println(evt);
      System.out.flush();
      Notification notification = new Notification("gridengine: " + evt.getType() + "/" + evt.getClass(), evt, 0);
      broadcaster.sendNotification(notification);
   }


   // JGDI Base methods
   
   public String getAdminUser() throws JGDIException {
      return jgdi.getAdminUser();
   }

   public File getSGERoot() throws JGDIException {
      return jgdi.getSGERoot();
   }

   public String getSGECell() throws JGDIException {
      return jgdi.getSGECell();
   }

   public String getActQMaster() throws JGDIException {
      return jgdi.getActQMaster();
   }

   public List getRealExecHostList() throws JGDIException {
      return jgdi.getRealExecHostList();
   }

   public QHostResult execQHost(QHostOptions options) throws JGDIException {
      return jgdi.execQHost(options);
   }

   public List getClusterQueueSummary(ClusterQueueSummaryOptions options) throws JGDIException {
      return jgdi.getClusterQueueSummary(options);
   }

   public QueueInstanceSummaryResult getQueueInstanceSummary(QueueInstanceSummaryOptions options) throws JGDIException {
      return jgdi.getQueueInstanceSummary(options);
   }

   public QQuotaResult getQQuota(QQuotaOptions options) throws JGDIException {
      return jgdi.getQQuota(options);
   }

   public void clearShareTreeUsage() throws JGDIException {
      jgdi.clearShareTreeUsage();
   }

   public void cleanQueues(String[] queues) throws JGDIException {
      jgdi.cleanQueues(queues);
   }

   public void killMaster() throws JGDIException {
      jgdi.killMaster();
   }

   public void killScheduler() throws JGDIException {
      jgdi.killScheduler();
   }

   public void killExecd(String[] hosts, boolean terminateJobs) throws JGDIException {
      jgdi.killExecd(hosts, terminateJobs);
   }

   public void killAllExecds(boolean terminateJobs) throws JGDIException {
      jgdi.killAllExecds(terminateJobs);
   }

   public void killEventClients(int[] ids) throws JGDIException {
      jgdi.killEventClients(ids);
   }

   public void killAllEventClients() throws JGDIException {
      jgdi.killAllEventClients();
   }

   public void triggerSchedulerMonitoring() throws JGDIException {
      jgdi.triggerSchedulerMonitoring();
   }

   public String getSchedulerHost() throws JGDIException {
      return jgdi.getSchedulerHost();
   }

   public void enableQueues(String[] queues, boolean force) throws JGDIException {
      jgdi.enableQueues(queues, force);
   }

   public void disableQueues(String[] queues, boolean force) throws JGDIException {
      jgdi.disableQueues(queues, force);
   }

   public void suspendQueues(String[] queues, boolean force) throws JGDIException {
      jgdi.suspendQueues(queues, force);
   }

   public void suspendJobs(String[] jobs, boolean force) throws JGDIException {
      jgdi.suspendJobs(jobs, force);
   }

   public void unsuspendQueues(String[] queues, boolean force) throws JGDIException {
      jgdi.unsuspendQueues(queues, force);
   }

   public void unsuspendJobs(String[] jobs, boolean force) throws JGDIException {
      jgdi.unsuspendJobs(jobs, force);
   }

   public void clearQueues(String[] queues, boolean force) throws JGDIException {
      jgdi.clearQueues(queues, force);
   }

   public void clearJobs(String[] jobs, boolean force) throws JGDIException {
      jgdi.clearJobs(jobs, force);
   }

   public void rescheduleQueues(String[] queues, boolean force) throws JGDIException {
      jgdi.rescheduleQueues(queues, force);
   }

   public void rescheduleJobs(String[] jobs, boolean force) throws JGDIException {
      jgdi.rescheduleJobs(jobs, force);
   }

   public String showDetachedSettings(String[] queues) throws JGDIException {
      return jgdi.showDetachedSettings(queues);
   }

   public String showDetachedSettingsAll() throws JGDIException {
      return jgdi.showDetachedSettingsAll();
   }

   public QHostOptions newQHostOptions() throws JGDIException {
      return new QHostOptions();
   }

   public ClusterQueueSummaryOptions newClusterQueueSummaryOptions() throws JGDIException {
      return new ClusterQueueSummaryOptions();
   }

   public QueueInstanceSummaryOptions newQueueInstanceSummaryOptions() throws JGDIException {
      return new QueueInstanceSummaryOptions();
   }

   public QQuotaOptions newQQuotaOptions() throws JGDIException {
      return new QQuotaOptions();
   }
}

