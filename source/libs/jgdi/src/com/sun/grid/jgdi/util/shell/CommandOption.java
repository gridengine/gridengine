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

import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * Abstract class representing single command option
 */
abstract class CommandOption {

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
         return new OptionInfo(od.getOption(), od.getType(), true, od.getCls(), od.getMethod());
      }
      return new OptionInfo(od.getOption(), od.getType(), argList, od.getCls(), od.getMethod());
   }
}

class OptionDescriptor {
   private CommandOption option;
   private int mandatoryArgCount;
   private int optionalArgCount;
   private String type;
   private Class cls;
   private Method method;

   public OptionDescriptor(String type, int mandatory, int optional, Class cls, Method method) throws InstantiationException,IllegalAccessException  {
      this.type = type;
      mandatoryArgCount = mandatory;
      optionalArgCount = optional;
      this.cls = cls;
      this.method = method;
      Object o;
      o = cls.newInstance();
      if (!(o instanceof CommandOption)) {
         throw new IllegalArgumentException(cls.getName() + " not an instance of CommandOption class!");
      }
      option = (CommandOption)o;
   }
   
   public int getMandatoryArgCount() {
      return mandatoryArgCount;
   }
   
   public int getOptionalArgCount() {
      return optionalArgCount;
   }
   
   public String getType() {
      return type;
   }
   
   public boolean isWithoutArgs() {
      return (mandatoryArgCount == 0 && optionalArgCount == 0) ? true : false;
   }
   
   public CommandOption getOption() {
      return option;
   }

   public Class getCls() {
      return cls;
   }

   public Method getMethod() {
      return method;
   }
}

class OptionFormatException extends IllegalArgumentException {
   public OptionFormatException(String msg) {
      super(msg);
   }
}

/**
 * Simple formatting class
 */
class Format {
   /** Adds (n - str.length) spaces to str */
   public static String left(String str, int n) {
      StringBuffer sb = new StringBuffer(str);
      for (int i=str.length(); i<n ; i++) {
         sb.append(' ');
      }
      return sb.toString();
   }
   public static String right(String str, int n) {
      StringBuffer sb = new StringBuffer();
      for (int i=str.length(); i<n ; i++) {
         sb.append(' ');
      }
      sb.append(str);
      return sb.toString();
   }
}