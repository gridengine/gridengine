/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package com.sun.grid.installer.gui;

import com.izforge.izpack.installer.GUIInstaller;
import com.izforge.izpack.panels.ProcessingClient;
import com.izforge.izpack.panels.Validator;
import com.izforge.izpack.util.Debug;
import com.izforge.izpack.util.VariableSubstitutor;
import com.sun.grid.installer.util.ExtendedFile;
import com.sun.grid.installer.util.Util;
import java.util.Map;

/**
 * Validator class.
 */
public class PermissionValidator  implements Validator {
    private static final String PARAM_USER = "user";
    private static final String PARAM_ACTIONS = "actions";

    public boolean validate(ProcessingClient client) {
        String file = client.getText();
        String[] actions = null;

        if (file.equals("")) {
            return true;
        }

        VariableSubstitutor vs = new VariableSubstitutor(GUIInstaller.getInstallData().getVariables());
        String userName  = vs.substituteMultiple(GUIInstaller.getInstallData().getVariables().getProperty("user.name"), null);
        String rootUser  = vs.substituteMultiple(GUIInstaller.getInstallData().getVariables().getProperty("root.user"), null);
        String adminUser = vs.substituteMultiple(GUIInstaller.getInstallData().getVariables().getProperty("cfg.admin.user"), null);

        

        if (client.hasParams()) {
                Map<String, String> params = client.getValidatorParams();

                if (params.containsKey(PARAM_USER)) {
                    userName = params.get(PARAM_USER);
                }

                if (params.containsKey(PARAM_ACTIONS)) {
                    actions = params.get(PARAM_ACTIONS).split(",");

                    Debug.trace("PermissionValidator - action(s) to validate: '" + params.get(PARAM_ACTIONS) + "'");
                }
        }

        Debug.trace("PermissionValidator - user="+userName+" root="+rootUser+" admin="+adminUser);

        ExtendedFile extendedFile = new ExtendedFile(file).getFirstExistingParent();
        Debug.trace("PermissionValidator - validate first existing parent '" + extendedFile.getAbsolutePath() + "' of '" + file + "'.");

        String[] groupIds = Util.getUserGroups(Host.localHostName, GUIInstaller.getInstallData().getVariables(), userName);
        String[] adminUserGroupIds = Util.getUserGroups(Host.localHostName, GUIInstaller.getInstallData().getVariables(), adminUser);

        if (actions == null || actions.length == 0) {
            actions = new String[]{"read", "write", "execute"};
        }

        boolean hasPermission = false;
        for (int i = 0; i < actions.length; i++) {
            hasPermission = false;
            actions[i] = actions[i].trim().toLowerCase();

            for (String groupId : groupIds) {
                if (actions[i].equals("read")) {
                    if (extendedFile.hasReadPermission(userName, groupId)) {
                        hasPermission = true;
                        break;
                    }
                } else if (actions[i].equals("write")) {
                    if (extendedFile.hasWritePermission(userName, groupId)) {
                        hasPermission = true;
                        break;
                    }
                } else if (actions[i].equals("execute")) {
                    if (extendedFile.hasExecutePermission(userName, groupId)) {
                        hasPermission = true;
                        break;
                    }
                } else {
                    Debug.error("PermissionValidator - The is '" + actions[i] + "' unknown action type! Should be: 'read' 'write' or 'execute'");
                }
            }

            if (!hasPermission) {
                for (String adminUserGroupId : adminUserGroupIds) {
                    if (actions[i].equals("read")) {
                        if (extendedFile.hasReadPermission(adminUser, adminUserGroupId)) {
                            hasPermission = true;
                            break;
                        }
                    } else if (actions[i].equals("write")) {
                        if (extendedFile.hasWritePermission(adminUser, adminUserGroupId)) {
                            hasPermission = true;
                            break;
                        }
                    } else if (actions[i].equals("execute")) {
                        if (extendedFile.hasExecutePermission(adminUser, adminUserGroupId)) {
                            hasPermission = true;
                            break;
                        }
                    } else {
                        Debug.error("PermissionValidator - The is '" + actions[i] + "' unknown action type! Should be: 'read' 'write' or 'execute'");
                    }
                }

                if (!hasPermission) {
                    return false;
                }
                //return userName.equals(rootUser) && hasPermission;
            }

        }

        return hasPermission;
    }

}
