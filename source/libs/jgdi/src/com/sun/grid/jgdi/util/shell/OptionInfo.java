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
import java.io.PrintWriter;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;


/**
 * OptionInfo class holds information about the command option
 */
public class OptionInfo {
   private CommandOption option;
   private String type;
   private List args;
   private boolean hasNoArgs;
   private Class cls;
   private Method method;
  
   /**
    * Constructs OptionInfo class
    * @param option Instance of {@link CommandOption} or its child
    * @param type {@link String} name of the JGDI object that the option is bind to.
    *             May not be neccessary to be set for option extending {@link CommandOption}
    * @param args {@link List} of arguments for the current option
    * @param cls {@link Class} of the option object
    * @param method {@link Method} of the {@link Class} cls to run
    */
   public OptionInfo(CommandOption option, String type, List args, Class cls, Method method) {
      this.option = option;
      this.type = type;
      this.args = args;
      this.hasNoArgs = false;
      this.cls = cls;
      this.method = method;
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
   public OptionInfo(CommandOption option, String type, boolean hasNoArgs, Class cls, Method method) {
      this.option = option;
      this.type = type;
      this.hasNoArgs = hasNoArgs;
      this.args = null;
      this.cls = cls;
      this.method = method;
   }
   
   /**
    * Invoked the method for current option
    * @param jgdi {@link JGDI} instance
    * @param pw {@link PrintWriter} instance
    */
   public void invokeOption(JGDI jgdi, PrintWriter pw) {
      Class[] paramClass = method.getParameterTypes();
      List objList = new ArrayList();
      for (int i=0; i<paramClass.length; i++) {
         if (paramClass[i] == JGDI.class) {
            objList.add(jgdi);
         } else if (paramClass[i] == PrintWriter.class) {
            objList.add(pw);
         //Object name to be invoked "User", "SchedConf", etc.   
         } else if (i == 1 && paramClass[i] == String.class) {
            objList.add(type);
         } else if (paramClass[i] == List.class) {
            objList.add(args);
         } else throw new IllegalArgumentException("Unexpected class "+paramClass[i].getName()
                           +" for "+option.getClass().getName()+"."+method.getName()+"() argument!");
      }
      //Invoke it
      try {
         method.invoke(option, objList.toArray());
      } catch (IllegalArgumentException ex) {
         ex.printStackTrace();
      } catch (IllegalAccessException ex) {
         ex.printStackTrace();
      } catch (InvocationTargetException ex) {
         ex.printStackTrace();
      }
   }
   
   /**
    * Gets the option
    * @return {@link CommandOption} object
    */
   public CommandOption getOption() {
      return option;
   }
   
   /**
    * Gets the arguments
    * @return {@link List} of option arguments
    */
   public List getArgs() {
      return args;
   }
}
