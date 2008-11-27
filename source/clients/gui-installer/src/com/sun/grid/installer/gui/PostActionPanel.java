/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package com.sun.grid.installer.gui;

import com.izforge.izpack.installer.InstallData;
import com.izforge.izpack.installer.InstallerFrame;

/**
 *
 * @author zg222182
 */
public class PostActionPanel extends ActionPanel {

    public PostActionPanel(InstallerFrame parent, InstallData idata) {
        super(parent, idata);
    }

    @Override
    public void doAction() {
        System.out.println("!!!!!!!!!!!!!!!!!!!!!!!PostActionPanel");
    }
}
