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
 */
public class UserFilter implements Serializable {
   
   private List<String> userList = new ArrayList<String>();
   
   /** 
    * Creates a new instance of UserFilter 
    */
   public UserFilter() {
   }

   public static UserFilter parse(String userList) {
       UserFilter ret = new UserFilter();
       return ret.fill(userList);
   }
   
   public UserFilter fill(String list) {
       String[] elems = list.split(",");
       for (String elem : elems) {
           addUser(elem);
       }
       return this;
   }
   
   
   public void addUser(String username) {
      userList.add(username);
   }
   
   public List getUsers() {
      return Collections.unmodifiableList(userList);
   }

   public String toString() {
      StringBuilder ret = new StringBuilder();
      
      ret.append("UserFilter[");
      Iterator iter = userList.iterator();
      if(iter.hasNext()) {
         ret.append(iter.next());
         while(iter.hasNext()) {
            ret.append(", ");
            ret.append(iter.next());
         }
      }
      ret.append("]");
      return ret.toString();
   }
}
