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

import com.sun.grid.jgdi.JGDIException;
import com.sun.grid.jgdi.JGDIFactory;
import com.sun.grid.jgdi.monitoring.QHostOptions;
import com.sun.grid.jgdi.monitoring.QHostResult;
import com.sun.grid.jgdi.monitoring.QueueInstanceSummaryPrinter;
import com.sun.grid.jgdi.monitoring.filter.HostFilter;
import com.sun.grid.jgdi.monitoring.filter.ResourceAttributeFilter;
import com.sun.grid.jgdi.monitoring.filter.ResourceFilter;
import com.sun.grid.jgdi.monitoring.filter.UserFilter;
import java.util.LinkedList;
import java.util.List;
import static com.sun.grid.jgdi.util.JGDIShell.getResourceString;

/**
 *
 */
@CommandAnnotation(value = "qhost")
public class QHostCommand extends AbstractCommand {

    public String getUsage() {
        return JGDIFactory.getJGDIVersion() + "\n" + getResourceString("usage.qhost");
    }

    public void run(String[] args) throws Exception {

        List<String> argList = new LinkedList<String>();
        for (String arg : args) {
            argList.add(arg);
        }
        try {
            QHostOptions options = parse(argList);
            if (options != null) {
                QHostResult res = jgdi.execQHost(options);
                if (options.showAsXML()) {
                    /*TODO LP: -xml is not implemented for object other that GEObjects
                    we could use a JAXB and some generator to get the schema for other objects*/
                    pw.println("XML OUTPUT NOT IMPLEMENTED");
                } else {
                    QueueInstanceSummaryPrinter.print(pw, res, options);
                }
            }
        } catch (JGDIException ex) {
            pw.println(ex.getMessage());
        }
    }

    private QHostOptions parse(List<String> argList) throws Exception {
        ResourceAttributeFilter resourceAttributeFilter = null;
        ResourceFilter resourceFilter = null;
        boolean showQueues = false;
        boolean showJobs = false;
        boolean showAsXML = false;
        UserFilter userFilter = null;
        HostFilter hostFilter = null;

        while (!argList.isEmpty()) {
            String arg = argList.remove(0);

            if (arg.equals("-help")) {
                pw.println(getUsage());
                return null;
            } else if (arg.equals("-h")) {
                if (argList.isEmpty()) {
                    pw.println("error: ERROR! -h option must have argument");
                    return null;
                }
                arg = argList.remove(0);
                //TODO LP: Qmaster should check if the value exists and is correct not the client
                //E.g.: qhost -h dfds -> qmaster should try to resolve the host a return error message
                hostFilter = HostFilter.parse(arg);
            } else if (arg.equals("-F")) {
                if (!argList.isEmpty()) {
                    arg = argList.get(0);
                    // we allow only a comma separated arg string
                    // qhost CLI allows also whitespace separated arguments
                    if (!arg.startsWith("-")) {
                        arg = argList.remove(0);
                        resourceAttributeFilter = ResourceAttributeFilter.parse(arg);
                    } else {
                        resourceAttributeFilter = new ResourceAttributeFilter();
                    }
                } else {
                    resourceAttributeFilter = new ResourceAttributeFilter();
                }
            } else if (arg.equals("-j")) {
                showJobs = true;
            } else if (arg.equals("-l")) {
                if (argList.isEmpty()) {
                    pw.println("error: ERROR! -l option must have argument");
                    return null;
                }
                resourceFilter = new ResourceFilter();
                arg = argList.remove(0);
                try {
                    //TODO LP: Qmaster should check if the value exists and is correct not the client
                    //E.g.: qhost -l bal=34 -> qmaster should say bla does not exist
                    //E.g.: qhost -l swap_total -> qmaster - no value to swap_total
                    resourceFilter = ResourceFilter.parse(arg);
                } catch (IllegalArgumentException ex) {
                    pw.println("error: " + ex.getMessage());
                    return null;
                }
            } else if (arg.equals("-q")) {
                showQueues = true;
            } else if (arg.equals("-u")) {
                if (argList.isEmpty()) {
                    pw.println("error: ERROR! -u option must have argument");
                    return null;
                }
                arg = argList.remove(0);
                userFilter = UserFilter.parse(arg);
                showJobs = true;
            } else if (arg.equals("-xml")) {
                showAsXML = true;
            } else {
                pw.print(getUsage());
                pw.println("error: ERROR! invalid option argument \"" + arg + "\"");
                return null;
            }
        }

        QHostOptions options = new QHostOptions();

        options.setIncludeJobs(showJobs);
        options.setIncludeQueue(showQueues);
        options.setShowAsXML(showAsXML);
        if (hostFilter != null) {
            options.setHostFilter(hostFilter);
        }
        if (userFilter != null) {
            options.setUserFilter(userFilter);
        }
        if (resourceFilter != null) {
            options.setResourceFilter(resourceFilter);
        }
        if (resourceAttributeFilter != null) {
            options.setResourceAttributeFilter(resourceAttributeFilter);
        }

        return options;
    }
}
