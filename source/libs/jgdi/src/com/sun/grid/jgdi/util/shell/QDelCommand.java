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
import com.sun.grid.jgdi.jni.JGDIBase;
import java.io.PrintWriter;
import java.io.StringWriter;

/**
 *
 */
public class QDelCommand extends AbstractCommand {
    
    
    /** Creates a new instance of QModCommand */
    public QDelCommand(Shell shell, String name) {
        super(shell, name);
    }
    
    public String getUsage() {
        StringWriter sw = new StringWriter();
        PrintWriter pw = new PrintWriter(sw);
        pw.println("usage: qdel [options] job_task_list");
        pw.println("[-f]                               force action");
        pw.println("[-help]                            print this help");
        pw.println("[-u user_list]                     delete all jobs of users specified in list");
        pw.println();
        pw.println("job_task_list                      delete all jobs given in list");
        pw.println("job_task_list  job_tasks[ job_tasks[ ...]]");
        pw.println("job_tasks      {job_id[.task_id_range]|job_name|pattern}[ -t task_id_range]");
        pw.println("task_id_range  task_id[-task_id[:step]]");
        pw.println("user_list      {user|pattern}[,{user|pattern}[,...]]");
        return sw.getBuffer().toString();
    }
    
    
    public void run(String[] args) throws Exception {
        
        JGDI jgdi = getShell().getConnection();
        
        if (jgdi == null) {
            throw new IllegalStateException("Not connected");
        }
        if (args.length == 0) {
            throw new IllegalArgumentException("Invalid number of arguments");
        }
        
        
        boolean force = false;
        
        for (int i = 0; i < args.length; i++) {
            if (args[i].equals("-f")) {
                System.out.println("-f option parsed");
                break;
            } else {
                throw new IllegalArgumentException("Unknown or not implemented option " + args[i]);
            }
        }
    }
    
    
    private String [] parseDestinIdList(String arg) {
        String [] ret = arg.split(" ");
        return ret;
    }
    
    
    
}
