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
package com.sun.grid.jgdi.monitoring.filter;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.StringTokenizer;


/**
 *
 * @todo    beta 3MT
 *          <p>ProjectFilter not yet implemented</p>
 */
public class ProjectFilter implements Serializable {
   
   private List prjList = new ArrayList();
   
   /** Creates a new instance of ParallelEnvironmentFilter */
   public ProjectFilter() {
   }

   public static ProjectFilter parse(String projectList) {
       ProjectFilter ret = new ProjectFilter();
       StringTokenizer st = new StringTokenizer(projectList, ",");
       while(st.hasMoreTokens()) {
           ret.addProject(st.nextToken());
       }
       return ret;
   }
   
   public void addProject(String prjName) {
      prjList.add(prjName);
   }
   
   public List getProjectList() {
      return Collections.unmodifiableList(prjList);
   }
   
   public int getProjectCount() {
      return prjList.size();
   }
   
   public String toString() {
      StringBuffer ret = new StringBuffer();
      
      ret.append("ProjectFilter[");
      Iterator iter = prjList.iterator();
      if(iter.hasNext()) {
         ret.append(iter.next());
         while(iter.hasNext()) {
            ret.append(",");
            ret.append(iter.next());
         }
      }
      return ret.toString();
   }
}
