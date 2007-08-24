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

import com.sun.grid.jgdi.configuration.ClusterQueue;
import com.sun.grid.jgdi.configuration.Configuration;
import com.sun.grid.jgdi.configuration.GEObject;
import com.sun.grid.jgdi.configuration.SchedConf;
import com.sun.grid.jgdi.configuration.UserSet;
import com.sun.grid.jgdi.configuration.reflect.DefaultListPropertyDescriptor;
import com.sun.grid.jgdi.configuration.reflect.DefaultMapListPropertyDescriptor;
import com.sun.grid.jgdi.configuration.reflect.DefaultMapPropertyDescriptor;
import com.sun.grid.jgdi.configuration.reflect.PropertyDescriptor;
import com.sun.grid.jgdi.configuration.reflect.SimplePropertyDescriptor;
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
 */
public class EditorParser {
   
   /**
    * Converts text to map of PropertyDescriptors ... line, based on obj type
    * Each line is also converted to the intermediate format.
    */
   static Map parsePlainText(GEObject obj, String text) throws IOException {
      String attr, line;
      PropertyDescriptor pd;
      Map map = new HashMap();
      text = trimText(text);
      String [] lines = text.split("\n");
      for (int i=0; i<lines.length; i++) {
         line = lines[i];
         attr = line.split(" ")[0];
         line = (line.length() == attr.length()) ? "" : line.substring(attr.length()+1);
         if (line.length() == 0) {  
            throw new IllegalArgumentException("Expected at least 2 tokens name and value got: \""+attr+"\"");
         }
         attr = EditorUtil.unifyClientNamesWithAttr(obj, attr);
         
         //CONFIGURATION special case
         if (obj instanceof Configuration) {
            map.put(attr, line);
            continue;
         }
         
         //CONFIGURATION special case
         if (obj instanceof Configuration) {
            map.put(attr, line.trim());
            continue;
         }
         
         if ((pd = getPropertyDescriptor(obj, attr))==null) {
               throw new IllegalArgumentException("Skipped: Unknown attribute \""+attr+"\"");
         }
         if (pd.isReadOnly()) {
            throw new IllegalArgumentException("Skipped: Read only attribute \""+attr+"\"");
         }
         
         line = parseOnePlainTextLine(obj, pd, line.trim());
         if (line == null || line.length() ==0) {
            throw new IllegalArgumentException("Invalid value format for attribute \""+attr+"\"");
         }       
         map.put(pd, line);
      }
      return (!map.isEmpty()) ? map : null;
   }
   
   /**
    * Finds a PropertyDescriptor for given obj and property name
    */
   private static PropertyDescriptor getPropertyDescriptor(GEObject obj, String propertyName) {
      propertyName = EditorUtil.c2javaName(obj, propertyName);
      PropertyDescriptor pd;
      String name;
      for (Iterator iter = GEObjectEditor.getAllProperties(obj).iterator(); iter.hasNext();) {
         pd = (PropertyDescriptor) iter.next();
         name = pd.getPropertyName();
         if (name.equals(propertyName)) {
            return pd;
         }
      }
      return null;
   }
   
   /**
    * Converts a single line to intermediate format.
    */
   private static String parseOnePlainTextLine(GEObject obj, PropertyDescriptor pd, String values) {
      String type = pd.getPropertyType().getName();
      if (pd instanceof SimplePropertyDescriptor) {
         List list = getSingleValueList(obj, pd, values," \t");
         if (list.size() > 1) {
            throw new IllegalArgumentException("Expected only 1 argument for type=\""+type+"\" got: "+list.size()+" in "+values);
         }
         String value = (String)list.get(0);
         if (value.split(" ").length > 1) {
            throw new IllegalArgumentException("Expected only 1 argument for type=\""+type+"\" got: "+values);
         }
         return value;
      } else if (pd instanceof DefaultListPropertyDescriptor) {
         List list = getListValueList(obj, pd, values," \t,");
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
   
   /**
    * Return list of elements on the line values
    */
   private static List getSingleValueList(GEObject obj, PropertyDescriptor pd, String values, String separators) {
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
         return Arrays.asList(new Object[] {adjustAttrValues(obj, pd, out.toString().trim())});
      }
      return null;
   }
      
   /**
    * Return list of elements on the line values. Expecting a list of values.
    */
   private static List getListValueList(GEObject obj, PropertyDescriptor pd, String values, String separators) {
      return getSingleValueList(obj, pd, values, separators);
   }
   
   /**
    * Return list of elements on the line values. Expecting a map value.
    */
   private static List getMapValueList(GEObject obj, DefaultMapPropertyDescriptor pd, String values, String separators) {
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
               list.add("[@/="+adjustAttrValues(obj, pd, out.toString().trim())+"]");
               defaultUsed = true;
               out = new StringBuffer();
            }
         } else if (c == ']') {
            elemStart = false;
            if (out.length()>0) {
               list.add("["+adjustAttrValues(obj, pd, out.toString().trim())+"]");
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
         list.add("[@/="+adjustAttrValues(obj, pd, out.toString().trim())+"]");
         defaultUsed = true;
      }
      return list != null ? list : null;
   }
   
   /** Converts attribute values to understandable string values, e.g.: qtype 3 == "BATCH INTERACTIVE" */
   private static String adjustAttrValues(GEObject obj, PropertyDescriptor pd, String value) {
      String key = pd.getPropertyName();
      //ClusterQueue QTYPE attr
      if (obj instanceof ClusterQueue && key.equalsIgnoreCase("qtype")) {
         return String.valueOf(EditorUtil.getQtypeValue(value).intValue());
      //UserSet - type
      } else if (obj instanceof UserSet && key.equalsIgnoreCase("type")) {
         return String.valueOf(EditorUtil.getUserSetTypeValue(value).intValue());
      }
      return value;
   }
   
   /**
    * Return list of elements on the line values. Expecting a map with list values.
    */
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
      
   /**
    * Validates a map line
    */
   private static String validateMapLine(GEObject obj, DefaultMapPropertyDescriptor pd, String values) {
      String validLine = "";
      String key, elem=null;
      String type = pd.getPropertyType().getName();
      //SchedConf usage_weight_list special case
      if (obj instanceof SchedConf && pd.getPropertyName().equals("usageWeight")) {
         return parseSchedConfUsageWeight(obj, pd, values);
      }
      
      List list = getMapValueList(obj, pd, values," \t,");
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
   
   /**
    * Validates a maplist line
    */
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
   
   /**
    * Validates map line entry
    */
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
   
   /**
    * Parses and validates usage_weight attribute in scheduler configuration
    */
   private static String parseSchedConfUsageWeight(GEObject obj, DefaultMapPropertyDescriptor pd, String values) {
      String result = "", elemStr = "", elem;
      String type = pd.getPropertyType().getName();
      String[] elems = values.split("[ \t,]");
      List queue = new ArrayList();
      for (int i=0; i<elems.length; i++) {
         queue.add(elems[i]);
      }
      double sum = 0;
      List expectedKeys = new ArrayList();
      expectedKeys.add("cpu"); expectedKeys.add("mem"); expectedKeys.add("io");
      int state = 0;
      
      //Remove empty elements
      while (!queue.isEmpty()) {
         elem = (String) queue.remove(0);
         if (elem.length() > 0) {
            //Expecting key
            if (state == 0) {
               //Split strings with =
               if (elem.indexOf('=') != -1) {
                  elems = elem.split("=");
                  elem = elems[0];
                  for (int i=elems.length-1; i>=1; i--) {
                     queue.add(0, elems[i]);
                     queue.add(0, "=");
                  }
               }
               if (expectedKeys.isEmpty()) {
                  throw new IllegalArgumentException("Unexpected element \""+elem+"\" in usage_weight_list definition.");
               } else if (!expectedKeys.contains(elem)) {
                  throw new IllegalArgumentException("Expected usage_weigth_list keys to be "+expectedKeys.toString());
               }
               expectedKeys.remove(elem);
               elemStr = elem;
               state = 1;
            //Expecting = 
            } else if (state == 1) {
               if (!elem.equals("=")) {
                  throw new IllegalArgumentException("Expected '=' after usage_weight_list "+elemStr);
               }
               elemStr += "=";
               state = 2;
            //Expecting value
            } else if (state == 2) {
               double d;
               try {
                  d = Double.parseDouble(elem);
                  sum += d;
               } catch (NumberFormatException ex) {
                  throw new IllegalArgumentException("Expected double value after usage_weight_list "+elemStr);
               }
               elemStr += elem;
               result += "["+elemStr+"] ";
               state = 0;
            }
         }
      }
      if (sum != 1.0) {
         throw new IllegalArgumentException("Sum of supplied double values does not equal to 1.0 in usage_weight_list "+elemStr);
      }
      return result.trim();
   }
   
   /**
    * Helper function. Testing if val is a map.
    */
   static boolean isMap(String val) {
      return (val.indexOf('=')==-1) ? false : true;
   }
   
   /**
    * Removes whitespaces. Text is now separed with single ' '. Lines ending with
    * \\ are join to one
    */
   //TODO LP: Decide how to report IOException in trimText and tokenizeToList
   public static String trimText(String text) throws IOException {
      return new Tokenizer(text).tokenizeToString();
   }
   
   /**
    * Removes whitespaces. Lines ending with  \\ are join to one.
    * @return List of lines cointaing a list of elements
    */
   public static List tokenizeToList(String text)  {
      List list = null;
      try {
         list = new Tokenizer(text).tokenizeToList();
      } catch (IOException ex) {
         ex.printStackTrace();
      }
      return list;
   }
   
   /**
    * Tokenizer. Reads text and removed whitespaces join lines on \\.
    */
   private static class Tokenizer {
      String text;
   
      public Tokenizer(String text) {
         this.text = text;
      };
      
      public List tokenizeToList() throws IOException {
         return tokenize();
      }
      
      public String tokenizeToString() throws IOException {
         String res = "";
         List elems = tokenize();
         for (Iterator lineIter = elems.iterator(); lineIter.hasNext(); ) {
            List singleLine = (List) lineIter.next();
            for (Iterator elemIter = singleLine.iterator(); elemIter.hasNext(); ) {
               res += (String) elemIter.next() + " ";
            }
            res = res.trim() + "\n";
         }
         return res.substring(0,res.length()-1);
      }
   
      private List tokenize() throws IOException {
         LineNumberReader lnr = new LineNumberReader(new StringReader(text));
         List list = new ArrayList();
         boolean eof = false;
         String line = "", temp;
         while (!eof) {
            line = lnr.readLine();
            if (line == null) {
               eof = true;
               continue;
            }
            line = line.trim();
            //Skip empty lines
            if (line.length() == 0) {
               continue;
            }
            //Join lines ended with \
            while (line.endsWith("\\") && !eof) {
               line = line.substring(0,line.length()-1);
               line = line.trim();
               eof = (temp = lnr.readLine()) == null;
               if (!eof) {
                  temp = temp.trim();
                  if (temp.length() > 0) {
                     line += " " + temp;
                  }
               }
            }
            list.add(getLineElems(line));
         }
         return (list.size() == 0) ? null : list;
      }
      
      private List getLineElems(String line) {
         String elems[] = line.split("[ \t]");
         String elem;
         List list = new ArrayList();
         for (int i=0; i < elems.length; i++) {
            elem = elems[i].trim();
            if (elem.length()==0) continue;
            list.add(elem);
         }
         return (list.size() == 0) ? null : list;
      }
      
      /*private String trimLine(String line) {
         String elems[] = line.split("[ \t]");
         String res = "", elem;
         for (int i=0; i < elems.length; i++) {
            elem = elems[i].trim();
            if (elem.length()==0) continue;
            res += elem + " ";
         }
         return res.substring(0,res.length()-1);
      }*/
   }
}
