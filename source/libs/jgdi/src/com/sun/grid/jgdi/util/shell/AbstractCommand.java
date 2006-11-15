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

import java.util.logging.Level;
import java.util.logging.Logger;

/**
 *
 * @author rh150277
 */
public abstract class AbstractCommand implements HistoryCommand {
   
   private Shell shell;
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
   
}
