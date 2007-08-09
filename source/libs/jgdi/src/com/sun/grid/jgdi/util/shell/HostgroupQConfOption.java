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
import com.sun.grid.jgdi.configuration.Hostgroup;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

/**
 * HostgroupQConfOption class
 * Special handling methods for {@link Hostgroup}
 * @see {@link QConfOption}
 */
public class HostgroupQConfOption extends QConfOption {
   
   //-shgrp_tree
   void showTree(final JGDI jgdi, final List args, final PrintWriter pw) {
      if (args.size() == 0) {
         pw.println("error: no option argument provided to \"-shgrp_tree\"");
         //TODO LP: clients show here getUsage()
         pw.println("Usage: qconf -help");
         pw.flush();
         return;
      }
      
      int i = 0;
      while (i < args.size()) {
         //Prepare the hgroup
         String hgroup = (String) args.get(i++);
         int level=0;
         Hostgroup obj = null;
         try {
            obj = jgdi.getHostgroup(hgroup);
         } catch (JGDIException ex) {
            pw.println("Host group \""+hgroup+"\" does not exist");
            pw.flush();
            continue;
         }
         //Print the tree
         if (obj != null) {
            printHostgroupTree(jgdi, obj, "", "   ", pw);
         }
      }
   }
   
   //-shgrp_resolved
   void showResolved(final JGDI jgdi, final List args, final PrintWriter pw) {
      if (args.size() == 0) {
         pw.println("error: no option argument provided to \"-shgrp_resolved\"");
         //TODO LP: clients show here getUsage()
         pw.println("Usage: qconf -help");
         pw.flush();
         return;
      }
      int i = 0;
      while (i < args.size()) {
         //Prepare the hgroup
         String hgroup = (String) args.get(i++);
         int level=0;
         Hostgroup obj = null;
         try {
            obj = jgdi.getHostgroup(hgroup);
         } catch (JGDIException ex) {
            pw.println("Host group \""+hgroup+"\" does not exist");
            pw.flush();
         }
         //Print the tree
         if (obj != null) {
            ArrayList hnames = new ArrayList();
            printHostgroupResolved(hnames, jgdi, obj, pw);
            String out = "";
            for (Iterator iter = hnames.iterator(); iter.hasNext(); ) {
               out += (String) iter.next() + " ";
            }
            pw.println(out.substring(0,out.length()-1));
            pw.flush();
         }
      }
   }
   
   //TODO LP: Remove recursion in shgrp_tree
   private void printHostgroupTree(final JGDI jgdi, Hostgroup obj, String prefix, final String tab, final PrintWriter pw) {
      pw.println(prefix+obj.getName());
      pw.flush();
      prefix += tab;
      
      String hgroup;
      for (Iterator iter = obj.getHostList().iterator(); iter.hasNext(); ) {
         hgroup = (String) iter.next();
         //Another hroup
         if (hgroup.startsWith("@")) {
            try {
               obj = jgdi.getHostgroup(hgroup);
            } catch (JGDIException ex) {
               pw.println(ex.getMessage());
               pw.flush();
            }
            printHostgroupTree(jgdi, obj, prefix, tab, pw);
         } else {
            pw.println(prefix+hgroup);
            pw.flush();
         }
      }
   }
   
   //TODO: Clients use postorder, better to use sort?
   private void printHostgroupResolved(List result, final JGDI jgdi, Hostgroup obj, final PrintWriter pw) {
      ArrayList queue = new ArrayList();
      queue.add(obj);
      String hgroup;
      while (!queue.isEmpty()) {
         obj = (Hostgroup) queue.get(0);
         queue.remove(0);
         for (Iterator iter = obj.getHostList().iterator(); iter.hasNext(); ) {
            hgroup = (String) iter.next();
            //Another hroup
            if (hgroup.startsWith("@")) {
               try {
                  obj = jgdi.getHostgroup(hgroup);
                  queue.add(obj);
               } catch (JGDIException ex) {
                  pw.println(ex.getMessage());
                  pw.flush();
               }
            } else {
               if (!result.contains(hgroup)) {
                  result.add(hgroup);
               }
            }
         }
      }
      //Collections.sort(result);
   }
   
}
