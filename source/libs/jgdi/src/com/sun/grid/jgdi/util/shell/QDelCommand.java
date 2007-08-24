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
import com.sun.grid.jgdi.util.JGDIShell;

/**
 *
 */
public class QDelCommand extends AbstractCommand {
    
    
    /** Creates a new instance of QModCommand */
    public QDelCommand(Shell shell, String name) {
        super(shell, name);
    }
    
   public String getUsage() {
      return JGDIShell.getResourceString("sge.version.string") + "\n" + 
             JGDIShell.getResourceString("usage.qdel");
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
            if (args[i].equals("-help")) {
                System.out.println(getUsage());
                break;
            } else if (args[i].equals("-f")) {
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
