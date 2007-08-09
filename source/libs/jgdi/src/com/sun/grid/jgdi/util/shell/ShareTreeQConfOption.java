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
import com.sun.grid.jgdi.configuration.ShareTree;
import java.io.PrintWriter;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

/**
 * ShareTreeQConfOption class
 * Special handling methods for {@link ShareTree}
 * @see {@link QConfOption}
 */
public class ShareTreeQConfOption extends QConfOption {
   
   //-astree
   void add(final JGDI jgdi, final PrintWriter pw) {
      try {
         String text = runEditor(showShareTreeNode(jgdi,"Root", true));
         pw.println("NOT IMPLEMENTED");
      } catch (JGDIException ex) {
         pw.println(ex.getMessage());
      } finally {
         pw.flush();
      }
   }
   
   //-Astree
   void addFromFile(final JGDI jgdi, final List args, final PrintWriter pw) {
      try {
         String inputText = getTextFromFile(args, pw);
         if (inputText == null) {
            return;
         }
         pw.println("NOT IMPLEMENTED");
      /*} catch (JGDIException ex) {
         pw.println(ex.getMessage());*/
      } finally {
         pw.flush();
      }
   }
   
   //-mstree
   void modify(final JGDI jgdi, final PrintWriter pw) {
      try {
         String text = runEditor(showShareTreeNode(jgdi,"Root", true));
         modifyShareTree(jgdi, text, pw);
      } catch (JGDIException ex) {
         pw.println(ex.getMessage());
         pw.println();
      }
   }
   
   //-Mstree
   void modifyFromFile(final JGDI jgdi, final List args, final PrintWriter pw) {
      String inputText = getTextFromFile(args, pw);
      pw.println("NOT IMPLEMENTED");
      pw.flush();
   }
   
   //-sstree
   void show(final JGDI jgdi, final PrintWriter pw) {
      try {
         pw.println(showShareTreeNode(jgdi, "Root", true));
      } catch (JGDIException ex) {
         pw.println(ex.getMessage());
      } finally {
         pw.flush();
      }
   }
   
   //-dstree
   void delete(final JGDI jgdi, final PrintWriter pw) {
      ShareTree empty = null;
      try {
         jgdi.updateShareTree(empty);
         pw.println(jgdi.getAdminUser()+"@"+java.net.InetAddress.getLocalHost().getHostName()+" removed sharetree list");
      } catch (JGDIException ex) {
         pw.println(ex.getMessage());
      } catch (UnknownHostException ex) {
         ex.printStackTrace();
      } finally {
         pw.flush();
      }
   }
   
   /**
    * Show sharetree node
    */
   String showShareTreeNode(final JGDI jgdi,final String name) throws JGDIException {
      return showShareTreeNode(jgdi, name, false);
   }
   
   /*
    * Show sharetree node
    */
   private String showShareTreeNode(final JGDI jgdi, final String name, final boolean showTree)  throws JGDIException {
      StringBuffer sb = new StringBuffer();
      List queue = new ArrayList(),childList;
      String childStr, stName;
      ShareTree shareTree = jgdi.getShareTree(name), tempTree;
      queue.add(shareTree);
      while (!queue.isEmpty()) {
         shareTree = (ShareTree) queue.remove(0);
         //Add children to queue
         childList = new ArrayList();
         childList.addAll(shareTree.getChildrenList());
         //Sort the list by ID
         Collections.sort(childList, new Comparator() {
            public int compare(Object a, Object b) {
               int na = ((ShareTree) a).getId();
               int nb = ((ShareTree) b).getId();
               return (na >= nb) ? ((na == nb) ? 0 : 1) : -1;
            }
         });
         childStr="";
         for (int i=0; i<childList.size(); i++) {
            tempTree = (ShareTree) childList.get(i);
            queue.add(tempTree);
            childStr += tempTree.getId()+",";
         }
         //For show node
         if (!showTree) {
            stName = shareTree.getName();
            stName = stName.equals("Root") ? "" : stName;
            sb.append("/"+stName+"="+shareTree.getShares()+"\n");
            //For show tree
         } else {
            sb.append("id="+shareTree.getId()+"\n");
            sb.append("name="+shareTree.getName()+"\n");
            sb.append("type="+shareTree.getType()+"\n");
            sb.append("shares="+shareTree.getShares()+"\n");
            childStr = (childStr.length()>0) ? childStr.substring(0, childStr.length()-1) : "NONE";
            sb.append("childnodes="+childStr+"\n");
         }
      }
      return sb.toString();
   }
   
   private void modifyShareTree(final JGDI jgdi, final String text, final PrintWriter pw) {
      pw.println("NOT IMPLEMENTED YET");
      pw.flush();
   }
}
