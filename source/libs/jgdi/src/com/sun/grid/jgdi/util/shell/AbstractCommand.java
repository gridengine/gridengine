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
import com.sun.grid.jgdi.configuration.JGDIAnswer;
import java.io.PrintWriter;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 *
 */
public abstract class AbstractCommand implements HistoryCommand {
   Shell shell=null;
   JGDI jgdi = null;
   PrintWriter pw=null;
   
   public Logger getLogger() {
      return shell.getLogger();
   }
   
   public Shell getShell() {
      return shell;
   }
   
   public void init(Shell shell) throws Exception {
      this.shell=shell;  
      this.jgdi = shell.getConnection();
      this.pw = shell.getPrintWriter();
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
    * <p>Prints the JGDI answer list to specified PrintWriter.</p>
    * <p>Helper method for JGDI methods *withAnswer</p>
    * @param answers a JGDI answer list
    * @return an int exit code
    */
    public int printAnswers(java.util.List<JGDIAnswer> answers) {
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
