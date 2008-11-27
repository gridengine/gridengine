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
import com.izforge.izpack.installer.IzPanel;
import com.sun.grid.installer.util.Config;

public class ActionPanel extends IzPanel implements Config {
    public static String PROP_NUM_OF_EXECUTION = "numOfExecution";

    private String panelId = "";    
    private int numOfMaxExecution = 1;

    public ActionPanel(InstallerFrame parent, InstallData idata) {
        super(parent, idata);
    }

    public void doAction() {}
    
    @Override
    public void panelActivate() {
        panelId = idata.panels.get(idata.curPanelNumber).getMetadata().getPanelid();

        if (getNumOfExecution() < numOfMaxExecution) {
            doAction();
        
            incNumOfExecution();
        }

        getInstallerFrame().skipPanel();
    }

    public int getNumOfExecution() {
        int num = 0;
        String numStr= null;
        
        if ((numStr = idata.getVariable(panelId + D + PROP_NUM_OF_EXECUTION)) != null) {
            num = Integer.parseInt(numStr);
        }
        
        return num;
    }

    public void incNumOfExecution() {
        int actNum = getNumOfExecution();
        actNum++;
        idata.setVariable(panelId + D + PROP_NUM_OF_EXECUTION, String.valueOf(actNum));
    }

    public String getPanelId() {
        return panelId;
    }

    public int getNumOfMaxExecution() {
        return numOfMaxExecution;
    }
    
    public void setNumOfMaxExecution(int numOfMaxExecution) {
        this.numOfMaxExecution = numOfMaxExecution;
    }
}
