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
package com.sun.grid.installer.util.cmd;

import com.izforge.izpack.util.Debug;
import com.sun.grid.installer.util.Util;
import com.sun.grid.installer.gui.Host;
import java.text.MessageFormat;
import java.util.Vector;

/**
 * Resolves the given host name/ip address
 */
public class ResolveHostCommand extends CmdExec {
    // SGE_ROOT/utilbin/ARCH/(gethostbyaddr | gethostbyname) host
    private static final String resolveCommand = "SGE_ROOT={0} ; export SGE_ROOT ; {0}/utilbin/{1}/{2} -all {3}";

    private String hostName = "";
    private String hostAddress = "";

    private String command = "";

    public ResolveHostCommand() {
        super(Util.DEF_RESOLVE_TIMEOUT);
    }
    
    public ResolveHostCommand(long timeout, String host, String sgeRoot) {
        super(timeout);

        if (Util.isIpPattern(host)) {
            command = MessageFormat.format(resolveCommand, sgeRoot, Host.localHostArch, "gethostbyaddr", host);
            hostName = host;
            hostAddress = host;
        } else {
            command = MessageFormat.format(resolveCommand, sgeRoot, Host.localHostArch, "gethostbyname", host);
            hostName = host;
            hostAddress = host;
        }
    }

    @Override
    public void execute(String commands) {
        throw new UnsupportedOperationException("Use execute() instead!");
    }
    
    public void execute() {
        super.execute(command);

        if (getExitValue() == EXIT_VAL_SUCCESS) {
            Vector<String> output = getOutput();

            try {
                /**
                 * Output:
                 * 
                 * Hostname: <host name>
                 * SGE name: <host name>
                 * Aliases: <not used>
                 * Host Address(es): <host addresses seperated by spaces(???), first is used>
                 */
                hostName = output.get(1).split(":")[1].trim(); // SGE name
                hostAddress = output.lastElement().split(":")[1].trim().split(" ")[0]; // Host Address(es)
            } catch (Exception e) {
                Debug.error("Can not parse the output of '" + command + "'. Out");
            }
        }
    }

    public String getHostName() {
        return hostName;
    }

    public String getHostAddress() {
        return hostAddress;
    }
}
