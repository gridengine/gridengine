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
import com.sun.grid.jgdi.configuration.ExecHost;
import com.sun.grid.jgdi.configuration.JGDIAnswer;
import com.sun.grid.jgdi.monitoring.ClusterQueueSummary;
import com.sun.grid.jgdi.monitoring.ClusterQueueSummaryOptions;
import com.sun.grid.jgdi.monitoring.QHostOptions;
import com.sun.grid.jgdi.monitoring.QHostResult;
import com.sun.grid.jgdi.monitoring.QQuotaOptions;
import com.sun.grid.jgdi.monitoring.QQuotaResult;
import com.sun.grid.jgdi.monitoring.QueueInstanceSummaryOptions;
import com.sun.grid.jgdi.monitoring.QueueInstanceSummaryResult;
import java.io.File;
import java.util.List;

/**
 * Base class for synchronized version of a <code>JGDI</code> connection.
 *
 * The end user should not use this class directory. Instead the factory
 * method @link{JGDIFactory#newSynchronizedInstance} should be used.
 *
 * @since  0.91
 * @see com.sun.grid.jgdi.JGDI
 * @see com.sun.grid.jgdi.JGDIFactory#newSynchronizedInstance
 */
public class SynchronizedJGDIBaseImpl implements com.sun.grid.jgdi.JGDIBase {
    
    protected JGDI jgdi;
    
    protected SynchronizedJGDIBaseImpl(JGDI jgdi) throws JGDIException {
        this.jgdi = jgdi;
    }
    
    public void close() throws JGDIException {
        synchronized(jgdi) {
            jgdi.close();
        }
    }
    
    
    public String getActQMaster() throws JGDIException {
        synchronized(jgdi) {
            return jgdi.getActQMaster();
        }
    }
    
    public String getAdminUser() throws JGDIException {
        synchronized(jgdi) {
            return jgdi.getAdminUser();
        }
    }
    
    public QHostResult execQHost(QHostOptions options) throws JGDIException {
        synchronized(jgdi) {
            return jgdi.execQHost(options);
        }
    }
    
    public File getSGERoot() throws JGDIException {
        synchronized(jgdi) {
            return jgdi.getSGERoot();
        }
    }
    
    public String getSGECell() throws JGDIException {
        synchronized(jgdi) {
            return jgdi.getSGECell();
        }
    }
    
    public List<ExecHost> getRealExecHostList() throws JGDIException {
        synchronized(jgdi) {
            return jgdi.getRealExecHostList();
        }
    }
    
    public List<ClusterQueueSummary> getClusterQueueSummary(ClusterQueueSummaryOptions options) throws JGDIException {
        synchronized(jgdi) {
            return jgdi.getClusterQueueSummary(options);
        }
    }
    
    public QueueInstanceSummaryResult getQueueInstanceSummary(QueueInstanceSummaryOptions options) throws JGDIException {
        synchronized(jgdi) {
            return jgdi.getQueueInstanceSummary(options);
        }
    }
    
    public QQuotaResult getQQuota(QQuotaOptions options) throws JGDIException {
        synchronized(jgdi) {
            return jgdi.getQQuota(options);
        }
    }
    
    public void clearShareTreeUsage() throws JGDIException {
        synchronized(jgdi) {
            jgdi.clearShareTreeUsage();
        }
    }
    
    public void clearShareTreeUsageWithAnswer(List<JGDIAnswer> answers) throws JGDIException {
        synchronized(jgdi) {
            jgdi.clearShareTreeUsageWithAnswer(answers);
        }
    }
    
    public void cleanQueues(String[] queues) throws JGDIException {
        synchronized(jgdi) {
            jgdi.cleanQueues(queues);
        }
    }
    
    public void cleanQueuesWithAnswer(String[] queues, List<JGDIAnswer> answers) throws JGDIException {
        synchronized(jgdi) {
            jgdi.cleanQueuesWithAnswer(queues, answers);
        }
    }
    
    public void killMaster() throws JGDIException {
        synchronized(jgdi) {
            jgdi.killMaster();
        }
    }
    
    public void killMasterWithAnswer(List<JGDIAnswer> answers) throws JGDIException {
        synchronized(jgdi) {
            jgdi.killMasterWithAnswer(answers);
        }
    }
    
    
    public void killScheduler() throws JGDIException {
        synchronized(jgdi) {
            jgdi.killScheduler();
        }
    }
    
    public void killSchedulerWithAnswer(List<JGDIAnswer> answers) throws JGDIException {
        synchronized(jgdi) {
            jgdi.killSchedulerWithAnswer(answers);
        }
    }
    
    public void killExecd(String[] hosts, boolean terminateJobs) throws JGDIException {
        synchronized(jgdi) {
            jgdi.killExecd(hosts, terminateJobs);
        }
    }
    
    public void killExecdWithAnswer(String[] hosts, boolean terminateJobs, List<JGDIAnswer> answers) throws JGDIException {
        synchronized(jgdi) {
            jgdi.killExecdWithAnswer(hosts, terminateJobs, answers);
        }
    }
    
    public void killAllExecds(boolean terminateJobs) throws JGDIException {
        synchronized(jgdi) {
            jgdi.killAllExecds(terminateJobs);
        }
    }
    
    public void killAllExecdsWithAnswer(boolean terminateJobs, List<JGDIAnswer> answers) throws JGDIException {
        synchronized(jgdi) {
            jgdi.killAllExecdsWithAnswer(terminateJobs, answers);
        }
    }
    
    public void killEventClients(int[] ids) throws JGDIException {
        synchronized(jgdi) {
            jgdi.killEventClients(ids);
        }
    }
    
    public void killEventClientsWithAnswer(int[] ids, List<JGDIAnswer> answers) throws JGDIException {
        synchronized(jgdi) {
            jgdi.killEventClientsWithAnswer(ids, answers);
        }
    }
    
    public void killAllEventClients() throws JGDIException {
        synchronized(jgdi) {
            jgdi.killAllEventClients();
        }
    }
    
    public void killAllEventClientsWithAnswer(List<JGDIAnswer> answers) throws JGDIException {
        synchronized(jgdi) {
            jgdi.killAllEventClientsWithAnswer(answers);
        }
    }
    
    public void triggerSchedulerMonitoring() throws JGDIException {
        synchronized(jgdi) {
            jgdi.triggerSchedulerMonitoring();
        }
    }
    
    public void triggerSchedulerMonitoringWithAnswer(List<JGDIAnswer> answers) throws JGDIException {
        synchronized(jgdi) {
            jgdi.triggerSchedulerMonitoringWithAnswer(answers);
        }
    }
    
    public String getSchedulerHost() throws JGDIException {
        synchronized(jgdi) {
            return jgdi.getSchedulerHost();
        }
    }
    
    public void enableQueues(String[] queues, boolean force) throws JGDIException {
        synchronized(jgdi) {
            jgdi.enableQueues(queues, force);
        }
    }
    
    public void enableQueuesWithAnswer(String[] queues, boolean force, List<JGDIAnswer> answers) throws JGDIException {
        synchronized(jgdi) {
            jgdi.enableQueuesWithAnswer(queues, force, answers);
        }
    }
    
    public void disableQueues(String[] queues, boolean force) throws JGDIException {
        synchronized(jgdi) {
            jgdi.disableQueues(queues, force);
        }
    }
    
    public void disableQueuesWithAnswer(String[] queues, boolean force, List<JGDIAnswer> answers) throws JGDIException {
        synchronized(jgdi) {
            jgdi.disableQueuesWithAnswer(queues, force, answers);
        }
    }
    
    public void suspend(String[] queues, boolean force) throws JGDIException {
        synchronized(jgdi) {
            jgdi.suspend(queues, force);
        }
    }
    
    public void suspendWithAnswer(String[] queues, boolean force, List<JGDIAnswer> answers) throws JGDIException {
        synchronized(jgdi) {
            jgdi.suspendWithAnswer(queues, force, answers);
        }
    }
    
    public void suspendQueues(String[] queues, boolean force) throws JGDIException {
        synchronized(jgdi) {
            jgdi.suspendQueues(queues, force);
        }
    }
    
    public void suspendQueuesWithAnswer(String[] queues, boolean force, List<JGDIAnswer> answers) throws JGDIException {
        synchronized(jgdi) {
            jgdi.suspendQueuesWithAnswer(queues, force, answers);
        }
    }
    
    public void suspendJobs(String[] jobs, boolean force) throws JGDIException {
        synchronized(jgdi) {
            jgdi.suspendJobs(jobs, force);
        }
    }
    
    public void suspendJobsWithAnswer(String[] jobs, boolean force, List<JGDIAnswer> answers) throws JGDIException {
        synchronized(jgdi) {
            jgdi.suspendJobsWithAnswer(jobs, force, answers);
        }
    }
    
    public void unsuspend(String[] queues, boolean force) throws JGDIException {
        synchronized(jgdi) {
            jgdi.unsuspend(queues, force);
        }
    }
    
    public void unsuspendWithAnswer(String[] queues, boolean force, List<JGDIAnswer> answers) throws JGDIException {
        synchronized(jgdi) {
            jgdi.unsuspendWithAnswer(queues, force, answers);
        }
    }
    
    public void unsuspendQueues(String[] queues, boolean force) throws JGDIException {
        synchronized(jgdi) {
            jgdi.unsuspendQueues(queues, force);
        }
    }
    
    public void unsuspendQueuesWithAnswer(String[] queues, boolean force, List<JGDIAnswer> answers) throws JGDIException {
        synchronized(jgdi) {
            jgdi.unsuspendQueuesWithAnswer(queues, force, answers);
        }
    }
    
    public void unsuspendJobs(String[] jobs, boolean force) throws JGDIException {
        synchronized(jgdi) {
            jgdi.unsuspendJobs(jobs, force);
        }
    }
    
    public void unsuspendJobsWithAnswer(String[] jobs, boolean force, List<JGDIAnswer> answers) throws JGDIException {
        synchronized(jgdi) {
            jgdi.unsuspendJobsWithAnswer(jobs, force, answers);
        }
    }
    
    public void clearQueues(String[] queues, boolean force) throws JGDIException {
        synchronized(jgdi) {
            jgdi.clearQueues(queues, force);
        }
    }
    
    public void clearQueuesWithAnswer(String[] queues, boolean force, List<JGDIAnswer> answers) throws JGDIException {
        synchronized(jgdi) {
            jgdi.clearQueuesWithAnswer(queues, force, answers);
        }
    }
    
    
    public void clearJobs(String[] jobs, boolean force) throws JGDIException {
        synchronized(jgdi) {
            jgdi.clearJobs(jobs, force);
        }
    }
    
    public void clearJobsWithAnswer(String[] jobs, boolean force, List<JGDIAnswer> answer) throws JGDIException {
        synchronized(jgdi) {
            jgdi.clearJobsWithAnswer(jobs, force, answer);
        }
    }
    
    public void reschedule(String[] queue_or_job, boolean force) throws JGDIException {
        synchronized(jgdi) {
            jgdi.reschedule(queue_or_job, force);
        }
    }
    
    public void rescheduleWithAnswer(String[] queue_or_job, boolean force, List<JGDIAnswer> answers) throws JGDIException {
        synchronized(jgdi) {
            jgdi.rescheduleWithAnswer(queue_or_job, force, answers);
        }
    }
    
    public void rescheduleQueues(String[] queues, boolean force) throws JGDIException {
        synchronized(jgdi) {
            jgdi.rescheduleQueues(queues, force);
        }
    }
    
    public void rescheduleQueuesWithAnswer(String[] queues, boolean force, List<JGDIAnswer> answers) throws JGDIException {
        synchronized(jgdi) {
            jgdi.rescheduleQueuesWithAnswer(queues, force, answers);
        }
    }
    
    public void rescheduleJobs(String[] jobs, boolean force) throws JGDIException {
        synchronized(jgdi) {
            jgdi.rescheduleJobs(jobs, force);
        }
    }
    
    public void rescheduleJobsWithAnswer(String[] jobs, boolean force, List<JGDIAnswer> answers) throws JGDIException {
        synchronized(jgdi) {
            jgdi.rescheduleJobsWithAnswer(jobs, force, answers);
        }
    }
    
    
    public String showDetachedSettings(String[] queues) throws JGDIException {
        synchronized(jgdi) {
            return jgdi.showDetachedSettings(queues);
        }
    }
    
    public String showDetachedSettingsAll() throws JGDIException {
        synchronized(jgdi) {
            return jgdi.showDetachedSettingsAll();
        }
    }
    
    public void deleteShareTree() throws JGDIException {
        synchronized(jgdi) {
            jgdi.deleteShareTree();
        }
    }
    
    public void deleteShareTreeWithAnswer(List<JGDIAnswer> answers) throws JGDIException {
        synchronized(jgdi) {
            jgdi.deleteShareTreeWithAnswer(answers);
        }
    }
    
}
