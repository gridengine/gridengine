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
package com.sun.grid.jgdi.rmi;

import com.sun.grid.jgdi.JGDI;
import com.sun.grid.jgdi.JGDIException;
import com.sun.grid.jgdi.JGDIFactory;
import com.sun.grid.jgdi.monitoring.ClusterQueueSummaryOptions;
import com.sun.grid.jgdi.monitoring.QHostOptions;
import com.sun.grid.jgdi.monitoring.QHostResult;
import com.sun.grid.jgdi.monitoring.QQuotaOptions;
import com.sun.grid.jgdi.monitoring.QueueInstanceSummaryOptions;
import com.sun.grid.jgdi.monitoring.QueueInstanceSummaryResult;
import java.io.File;
import java.rmi.RemoteException;
import java.rmi.server.UnicastRemoteObject;
import java.util.List;
import java.util.logging.Logger;


/**
 *
 * @author  richard.hierlmeier@sun.com
 */
public class JGDIRemoteBaseImpl extends UnicastRemoteObject implements JGDIRemoteBase {
    
    protected JGDI jgdi;
    protected Logger logger = Logger.getLogger("com.sun.grid.jgdi.rmi");
    
    protected JGDIRemoteBaseImpl(String url)
    throws RemoteException, JGDIException {
        jgdi = JGDIFactory.newInstance(url);
    }
    
    public void close() throws RemoteException {
        logger.entering("JGDIRemoteBaseImpl","close");
        JGDI jgdi = this.jgdi;
        this.jgdi = null;
        if( jgdi != null ) {
            try {
                jgdi.close();
            } catch( JGDIException e ) {
                throw new RemoteException(e.getMessage(), e);
            }
        }
        logger.exiting("JGDIRemoteBaseImpl","close");
    }
    
    
    public String getActQMaster() throws java.rmi.RemoteException {
        if( jgdi == null ) {
            return null;
        } else {
            try {
                return jgdi.getActQMaster();
            } catch( JGDIException e ) {
                throw new RemoteException("Can not get act qmaster", e );
            }
        }
    }
    
    public String getAdminUser() throws RemoteException {
        if( jgdi == null ) {
            return null;
        } else {
            try {
                return jgdi.getAdminUser();
            } catch( JGDIException e ) {
                throw new RemoteException("Can not get admin user", e );
            }
        }
    }
    
    public QHostResult execQHost(QHostOptions options) throws RemoteException {
        if( jgdi == null ) {
            return null;
        } else {
            try {
                return jgdi.execQHost(options);
            } catch( JGDIException e ) {
                logger.throwing(getClass().getName(), "execQHost", e);
                throw new RemoteException("Error while executing qhost", e );
            }
        }
    }

    public File getSGERoot() throws RemoteException {
        return null;
    }

    public String getSGECell() throws RemoteException {
        return null;
    }

    public List getRealExecHostList() throws RemoteException {
        return null;
    }

    public List getClusterQueueSummary(ClusterQueueSummaryOptions options) throws RemoteException {
        return null;
    }

    public QueueInstanceSummaryResult getQueueInstanceSummary(QueueInstanceSummaryOptions options) throws RemoteException {
        return null;
    }

    public List getQQuota(QQuotaOptions options) throws RemoteException {
        return null;
    }

    public void clearShareTreeUsage() throws RemoteException {
    }

    public void cleanQueues(String[] queues) throws RemoteException {
    }

    public void killMaster() throws RemoteException {
    }

    public void killScheduler() throws RemoteException {
    }

    public void killExecd(String[] hosts, boolean terminateJobs) throws RemoteException {
    }

    public void killAllExecds(boolean terminateJobs) throws RemoteException {
    }

    public List getEventClients() throws RemoteException {
        return null;
    }

    public void killEventClients(int[] ids) throws RemoteException {
    }

    public void killAllEventClients() throws RemoteException {
    }

    public void triggerSchedulerMonitoring() throws RemoteException {
    }

    public String getSchedulerHost() throws RemoteException {
        return null;
    }

    public void enableQueues(String[] queues, boolean force) throws RemoteException {
    }

    public void disableQueues(String[] queues, boolean force) throws RemoteException {
    }

    public void suspendQueues(String[] queues, boolean force) throws RemoteException {
    }

    public void suspendJobs(String[] jobs, boolean force) throws RemoteException {
    }

    public void unsuspendQueues(String[] queues, boolean force) throws RemoteException {
    }

    public void unsuspendJobs(String[] jobs, boolean force) throws RemoteException {
    }

    public void clearQueues(String[] queues, boolean force) throws RemoteException {
    }

    public void clearJobs(String[] jobs, boolean force) throws RemoteException {
    }

    public void rescheduleQueues(String[] queues, boolean force) throws RemoteException {
    }

    public void rescheduleJobs(String[] jobs, boolean force) throws RemoteException {
    }
    
    
    
}
