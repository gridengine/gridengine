/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package com.sun.grid.installer.gui;

import com.izforge.izpack.installer.InstallData;
import com.izforge.izpack.installer.InstallerFrame;
import com.izforge.izpack.util.Debug;
import com.izforge.izpack.util.VariableSubstitutor;
import com.sun.grid.installer.util.CommandExecutor;
import com.sun.grid.installer.util.ExtendedFile;
import com.sun.grid.installer.util.Util;
import java.util.Enumeration;
import java.util.Properties;
import javax.swing.JOptionPane;

public class IntermediateActionPanel extends ActionPanel {

    public IntermediateActionPanel(InstallerFrame parent, InstallData idata) {
        super(parent, idata);

        setNumOfMaxExecution(Integer.MAX_VALUE);
    }

    @Override
    public void doAction() {
        initializeVariables();
    }

    private void initializeVariables() {
        boolean isQmasterInst, isShadowdInst, isExecdInst;
        isQmasterInst = parent.getRules().isConditionTrue(COND_INSTALL_QMASTER, idata.getVariables());
        isShadowdInst = parent.getRules().isConditionTrue(COND_INSTALL_SHADOWD, idata.getVariables());
        isExecdInst = parent.getRules().isConditionTrue(COND_INSTALL_EXECD, idata.getVariables());

        VariableSubstitutor vs = new VariableSubstitutor(idata.getVariables());

        // Only once...
        if (getNumOfExecution() == 0) {
            // Localhost arch
            CommandExecutor cmd = new CommandExecutor(idata.getVariable(VAR_SGE_ROOT) + "/util/arch");
            cmd.execute();
            if (cmd.getExitValue() == 0 && cmd.getOutput().size() > 0) {
                Host.localHostArch = cmd.getOutput().get(0).trim();
                idata.setVariable(VAR_LOCALHOST_ARCH, Host.localHostArch);
                Debug.trace("localhost.arch='" + idata.getVariable(VAR_LOCALHOST_ARCH) + "'");
            }

            // Check user
            if (idata.getVariables().getProperty(ARG_CONNECT_USER, "").equals("")) {
                String sgeRoot = vs.substitute(idata.getVariable(VAR_SGE_ROOT), null);
                String userName = vs.substitute(idata.getVariable(VAR_USER_NAME), null);

                ExtendedFile sgeRootDir = new ExtendedFile(sgeRoot);

                Debug.trace(sgeRootDir.getPermissions() + " " + sgeRootDir.getOwner() + " " + sgeRootDir.getGroup() + " " + sgeRoot);

                if (!userName.equals("root")) {
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
            } 
        }

        // Set value for 'cond.qmaster.on.localhost' condition
        Host.IS_QMASTER_ON_LOCALHOST = Host.localHostName.equalsIgnoreCase(idata.getVariable(VAR_QMASTER_HOST)) ||
                Host.localHostIP.equalsIgnoreCase(idata.getVariable(VAR_QMASTER_HOST));

        // cfg.spooling.method
        if (parent.getRules().isConditionTrue(COND_INSTALL_BDB, idata.getVariables())) {
            idata.setVariable(VAR_SPOOLING_METHOD, vs.substituteMultiple(idata.getVariable(VAR_SPOOLING_METHOD_BERKELEYDBSERVER), null));
        } else {
            idata.setVariable(VAR_SPOOLING_METHOD, vs.substituteMultiple(idata.getVariable(VAR_SPOOLING_METHOD_BERKELEYDB), null));
        }

        // In case of execd and/or shadow host stabdalone installation source the settings.sh boostrap and act_qmaster files
        if (!isQmasterInst && (isExecdInst || isShadowdInst)) {
            try {
                Properties settingsProps = Util.sourceSGESettings(idata.getVariable(VAR_SGE_ROOT), idata.getVariable(VAR_SGE_CELL_NAME));
                Properties bootsrapProps = Util.sourceSGEBootstrap(idata.getVariable(VAR_SGE_ROOT), idata.getVariable(VAR_SGE_CELL_NAME));
                String qmasterHost = Util.getQmasterHost(idata.getVariable(VAR_SGE_ROOT), idata.getVariable(VAR_SGE_CELL_NAME));

                String origKey = "";
                String newKey = "";
                for (Enumeration<Object> enumer = settingsProps.keys(); enumer.hasMoreElements();) {
                    origKey = (String) enumer.nextElement();
                    newKey = CONFIG_VAR_PREFIX + "." + origKey.toLowerCase().replace('_', '.');
                    idata.setVariable(newKey, settingsProps.getProperty(origKey));
                }

                idata.setVariable(VAR_QMASTER_HOST, qmasterHost);
                idata.setVariable(VAR_ADMIN_USER, bootsrapProps.getProperty("admin_user"));
                idata.setVariable("add.product.mode", bootsrapProps.getProperty("add.product.mode")); //To correctly set CSP mode in execd only installs
            } catch (Exception ex) {
                Debug.error("Can not source 'settings.sh' and/or 'bootstrap' and/or 'act_qmaster' files! " + ex);
            }
        }

        // set cfg.sge.enable.smf and cfg.remove.rc
        idata.setVariable(VAR_SGE_ENABLE_SMF, idata.getVariable(VAR_ADD_TO_RC));
        idata.setVariable(VAR_REMOVE_RC, idata.getVariable(VAR_ADD_TO_RC));
    }
}
