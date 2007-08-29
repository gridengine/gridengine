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

import com.sun.grid.jgdi.configuration.JGDIAnswer;
import java.util.ArrayList;
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
   static OptionInfo getOptionInfo(final Map<String, OptionDescriptor> optMap, List<String> args) {
      //Check we have a map set
      if (optMap.isEmpty()) {
         throw new UnsupportedOperationException("Cannot get OptionInfo from the abstract class CommandOption directly!");
      }
      String option = args.remove(0);
      String msg;
      if (!optMap.containsKey(option)) {
         if (option.startsWith("-")) {
            msg = "error: unknown option \""+option+"\"\nUsage: qconf -help";
         } else {
            msg = "error: invalid option argument \"" + option + "\"\nUsage: qconf -help";
         }
         throw new IllegalArgumentException(msg);
      }
      OptionDescriptor od = optMap.get(option);
      List<String> argList = new ArrayList<String>();
      String arg;
      
      if (!od.isWithoutArgs()) {
         int i=0;
         //Try to ge all mandatory args
         while (i<od.getMandatoryArgCount() && args.size() > 0) {
            arg = args.remove(0);
            argList.add(arg);
            i++;
         }
         //Check we have all mandatory args
         if (i != od.getMandatoryArgCount()) {
            throw new IllegalArgumentException("Expected "+od.getMandatoryArgCount()+
                     " arguments for "+option+" option. Got only "+argList.size()+".");
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
      
      //Check if we have more args than expected
      if (args.size() > od.getMaxArgCount()) {
         msg = "Expected only "+od.getMaxArgCount()+" arguments for "+option+" option. Got "+argList.size()+".";
         throw new IllegalArgumentException(msg);
      }
      
      boolean hasNoArgs = (od.getMandatoryArgCount() == 0 && od.getOptionalArgCount() == 0);
      if (hasNoArgs) {
         return new OptionInfo(od, optMap);
      }
      //TODO: Check we have correct args
      return new OptionInfo(od, argList, optMap);
   }
   
   /**
     * <p>Prints the JGDI answer list to specified PrintWriter.</p>
     * <p>Helper method for JGDI methods *withAnswer</p>
     */
    public int printAnswers(java.util.List<JGDIAnswer> answers, java.io.PrintWriter pw) {
       int exitCode = 0;
       int status;
       for (JGDIAnswer answer : answers) {
          status = answer.getStatus(); 
          if ( status != 0) {
             exitCode = status;
          }
          pw.println(answer.getText());
       }
       return exitCode;
    }
}
