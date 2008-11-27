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
        VariableSubstitutor vs = new VariableSubstitutor(idata.getVariables());
        
        // Localhost arch
        if (getNumOfExecution() == 0) {
            CommandExecutor cmd = new CommandExecutor(idata.getVariable(VAR_SGE_ROOT) + "/util/arch");
            cmd.execute();
            if (cmd.getExitValue() == 0 && cmd.getOutput().size() > 0) {
                Host.localHostArch = cmd.getOutput().get(0).trim();
                idata.setVariable(VAR_LOCALHOST_ARCH, Host.localHostArch);
                Debug.trace("localhost.arch='" + idata.getVariable(VAR_LOCALHOST_ARCH) + "'");
            }
        }

        // cfg.spooling.method
        if (parent.getRules().isConditionTrue(COND_INSTALL_BDB, idata.getVariables())) {
            idata.setVariable(VAR_SPOOLING_METHOD, vs.substituteMultiple(idata.getVariable(VAR_SPOOLING_METHOD_BERKELEYDBSERVER), null));
        } else {
            idata.setVariable(VAR_SPOOLING_METHOD, vs.substituteMultiple(idata.getVariable(VAR_SPOOLING_METHOD_BERKELEYDB), null));
        }
    }
}
