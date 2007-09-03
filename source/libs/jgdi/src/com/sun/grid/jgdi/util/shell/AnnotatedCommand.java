/*___INFO__MARK_BEGIN__*/ /*************************************************************************
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
import java.lang.annotation.Annotation;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;


/**
 *
 */
public abstract class AnnotatedCommand extends AbstractCommand {

   /* Map holding all qconf options and method that should be invoked for it */
   private Map<String, OptionDescriptor> optMap = null;

   /**
    * Initialize the option map optMap if not yet created.
    * Map is created by scanning all OptionMethod annotated functions.
    * NOTE: Only options in the map will be recognized as implemented
    * @return 
    */
   public Map<String, OptionDescriptor> getOptMap() {
      if (optMap == null) {
         optMap = new HashMap<String, OptionDescriptor>(140);

         for (Method m : this.getClass().getMethods()) {
            for (Annotation a : m.getDeclaredAnnotations()) {
               if (a instanceof OptionAnnotation) {
                  OptionAnnotation o = (OptionAnnotation) a;
                  //Add method to the optMap
                  addSingleMethod(o.value(), o.min(), o.extra(), this, m);
               }
            }
         }
      }

      return optMap;
   }
   
   /**
    * Parse all arguments and invoke appropriate method
    * It call Annotated functions for every recognized option.
    * @param args command line options
    * @throws java.lang.Exception en exception during the option call
    * or some runable exception, when the options are wrong
    */
   protected void parseArgsInvokeOptions(String[] args) throws Exception {
      List<String> argList = parseArgs(args);

      while (!argList.isEmpty()) {
         //Get option info, token are divided by known option
         //to option and reladet arguments
         // The arguments are tested for declared multiplicity  
         OptionInfo info = getOptionInfo(getOptMap(), argList);
         //At the end the option handling function is called
         info.invokeOption();
      }
   }

   /**
    * Parse the ergument array to Array list of separated tokens
    * The tokens are then divided by known options
    * @param args command line option
    * @return List of tokens
    */
   protected List<String> parseArgs(String[] args) {
      ArrayList<String> argList = new ArrayList<String>();
      //Expand args to list of args, 'arg1,arg2 arg3' -> 3 args
      for (String arg : args) {
         String[] subElems = arg.split("[,]");
         for (String subElem : subElems) {
            subElem = subElem.trim();
            if (subElem.length() > 0) {
               argList.add(subElem);
            }
         }
      }
      return argList;
   }   
   
   @OptionAnnotation(value = "-list", min=0)
   public void listOptions(final OptionInfo oi) throws JGDIException {
      String str = new String();
      Set<String> set = oi.getMap().keySet();
      String[] options = oi.getMap().keySet().toArray(new String[set.size()]);
      Arrays.sort(options);
      for (String option : options) {
         pw.println(option);
      }
      //to stop further processing
      throw new IllegalArgumentException("");
   }   
   
   /**
    * Gets option info. Returns correct Option object and all its arguments
    * Default behavior: 1) Read all mandatory args. Error if not complete
    *                   2) Read max optional args until end or next option found
    * Arguments have to be already expanded
    * @param optMap {@link Map} holding all options for current command.
    * @param args argument list
    * @return return the option info structure
    */
   //TODO LP: Discuss this enhancement. We now accept "arg1,arg2 arg3,arg4" as 4 valid args
   static OptionInfo getOptionInfo(final Map<String, OptionDescriptor> optMap, List<String> args) {
      //Check we have a map set
      if (optMap.isEmpty()) {
         throw new UnsupportedOperationException("Cannot get OptionInfo from the abstract class CommandOption directly!");
      }

      String option = args.remove(0);
      String msg;

      if (!optMap.containsKey(option)) {
         if (option.startsWith("-")) {
            msg = "error: unknown option \"" + option + "\"\nUsage: qconf -help";
         } else {
            msg = "error: invalid option argument \"" + option + "\"\nUsage: qconf -help";
         }

         throw new IllegalArgumentException(msg);
      }

      OptionDescriptor od = optMap.get(option);
      List<String> argList = new ArrayList<String>();
      String arg;

      if (!od.isWithoutArgs()) {
         int i = 0;

         //Try to ge all mandatory args
         while ((i < od.getMandatoryArgCount()) && (args.size() > 0)) {
            arg = args.remove(0);
            argList.add(arg);
            i++;
         }

         //Check we have all mandatory args
         if (i != od.getMandatoryArgCount()) {
            throw new IllegalArgumentException("Expected " + od.getMandatoryArgCount() + " arguments for " + option + " option. Got only " + argList.size() + ".");
         }

         //Try to get as many optional args as possible
         i = 0;

         while ((i < od.getOptionalArgCount()) && (args.size() > 0)) {
            arg = args.remove(0);

            //Not enough args?
            if (optMap.containsKey(arg)) {
               args.add(0, arg);
               break;
            }

            argList.add(arg);
            i++;
         }
      }

      //Check if we have more args than expected
      if (argList.size() > od.getMaxArgCount()) {
         msg = "Expected only " + od.getMaxArgCount() + " arguments for " + option + " option. Got " + argList.size() + ".";
         throw new IllegalArgumentException(msg);
      }

      boolean hasNoArgs = (od.getMandatoryArgCount() == 0) && (od.getOptionalArgCount() == 0);

      if (hasNoArgs) {
         return new OptionInfo(od, optMap);
      }

      //TODO: Check we have correct args
      return new OptionInfo(od, argList, optMap);
   }

   void addSingleMethod(String optionStr, int mandatory, int optional, AbstractCommand option, Method method) {
      try {
         optMap.put(optionStr, new OptionDescriptor(mandatory, optional, option, method));
      } catch (Exception ex) {
         throw new ExceptionInInitializerError(new Exception("<QConfCommand> failed: " + ex.getMessage()));
      }
   }
   
}