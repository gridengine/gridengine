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

import com.sun.grid.jgdi.configuration.AbstractUser;
import com.sun.grid.jgdi.configuration.ClusterQueue;
import com.sun.grid.jgdi.configuration.ClusterQueueImpl;
import com.sun.grid.jgdi.configuration.ComplexEntry;
import com.sun.grid.jgdi.configuration.ComplexEntryImpl;
import com.sun.grid.jgdi.configuration.Project;
import com.sun.grid.jgdi.configuration.User;
import com.sun.grid.jgdi.configuration.Util;
import com.sun.grid.jgdi.configuration.GEObject;
import com.sun.grid.jgdi.configuration.reflect.ClassDescriptor;
import com.sun.grid.jgdi.configuration.reflect.DefaultListPropertyDescriptor;
import com.sun.grid.jgdi.configuration.reflect.DefaultMapListPropertyDescriptor;
import com.sun.grid.jgdi.configuration.reflect.DefaultMapPropertyDescriptor;
import com.sun.grid.jgdi.configuration.reflect.PropertyDescriptor;
import com.sun.grid.jgdi.configuration.reflect.SimplePropertyDescriptor;
import com.sun.grid.jgdi.configuration.xml.XMLUtil;
import com.sun.grid.jgdi.JGDI;
import java.io.IOException;
import java.io.LineNumberReader;
import java.io.PrintWriter;
import java.io.StringReader;
import java.io.StringWriter;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

/**
 * GEObject editor is a class for editing any GEObject based on text. This text
 * will be parsed. If it's format is valid atrributes that were provided will be
 * updated based on the values next to the attribute.
 *
 * Less coding when creating a new GeObject:
 *
 * <CODE>ClusterQueue geObj = new ClusterQueue(true);
 * GEObjectEditor.updateObjectWithText(jgdi, geObj, "name test\n acl user1 user2\n xacl user3");
 *   or
 * GEObjectEditor.updateObjectWithText(geObj, "name test\n acl user1 user2\n xacl user3");
 * jgdi.addClusterQueue(geObj);</CODE>
 *
 * Only difference between these methods is that whenJGDI reference is passed attributes
 * are checked if they are known to Grid Engine and their structure is more advaced,
 * they are retrived.
 * Such as can have ...
 *
 * While when not passing JGDI reference, new Objects are created with default values.
 * This can lead to unexpected behaviour when you try to update already existing object
 * or even create a new one.
 *
 * <B>RECOMMENDED IS TO USE <CODE>updateObjectWithText(JGDI jgdi, GEObject obj, String text)</CODE> method.</B>
 *
 * Modifying existing GEObjects:
 *
 * <CODE>ClusterQueue geObj = jgdi.getClusterQueue("test");
 * GEObjectEditor.updateObjectWithText(jgdi, geObj, "xacl user4");
 * jgdi.updateClusterQueue(geObj);</CODE>
 */
public class GEObjectEditor {
   
   /**
    * Updates GEObject based on the text. Text is parsed and if correct, object is updated.
    * Creates new objects where necessary, based on values provided in text, therefore
    * it is recommended to use method <CODE>updateObjectWithText(JGDI jgdi, GEObject obj, String text)</CODE>
    * that retrived these objects from Grind Engine instead.
    */
   public static GEObject updateObjectWithText(GEObject obj, String text) {
      return doUpdate(null, obj, text);
   }
   
   /**
    * Updates GEObject based on the text. Text is parsed and if correct, object is updated.
    * Retrives objects from Grid Engine where necessary.
    * Recommended method.
    */
   public static GEObject updateObjectWithText(JGDI jgdi, GEObject obj, String text) {
      if (jgdi == null) {
         throw new IllegalArgumentException("JGDI is NULL");
      }
      return doUpdate(jgdi, obj, text);
   }
   
   private static GEObject doUpdate(JGDI jgdi, GEObject obj, String text) {
      PropertyDescriptor pd;
      String line;
      if (obj == null) {
         throw new IllegalArgumentException("GEObject is NULL");
      }
      if (text.startsWith(XMLUtil.HEADER)) {
         throw new UnsupportedOperationException("XML based editing not yet implemented!!!");
      } else {
         try {
            Map propertyMap = EditorParser.parsePlainText(obj, text);
            for (Iterator iter=propertyMap.keySet().iterator(); iter.hasNext();) {
               pd = (PropertyDescriptor)iter.next();
               line = (String) propertyMap.get(pd);
               updatePropertyValue(jgdi, obj, pd, line);
            }
         } catch (IOException ex) {
            ex.printStackTrace();
         }
      }
      return obj;
   }
   
   private static void updatePropertyValue(JGDI jgdi, GEObject obj, PropertyDescriptor pd, String values) {
      if (pd instanceof SimplePropertyDescriptor) {
         updateSimpleProperty(jgdi, obj, (SimplePropertyDescriptor)pd, values);
      } else if (pd instanceof DefaultListPropertyDescriptor) {
         updateListProperty(jgdi, obj, (DefaultListPropertyDescriptor)pd, values);
      } else if (pd instanceof DefaultMapPropertyDescriptor) {
         updateMapProperty(jgdi, obj, (DefaultMapPropertyDescriptor)pd, values);
      } else if (pd instanceof DefaultMapListPropertyDescriptor) {
         updateMapListProperty(jgdi, obj, (DefaultMapListPropertyDescriptor)pd, values);
      } else {
         new IllegalArgumentException("Unknown descriptor type=\""+pd.getPropertyType().getName()+"\"");
      }
   }
   
   private static void updateSimpleProperty(JGDI jgdi, GEObject obj, SimplePropertyDescriptor pd, String value) {
      String type = pd.getPropertyType().getName();
      pd.setValue(obj, EditorUtil.getParsedValueAsObject(jgdi, pd.getPropertyName(), type, value));
   }
   
   private static void updateListProperty(JGDI jgdi, GEObject obj, DefaultListPropertyDescriptor pd, String values) {
      String type = pd.getPropertyType().getName();
      String[] elems = values.split(" ");
      Object value;
      pd.removeAll(obj);
      for (int i = 0; i < elems.length; i++) {
         value = EditorUtil.getParsedValueAsObject(jgdi, pd.getPropertyName(), type, elems[i]);
         if (value != null) {
            pd.add(obj, value);
         }
      }
   }
   
   private static void updateMapProperty(JGDI jgdi, GEObject obj, DefaultMapPropertyDescriptor pd, String values) {
      String attr = pd.getPropertyName();
      String key, elem=null;
      Object val;
      String[] elems = values.substring(1,values.length()-1).split("] \\[");
      
      pd.removeAll(obj);
      for (int i=0; i < elems.length; i++) {
         elem = elems[i];
         //Get a key for the map
         int keyEndPos = elem.indexOf('=');
         key = elem.substring(0, keyEndPos);
         elem = elem.substring(keyEndPos+1, elem.length());
         //ClusterQueue - QTYPE we already have the int value as String, so we don't convert
         if (attr.equalsIgnoreCase("qtype")) {
            pd.put(obj, key, Integer.valueOf(elem));
            return;
         }
         //TODO Should there be a 'val = getParsedValue(elem)' or map is just String
         val = EditorUtil.getParsedValueAsObject(jgdi, pd.getPropertyName(), pd.getPropertyType().getName(), elem);
         pd.put(obj, key, EditorUtil.translateStringValueToObject(val));
      }
   }
   
   private static void updateOneMapListEntry(JGDI jgdi, GEObject obj, DefaultMapListPropertyDescriptor pd, String key, String elem) {
      String type = pd.getPropertyType().getName();
      String[] elems, subElems;
      String name, strVal;
      Object val;
      boolean isCurrentElemMap;
      //Get elements from the line
      elems = elem.split(" ");
      //Set new value for each of them
      for (int i=0; i < elems.length; i++) {
         isCurrentElemMap = EditorParser.isMap(elems[i]);
         if (isCurrentElemMap) {
            subElems = elems[i].split("=");
            name = subElems[0];
            strVal = (String) EditorUtil.translateStringValueToObject(subElems[1]);
            val = EditorUtil.getParsedValueAsObject(jgdi, pd.getPropertyName(), type, name);
            if (val == null) {
               continue;
            }
            if (val instanceof ComplexEntry) {
               ((ComplexEntryImpl)val).setStringval((String)EditorUtil.translateObjectToStringValue(pd.getPropertyName(), strVal));
            } else if (strVal == null) {
               continue;
            } else if (obj instanceof TestGEObject) {
               val = val+"="+strVal; 
            } else {
               throw new IllegalArgumentException("MapList with map elements not expected (not implemented) for "+obj.getClass().getName()+" class.");
            }
         } else {
            name = elems[i];
            val = EditorUtil.getParsedValueAsObject(jgdi, pd.getPropertyName(), type, name);
            //Add only 1 NULL value for default key
            if (val == null) {
               if (!key.equals(EditorUtil.VALUE_DEFAULT)) {
                  continue;
               }
               List l = pd.getList(obj, key);
               if (l != null && l.size() > 0) {
                  continue;
               }
            }
         }
         if (val == null) {
            pd.addEmpty(obj, key);
         } else {
            pd.add(obj, key, val);
         }
      }
   }
   
   private static void updateMapListProperty(JGDI jgdi, GEObject obj, DefaultMapListPropertyDescriptor pd, String values) {
      String key, elem=null;
      String[] elems = values.substring(1,values.length()-1).split("] \\[");
      
      pd.removeAll(obj);
      for (int i=0; i < elems.length; i++) {
         elem = elems[i];
         //Get a key for the map
         int keyEndPos = elem.indexOf('=');
         key = elem.substring(0, keyEndPos);
         elem = elem.substring(keyEndPos+1, elem.length());
         updateOneMapListEntry(jgdi, obj, pd, key, elem);
      }
   }
    
   /**
    * Retrives configurable properties for specified GEObject as text. Each property on one line.
    */
   public static String getConfigurablePropertiesAsText(GEObject obj) {
      Object o;
      int maxLen = 0;
      String name, value;
      StringBuffer sb = new StringBuffer();
      PropertyDescriptor pd;
      for (Iterator iter = getConfigurableProperties(obj).iterator(); iter.hasNext(); ) {
         pd = (PropertyDescriptor) iter.next();
         name = EditorUtil.java2cName(obj, pd.getPropertyName());
         maxLen = Math.max(maxLen, name.length());
      }
      
      for (Iterator iter = getConfigurableProperties(obj).iterator(); iter.hasNext(); ) {
         pd = (PropertyDescriptor) iter.next();
         name = EditorUtil.java2cName(obj, pd.getPropertyName());
         //sb.append(String.format("%-"+maxLen+"."+maxLen+"s\t%s\n", name, translateObjectToStringValue(getPropertyValue(obj, pd))));
         sb.append(name);
         for (int i=name.length(); i<maxLen; i++) {
            sb.append(' ');
         }
         sb.append("\t"+EditorUtil.translateObjectToStringValue(pd.getPropertyName(), EditorUtil.getPropertyValue(obj, pd))+"\n");
      }
      return sb.toString();
   }
   
   private static List getConfigurableProperties(GEObject obj) {
      List propList = new ArrayList();
      ClassDescriptor cd = Util.getDescriptor(obj.getClass());
      Iterator iter = cd.getProperties().iterator();
      while (iter.hasNext()) {
         PropertyDescriptor pd = (PropertyDescriptor)iter.next();
         if (EditorUtil.doNotDisplayConfigurableAttr(obj, pd)) {
            continue;
         }
         if (pd.isConfigurable()) {
            propList.add(pd);
         }
      }
      return propList;
   }
   
   /**
    * Retrives read-only properties for specified GEObject as text. Each property on one line.
    */
   public static String getReadOnlyPropertiesAsText(GEObject obj) {
      Object o;
      int maxLen = 0;
      String name, value;
      StringBuffer sb = new StringBuffer();
      PropertyDescriptor pd;
      for (Iterator iter = getReadOnlyProperties(obj).iterator(); iter.hasNext(); ) {
         pd = (PropertyDescriptor) iter.next();
         name = EditorUtil.java2cName(obj, pd.getPropertyName());
         maxLen = Math.max(maxLen, name.length());
      }
      
      for (Iterator iter = getReadOnlyProperties(obj).iterator(); iter.hasNext(); ) {
         pd = (PropertyDescriptor) iter.next();
         name = EditorUtil.java2cName(obj, pd.getPropertyName());
         //sb.append(String.format("%-"+maxLen+"."+maxLen+"s\t%s\n", name, translateObjectToStringValue(getPropertyValue(obj, pd))));
         sb.append(name);
         for (int i=name.length(); i<maxLen; i++) {
            sb.append(' ');
         }
         sb.append("\t"+EditorUtil.translateObjectToStringValue(pd.getPropertyName(), EditorUtil.getPropertyValue(obj, pd))+"\n");
      }
      return sb.toString();
   }
   
   private static List getReadOnlyProperties(GEObject obj) {
      List propList = new ArrayList();
      ClassDescriptor cd = Util.getDescriptor(obj.getClass());
      Iterator iter = cd.getProperties().iterator();
      while(iter.hasNext()) {
         PropertyDescriptor pd = (PropertyDescriptor)iter.next();
         if(pd.isReadOnly()) {
            propList.add(pd);
         }
      }
      return propList;
   }
   
   /**
    * Retrives all properties known to JGDI for specified GEObject as text. Each property on one line.
    */
   public static String getAllPropertiesAsText(GEObject obj) {
      Object o;
      int maxLen = 0;
      String name, value;
      StringBuffer sb = new StringBuffer();
      PropertyDescriptor pd;
      for (Iterator iter = getAllProperties(obj).iterator(); iter.hasNext();) {
         pd = (PropertyDescriptor) iter.next();
         name = EditorUtil.java2cName(obj, pd.getPropertyName());
         maxLen = Math.max(maxLen, name.length());
      }
      
      for (Iterator iter = getAllProperties(obj).iterator(); iter.hasNext();) {
         pd = (PropertyDescriptor) iter.next();
         name = EditorUtil.java2cName(obj, pd.getPropertyName());
         //sb.append(String.format("%-"+maxLen+"."+maxLen+"s\t%s\n", name, translateObjectToStringValue(getPropertyValue(obj, pd))));
         sb.append(name);
         for (int i=name.length(); i<maxLen; i++) {
            sb.append(' ');
         }
         //TODO LP discuss the format. MaxLen +4 vs. MaxLen+1 + \t? Add to other printing methods
         sb.append("    "+EditorUtil.translateObjectToStringValue(pd.getPropertyName(), EditorUtil.getPropertyValue(obj, pd))+"\n");
      }
      return sb.toString();
   }
   
   static List getAllProperties(GEObject obj) {
      List propList = new ArrayList();
      ClassDescriptor cd = Util.getDescriptor(obj.getClass());
      Iterator iter = cd.getProperties().iterator();
      while (iter.hasNext()) {
         PropertyDescriptor pd = (PropertyDescriptor)iter.next();
         if (EditorUtil.doNotDisplayAttr(obj, pd)) {
            continue;
         }
         propList.add(pd);
      }
      return propList;
   }
   
   public static void main(String[] args) {
      ClusterQueue geObject = new ClusterQueueImpl(true);
      System.out.println(GEObjectEditor.getConfigurablePropertiesAsText(geObject));
      System.out.println(GEObjectEditor.getReadOnlyPropertiesAsText(geObject));
      System.out.println(GEObjectEditor.getAllPropertiesAsText(geObject));
   }
}