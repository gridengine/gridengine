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

package com.sun.grid.jgdi.management.mbeans;

import com.sun.grid.jgdi.JGDI;
import com.sun.grid.jgdi.JGDIException;
import com.sun.grid.jgdi.JGDIFactory;
import com.sun.grid.jgdi.event.EventTypeEnum;
import com.sun.grid.jgdi.management.NotificationBridge;
import com.sun.grid.jgdi.management.NotificationBridgeFactory;
import com.sun.grid.jgdi.monitoring.ClusterQueueSummaryOptions;
import com.sun.grid.jgdi.monitoring.QHostOptions;
import com.sun.grid.jgdi.monitoring.QHostResult;
import com.sun.grid.jgdi.monitoring.QQuotaOptions;
import com.sun.grid.jgdi.monitoring.QQuotaResult;
import com.sun.grid.jgdi.monitoring.QueueInstanceSummaryOptions;
import com.sun.grid.jgdi.monitoring.QueueInstanceSummaryResult;
import java.io.File;
import java.lang.reflect.UndeclaredThrowableException;
import java.util.Collections;
import java.util.List;
import java.util.Set;
import java.util.logging.Logger;
import javax.management.*;

/**
 * Class JGDIJMXBase
 * JGDIJMXBase MBean
 */
public class JGDIJMXBase implements java.io.Serializable, JGDIJMXBaseMBean, NotificationEmitter, MBeanRegistration {
    
    protected final Logger logger = Logger.getLogger("com.sun.grid.jgdi.management.mbeans");
    protected JGDI jgdi;
    private final String url;
    private final NotificationBridge notificationBridge;
    
    /* Creates a new instance of JGDIJMXBase */
    public JGDIJMXBase(String url) throws JGDIException {
        this.url = url;
        notificationBridge = NotificationBridgeFactory.newInstance(url);
    }
    
    public String getCurrentJGDIVersion() throws JGDIException {
        return JGDIFactory.getJGDIVersion();
    }
    
    public void subscribe(Set<EventTypeEnum> subscription) throws JGDIException {
        logger.entering("JGDIJMXBase", "subscribe");
        notificationBridge.subscribe(subscription);
        logger.exiting("JGDIJMXBase", "subscribe");
    }
    
    public void unsubscribe(Set<EventTypeEnum> subscription) throws JGDIException {
        logger.entering("JGDIJMXBase", "unsubscribe");
        notificationBridge.unsubscribe(subscription);
        logger.exiting("JGDIJMXBase", "unsubscribe");
    }
    
    public Set<EventTypeEnum> getSubscription() {
        return notificationBridge.getSubscription();
    }
    
    public void setSubscription(Set<EventTypeEnum> subscription) {
        try {
            notificationBridge.setSubscription(subscription);
        } catch (JGDIException ex) {
            throw new IllegalStateException(ex.getLocalizedMessage(), ex);
        }
    }
    
    public void removeNotificationListener(NotificationListener listener, NotificationFilter filter, Object handback) throws ListenerNotFoundException {
        try {
            notificationBridge.removeNotificationListener(listener);
        } catch (JGDIException ex) {
            throw new UndeclaredThrowableException(ex);
        }
    }
    
    public void addNotificationListener(NotificationListener listener, NotificationFilter filter, Object handback) throws IllegalArgumentException {
        try {
            notificationBridge.addNotificationListener(listener, filter, handback);
        } catch (JGDIException ex) {
            throw new UndeclaredThrowableException(ex);
        }
    }
    
    public void removeNotificationListener(NotificationListener listener) throws ListenerNotFoundException {
        try {
            notificationBridge.removeNotificationListener(listener);
        } catch (JGDIException ex) {
            throw new UndeclaredThrowableException(ex);
        }
    }
    
    
    public MBeanNotificationInfo[] getNotificationInfo() {
        return notificationBridge.getMBeanNotificationInfo();
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
    
    // MBeanRegistration interface ----------------------------------------------
    public ObjectName preRegister(MBeanServer server, ObjectName name) throws Exception {
        logger.entering("JGDIJMXBase", "preRegister");
        // jgdi = JGDIFactory.newSynchronizedInstance(url);
        jgdi = JGDIFactory.newInstance(url);
        logger.exiting("JGDIJMXBase", "preRegister");
        return name;
    }
    
    public void postRegister(Boolean registrationDone) {
        // Ignore
    }
    
    public void preDeregister() throws Exception {
        logger.entering("JGDIJMXBase", "preDeregister");
        notificationBridge.close();
        logger.exiting("JGDIJMXBase", "preDeregister");
    }
    
    public void postDeregister() {
        // Ignore
    }
}
