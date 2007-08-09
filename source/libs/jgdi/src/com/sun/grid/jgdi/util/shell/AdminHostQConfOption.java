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
package com.sun.grid.jgdi.util.shell;

import com.sun.grid.jgdi.JGDI;
import com.sun.grid.jgdi.JGDIException;
import com.sun.grid.jgdi.configuration.AdminHost;
import com.sun.grid.jgdi.configuration.AdminHostImpl;
import java.io.PrintWriter;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;

/**
 * AdminHostQConfOption class
 * Special handling methods for {@link AdminHost}
 * @see {@link QConfOption}
 */
public class AdminHostQConfOption extends QConfOption {
   
   //-sh
   void showList(final JGDI jgdi, final List args, final PrintWriter pw) {
      try {
         printListSortedByName(jgdi.getAdminHostList(), args, pw);
      } catch (JGDIException ex) {
         pw.println(ex.getMessage());
      } finally {
         pw.flush();
      }
   }
   
   //-ah
   void add(final JGDI jgdi, final List args, final PrintWriter pw) {
      if (args.size() == 0) {
         pw.println("error: no option argument provided to \"-ah\"");
         //TODO LP: clients show here getUsage()
         pw.println("Usage: qconf -help");
         return;
      }
      String userName = (String) args.get(0);
      List names = args;
      List notAdded = new ArrayList();
      List list;
      try {
         list = jgdi.getAdminHostList();
      } catch (JGDIException ex) {
         pw.println(ex.getMessage());
         pw.flush();
         return;
      }
      String name;
      boolean found;
      for (Iterator nameIter = names.iterator(); nameIter.hasNext(); ) {
         name = (String) nameIter.next();
         found = false;
         for (Iterator iter = list.iterator(); iter.hasNext(); ) {
            AdminHost host = (AdminHost) iter.next();
            if (host.getName().equals(name)) {
               pw.println("administrative host \""+name+"\" already exists");
               pw.flush();
               found = true;
               break;
            }
         }
         if (!found) {
            notAdded.add(name);
         }
      }
      for (Iterator iter = notAdded.iterator(); iter.hasNext(); ) {
         AdminHost host = new AdminHostImpl(true);
         name = (String) iter.next();
         host.setName(name);
         try {
            jgdi.addAdminHost(host);
            pw.println(name+"  added to administrative host list");
         } catch (JGDIException ex) {
            pw.println(ex.getMessage());
         } finally {
            pw.flush();
         }
      }
   }
   
   //-dh
   void delete(final JGDI jgdi, final List args, final PrintWriter pw) {
      if (args.size() == 0) {
         pw.println("error: no option argument provided to \"-dh\"");
         //TODO LP: clients show here getUsage()
         pw.println("Usage: qconf -help");
         return;
      }
      
      List names = args;
      List deleteList = new ArrayList();
      List list;
      try {
         list = jgdi.getAdminHostList();
      } catch (JGDIException ex) {
         pw.println(ex.getMessage());
         pw.flush();
         return;
      }
      AdminHost obj;
      String name;
      boolean found;
      for (Iterator nameIter = names.iterator(); nameIter.hasNext(); ) {
         name = (String) nameIter.next();
         found = false;
         for (Iterator iter = list.iterator(); iter.hasNext(); ) {
            AdminHost host = (AdminHost) iter.next();
            if (host.getName().equals(name)) {
               found = true;
               deleteList.add(host);
               break;
            }
         }
         if (!found) {
            pw.println("denied: administrative host \""+name+"\" does not exist");
            pw.flush();
         }
      }
      for (Iterator iter = deleteList.iterator(); iter.hasNext(); ) {
         obj = (AdminHost)iter.next();
         try {
            jgdi.deleteAdminHost(obj);
            pw.println(jgdi.getAdminUser()+"@"+java.net.InetAddress.getLocalHost().getHostName()+
                  " removed \""+obj.getName()+"\" from administrative host list");
         } catch (JGDIException ex) {
            pw.print(ex.getMessage());
         } catch (UnknownHostException ex) {
            ex.printStackTrace();
         } finally {
            pw.flush();
         }
      }
   }
}
