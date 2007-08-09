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
import com.sun.grid.jgdi.configuration.ConfigurationImpl;
import com.sun.grid.jgdi.configuration.GEObject;
import com.sun.grid.jgdi.util.shell.editor.EditorUtil;
import com.sun.grid.jgdi.util.shell.editor.GEObjectEditor;
import com.sun.grid.jgdi.util.shell.editor.TextEditor;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.LineNumberReader;
import java.io.PrintWriter;
import java.io.StringReader;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

/**
 * QConfOption class implements default generic methods for qconf command options
 * Special handling should be done in extending classes. Such as AbstractUserQConfOption.
 */
public class QConfOption extends CommandOption {
   
   //-help
   public void printUsage(final PrintWriter pw) {
      pw.println(new QConfCommand().getUsage());
      pw.flush();
   }
   
   void add(final JGDI jgdi, String type, final List args, final PrintWriter pw) {
      Object obj;
      Class cls;
      String name = (args.size() == 1) ? (String)args.get(0) : "template";
      if (args.size()>1) {
         pw.println("error: invalid option argument \"" + args.get(1) + "\"");
         pw.println("Usage: qconf -help");
         pw.flush();
         return;
      }
      try {
         cls = Class.forName("com.sun.grid.jgdi.configuration." + type + "Impl");
         Constructor c = cls.getConstructor(new Class[] {boolean.class});
         obj = c.newInstance(new Object[] {Boolean.TRUE});
         if (!(obj instanceof GEObject)) {
            throw new IllegalAccessException("Class for type "+type+" is not an instance of GEObject");
         }
         GEObject geObj = (GEObject)obj;
         Method setName = obj.getClass().getDeclaredMethod("setName", new Class[] {String.class});
         setName.invoke(geObj, new Object[] {name});
         String userTypedText = runEditor(GEObjectEditor.getConfigurablePropertiesAsText(geObj));
         GEObjectEditor.updateObjectWithText(jgdi, geObj, userTypedText);
         Class paramType = Class.forName("com.sun.grid.jgdi.configuration." + type);
         Method m = JGDI.class.getDeclaredMethod("add"+type, new Class[] {paramType});
         m.invoke(jgdi, new Object[] {geObj});
         //TODO LP: Fix the messages for clients convertAddRemoveTypeToMessage
         pw.println(jgdi.getAdminUser()+"@"+java.net.InetAddress.getLocalHost().getHostName()+" added \""+geObj.getName()+"\" to "+type.toLowerCase()+" list");
         pw.flush();
      } catch (ClassNotFoundException ex) {
         ex.printStackTrace();
      } catch (SecurityException ex) {
         ex.printStackTrace();
      } catch (NoSuchMethodException ex) {
         ex.printStackTrace();
      } catch (InstantiationException ex) {
         ex.printStackTrace();
      } catch (IllegalAccessException ex) {
         ex.printStackTrace();
      } catch (InvocationTargetException ex) {
         ex.printStackTrace();
      } catch (UnknownHostException ex) {
         ex.printStackTrace();
      } catch (JGDIException ex) {
         pw.println(ex.getMessage());
      } finally {
         pw.flush();
      }
   }
   
   void addFromFile(final JGDI jgdi, final String type, final List args, final PrintWriter pw) {
      Object obj;
      Class cls;
      if (args.size() <= 0) {
         pw.println("no file argument given");
         pw.flush();
         return;
      }
      if (args.size() != 1) {
         pw.println("error: invalid option argument \"" + args.get(1) + "\"");
         pw.println("Usage: qconf -help");
         pw.flush();
         return;
      }
      String fileName = (String)args.get(0);
      String keyAttrValue = null;
      try {
         cls = Class.forName("com.sun.grid.jgdi.configuration." + type + "Impl");
         Constructor c = cls.getConstructor(new Class[] {boolean.class});
         obj = c.newInstance(new Object[] {Boolean.TRUE});
         if (!(obj instanceof GEObject)) {
            throw new IllegalAccessException("Class for type "+type+" is not an instance of GEObject");
         }
         String inputText = readFile(fileName);
         String setNameMethod = "setName";
         //CONFIGURATION special handling
         if (type.equals("Configuration")) {
            keyAttrValue = new File(fileName).getName();
            setNameMethod = "setHname";
         } else {
            keyAttrValue = getKeyAttributeValueFromString(pw, type, fileName, inputText);
         }
         if (keyAttrValue == null) {
            return;
         }
         GEObject geObj = (GEObject)obj;
         Method setName = obj.getClass().getDeclaredMethod(setNameMethod, new Class[] {String.class});
         setName.invoke(obj, new Object[] {keyAttrValue});
         GEObjectEditor.updateObjectWithText(jgdi, geObj, inputText);
         Class paramType = Class.forName("com.sun.grid.jgdi.configuration." + type);
         Method m = JGDI.class.getDeclaredMethod("add"+type, new Class[] {paramType});
         m.invoke(jgdi, new Object[] {obj});
         pw.println(jgdi.getAdminUser()+"@"+java.net.InetAddress.getLocalHost().getHostName()+" added \""+keyAttrValue+"\" to "+type.toLowerCase()+" list");
         pw.flush();
      } catch (ClassNotFoundException ex) {
         ex.printStackTrace();
      } catch (SecurityException ex) {
         ex.printStackTrace();
      } catch (NoSuchMethodException ex) {
         ex.printStackTrace();
      } catch (InstantiationException ex) {
         ex.printStackTrace();
      } catch (IllegalAccessException ex) {
         ex.printStackTrace();
      } catch (InvocationTargetException ex) {
         ex.printStackTrace();
      } catch (UnknownHostException ex) {
         ex.printStackTrace();
      } catch (JGDIException ex) {
         pw.println(ex.getMessage());
      } catch (IOException ex) {
         pw.println("error: error opening file \""+fileName+"\" for reading: "+ex.getMessage());
         pw.println("error reading in file");
      } finally {
         pw.flush();
      }
   }
   
   void modify(final JGDI jgdi, final String type, final List args, final PrintWriter pw) {
      Object obj;
      
      Class cls;
      String name;
      Class[] paramCls;
      Object[] params;
      if (type.equalsIgnoreCase("SchedConf")) {
         name = null;
         paramCls = new Class[] {};
         params = new Object[] {};
      } else {
         if (args.size() <= 0 && !type.equalsIgnoreCase("SchedConf")) {
            pw.println("error: missing "+type.toLowerCase()+" name");
            pw.flush();
            return;
         }
         if (args.size() != 1) {
            pw.println("error: invalid option argument \"" + args.get(1) + "\"");
            pw.println("Usage: qconf -help");
            pw.flush();
            return;
         }
         name = (String)args.get(0);
         paramCls = new Class[] {String.class};
         params = new Object[] {name};
      }
      try {
         cls = Class.forName("com.sun.grid.jgdi.configuration." + type + "Impl");
         Method getGEObj = JGDI.class.getDeclaredMethod("get"+type, paramCls);
         obj = getGEObj.invoke(jgdi, params);
         if (obj == null) {
            pw.println(name+" is not known as "+type);
            pw.flush();
            return;
         }
         GEObject geObj = (GEObject)obj;
         String userTypedText = runEditor(GEObjectEditor.getConfigurablePropertiesAsText(geObj));
         GEObjectEditor.updateObjectWithText(jgdi, geObj, userTypedText);
         Class paramType = Class.forName("com.sun.grid.jgdi.configuration." + type);
         Method m = JGDI.class.getDeclaredMethod("update"+type, new Class[] {paramType});
         m.invoke(jgdi, new Object[] {geObj});
         //SchedConf special case
         if (type.equalsIgnoreCase("SchedConf")) {
            pw.println("changed scheduler configuration");
         } else {
            pw.println(jgdi.getAdminUser()+"@"+java.net.InetAddress.getLocalHost().getHostName()+" modified \""+geObj.getName()+"\" in "+type.toLowerCase()+" list");
         }
         pw.flush();
      } catch (ClassNotFoundException ex) {
         ex.printStackTrace();
      } catch (SecurityException ex) {
         ex.printStackTrace();
      } catch (NoSuchMethodException ex) {
         ex.printStackTrace();
      } catch (IllegalAccessException ex) {
         ex.printStackTrace();
      } catch (InvocationTargetException ex) {
         ex.printStackTrace();
      } catch (UnknownHostException ex) {
         ex.printStackTrace();
      } catch (JGDIException ex) {
         pw.println(ex.getMessage());
      } finally {
         pw.flush();
      }
   }
   
   void modifyFromFile(final JGDI jgdi, final String type, final List args, final PrintWriter pw) {
      Object obj;
      Class cls;
      String keyAttrValue = null;
      String inputText;
      if ((inputText = getTextFromFile(args, pw)) == null) {
         return;
      }
      try {
         //SchedConf special handling, has no 'name' (primary key)
         if (type.equals("SchedConf")) {
            obj = jgdi.getSchedConf();
         } else {
            keyAttrValue = getKeyAttributeValueFromString(pw, type, (String) args.get(0), inputText);
            if (keyAttrValue == null) {
               return;
            }
            //Lookup the object based on the key atrribute value (name)
            Method getGEObj = JGDI.class.getDeclaredMethod("get"+type, new Class[] {String.class});
            obj = getGEObj.invoke(jgdi, new Object[] {keyAttrValue});
            if (obj == null) {
               pw.println(keyAttrValue+" is not known as "+type);
               pw.flush();
               return;
            }
         }
         //And try to update the object based on the inputText
         GEObject geObj = (GEObject)obj;
         GEObjectEditor.updateObjectWithText(jgdi, geObj, inputText);
         Class paramType = Class.forName("com.sun.grid.jgdi.configuration." + type);
         Method m = JGDI.class.getDeclaredMethod("update"+type, new Class[] {paramType});
         m.invoke(jgdi, new Object[] {geObj});
         pw.println(jgdi.getAdminUser()+"@"+java.net.InetAddress.getLocalHost().getHostName()+" modified \""+geObj.getName()+"\" in "+type.toLowerCase()+" list");
         pw.flush();
      } catch (ClassNotFoundException ex) {
         ex.printStackTrace();
      } catch (SecurityException ex) {
         ex.printStackTrace();
      } catch (NoSuchMethodException ex) {
         ex.printStackTrace();
      } catch (IllegalAccessException ex) {
         ex.printStackTrace();
      } catch (InvocationTargetException ex) {
         ex.printStackTrace();
      } catch (UnknownHostException ex) {
         ex.printStackTrace();
      } catch (JGDIException ex) {
         pw.println(ex.getMessage());
      } finally {
         pw.flush();
      }
   }
   
   
   void show(final JGDI jgdi, final String type, final List args, final PrintWriter pw) {
      show(jgdi, type, false, args, pw);
   }
   
   void showWithNoArgs(final JGDI jgdi, final String type, final List args, final PrintWriter pw) {
      show(jgdi, type, true, new ArrayList(), pw);
   }
   
   private void show(final JGDI jgdi, final String type, boolean hasNoArgs, final List args, final PrintWriter pw) {
      Object obj;
      Class cls;
      if (args.size() <= 0 && !hasNoArgs) {
         pw.println("error: missing "+type.toLowerCase()+"_list");
         pw.flush();
         return;
      }
      try {
         List strList = new ArrayList();
         cls = Class.forName("com.sun.grid.jgdi.configuration." + type + "Impl");
         Class[] argClass = (hasNoArgs) ? null : new Class[] {String.class};
         Method getGEObj = JGDI.class.getDeclaredMethod("get"+type, argClass);
         if (hasNoArgs) {
            obj = getGEObj.invoke(jgdi,null);
            String text = GEObjectEditor.getAllPropertiesAsText((GEObject) obj);
            pw.println(text);
            pw.flush();
            return;
         }
         for (int i=0; i < args.size(); i++) {
            String name = (String)args.get(i);
            String[] names = name.split(",");
            for (int j=0; j < names.length; j++) {
               if (names[j].length()==0) {
                  continue;
               }
               obj = getGEObj.invoke(jgdi, new Object[] {names[j]});
               if (obj == null) {
                  String text = names[j]+" is not known as "+type+"\n";
                  if (!strList.contains(text)) {
                     strList.add(text);
                  }
               } else {
                  String text = GEObjectEditor.getAllPropertiesAsText((GEObject) obj);
                  if (!strList.contains(obj)) {
                     strList.add(text);
                  }
               }
            }
         }
         int i=0;
         pw.print((String)strList.get(i));
         for (i=1; i < strList.size(); i++) {
            //ClusterQueues are without the empty lines
            if (!type.equals("ClusterQueue")) pw.println();
            pw.print((String)strList.get(i));
         }
         //CONFIGURATION special handling
         if (type.equals("Configuration")) {
            pw.println();
         }
         pw.flush();
      } catch (ClassNotFoundException ex) {
         ex.printStackTrace();
      } catch (SecurityException ex) {
         ex.printStackTrace();
      } catch (NoSuchMethodException ex) {
         ex.printStackTrace();
      } catch (IllegalAccessException ex) {
         ex.printStackTrace();
      } catch (InvocationTargetException ex) {
         ex.printStackTrace();
      }
   }
   
   void showList(final JGDI jgdi, final String type, final List args, final PrintWriter pw) {
      Class cls;
      try {
         Class paramType = Class.forName("com.sun.grid.jgdi.configuration." + type);
         Method getGEObjList = JGDI.class.getDeclaredMethod("get" + type + "List", null);
         List list = (List) getGEObjList.invoke(jgdi, null);
         //CONFIGURATION special handling
         if (type.equals("Configuration")) {
            List relevantList = new ArrayList();
            for (Iterator iter = list.iterator(); iter.hasNext(); ) {
               String name = ((ConfigurationImpl) iter.next()).getHname();
               if (!name.equals("global")) {
                  relevantList.add(name);
               }
            }
            Collections.sort(relevantList);
            for (Iterator iter = relevantList.iterator(); iter.hasNext(); ) {
               pw.println(iter.next());
            }
            pw.flush();
            return;
         }
         list = getNameList(list);
         adjustListValues(type, list);
         Collections.sort(list);
         //TODO LP: File bug for client - typeConvertor map
         //String typeStr = (typeConvertor.containsKey(type)) ? (String) typeConvertor.get(type) : type;
         if (list.size() == 0) {
            pw.println("no "+type.toLowerCase()+" defined");
            pw.flush();
            return;
         }
         for (Iterator iter = list.iterator(); iter.hasNext(); ) {
            pw.println(iter.next());
         }
         pw.flush();
      } catch (ClassNotFoundException ex) {
         ex.printStackTrace();
      } catch (SecurityException ex) {
         ex.printStackTrace();
      } catch (NoSuchMethodException ex) {
         ex.printStackTrace();
      } catch (IllegalArgumentException ex) {
         ex.printStackTrace();
      } catch (IllegalAccessException ex) {
         ex.printStackTrace();
      } catch (InvocationTargetException ex) {
         ex.printStackTrace();
      }
   }
   
   void delete(final JGDI jgdi, final String type, final List args, final PrintWriter pw) {
      Object obj;
      Class cls;
      if (args.size() <= 0) {
         pw.println("error: missing "+type.toLowerCase()+"_list");
         pw.flush();
         return;
      }
      try {
         cls = Class.forName("com.sun.grid.jgdi.configuration." + type + "Impl");
         Method getGEObj = JGDI.class.getDeclaredMethod("get"+type, new Class[] {String.class});
         Class paramType = Class.forName("com.sun.grid.jgdi.configuration." + type);
         Method deleteGEObj = JGDI.class.getDeclaredMethod("delete"+type, new Class[] {paramType});
         List objList = new ArrayList();
         List errList = new ArrayList();
         for (int i=0; i < args.size(); i++) {
            String name = (String)args.get(i);
            String[] names = name.split(",");
            for (int j=0; j < names.length; j++) {
               if (names[j].length()==0) {
                  continue;
               }
               obj = getGEObj.invoke(jgdi, new Object[] {names[j]});
               if (obj == null) {
                  String text = names[j]+" is not known as "+type;
                  if (!errList.contains(text)) {
                     errList.add(text);
                  }
               } else {
                  if (!objList.contains(obj)) {
                     objList.add(obj);
                  }
               }
            }
         }
         //TODO LP Decide what behavior do we want
         //Finally display the output
         if (errList.size()>0) {
            for (Iterator iter = errList.iterator(); iter.hasNext();) {
               pw.println((String)iter.next());
               pw.flush();
            }
         } else {
            int i;
            for (Iterator iter = objList.iterator(); iter.hasNext();) {
               GEObject delObj = (GEObject) iter.next();
               deleteGEObj.invoke(jgdi, new Object[] {delObj});
               try {
                  pw.println(jgdi.getAdminUser()+"@"+java.net.InetAddress.getLocalHost().getHostName()+" removed \""+delObj.getName()+"\" from "+type.toLowerCase()+" list");
                  pw.flush();
               } catch (UnknownHostException ex) {
                  ex.printStackTrace();
               }
            }
         }
      } catch (ClassNotFoundException ex) {
         ex.printStackTrace();
      } catch (SecurityException ex) {
         ex.printStackTrace();
      } catch (NoSuchMethodException ex) {
         ex.printStackTrace();
      } catch (IllegalAccessException ex) {
         ex.printStackTrace();
      } catch (InvocationTargetException ex) {
         ex.printStackTrace();
      } catch (JGDIException ex) {
         pw.println(ex.getMessage());
      } finally {
         pw.flush();
      }
   }
   
   String getTextFromFile(final List args, final PrintWriter pw) {
      if (args.size() <= 0) {
         pw.println("no file argument given");
         pw.flush();
         return null;
      }
      if (args.size() != 1) {
         pw.println("error: invalid option argument \"" + args.get(1) + "\"");
         pw.println("Usage: qconf -help");
         pw.flush();
         return null;
      }
      String fileName = (String)args.get(0);
      String inputText = null;
      try {
         inputText = readFile(fileName);
      } catch (IOException ex) {
         pw.println("error: error opening file \""+fileName+"\" for reading: "+ex.getMessage());
         pw.println("error reading in file");
         pw.flush();
      }
      return inputText;
   }
   
   private String readFile(final String fileName) throws IOException {
      File f = new File(fileName);
      long fileSize = f.length();
      FileReader fr = new FileReader(f);
      StringBuffer sb = new StringBuffer();
      char[] buff = new char[2048];
      int r;
      while (fr.ready()) {
         r = fr.read(buff);
         if (r > 0) {
            sb.append(buff, 0, r);
         }
      }
      //Check we have whole content
      if (sb.length() != fileSize) {
         throw new IOException("Unable to read whole file content. Filesize is "+fileSize+" got only "+sb.length());
      }
      return sb.toString();
   }
   
   private List getNameList(final List objList) {
      List nameList = new ArrayList();
      Object obj;
      Class cls;
      String name;
      for (Iterator iter = objList.iterator(); iter.hasNext(); ) {
         obj = iter.next();
         Method getName;
         try {
            getName = obj.getClass().getDeclaredMethod("getName", null);
            name = (String) getName.invoke(obj, null);
            nameList.add(name);
         } catch (SecurityException ex) {
            ex.printStackTrace();
         } catch (NoSuchMethodException ex) {
            ex.printStackTrace();
         } catch (IllegalArgumentException ex) {
            ex.printStackTrace();
         } catch (IllegalAccessException ex) {
            ex.printStackTrace();
         } catch (InvocationTargetException ex) {
            ex.printStackTrace();
         }
      }
      return nameList;
   }
   
   private void adjustListValues(final String type, List list) {
      if (type.equals("ExecHost")) {
         String name;
         int i=0;
         while (i < list.size()) {
            name = (String) list.get(i);
            if (name.equals("global") || name.equals("template")) {
               list.remove(i);
            } else {
               i++;
            }
         }
      }
   }
   
   private String getKeyAttributeValueFromString(final PrintWriter pw, final String type, final String fileName, final String inputText) {
      String keyAttrValue = null;
      //Get the key attribute value form the file
      String keyAttr = EditorUtil.unifyAttrWithClientNames(type, "name");
      StringReader sr = new StringReader(inputText);
      LineNumberReader lnr = new LineNumberReader(sr);
      String keyAttrLine = null;
      try {
         while (lnr.ready()) {
            keyAttrLine = lnr.readLine().trim();
            if (keyAttrLine.startsWith(keyAttr)) {
               keyAttrValue = keyAttrLine.substring(keyAttr.length()).trim();
               break;
            }
         }
         //Exit if the key attribute is missing
         if (keyAttrValue == null) {
            pw.println("error: required attribute \""+keyAttr+"\" is missing");
            pw.println(type+" file \""+fileName+"\" is not correct");
            pw.flush();
         }
      } catch (IOException ex) {
         pw.println(ex.getMessage());
         pw.flush();
      }
      return keyAttrValue;
   }
   
   void printListSortedByName(List list, final List args, final PrintWriter pw) {
      list = getNameList(list);
      Collections.sort(list);
      printList(list, args, pw);
   }
   
   private void printList(List list, final List args, final PrintWriter pw) {
      for (Iterator iter = list.iterator(); iter.hasNext(); ) {
         pw.println(iter.next());
      }
      pw.flush();
   }
   
   private String runJavaEditor(final String text) {
      TextEditor ted = new TextEditor(text);
      while (!ted.isDone()) {
         try {
            Thread.currentThread().sleep(1000);
         } catch (InterruptedException ex) {
            ex.printStackTrace();
         }
      }
      return ted.getText();
   }
   
   String runEditor(final String text) {
      String editor;
      String version = System.getProperty("java.specification.version");
      //TODO LP <1.5 doesn't list properties like EDITOR...
      if (Double.parseDouble(version) < 1.5) {
         editor = System.getProperty("EDITOR");
      } else {
         editor = System.getenv("EDITOR");
      }
      //System.out.println("Version = "+version+" prop=\""+System.getProperty("EDITOR")+"\" editor=\""+editor+"\"");
      if (editor == null) {
         return runJavaEditor(text);
      }
      StringBuffer sb = new StringBuffer();
      try {
         File f = File.createTempFile("edit", null, new File("/tmp"));
         FileWriter fw = new FileWriter(f);
         fw.write(text);
         fw.flush();
         fw.close();
         
         Object[] cmds;
         //TODO LP Think about this. Vi has to be execed in tty. Graphic editors not.
         if (editor.equalsIgnoreCase("vi") || editor.equalsIgnoreCase("vim")) {
            List list = Arrays.asList( new String[] {/*"xterm", "-e",*/ editor, f.getAbsolutePath()});
            cmds = list.toArray();
         } else {
            List list = Arrays.asList(new String[] {editor, f.getAbsolutePath()});
            cmds = list.toArray();
         }
         
         Process p = Runtime.getRuntime().exec((String[])cmds/*, new String[] {"TERM=xterm"}*/);
         int exitCode = p.waitFor();
         if (exitCode != 0) {
            return null;
         }
         char[] buff = new char[1024];
         FileReader fr = new FileReader(f);
         int readChars=0;
         while (fr.ready() && readChars != -1) {
            readChars = fr.read(buff);
            if (readChars > 0) {
               sb.append(buff,0,readChars);
            }
         }
      } catch (IOException ioe) {
         ioe.printStackTrace();
      } catch (InterruptedException ie) {
         ie.printStackTrace();
      }
      return sb.toString();
   }
   
   
   //TODO LP: Handle error messages in add, addFromFile ,.. using tghse functions. Take messages form ResourceBundle
    /*private boolean argCountPreCheckOK(final String type, final List args, final PrintWriter pw, int required, int optional, boolean hasFileArg) {
       boolean res = true;
       //-A, -M
       if (hasFileArg && args.size() == 0) {
          pw.println("no file argument given");
          res = false;
       }
       //-A, -M
       else if (required == 1 && args.size() > 1) {
          pw.println("error: invalid option argument \"" + args.get(1) + "\"");
          pw.println("Usage: qconf -help");
          res = false;
       //-s
       } else if (args.size() == 0) {
          pw.println("error: missing argument list");
          res = false;
       }
       pw.flush();
       return res;
    }
     
    private void argCountPostCheck(final String type, final List args, final PrintWriter pw, int required, int optional) {
       if (required < Integer.MAX_VALUE && optional != Integer.MAX_VALUE && args.size() > (required+optional) ) {
          pw.println("error: invalid option argument \"" + args.get(required + optional) + "\"");
          pw.println("Usage: qconf -help");
       }
       pw.flush();
    }*/
}
