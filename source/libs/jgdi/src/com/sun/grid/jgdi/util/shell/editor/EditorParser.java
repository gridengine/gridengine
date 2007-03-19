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

package com.sun.grid.jgdi.util.shell.editor;

import com.sun.grid.jgdi.JGDI;
import com.sun.grid.jgdi.configuration.ComplexEntry;
import com.sun.grid.jgdi.configuration.ComplexEntryImpl;
import com.sun.grid.jgdi.configuration.GEObject;
import com.sun.grid.jgdi.configuration.reflect.DefaultListPropertyDescriptor;
import com.sun.grid.jgdi.configuration.reflect.DefaultMapListPropertyDescriptor;
import com.sun.grid.jgdi.configuration.reflect.DefaultMapPropertyDescriptor;
import com.sun.grid.jgdi.configuration.reflect.PropertyDescriptor;
import com.sun.grid.jgdi.configuration.reflect.SimplePropertyDescriptor;
import com.sun.media.protocol.sunvideoplus.OPICapture;
import java.io.IOException;
import java.io.LineNumberReader;
import java.io.StringReader;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

/**
 *
 * @author lp195527
 */
public class EditorParser {
   
   static Map parsePlainText(GEObject obj, String text) throws IOException {
      int i, j;
      String attr, line;
      PropertyDescriptor pd;
      LineNumberReader lnr = new LineNumberReader(new StringReader(text));
      Map map = new HashMap();
      boolean eof = false;
      while (!eof) {
         attr = lnr.readLine();
         if (attr == null) {
            eof = true;
            continue;
         }
         attr = attr.trim();
         //Skip empty lines
         if (attr.length() == 0) {
            continue;
         }
         //Join lines ended with \
         while (attr.endsWith("\\") && !eof) {
            attr = attr.substring(0,attr.length()-1);
            attr = attr.trim();
            eof = (line = lnr.readLine()) == null;
            if (!eof) {
               line = line.trim();
               if (line.length() > 0) {
                  attr += " " + line;
               }
            }
         }
         i = attr.indexOf(' ');
         j = attr.indexOf('\t');
         if (i == j) {  //i=j=-1 (not found)
            throw new IllegalArgumentException("Expected at least 2 tokens name and value got: \""+attr+"\"");
         }
         i = (i == -1) ? j : i;
         line = attr.substring(i);
         attr = EditorUtil.unifyClientNamesWithAttr(obj, attr.substring(0,i));
         
         if ((pd = getPropertyDescriptor(obj, attr))==null) {
               throw new IllegalArgumentException("Skipped: Unknown attribute \""+attr+"\"");
         }
         if (pd.isReadOnly()) {
            throw new IllegalArgumentException("Skipped: Read only attribute \""+attr+"\"");
         }
         line = parseOnePlainTextLine(obj,pd, line.trim());
         if (line == null || line.length() ==0) {
            throw new IllegalArgumentException("Invalid value format for attribute \""+attr+"\"");
         }
         map.put(pd, line);
      }
      return (!map.isEmpty()) ? map : null;
   }
   
   private static PropertyDescriptor getPropertyDescriptor(GEObject obj, String propertyName) {
      propertyName = EditorUtil.c2javaName(obj, propertyName);
      PropertyDescriptor pd;
      for (Iterator iter = GEObjectEditor.getAllProperties(obj).iterator(); iter.hasNext();) {
         pd = (PropertyDescriptor) iter.next();
         if (pd.getPropertyName().equals(propertyName)) {
            return pd;
         }
      }
      return null;
   }
   
   private static String parseOnePlainTextLine(GEObject obj, PropertyDescriptor pd, String values) {
      String type = pd.getPropertyType().getName();
      if (pd instanceof SimplePropertyDescriptor) {
         List list = getSingleValueList(values," \t");
         if (list.size() > 1) {
            throw new IllegalArgumentException("Expected only 1 argument for type=\""+type+"\" got: "+list.size()+" in "+values);
         }
         String value = (String)list.get(0);
         if (value.split(" ").length > 1) {
            throw new IllegalArgumentException("Expected only 1 argument for type=\""+type+"\" got: "+values);
         }
         return value;
      } else if (pd instanceof DefaultListPropertyDescriptor) {
         List list = getListValueList(values," \t,");
         if (list.size() != 1) {
            throw new IllegalArgumentException("Expected only list of arguments for type=\""+type+"\" got: "+list.toString());
         }
         return (String)list.get(0);
      } else if (pd instanceof DefaultMapPropertyDescriptor) {
         return validateMapLine(obj, (DefaultMapPropertyDescriptor) pd, values);
      } else if (pd instanceof DefaultMapListPropertyDescriptor) {
         return validateMapListLine(obj, (DefaultMapListPropertyDescriptor) pd, values);
      } else {
         throw new IllegalArgumentException("Unknown descriptor type=\""+pd.getPropertyType().getName()+"\"");
      }
   }
   
   private static List getSingleValueList(String values, String separators) {
      StringBuffer sb = new StringBuffer(values);
      StringBuffer out = new StringBuffer();
      char c;
      boolean skipped = false;
      for (int i=0; i < values.length(); i++) {
         //Skip separators
         while (i < values.length() && separators.indexOf(values.charAt(i)) != -1) {
            i++;
            skipped = true;
         }
         if (i >= values.length()) {
            break;
         }
         c = values.charAt(i);
         if (skipped && i < values.length() && out.length() > 0) {
            out.append(' ');
         }
         skipped = false;
         out.append(c);
      }
      if (out.length()>0) {
         return Arrays.asList(new Object[] {out.toString().trim()});
      }
      return null;
   }
   
   private static List getListValueList(String values, String separators) {
      return getSingleValueList(values, separators);
   }
   
   private static List getMapValueList(DefaultMapPropertyDescriptor pd, String values, String separators) {
      List list = new ArrayList();
      StringBuffer sb = new StringBuffer(values);
      StringBuffer out = new StringBuffer();
      char c;
      boolean elemStart = false;
      boolean skipped = false;
      boolean defaultUsed = false;
      for (int i=0; i < values.length(); i++) {
         //Skip separators
         while (i < values.length() && separators.indexOf(values.charAt(i)) != -1) {
            i++;
            skipped = true;
         }
         if (i >= values.length()) {
            break;
         }
         c = values.charAt(i);
         if (skipped && i < values.length() && out.length() > 0) {
            out.append(' ');
         }
         skipped = false;
         if (c == '[') {
            if (elemStart) {
               throw new IllegalArgumentException("Unsupported format when parsing data. Got: "+values+" Expected: unique [key=value] pairs.");
            }
            elemStart = true;
            if (out.length()>0) {
               if (defaultUsed) {
                  throw new IllegalArgumentException("Unsupported format when parsing data. Got: "+values+" Expected: unique [key=value] pairs.");
               }
               list.add("[@/="+adjustAttrValues(pd, out.toString().trim())+"]");
               defaultUsed = true;
               out = new StringBuffer();
            }
         } else if (c == ']') {
            elemStart = false;
            if (out.length()>0) {
               list.add("["+adjustAttrValues(pd, out.toString().trim())+"]");
               out = new StringBuffer();
            }
         } else {
            out.append(c);
         }
      }
      if (out.length()>0 && list.size()==0) {
         if (defaultUsed) {
            throw new IllegalArgumentException("Unsupported format when parsing data. Got: "+values+" Expected: unique [key=value] pairs.");
         }
         list.add("[@/="+adjustAttrValues(pd, out.toString().trim())+"]");
         defaultUsed = true;
      }
      return list != null ? list : null;
   }
   
   /** Converts attribute values to understandable string values, e.g.: qtype 3 == "BATCH INTERACTIVE" */
   private static String adjustAttrValues(PropertyDescriptor pd, String value) {
      String key = pd.getPropertyName();
      //ClusterQueue QTYPE attr
      if (key.equalsIgnoreCase("qtype")) {
         return String.valueOf(EditorUtil.getQtypeValue(value).intValue());
      }
      return value;
   }
   
   private static List getMapListValueList(String values, String separators) {
      List list = new ArrayList();
      StringBuffer sb = new StringBuffer(values);
      StringBuffer out = new StringBuffer();
      char c;
      boolean elemStart = false;
      boolean skipped = false;
      boolean defaultUsed = false;
      for (int i=0; i < values.length(); i++) {
         //Skip separators
         while (i < values.length() && separators.indexOf(values.charAt(i)) != -1) {
            i++;
            skipped = true;
         }
         if (i >= values.length()) {
            break;
         }
         c = values.charAt(i);
         if (skipped && i < values.length() && out.length() > 0
               && c != '=' && out.charAt(out.length()-1) != '=') {
            out.append(' ');
         }
         skipped = false;
         if (c == '[') {
            if (elemStart) {
               throw new IllegalArgumentException("Unsupported format when parsing data. Got: "+values+" Expected: unique [key=value] pairs.");
            }
            elemStart = true;
            if (out.length()>0) {
               if (defaultUsed) {
                  throw new IllegalArgumentException("Unsupported format when parsing data. Got: "+values+" Expected: unique [key=value] pairs.");
               }
               list.add("[@/="+out.toString().trim()+"]");
               defaultUsed = true;
               out = new StringBuffer();
            }
         } else if (c == ']') {
            elemStart = false;
            if (out.length()>0) {
               list.add("["+out.toString().trim()+"]");
               out = new StringBuffer();
            }
         } else {
            out.append(c);
         }
      }
      if (out.length()>0 && list.size()==0) {
         if (defaultUsed) {
            throw new IllegalArgumentException("Unsupported format when parsing data. Got: "+values+" Expected: unique [key=value] pairs.");
         }
         list.add("[@/="+out.toString().trim()+"]");
         defaultUsed = true;
      }
      return list != null ? list : null;
   }
   
   private static String validateMapLine(GEObject obj, DefaultMapPropertyDescriptor pd, String values) {
      String validLine = "";
      String key, elem=null;
      String type = pd.getPropertyType().getName();
      List list = getMapValueList(pd, values," \t,");
      if (list == null || list.size() == 0) {
         throw new IllegalArgumentException("Got empty list. Argument \""+values+"\" has invalid format.");
      }
      boolean isFirstElemMap, isCurrentElemMap;
      String[] elems;
      Iterator iter = list.iterator();
      while (iter.hasNext()) {
         elem = (String) iter.next();
         //Check we have data in brackets [data]
         if (elem.charAt(0) != '[' || elem.charAt(elem.length()-1) != ']') {
            throw new IllegalArgumentException("Each element must be enclosed in a brackets. Got: "+values);
         }
         elems = elem.split("=");
         //Check we have key=val
         if (elems.length != 2) {
            throw new IllegalArgumentException("Expected [key=val] elements got: "+elem);
         }
         elem = elems[0].trim()+"="+elems[1].trim();
         elems = elem.split(" ");
         //Check we have only one value
         if (elems.length !=1) {
            throw new IllegalArgumentException("Expected [key=val] elements got: "+elem);
         }
         validLine += elem + " ";
      }
      return validLine.trim();
   }
   
   private static String validateMapListLine(GEObject obj, DefaultMapListPropertyDescriptor pd, String values) {
      String validLine = "";
      String key, elem=null;
      String type = pd.getPropertyType().getName();
      List list = getMapListValueList(values," \t,");
      if (list == null || list.size() == 0) {
         throw new IllegalArgumentException("Got empty list. Argument \""+values+"\" has invalid format.");
      }
      boolean isFirstElemMap, isCurrentElemMap;
      
      Iterator iter = list.iterator();
      while (iter.hasNext()) {
         elem = (String) iter.next();
         if (elem.charAt(0) != '[' || elem.charAt(elem.length()-1) != ']') {
            throw new IllegalArgumentException("Each element must be enclosed in a brackets. Got: "+values);
         }
         //Get a key for the map
         int keyEndPos = elem.indexOf('=');
         if (keyEndPos == -1) {
            throw new IllegalArgumentException("Expected [key=val1 val2] elements got: "+elem);
         }
         key = elem.substring(1, keyEndPos);
         elem = elem.substring(keyEndPos+1, elem.length()-1);
         validateOneMapListLineEntry(obj, pd, key, elem);
      }
      for (int i=0; i < list.size(); i++) {
         validLine += (String)list.get(i) + " ";
      }
      return validLine.trim();
   }
   
   private static void validateOneMapListLineEntry(GEObject obj, DefaultMapListPropertyDescriptor pd, String key, String elem) {
      String[] subElems;
      boolean isCurrentElemMap;
      //Get elements from the line
      String[] elems = elem.split(" ");
      boolean isFirstElemMap = isMap(elems[0]);
      //Set new value for each of them
      for (int i=0; i < elems.length; i++) {
         isCurrentElemMap = isMap(elems[i]);
         if (isCurrentElemMap != isFirstElemMap) {
            throw new IllegalArgumentException("Expected all elements to be of a same kind. First elements were a "+((isFirstElemMap)? "Map" : "List")+" current element \""+elems[i]+"\" is not.");
         } else if (isCurrentElemMap) {
            subElems = elems[i].split("=");
            if (subElems.length != 2) {
               throw new IllegalArgumentException("Expected key=value. Got: "+elems[i]);
            }
         }
      }
   }
   
   static boolean isMap(String val) {
      return (val.indexOf('=')==-1) ? false : true;
   }
}
