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
import java.io.PrintWriter;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;


/**
 * OptionInfo class holds information about the command option
 */
public class OptionInfo {
   private List<String> args=new ArrayList<String>();
   private OptionDescriptor od=null;
   private Map<String, OptionDescriptor> map=null;
   private boolean optionDone=false;
  
   /**
    * Constructs OptionInfo class
    * @param od 
    * @param args {@link List} of arguments for the current option
    * @param map 
    */
   public OptionInfo(OptionDescriptor od, List<String> args, Map<String, OptionDescriptor> map) {
      this.od = od;
      this.args = new ArrayList<String>(args); // TODO consider not to copy all of this
      this.map = map;
   }
   
   /**
    * Constructs OptionInfo class
    * @param od tho option descriptor class
    * @param map a map of
    */
   public OptionInfo(OptionDescriptor od, Map<String, OptionDescriptor> map) {
      this(od, new ArrayList<String>(), map);
   }
   
   /**
    * Invokes the method for current option in a loop until all argumetns 
    * are processed.
    * NOTE: Use getFirstArg() and optionDone() to avoid infinite loop.
    * @see OptionInfo#getFirstArg() OptionInfo#optionDone() {@link OptionInfo}
    * @throws java.lang.Exception 
    */
   public void invokeOption(Command command) throws Exception {
      //Invoke it
      do {
         try {
            final Method method = getMethod();
            List<OptionInfo> objList = new ArrayList<OptionInfo>();
            objList.add(this);
            method.invoke(command, objList.toArray());
         } catch (Exception ex) {
            Throwable cause = ex.getCause();
            if (cause != null && cause instanceof JGDIException) {
                PrintWriter pw = od.getPw();
                String[] elems = ((JGDIException)cause).getMessage().split("\n");
                String msg = "";
                for (String elem : elems) {
                    if (elem.trim().length() > 0) {
                        msg += elem + "\n";
                    }
                }
                pw.print(msg);
                pw.flush();
                continue;
            }
            // Rethrow the generated Exception, end the option execution if not JGDIException
            optionDone();
            if (cause != null && cause instanceof Exception) {
                throw (Exception) cause;
             } else {    
                throw ex;
             }
         }
      } while (!isOptionDone() && getOd().isMultiple());
   }
   
   /**
    * Gets the arguments
    * @return {@link List} of option arguments
    */
   public List<String> getArgs() {
      List<String> ret = new ArrayList<String>(args);
      optionDone();
      return ret;
   }
   
   /**
    * Gets the arguments
    * @return {@link List} of option arguments
    */
   public String getArgsAsString() {
      if (args.size() == 0) {
         return "";
      }
      StringBuilder sb = new StringBuilder();
      for(String arg: args){
          sb.append(arg);
          sb.append(",");
      }
      // Return string without the last comma
      optionDone();
      return sb.deleteCharAt(sb.length()-1).toString();
   }
   /**
    * Getter method
    * @return value
    */
   public Method getMethod() {
      return getOd().getMethod();
   }

   /**
    * Getter method
    * @return value
    */
   public OptionDescriptor getOd() {
      return od;
   }
   
   /**
    * Contract method.
    * Terminates option loop. Should be called at the at of every option.
    * @see #getFirstArg()
    */
   public void optionDone(){
       optionDone=true;
   }

   /**
    * Contract method. 
    * Developer should use it to retreave option arguments one-by-one. So the 
    * option loop is terminated.
    * @return String - first argument of the option or null if no arg
    */
   public String getFirstArg() {
      if (args == null || args.size() == 0) { 
         return null;
      }
      final String ret = args.remove(0);
      if(args.isEmpty()) {
          optionDone();
      }
      return ret;
   }
   
   /**
    * Contract method. 
    * Developer should use it to retreave look at the last argument. So the 
    * option loop is terminated.
    * @return String - last argument of the option or null if no arg
    */
   public String showLastArg() {
      if (args == null || args.size() == 0) { 
         return null;
      }
      final String ret = args.get(args.size()-1);
      if (args.size() ==1) {
          optionDone();
      }
      return ret;
   }

   /**
    * Getter method
    * @return value
    */
   public Map<String, OptionDescriptor> getMap() {
      return map;
   }

   public boolean isOptionDone() {
      return optionDone;
   }
}
