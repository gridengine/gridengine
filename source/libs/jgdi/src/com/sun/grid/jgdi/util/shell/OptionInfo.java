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
   private JGDI jgdi=null;
   private PrintWriter pw=null;
   private Map<String, OptionDescriptor> map=null;
  
   /**
    * Constructs OptionInfo class
    * @param option Instance of {@link CommandOption} or its child
    * @param type {@link String} name of the JGDI object that the option is bind to.
    *             May not be neccessary to be set for option extending {@link CommandOption}
    * @param args {@link List} of arguments for the current option
    * @param cls {@link Class} of the option object
    * @param method {@link Method} of the {@link Class} cls to run
    */
   public OptionInfo(OptionDescriptor od, List<String> args, Map<String, OptionDescriptor> map) {
      this.od = od;
      this.args = new ArrayList<String>(args);
      this.map = map;
   }
   
   /**
    * Constructs OptionInfo class
    * @param option Instance of {@link CommandOption} or its child
    * @param type {@link String} name of the JGDI object that the option is bind to.
    *             May not be neccessary to be set for option extending {@link CommandOption}
    * @param hasNoArgs True if option has no arguments
    * @param cls {@link Class} of the option object
    * @param method {@link Method} of the {@link Class} cls to run
    */
   public OptionInfo(OptionDescriptor od, Map<String, OptionDescriptor> map) {
      this(od, new ArrayList<String>(), map);
   }
   
   /**
    * Invokes the method for current option in a loop until all argumetns 
    * are processed.
    *  NOTE: Use getFirstArg() and optionDone() to avoid infinite loop.
    * @see getFirstArg(), optionDone(), @link {OptionMethod}
    * @param jgdi {@link JGDI} instance
    * @param pw {@link PrintWriter} instance
    */
   public void invokeOption(JGDI jgdi, PrintWriter pw) {
      List<OptionInfo> objList = new ArrayList<OptionInfo>();
      this.pw=pw;
      this.jgdi=jgdi;
      objList.add(this);

      //Invoke it
      do  {
         try {
            getMethod().invoke(getOd().getCommand(), objList.toArray());
         } catch (Exception ex) {
            Throwable cause = ex.getCause();
            if (cause != null && cause instanceof JGDIException) {
               pw.print(cause.getMessage());
            } else {
               ex.printStackTrace();
            }
         } finally {
             pw.flush();
         }
      }  while(!getArgs().isEmpty() && getOd().isMultiple());      
   }
   
   /**
    * Getter method
    * @return {@link AbstractCommand} object
    */
   public AbstractCommand getCommand() {
      return getOd().getCommand();
   }
   
   /**
    * Gets the arguments
    * @return {@link List} of option arguments
    */
   public List<String> getArgs() {
      return args;
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
   public JGDI getJgdi() {
      return jgdi;
   }

   /**
    * 
    * @return 
    */
   public PrintWriter getPw() {
      return pw;
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
    * @see getFirstArg()
    */
   public void optionDone(){
       args.clear();
   }

   /**
    * Contract method. 
    * Developer should use it to retreave option arguments one-by-one. So the 
    * option loop is terminited.
    * @return String - first argument of the option or null if no arg
    */
   public String getFirstArg() {
      if (args == null || args.size() == 0) {
         return null;
      }
      return getArgs().remove(0);
   }

   /**
    * Getter method
    * @return value
    */
   public Map<String, OptionDescriptor> getMap() {
      return map;
   }
}
