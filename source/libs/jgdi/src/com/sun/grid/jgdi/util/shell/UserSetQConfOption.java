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
import com.sun.grid.jgdi.configuration.UserSet;
import com.sun.grid.jgdi.configuration.UserSetImpl;
import java.io.PrintWriter;
import java.util.Iterator;
import java.util.List;

/**
 * UserSetQConfOption class
 * Special handling methods for {@link UserSet}
 * @see {@link QConfOption}
 */
public class UserSetQConfOption extends QConfOption {
   
   //-sul
   void showList(final JGDI jgdi, final List args, final PrintWriter pw) {
      try {
         printListSortedByName(jgdi.getUserSetList(), args, pw);
      } catch (JGDIException ex) {
         pw.println(ex.getMessage());
      } finally {
         pw.flush();
      }
   }
   
   //-au
   void add(final JGDI jgdi, final List args, final PrintWriter pw) {
      if (args.size() == 0) {
         pw.println("error: no option argument provided to \"-au\"");
         //TODO LP: clients show here getUsage()
         pw.println("Usage: qconf -help");
         return;
      }
      String userName = (String) args.get(0);
      if (args.size() == 1) {
         pw.println("error: no list_name provided to \"-au" + userName + "\"");
         //TODO LP: clients show here getUsage()
         pw.println("Usage: qconf -help");
         pw.flush();
         return;
      }
      String setName = (String) args.get(1);
      
      UserSet us = null;
      boolean create = false;
      try {
         us = jgdi.getUserSet(setName);
         if (us == null) {
            create = true;
            us = new UserSetImpl(true);
            us.setName(setName);
         }
         boolean entryExists = false;
         for (Iterator iter = us.getEntriesList().iterator(); iter.hasNext(); ) {
            String entry = (String) iter.next();
            if (entry.equals(userName)) {
               entryExists = true;
               break;
            }
         }
         if (entryExists) {
            pw.println("\""+userName+"\" is already in access list \""+setName+"\"");
         } else {
            us.addEntries(userName);
            if (create) {
               jgdi.addUserSet(us);
            } else {
               jgdi.updateUserSet(us);
            }
            pw.println("added \"" + userName + "\" to access list \"" + setName + "\"");
         }
      /*if (args.length > ++i) {
         pw.println("error: invalid option argument \"" + args[i] + "\"");
         pw.println("Usage: qconf -help");
      }*/
      } catch (JGDIException ex) {
         pw.println(ex.getMessage());
      } finally {
         pw.flush();
      }
   }
   
   //-du
   void deleteUser(final JGDI jgdi, final List args, final PrintWriter pw) {
      if (args.size() == 0) {
         pw.println("error: no option argument provided to \"-au\"");
         //TODO LP: clients show here getUsage()
         pw.println("Usage: qconf -help");
         return;
      }
      String userName = (String) args.get(0);
      if (args.size() == 1) {
         pw.println("error: no list_name provided to \"-au" + userName + "\"");
         //TODO LP: clients show here getUsage()
         pw.println("Usage: qconf -help");
         pw.flush();
         return;
      }
      String setName = (String) args.get(1);
      
      UserSet us = null;
      boolean create = false;
      try {
         us = jgdi.getUserSet(setName);
         if (us != null) {
            boolean entryExists = false;
            for (Iterator iter = us.getEntriesList().iterator(); iter.hasNext();) {
               String entry = (String) iter.next();
               if (entry.equals(userName)) {
                  entryExists = true;
                  break;
               }
            }
            if (!entryExists) {
               pw.println("user \"" + userName + "\" is not in access list \"" + setName + "\"");
            } else {
               us.removeEntries(userName);
               jgdi.updateUserSet(us);
               pw.println("deleted user \"" + userName + "\" from access list \"" + setName + "\"");
            }
         } else {
            pw.println("access list \"" + setName + "\" doesn't exist");
         }
      /*if (args.length > ++i) {
         pw.println("error: invalid option argument \"" + args[i] + "\"");
         pw.println("Usage: qconf -help");
      }*/
      } catch (JGDIException ex) {
         pw.println(ex.getMessage());
      } finally {
         pw.flush();
      }
   }
}
