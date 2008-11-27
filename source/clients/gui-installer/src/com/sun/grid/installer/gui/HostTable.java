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
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import javax.swing.JButton;
import javax.swing.JMenuItem;
import javax.swing.JPopupMenu;
import javax.swing.JTabbedPane;
import javax.swing.JTable;
import javax.swing.SwingUtilities;
import javax.swing.table.TableModel;

public class HostTable extends JTable {
    private HostPanel panel;
    private int tabPos;
    //private JTabbedPane tab;
    private String[] TABS;

    public HostTable(final HostPanel panel, final int tabPos) {
        super();
        this.panel = panel;   
        this.tabPos = tabPos;
        //tab = panel.getTabbedPane();
    }

    public HostTable(final HostPanel panel, TableModel model, final int tabPos) {
        super(model);
        this.panel = panel;
        this.tabPos = tabPos;
        //tab = panel.getTabbedPane();
        initPopupMenu();
    }

    @Override
    public void setModel(TableModel dataModel) {
        super.setModel(dataModel);
        initPopupMenu();
    }

    private void initPopupMenu() {
        //Add popup menu to offer delete/save operations for items in the SelectionTable
        if (this.getModel() instanceof HostSelectionTableModel) {
            createSelectionPopupMenu(panel.getLabel("menu.title"));
            TABS = HostPanel.SELECTION_TABS;
        } else if (this.getModel() instanceof HostInstallTableModel) {
            //TODO: createInstallPopupMenu("Edit Actions");
            TABS = HostPanel.INSTALL_TABS;
        }
    }

    /**
     * Transforms selected indexes into sorted host indexes
     * @return The sorted indexes of the selected rows
     */
    public int[] getSelectedHostIndexes() {
        SortedTableModel tableModel = (SortedTableModel)getModel();
        int[] selectedRows = getSelectedRows();

        for (int i = 0; i < selectedRows.length; i++) {
            if (selectedRows[i] > -1) {
                selectedRows[i] = tableModel.getSortedRowIndex(selectedRows[i]);
            }
        }

        return selectedRows;
    }

    private void deleteHostList(HostTable invoker, HostList hostList) {
        List<Host> list = new ArrayList<Host>();
        for (Host h : hostList) {
            list.add(h);
        }
        deleteHostList(invoker, list);
    }

    private void deleteHostList(HostTable invoker, List<Host> list) {
        HostTable table;
        HostSelectionTableModel model;
        Host h;

        //tab.invalidate();
        for (int i = list.size() - 1; i >= 0; i--) {
            h = list.get(i);
            for (Iterator<HostTable> iter = panel.getHostTableIterator(); iter.hasNext();) {
                model = (HostSelectionTableModel) iter.next().getModel();
                model.removeHost(h);
            }
        }
        int pos = 0;
        for (Iterator<HostTable> iter = panel.getHostTableIterator(); iter.hasNext();) {
            table = iter.next();
            model = (HostSelectionTableModel) table.getModel();
            //tab.setTitleAt(pos, TABS[pos] + " (" + model.getRowCount() + ")");
            //Disable installButton in no valid hosts
            if (pos == 1 && model.getRowCount() == 0) {
                panel.enableInstallButton(false);
            }
            pos++;
        }

        invoker.clearSelection();

        //Disable install button if no hosts left in the valid list
        if (panel.getHostListAt(1).size() == 0) {
            panel.enableInstallButton(false);
        }

        //tab.validate();
    }

    private JPopupMenu createSelectionPopupMenu(String label) {
        final JPopupMenu tableMenu = new JPopupMenu(label);
        tableMenu.setToolTipText(panel.getTooltip(label));
        final JMenuItem deleteSelectionMI = new JMenuItem(panel.getLabel("menu.remove.selected"));
        deleteSelectionMI.setToolTipText(panel.getTooltip("menu.remove.selected"));
        deleteSelectionMI.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                int[] selectedHostIndexes = getSelectedHostIndexes();
                HostList hostList = panel.getHostListAt(tabPos);
                List<Host> list = new ArrayList<Host>();
                Host h;
                for (int i = selectedHostIndexes.length - 1 ; i >= 0; i--) {
                    h = hostList.get(selectedHostIndexes[i]);
                    list.add(h);
                }

                deleteHostList(HostTable.this, list);
            }
        });
        tableMenu.add(deleteSelectionMI);

        final JMenuItem deleteAllMI = new JMenuItem(panel.getLabel("menu.remove.all"));
        deleteAllMI.setToolTipText(panel.getTooltip("menu.remove.all"));
        deleteAllMI.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                deleteHostList(HostTable.this, panel.getHostListAt(tabPos));
            }
        });
        tableMenu.add(deleteAllMI);

        tableMenu.addSeparator();

        final JMenuItem saveSelectionMI = new JMenuItem(panel.getLabel("menu.save.selection"));
        saveSelectionMI.setToolTipText(panel.getTooltip("menu.save.selection"));
        saveSelectionMI.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                int[] selectedHostIndexes = getSelectedHostIndexes();
                HostList hostList = panel.getHostListAt(tabPos);
                List<String> list = new ArrayList<String>();
                Host h;
                for (int i = selectedHostIndexes.length - 1 ; i >= 0; i--) {
                    h = hostList.get(selectedHostIndexes[i]);
                    list.add(h.getHostAsString());
                }

                Util.saveListToFile(HostTable.this, list);
                HostTable.this.clearSelection();
                HostTable.this.updateUI();
            }
        });
        tableMenu.add(saveSelectionMI);

        final JMenuItem saveAllMI = new JMenuItem(panel.getLabel("menu.save.all"));
        saveAllMI.setToolTipText(panel.getTooltip("menu.save.all"));
        saveAllMI.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                HostList hostList = panel.getHostListAt(tabPos);
                List<String> list = new ArrayList<String>();
                for (Host h : hostList) {
                    list.add(h.getHostAsString());
                }

                Util.saveListToFile(HostTable.this, list);
                HostTable.this.clearSelection();
                HostTable.this.updateUI();
            }
        });
        tableMenu.add(saveAllMI);

        this.addMouseListener(new MouseAdapter() {
            @Override
            public void mousePressed(MouseEvent e) {
                HostTable table = (HostTable) e.getSource();
                if (SwingUtilities.isRightMouseButton(e) && !tableMenu.isVisible()) {
                    deleteSelectionMI.setVisible(table.getSelectedRowCount() > 0);
                    saveSelectionMI.setVisible(table.getSelectedRowCount() > 0);
                    tableMenu.show(e.getComponent(), e.getX(), e.getY());
                } else {
                    tableMenu.setVisible(false);
                    super.mousePressed(e);
                }
            }
        });
        return tableMenu;
    }
}
