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
public class SynchronizedJGDIBase implements com.sun.grid.jgdi.JGDIBase {
    
    protected JGDI jgdi;
    
    protected SynchronizedJGDIBase(JGDI jgdi)
    throws JGDIException {
        jgdi = jgdi;
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

    public List getRealExecHostList() throws JGDIException {
        synchronized(jgdi) {
            return jgdi.getRealExecHostList();
        }
    }

    public List getClusterQueueSummary(ClusterQueueSummaryOptions options) throws JGDIException {
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

    public void cleanQueues(String[] queues) throws JGDIException {
        synchronized(jgdi) {
            jgdi.cleanQueues(queues);
        }
    }

    public void killMaster() throws JGDIException {
        synchronized(jgdi) {
            jgdi.killMaster();
        }
    }

    public void killScheduler() throws JGDIException {
        synchronized(jgdi) {
            jgdi.killScheduler();
        }
    }

    public void killExecd(String[] hosts, boolean terminateJobs) throws JGDIException {
        synchronized(jgdi) {
            jgdi.killExecd(hosts, terminateJobs);
        }
    }

    public void killAllExecds(boolean terminateJobs) throws JGDIException {
        synchronized(jgdi) {
            jgdi.killAllExecds(terminateJobs);
        }
    }

    public void killEventClients(int[] ids) throws JGDIException {
        synchronized(jgdi) {
            jgdi.killEventClients(ids);
        }
    }

    public void killAllEventClients() throws JGDIException {
        synchronized(jgdi) {
            jgdi.killAllEventClients();
        }
    }

    public void triggerSchedulerMonitoring() throws JGDIException {
        synchronized(jgdi) {
            jgdi.triggerSchedulerMonitoring();
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

    public void disableQueues(String[] queues, boolean force) throws JGDIException {
        synchronized(jgdi) {
            jgdi.disableQueues(queues, force);
        }
    }

    public void suspendQueues(String[] queues, boolean force) throws JGDIException {
        synchronized(jgdi) {
            jgdi.suspendQueues(queues, force);
        }
    }

    public void suspendJobs(String[] jobs, boolean force) throws JGDIException {
        synchronized(jgdi) {
            jgdi.suspendJobs(jobs, force);
        }
    }

    public void unsuspendQueues(String[] queues, boolean force) throws JGDIException {
        synchronized(jgdi) {
            jgdi.unsuspendQueues(queues, force);
        }
    }

    public void unsuspendJobs(String[] jobs, boolean force) throws JGDIException {
        synchronized(jgdi) {
            jgdi.unsuspendJobs(jobs, force);
        }
    }

    public void clearQueues(String[] queues, boolean force) throws JGDIException {
        synchronized(jgdi) {
            jgdi.clearQueues(queues, force);
        }
    }

    public void clearJobs(String[] jobs, boolean force) throws JGDIException {
        synchronized(jgdi) {
            jgdi.clearJobs(jobs, force);
        }
    }

    public void rescheduleQueues(String[] queues, boolean force) throws JGDIException {
        synchronized(jgdi) {
            jgdi.rescheduleQueues(queues, force);
        }
    }

    public void rescheduleJobs(String[] jobs, boolean force) throws JGDIException {
        synchronized(jgdi) {
            jgdi.rescheduleJobs(jobs, force);
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
}
