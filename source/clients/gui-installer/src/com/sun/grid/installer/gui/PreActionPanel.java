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
package com.sun.grid.installer.gui;

import com.izforge.izpack.installer.InstallData;
import com.izforge.izpack.installer.InstallerFrame;
import com.izforge.izpack.util.Debug;
import com.izforge.izpack.util.VariableSubstitutor;
import com.sun.grid.installer.util.Config;
import com.sun.grid.installer.util.ExtendedFile;

import com.sun.grid.installer.util.Util;
import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import javax.swing.JOptionPane;

public class PreActionPanel extends ActionPanel {

    public PreActionPanel(InstallerFrame parent, InstallData idata) {
        super(parent, idata);
    }

    @Override
    public void doAction() {
        initializeVariables();
 
    }

    private void initializeVariables() {
        VariableSubstitutor vs = new VariableSubstitutor(idata.getVariables());

        // set thread spool sizes
        int size = 0;
        if (idata.getVariable(ARG_RESOLVE_THREAD_POOL_SIZE) != null) {
            try {
                size = Integer.valueOf(idata.getVariable(ARG_RESOLVE_THREAD_POOL_SIZE));
                if (size < 1) {
                    throw new NumberFormatException();
                }
                Util.RESOLVE_THREAD_POOL_SIZE = size;
            } catch (NumberFormatException e) {
                Debug.error("Invalid '" + ARG_RESOLVE_THREAD_POOL_SIZE + "' value: " + idata.getVariable(ARG_RESOLVE_THREAD_POOL_SIZE));
            } finally {
                Debug.trace(ARG_RESOLVE_THREAD_POOL_SIZE +" is now set to "+Util.RESOLVE_THREAD_POOL_SIZE);
            }
        }
        if (idata.getVariable(ARG_INSTALL_THREAD_POOL_SIZE) != null) {
            try {
                size = Integer.valueOf(idata.getVariable(ARG_INSTALL_THREAD_POOL_SIZE));
                if (size < 1) {
                    throw new NumberFormatException();
                }
                Util.INSTALL_THREAD_POOL_SIZE = size;
            } catch (NumberFormatException e) {
                Debug.error("Invalid '" + ARG_INSTALL_THREAD_POOL_SIZE + "' value: " + idata.getVariable(ARG_INSTALL_THREAD_POOL_SIZE));
            } finally {
                Debug.trace(ARG_INSTALL_THREAD_POOL_SIZE +" is now set to "+Util.INSTALL_THREAD_POOL_SIZE);
            }
        }
        //Set the timeout values
        if (idata.getVariable(ARG_RESOLVE_TIMEOUT) != null) {
            try {
                size = Integer.valueOf(idata.getVariable(ARG_RESOLVE_TIMEOUT));
                if (size < 1) {
                    throw new NumberFormatException();
                }
                Util.RESOLVE_TIMEOUT = size * 1000; //we need ms not seconds
            } catch (NumberFormatException e) {
                Debug.error("Invalid '" + ARG_RESOLVE_TIMEOUT + "' value: " + idata.getVariable(ARG_RESOLVE_THREAD_POOL_SIZE));
            } finally {
                Debug.trace(ARG_RESOLVE_TIMEOUT +" is now set to "+Util.RESOLVE_TIMEOUT);
            }
        }
        if (idata.getVariable(ARG_INSTALL_TIMEOUT) != null) {
            try {
                size = Integer.valueOf(idata.getVariable(ARG_INSTALL_TIMEOUT));
                if (size < 1) {
                    throw new NumberFormatException();
                }
                Util.INSTALL_TIMEOUT = size * 1000; //we need ms not seconds
            } catch (NumberFormatException e) {
                Debug.error("Invalid '" + ARG_INSTALL_TIMEOUT + "' value: " + idata.getVariable(ARG_INSTALL_THREAD_POOL_SIZE));
            } finally {
                Debug.trace(ARG_INSTALL_TIMEOUT +" is now set to "+Util.INSTALL_TIMEOUT);
            }
        }
        if (Util.isWindowsMode(idata.getVariables())) {
           Debug.trace("Using mode "+CONST_MODE_WINDOWS);
        }
        
        String sgeRootPath = vs.substitute(idata.getVariable(VAR_SGE_ROOT), null);
        String userName = vs.substitute(idata.getVariable(VAR_USER_NAME), null);
        String rootUser = idata.getVariable(VAR_ROOT_USER);

        ExtendedFile sgeRootDir = new ExtendedFile(sgeRootPath);

        Debug.trace(sgeRootDir.getPermissions() + " " + sgeRootDir.getOwner() + " " + sgeRootDir.getGroup() + " " + sgeRootPath);

        // cfg.admin.user
        idata.setVariable(VAR_ADMIN_USER, sgeRootDir.getOwner());
        Debug.trace("cfg.admin.user='" + idata.getVariable(VAR_ADMIN_USER) + "'");

        // Check user
        if (!userName.equals(rootUser)) {
             if (userName.equals(sgeRootDir.getOwner())) {
                 if (JOptionPane.NO_OPTION == JOptionPane.showOptionDialog(this, vs.substituteMultiple(idata.langpack.getString(WARNING_USER_NOT_ROOT), null),
                         idata.langpack.getString("installer.warning"), JOptionPane.YES_NO_OPTION, JOptionPane.WARNING_MESSAGE, null,
                         new Object[]{idata.langpack.getString("installer.yes"), idata.langpack.getString("installer.no")}, idata.langpack.getString("installer.yes"))) {
                     parent.exit(true);
                 }
             } else {
                 JOptionPane.showOptionDialog(this, vs.substituteMultiple(idata.langpack.getString(ERROR_USER_INVALID), null),
                         idata.langpack.getString("installer.error"), JOptionPane.CANCEL_OPTION, JOptionPane.ERROR_MESSAGE, null,
                         new Object[]{idata.langpack.getString("button.exit.label")}, idata.langpack.getString("button.exit.label"));

                 parent.exit(true);
             }
        }

        // set cfg.add.to.rc
        idata.setVariable(VAR_ADD_TO_RC, parent.getRules().isConditionTrue(COND_USER_ROOT) ? "true" : "false");

        // set user group
//        String[] groups = Util.getUserGroups(Host.localHostName, idata.getVariables(), idata.getVariable(VAR_USER_NAME));
//        idata.setVariable(VAR_USER_GROUP, group);
//        Debug.trace("Group of executing user '" + idata.getVariable(VAR_USER_NAME) + "' is '" + group + "'.");

        // add.qmaster.host
        idata.setVariable(VAR_QMASTER_HOST, Host.localHostName);
        Debug.trace("add.qmaster.host='" + idata.getVariable(VAR_QMASTER_HOST) + "'");

        // cfg.db.spooling.server
        idata.setVariable(VAR_DB_SPOOLING_SERVER, Host.localHostName);
        Debug.trace("cfg.db.spooling.server='" + idata.getVariable(VAR_DB_SPOOLING_SERVER) + "'");

        // cfg.sge.jvm.lib.path
        List<String> libPaths = new ArrayList<String>();
        libPaths.addAll(Arrays.asList(idata.getVariable("SYSTEM_java_library_path").split(":")));
        libPaths.add(idata.getVariable("SYSTEM_sun_boot_library_path"));
        //MacOS last resort
        libPaths.add("/System/Library/Frameworks/JavaVM.framework/Libraries");

        String libjvm = "/" + System.mapLibraryName("jvm");
        if (libjvm.endsWith(".jnilib")) {
            libjvm = libjvm.substring(0, libjvm.lastIndexOf(".jnilib")) + ".dylib";
        }

        for (String libPath : libPaths) {
            if (new File(libPath + libjvm).exists()) {
                idata.setVariable(VAR_JVM_LIB_PATH, libPath + libjvm);
                Debug.trace("cfg.sge.jvm.lib.path='" + idata.getVariable(VAR_JVM_LIB_PATH) + "'");
                break;
            }
        }
    }
}
