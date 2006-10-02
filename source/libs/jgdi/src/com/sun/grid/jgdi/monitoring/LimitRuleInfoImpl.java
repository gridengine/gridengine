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

/**
 * Objects of this class holds the monitoring information about
 * a cluster limit rule.
 *
 */
public class LimitRuleInfoImpl implements LimitRuleInfo, Serializable {
   
   private String limitRuleName;
   private List users = new ArrayList();
   private List pes = new ArrayList();
   private List hosts = new ArrayList();
   private List queues = new ArrayList();
   private List projects = new ArrayList();
   private List xusers = new ArrayList();
   private List xpes = new ArrayList();
   private List xhosts = new ArrayList();
   private List xqueues = new ArrayList();
   private List xprojects = new ArrayList();

   private List resourceLimits = new ArrayList();
   
   /**
    * Create a new limit rule info object
    */
   public LimitRuleInfoImpl() {}
   
   /**
    * Create a new limit rule info object
    * @param limitRuleName  name of the limit rule
    */
   public LimitRuleInfoImpl(String limitRuleName) {
      setLimitRuleName(limitRuleName);
   }
   
   /**
    *  Get the limit rule name
    *
    *  @return the limit rule name
    */
   public String getLimitRuleName() {
      return this.limitRuleName;
   }
   
   /**
    *  Set the limit rule name
    *  @param limitRuleName the limit rule name
    */
   public void setLimitRuleName(String limitRuleName) {
      this.limitRuleName = limitRuleName;
   }
   
   
   /**
    *  Get users of limit rule
    *
    *  @return list of user names
    */
   public List getUsers() {
      return Collections.unmodifiableList(users);
   }
   
   /**
    *  Add user to limit rule users
    *
    *  @param user the user
    */
   public void addUser(String user) {
      users.add(user);
   }
   
   /**
    *  Get projects of limit rule
    *
    *  @return list of project names
    */
   public List getProjects() {
      return Collections.unmodifiableList(projects);
   }
   
   /**
    *  Add project to limit rule projects
    *
    *  @param project the project
    */
   public void addProject(String project) {
      projects.add(project);
   }
   
   
   /**
    *  Get PEs of limit rule
    *
    *  @return list of PE names
    */
   public List getPes() {
      return Collections.unmodifiableList(pes);
   }
   
   /**
    *  Add pe to limit rule pes
    *
    *  @param pe the pe
    */
   public void addPe(String pe) {
      pes.add(pe);
   }
   
   
   /**
    *  Get Queues of limit rule
    *
    *  @return list of queue names
    */
   public List getQueues() {
      return Collections.unmodifiableList(queues);
   }
   
   /**
    *  Add queue to limit rule queues
    *
    *  @param queue the queue
    */
   public void addQueue(String queue) {
      queues.add(queue);
   }
   
   
   /**
    *  Get hosts of limit rule
    *
    *  @return list of host names
    */
   public List getHosts() {
      return Collections.unmodifiableList(hosts);
   }
   
   /**
    *  Add host to limit rule hosts
    *
    *  @param host the host
    */
   public void addHost(String host) {
      hosts.add(host);
   }
   
   /**
    *  Get excluded users of limit rule
    *
    *  @return list of excluded user names
    */
 
   public List getXUsers() {
      return Collections.unmodifiableList(xusers);
   }
   
   /**
    *  Add user to limit rule excluded users
    *
    *  @param user the user
    */
   public void addXUser(String user) {
      xusers.add(user);
   }
   
   /**
    *  Get excluded projects of limit rule
    *
    *  @return list of excluded project names
    */
   public List getXProjects() {
      return Collections.unmodifiableList(xprojects);
   }
   
   /**
    *  Add project to limit rule excluded projects
    *
    *  @param project the project
    */
   public void addXProject(String project) {
      xprojects.add(project);
   }
   
   
   /**
    *  Get excluded PEs of limit rule
    *
    *  @return list of excluded PE names
    */
   public List getXPes() {
      return Collections.unmodifiableList(xpes);
   }
   
   /**
    *  Add pe to limit rule excluded pes
    *
    *  @param pe the pe
    */
   public void addXPe(String pe) {
      xpes.add(pe);
   }
   
   
   /**
    *  Get excluded Queues of limit rule
    *
    *  @return list of excluded queue names
    */
   public List getXQueues() {
      return Collections.unmodifiableList(xqueues);
   }
   
   /**
    *  Add queue to limit rule excluded queues
    *
    *  @param queue the queue
    */
   public void addXQueue(String queue) {
      xqueues.add(queue);
   }
   
   
   /**
    *  Get excluded hosts of limit rule
    *
    *  @return list of excluded host names
    */
   public List getXHosts() {
      return Collections.unmodifiableList(xhosts);
   }
   
   /**
    *  Add host to limit rule excluded hosts
    *
    *  @param host the host
    */
   public void addXHost(String host) {
      xhosts.add(host);
   }
   
   
   /**
    *  Get resource limits of limit rule
    * 
    * @return list of ResourceLimit objects
    */
   public List getLimits() {
      return Collections.unmodifiableList(resourceLimits);
   }
   
   /**
    *  Add resourceLimit to limit rule resourceLimits
    *
    *  @param resourceLimit the resourceLimit
    */
   public void addLimit(ResourceLimit resourceLimit) {
      resourceLimits.add(resourceLimit);
   }
   
   
}
