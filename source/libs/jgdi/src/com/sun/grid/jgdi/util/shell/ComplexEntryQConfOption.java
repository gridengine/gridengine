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
import com.sun.grid.jgdi.configuration.ComplexEntry;
import com.sun.grid.jgdi.configuration.ComplexEntryImpl;
import com.sun.grid.jgdi.configuration.GEObject;
import com.sun.grid.jgdi.util.shell.editor.EditorParser;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;


/**
 * ComplexEntryQConfOption class
 * Special handling methods for {@link ComplexEntry}
 * @see {@link QConfOption}
 */
public class ComplexEntryQConfOption extends QConfOption {
   
   //-mc
   void modify(final JGDI jgdi, final List args, final PrintWriter pw) {
      String text = runEditor(showComplexes(jgdi));
      modifyComplexes(jgdi, text, pw);
   }
   
   //-Mc
   void modifyFromFile(final JGDI jgdi, final List args, final PrintWriter pw) {
      String inputText = getTextFromFile(args, pw);
      if (inputText == null) {
         return;
      }
      modifyComplexes(jgdi, inputText, pw);
   }
   
   //-sc
   void show(final JGDI jgdi, final List args, final PrintWriter pw) {
      pw.println(showComplexes(jgdi));
      pw.flush();
   }
   
   /**
    * Updates the complex entry list based on the text
    */
   private void modifyComplexes(final JGDI jgdi, final String text, final PrintWriter pw) {
      ComplexEntry ce;
      //Now parse lines and fields ignore whitespaces join lines on \
      List lineList = EditorParser.tokenizeToList(text);
      List singleLine;
      Map newCEMap = new HashMap();
      String elem;
      int val;
      Boolean boolVal;
      for (Iterator lineIter = lineList.iterator(); lineIter.hasNext(); ) {
         singleLine = (List) lineIter.next();
         elem = (String) singleLine.get(0);
         
         //We check if the line starts with # and skip it
         if (elem.startsWith("#")) {  
            continue;
         }
         if (singleLine.size() != 8) {
            throw new IllegalArgumentException("Expected 8 elements: name, shortcut, type, relop, requestable, consumable, default, urgency");
         }
         ce = new ComplexEntryImpl(true);
         ce.setName(elem);
         ce.setShortcut((String) singleLine.get(1));
         
         val = ce.typeToInt((String) singleLine.get(2));
         //TODO LP: deny invalid
         ce.setValtype(val);
         
         val = ce.opToInt((String) singleLine.get(3));
         //TODO LP: deny invalid
         ce.setRelop(val);
         
         val = ce.reqToInt((String) singleLine.get(4));
         //TODO LP: deny invalid
         ce.setRequestable(val);
         
         boolVal = null;
         if (((String) singleLine.get(5)).equalsIgnoreCase("YES")) {
            boolVal = Boolean.TRUE;
         } else if (((String) singleLine.get(5)).equalsIgnoreCase("NO")) {
            boolVal = Boolean.FALSE;
         }
         if (boolVal == null) {
            throw new IllegalArgumentException("Expected consumable YES or NO. Got: \""+(String) singleLine.get(5)+"\"");
         }
         ce.setConsumable(boolVal.booleanValue());
         
         ce.setDefault((String) singleLine.get(6));
         
         //val = Integer.parseInt((String) singleLine.get(7));
         ce.setUrgencyWeight((String) singleLine.get(7));
         
         newCEMap.put(elem, ce);
      }
      //TODO LP: Better to have jgdi.replaceCEList(List newList) and make the checks on qmaster side
      List toModify = new ArrayList();
      List toDelete = new ArrayList();
      List origList;
      try {
         origList = jgdi.getComplexEntryList();
         String name, shortCut;
         for (Iterator iter = origList.iterator(); iter.hasNext(); ) {
            ce = (ComplexEntry) iter.next();
            name = ce.getName();
            //Check if existing value is modified
            if (newCEMap.containsKey(name)) {
               if (!ce.equalsCompletely(newCEMap.get(name))) {
                  toModify.add(newCEMap.get(name));
               }
               newCEMap.remove(name);
            } else {
               toDelete.add(ce);
            }
         }
         //Add new complexes
         for (Iterator iter = newCEMap.keySet().iterator(); iter.hasNext(); ) {
            ce = (ComplexEntry) newCEMap.get(iter.next());
            try {
               jgdi.addComplexEntry(ce);
            } catch (JGDIException ex) {
               pw.println(ex.getMessage());
               pw.flush();
            }
         }
         //Modify existing
         for (Iterator iter = toModify.iterator(); iter.hasNext(); ) {
            ce = (ComplexEntry) iter.next();
            try {
               jgdi.updateComplexEntry(ce);
            } catch (JGDIException ex) {
               pw.println(ex.getMessage());
               pw.flush();
            }
         }
         //Remove not defined anymore
         for (Iterator iter = toDelete.iterator(); iter.hasNext(); ) {
            ce = (ComplexEntry) iter.next();
            try {
               jgdi.deleteComplexEntry(ce);
            } catch (JGDIException ex) {
               pw.println(ex.getMessage());
               pw.flush();
            }
         }
      } catch (JGDIException ex) {
         pw.println(ex.getMessage());
      }
   }
   
   private static String showComplexes(final JGDI jgdi) {
       StringBuffer sb = new StringBuffer();
       List cList = null;
       try {
          cList = jgdi.getComplexEntryList();
       } catch (JGDIException ex) {
          return ex.getMessage();
       }
       //Format is:nameLen+3 shortcutLen+3 typeLen+3 relopLen+3 requestableLen+8 consumableLen+7 defaultLen+3 urgencyLen+4
       int nameLen = 0, shortcutLen = 0, typeLen = 0, relopLen = 2,
             requestableLen = 0, consumableLen = 0, defaultLen = 0, urgencyLen = 0;
       //Sort the list alphabetically
       List complexList = sortListByName(cList);
       ComplexEntry ceDummy = new ComplexEntryImpl(true);
       //Need to first get the maximum column lengths
       for (Iterator iter=complexList.iterator(); iter.hasNext();) {
          ComplexEntry complex = (ComplexEntry)iter.next();
          nameLen = Math.max(nameLen, complex.getName().length());
          shortcutLen = Math.max(shortcutLen, complex.getShortcut().length());
          typeLen = Math.max(typeLen, ceDummy.typeToString(complex.getValtype()).length());
          requestableLen = Math.max(requestableLen, (complex.getRequestable()==1) ? 2 : 3); //length of YES, NO
          consumableLen = Math.max(consumableLen, (complex.isConsumable()) ? 3 : 2); //length of YES, NO);
          defaultLen = Math.max(defaultLen, complex.getDefault().length());
          urgencyLen = Math.max(urgencyLen, complex.getUrgencyWeight().length());
       }
       //Now format the columns
       String header0 = Format.left("#name", nameLen+4) + Format.left("shortcut", shortcutLen+4) +
             Format.left("type", typeLen+4) + Format.left("relop", relopLen+4) +
             Format.left("requestable", requestableLen+9) + Format.left("consumable", consumableLen+8) +
             Format.left("default", defaultLen+4) + Format.left("urgency", urgencyLen+4);
       StringBuffer header1 = new StringBuffer("#");
       for (int j=0; j<header0.length()-1; j++) {
          header1.append("-");
       }
       sb.append(header0+"\n");
       sb.append(header1+"\n");
       String val;
       //And finally print the columns
       for (Iterator iter=complexList.iterator(); iter.hasNext();) {
          ComplexEntry complex = (ComplexEntry)iter.next();
          val = Format.left(complex.getName(), nameLen+4) + Format.left(complex.getShortcut(), shortcutLen+4) +
                Format.left(ceDummy.typeToString(complex.getValtype()),typeLen+4) +
                Format.left(ceDummy.opToString(complex.getRelop()), relopLen+4) +
                Format.left(ceDummy.reqToString(complex.getRequestable()), requestableLen+9) +  //1 is NO
                Format.left((complex.isConsumable()) ? "YES" : "NO", consumableLen+8) +
                Format.left(complex.getDefault(), defaultLen+4) + complex.getUrgencyWeight();
          sb.append(val+"\n");
       }
       sb.append("# >#< starts a comment but comments are not saved across edits --------\n");
       return sb.toString();
    }
   
   
   private static List sortListByName(final List list) {
       Collections.sort(list, new Comparator() {
          public int compare(Object o1, Object o2) {
             if (o1 == null && o2 == null) {
                return 0;
             }
             if (o1 ==null) {
                return -1;
             }
             if (o2 == null) {
                return 1;
             }
             return ((GEObject)o1).getName().compareTo(((GEObject)o2).getName());
          }
       });
       return list;
    }
}
