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

import com.sun.grid.jgdi.JGDIException;
import com.sun.grid.jgdi.configuration.JGDIAnswer;
import com.sun.grid.jgdi.util.shell.editor.EditorUtil;
import com.sun.grid.jgdi.util.shell.editor.TextEditor;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.LineNumberReader;
import java.io.PrintWriter;
import java.io.StringReader;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 *
 */
public abstract class AbstractCommand implements HistoryCommand {
   
   final Shell shell;
   private String name;
   
   /** Creates a new instance of AbstractCommand */
   protected AbstractCommand(Shell shell, String name) {
      this.shell = shell;
      this.name = name;
   }
   
   public String getName() {
      return name;
   }
   
   public Logger getLogger() {
      return shell.getLogger();
   }
   
   public Shell getShell() {
      return shell;
   }
   
   public String[] parseWCQueueList(String arg) {
      String [] ret = arg.split(",");
      if(getLogger().isLoggable(Level.FINE)) {
         StringBuffer buf = new StringBuffer();
         buf.append("wc_queue_list [");
         for(int i = 0; i < ret.length; i++) {
            if(i>0) {
               buf.append(", ");
            }
            buf.append(ret[i]);
         }
         buf.append("]");
         getLogger().fine(buf.toString());
      }
      return ret;
   }
   
   public String[] parseJobWCQueueList(String arg) {
      String [] ret = arg.split(",");
      if(getLogger().isLoggable(Level.FINE)) {
         StringBuffer buf = new StringBuffer();
         buf.append("job_wc_queue_list [");
         for(int i = 0; i < ret.length; i++) {
            if(i>0) {
               buf.append(", ");
            }
            buf.append(ret[i]);
         }
         buf.append("]");
         getLogger().fine(buf.toString());
      }
      return ret;
   }

   public String[] parseJobList(String arg) {
      String [] ret = arg.split(",");
      if(getLogger().isLoggable(Level.FINE)) {
         StringBuffer buf = new StringBuffer();
         buf.append("job_list [");
         for(int i = 0; i < ret.length; i++) {
            if(i>0) {
               buf.append(", ");
            }
            buf.append(ret[i]);
         }
         buf.append("]");
         getLogger().fine(buf.toString());
      }
      return ret;
   }
   

    /**
    * Gets option info. Returns correct Option object and all its arguments
    * Default behavior: 1) Read all mandatory args. Error if not complete
    *                   2) Read max optional args until end or next option found
    * Arguments have to be already expanded
    * @param optMap {@link Map} holding all options for current command.
    */
   //TODO LP: Discuss this enhancement. We now accept "arg1,arg2 arg3,arg4" as 4 valid args
   static OptionInfo getOptionInfo(final Map optMap, List args) {
      //Check we have a map set
      if (optMap.isEmpty()) {
         throw new UnsupportedOperationException("Cannot get OptionInfo from the abstract class CommandOption directly!");
      }
      String option = (String) args.remove(0);
      if (!optMap.containsKey(option)) {
         String msg;
         if (option.startsWith("-")) {
            msg = "error: unknown option \""+option+"\"\nUsage: qconf -help";
         } else {
            msg = "error: invalid option argument \"" + option + "\"\nUsage: qconf -help";
         }
         throw new OptionFormatException(msg);
      }
      OptionDescriptor od = (OptionDescriptor) optMap.get(option);
      List argList = new ArrayList();
      String arg;
      //TODO LP: Move the argument checking here
      if (!od.isWithoutArgs()) {
         int i=0;
         //Try to ge all mandatory args
         while (i<od.getMandatoryArgCount() && args.size() > 0) {
            arg = (String) args.remove(0);
            argList.add(arg);
            i++;
         }
         //Check we have all mandatory args
         if (i != od.getMandatoryArgCount()) {
            throw new OptionFormatException("Expected "+od.getMandatoryArgCount()+
                     " arguments for "+option+" option. Got only "+argList.size());
         }
         //Try to get as many optional args as possible
         i=0;
         while (i<od.getOptionalArgCount() && args.size() > 0) {
            arg = (String) args.remove(0);      
            //Not enough args?
            if (optMap.containsKey(arg)) {
               args.add(0,arg);
               break;
            }
            argList.add(arg);
            i++;
         }
      }
      boolean hasNoArgs = (od.getMandatoryArgCount() == 0 && od.getOptionalArgCount() == 0);
      if (hasNoArgs) {
         return new OptionInfo(od, optMap);
      }
      //TODO: Check we have correct args
      return new OptionInfo(od, argList, optMap);
   }
   
   String getKeyAttributeValueFromString(final PrintWriter pw, final String type, final String fileName, final String inputText) {
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
            pw.println("error: required attribute \"" + keyAttr + "\" is missing");
            pw.println(type + " file \"" + fileName + "\" is not correct");
            pw.flush();
         }
      } catch (IOException ex) {
         pw.println(ex.getMessage());
         pw.flush();
      }
      return keyAttrValue;
   }
   
   /**
     * <p>Prints the JGDI answer list to specified PrintWriter.</p>
     * <p>Helper method for JGDI methods *withAnswer</p>
     */
    public void printAnswers(java.util.List<JGDIAnswer> answers, java.io.PrintWriter pw) {
       for (JGDIAnswer answer : answers) {
          pw.println(answer.getText());
       }
    }
   
   String readFile(final String fileName) throws JGDIException {
      File f = new File(fileName);
      long fileSize = f.length();
      FileReader fr;
      StringBuffer sb = new StringBuffer();
      try {
         fr = new FileReader(f);
         char[] buff = new char[2048];
         int r;
         while (fr.ready()) {
            r = fr.read(buff);
            if (r > 0) {
               sb.append(buff, 0, r);
            }
         }
      } catch (IOException ex) {
         throw new JGDIException(ex.getMessage());
      }
      //Check we have whole content
      if (sb.length() != fileSize) {
         throw new JGDIException("Unable to read whole file content. Filesize is " + fileSize + " got only " + sb.length());
      }
      return sb.toString();
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
            List list = Arrays.asList(new String[]{editor, f.getAbsolutePath()});
            cmds = list.toArray();
         } else {
            List list = Arrays.asList(new String[]{editor, f.getAbsolutePath()});
            cmds = list.toArray();
         }

         Process p = Runtime.getRuntime().exec((String[]) cmds);
         int exitCode = p.waitFor();
         if (exitCode != 0) {
            return null;
         }
         char[] buff = new char[1024];
         FileReader fr = new FileReader(f);
         int readChars = 0;
         while (fr.ready() && readChars != -1) {
            readChars = fr.read(buff);
            if (readChars > 0) {
               sb.append(buff, 0, readChars);
            }
         }
      } catch (IOException ioe) {
         ioe.printStackTrace();
      } catch (InterruptedException ie) {
         ie.printStackTrace();
      }
      return sb.toString();
   }
}
