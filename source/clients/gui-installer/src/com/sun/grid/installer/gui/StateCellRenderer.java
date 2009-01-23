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

import com.sun.grid.installer.util.Util;
import java.awt.Color;
import java.awt.Component;
import java.util.Hashtable;
import javax.swing.JLabel;
import javax.swing.JTable;
import javax.swing.border.EmptyBorder;
import javax.swing.table.TableCellRenderer;

/**
 * Class for rendering State syled cells
 *
 * @see Host.State
 */
public class StateCellRenderer implements TableCellRenderer {
    private JLabel label = null;
    private Hashtable<Integer, HostProgressBar> progressBars = null;

    private static final Color COLOR_GOOD = Color.GREEN;
    private static final Color COLOR_BAD  = Color.RED;
    private static final Color COLOR_WARNING  = new Color(255, 225, 0); // Yellow

    /**
     * Constructor
     */
    public StateCellRenderer() {
        // create progress bar storage
        int initCapacity = Math.max(Util.INSTALL_THREAD_POOL_SIZE, Util.RESOLVE_THREAD_POOL_SIZE) + 1;
        progressBars = new Hashtable<Integer, HostProgressBar>(initCapacity, 1);

        label = new JLabel();
        label.setOpaque(true);
        label.setBorder(new EmptyBorder(0, 1, 0, 1));
    }

    public Component getTableCellRendererComponent(JTable table, Object value, boolean isSelected, boolean hasFocus, int row, int column) {
        Component comp = null;

        if (value != null && value instanceof Host.State) {
            Host.State state = (Host.State)value;

            Color backColor = table.getBackground();
            label.setText(state.toString());
            label.setToolTipText(state.getTooltip());

            // differenciate between states
            switch (state) {
                case RESOLVING:
                case INSTALLING:
                case CONTACTING: { // running states
                    if (!progressBars.containsKey(Integer.valueOf(row))) {
                        progressBars.put(Integer.valueOf(row), new HostProgressBar(state.toString()));
                    }
                    comp = progressBars.get(Integer.valueOf(row));
                    break;
                }
                case OK:
                case SUCCESS:
                case READY_TO_INSTALL:
                case REACHABLE: { // good states
                    if (progressBars.containsKey(Integer.valueOf(row))) {
                        progressBars.remove(Integer.valueOf(row));
                    }

                    comp = label;
                    backColor = COLOR_GOOD;
                    break;
                }
                case PERM_QMASTER_SPOOL_DIR:
                case PERM_EXECD_SPOOL_DIR:
                case PERM_BDB_SPOOL_DIR:
                case PERM_JMX_KEYSTORE:
                case BDB_SPOOL_DIR_EXISTS:
                case USED_QMASTER_PORT:
                case USED_EXECD_PORT:
                case USED_JMX_PORT:
                case UNKNOWN_ERROR: { // warning states
                    if (progressBars.containsKey(Integer.valueOf(row))) {
                        progressBars.remove(Integer.valueOf(row));
                    }
                    comp = label;
                    backColor = COLOR_WARNING;
                    break;
                }
                default: { // bad states
                    if (progressBars.containsKey(Integer.valueOf(row))) {
                        progressBars.remove(Integer.valueOf(row));
                    }
                    comp = label;
                    backColor = COLOR_BAD; // RESOLVABLE, UNREACHABLE, NEW_UNKNOWN_HOST, UNKNOWN_HOST, FAILED, COPY_TIMEOUT, COPY_FAILED, ADMIN_USER_MISSING
                }
            }

            if (isSelected) {
                comp.setForeground(backColor);
                comp.setBackground(table.getSelectionBackground());
            } else {
                comp.setForeground(backColor);
                comp.setBackground(table.getBackground());
            }
        }

        return comp;
    }
}
