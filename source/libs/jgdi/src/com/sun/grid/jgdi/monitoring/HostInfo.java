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
package com.sun.grid.jgdi.monitoring;
import java.io.Serializable;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Objects of this interface holds the monitoring information about
 * a cluster host.
 *
 */
public interface HostInfo {

   /**
    * Get a host value
    * @param name name of the host value
    * @return  the host value or <code>null</code>
    */
   public Object getHostValue(String name);

   /**
    *  Get the architecture of the host
    *  @return the architecture of the host
    */
   public String getArch();
   
   /**
    *  Get the number of processors of the host
    *  @return number of processors of the host
    */
   public String getNumberOfProcessors();
   
   /**
    *  Get the load average of the host
    *
    *  @return the load average of the host
    */
   public String getLoadAvg();
   
   /**
    *  Get the total memory of the host
    *
    *  @return total memory of the host
    */
   public String getMemTotal();

   /**
    *  Get the used memory of the host
    *
    *  @return the used memory
    */
   public String getMemUsed();
   
   /**
    *  Get the size of the swap of the host
    *
    *  @return size of the swap of the host
    */
   public String getSwapTotal();
   
   /**
    *  Get the size of used swap space
    *
    *  @return siez of the used swap space
    */
   public String getSwapUsed();
   
   /**
    *  Get the number of host values
    *
    *  @return number of host values
    */
   public int getHostValueCount();
   
   /**
    *  Get the set of host value names
    *
    *  @return set of host value names
    */
   public Set getHostValueKeys();
   
   /**
    * Get a resource value of the host
    * @param dominance  dominace of the resource value
    * @param name       name of the resource
    * @return  the resource value
    */
   public Object getResourceValue(String dominance, String name);
   
   /**
    * Get the set of available domincances
    * @return  the set of available domincances
    */
   public Set getDominanceSet();
   
   /**
    * Get the set of resource names for a dominance
    * @param dominance  the dominance
    * @return set of resource names
    */
   public Set getResourceValueNames(String dominance);
   
   
   /**
    * Get a queue value for the host
    * @param qname  name of the queue
    * @param name   name of the queue value
    * @return  the queue value or <code>null</code>
    */
   public Object getQueueValue(String qname, String name);
   
   
   /**
    *  Get the set of queue names
    *
    *  @return the set of queue names
    */
   public Set getQueueNames();
   
   /**
    *  Get the set of queue value names for a queue
    *  @param  qname of the queue
    *  @return the set of queue value names
    */
   public Set getQueueValueNames(String qname);
   
   /**
    *  Get the hostname
    *
    *  @return the hostname
    */
   public String getHostname();

   
   /**
    *  Get the list of jobs which are running on the host
    *
    *  <p><b>Note:</b> The job list is only set if the
    *  {@link QHostOptions#includeJobs} flag is set.</p>
    *
    *  @return list of jobs (instances of {@link JobInfo})
    *  @see com.sun.grid.jgdi.JGDI#execQHost
    */
   public List getJobList();

   /**
    *  Get the number of entries in the job list
    *
    *  @return the number of entries in the job list
    */
   public int getJobCount();
   
}
