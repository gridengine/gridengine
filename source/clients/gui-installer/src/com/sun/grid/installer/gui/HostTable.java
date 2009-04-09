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

import com.sun.grid.installer.task.TaskHandler;
import com.sun.grid.installer.util.Util;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.util.ArrayList;
import java.util.List;
import javax.swing.JMenuItem;
import javax.swing.JPopupMenu;
import javax.swing.JTable;
import javax.swing.SwingUtilities;
import javax.swing.table.TableModel;

/**
 * Table provides advanced host managing for models which implements
 * {@link HostTableModel} interface
 */
public class HostTable extends JTable {
    private TaskHandler handler;

    /**
     * Constructor
     * @param handler The {@link TaskHandler} which provides access to the global data
     */
    public HostTable(TaskHandler handler) {
        super();

        this.handler = handler;

        initPopupMenu();
    }

    /**
     * Constructor
     * @param handler The {@link TaskHandler} which provides access to the global data
     * @param model The table model
     */
    public HostTable(TaskHandler handler, TableModel model) {
        super(model);

        this.handler = handler;

        initPopupMenu();
    }

    @Override
    public void setModel(TableModel dataModel) {
        super.setModel(dataModel);
    }

    /**
     * Returns with the host list stored in the table model
     * @return The host list data
     */
    public HostList getHostList() {
        return ((HostTableModel)getModel()).getHostList();
    }

    /**
     * Initializes the popup menu for the table
     */
    private void initPopupMenu() {
        createPopupMenu(handler.getLabel("menu.title"));
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

    /**
     * Delete the given host list
     * @param hostList The host list to delete
     */
    private void deleteHostList(HostList hostList) {
        List<Host> list = new ArrayList<Host>();
        for (Host h : hostList) {
            list.add(h);
        }
        deleteHostList(list);
    }

    /**
     * Delete the given list of hosts
     * @param list The list of hosts to delete
     */
    private void deleteHostList(List<Host> list) {
        Host h;

        for (int i = list.size() - 1; i >= 0; i--) {
            h = list.get(i);
            handler.removeHost(h);
        }

        clearSelection();
    }

    /**
     * Creates a popup menu for the table
     * @param label The main label of the popup menu
     * @return The created menu
     */
    private JPopupMenu createPopupMenu(String label) {
        final JPopupMenu tableMenu = new JPopupMenu(label);
        tableMenu.setToolTipText(handler.getTooltip(label));

        // menu item for resolving the selected hosts in the table
        final JMenuItem configureMI = new JMenuItem(handler.getLabel("menu.configure"));
        configureMI.setToolTipText(handler.getTooltip("menu.configure"));
        configureMI.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                if (getSelectedRowCount() > 0) {
                    int[] selectedHostIndexes = getSelectedHostIndexes();
                    HostList hostList = getHostList();
                    HostList resultHostList = new HostList();
                    Host host;
                    for (int i = selectedHostIndexes.length - 1; i >= 0; i--) {
                        host = hostList.get(selectedHostIndexes[i]);
                        resultHostList.addUnchecked(host);
                    }

                    HostConfigFrame.getInstance().open(resultHostList);
                } else {
                    HostConfigFrame.getInstance().open(getHostList());
                }
            }
        });
        tableMenu.add(configureMI);

        final JPopupMenu.Separator configureSeparator = new JPopupMenu.Separator();
        tableMenu.add(configureSeparator);

        // menu item for resolving the selected hosts in the table
        final JMenuItem resolveSelectionMI = new JMenuItem(handler.getLabel("menu.resolve.selected"));
        resolveSelectionMI.setToolTipText(handler.getTooltip("menu.resolve.selected"));
        resolveSelectionMI.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                int[] selectedHostIndexes = getSelectedHostIndexes();
                HostList hostList = getHostList();
                List<Host> list = new ArrayList<Host>();
                Host host;
                for (int i = selectedHostIndexes.length - 1 ; i >= 0; i--) {
                    host = hostList.get(selectedHostIndexes[i]);
                    host.setState(Host.State.NEW_UNKNOWN_HOST);
                    list.add(host);
                }

                deleteHostList(list);
                handler.addHosts(list);
            }
        });
        tableMenu.add(resolveSelectionMI);

        // menu item for resolving all hosts in the table
        final JMenuItem resolveAllMI = new JMenuItem(handler.getLabel("menu.resolve.all"));
        resolveAllMI.setToolTipText(handler.getTooltip("menu.resolve.all"));
        resolveAllMI.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                HostList hostList = getHostList();
                List<Host> list = new ArrayList<Host>();
                for (Host host : hostList) {
                    host.setState(Host.State.NEW_UNKNOWN_HOST);
                    list.add(host);
                }

                deleteHostList(list);
                handler.addHosts(list);
            }
        });
        tableMenu.add(resolveAllMI);

        final JPopupMenu.Separator refreshSeparator = new JPopupMenu.Separator();
        tableMenu.add(refreshSeparator);

        // menu item for deleting selected hosts in the table
        final JMenuItem deleteSelectionMI = new JMenuItem(handler.getLabel("menu.remove.selected"));
        deleteSelectionMI.setToolTipText(handler.getTooltip("menu.remove.selected"));
        deleteSelectionMI.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                int[] selectedHostIndexes = getSelectedHostIndexes();
                HostList hostList = getHostList();
                List<Host> list = new ArrayList<Host>();
                Host h;
                for (int i = selectedHostIndexes.length - 1 ; i >= 0; i--) {
                    h = hostList.get(selectedHostIndexes[i]);
                    list.add(h);
                }

                deleteHostList(list);
            }
        });
        tableMenu.add(deleteSelectionMI);

        // menu item for deleting all hosts in the table
        final JMenuItem deleteAllMI = new JMenuItem(handler.getLabel("menu.remove.all"));
        deleteAllMI.setToolTipText(handler.getTooltip("menu.remove.all"));
        deleteAllMI.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                deleteHostList(getHostList());
            }
        });
        tableMenu.add(deleteAllMI);

        tableMenu.addSeparator();

        // menu item for saving selected hosts in the table
        final JMenuItem saveSelectionMI = new JMenuItem(handler.getLabel("menu.save.selection"));
        saveSelectionMI.setToolTipText(handler.getTooltip("menu.save.selection"));
        saveSelectionMI.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                int[] selectedHostIndexes = getSelectedHostIndexes();
                HostList hostList = getHostList();
                List<String> list = new ArrayList<String>();
                Host h;
                for (int i = selectedHostIndexes.length - 1 ; i >= 0; i--) {
                    h = hostList.get(selectedHostIndexes[i]);
                    if (!h.isFirstTask() && !h.isLastTask()) {
                        list.add(h.toStringInstance());
                    }
                }

                Util.saveListToFile(HostTable.this, list);
                HostTable.this.clearSelection();
                HostTable.this.updateUI();
            }
        });
        tableMenu.add(saveSelectionMI);

        // menu item for saving all hosts in the table
        final JMenuItem saveAllMI = new JMenuItem(handler.getLabel("menu.save.all"));
        saveAllMI.setToolTipText(handler.getTooltip("menu.save.all"));
        saveAllMI.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                HostList hostList = getHostList();
                List<String> list = new ArrayList<String>();
                for (Host h : hostList) {
                    if (!h.isFirstTask() && !h.isLastTask()) {
                        list.add(h.toStringInstance());
                    }
                }

                Util.saveListToFile(HostTable.this, list);
                HostTable.this.clearSelection();
                HostTable.this.updateUI();
            }
        });
        tableMenu.add(saveAllMI);

        // listener to show/hide the menu and filter options
        this.addMouseListener(new MouseAdapter() {
            @Override
            public void mousePressed(MouseEvent e) {
                HostTable table = (HostTable) e.getSource();
                if (SwingUtilities.isRightMouseButton(e) && !tableMenu.isVisible()) {
                    resolveSelectionMI.setVisible(table.getSelectedRowCount() > 0);
                    deleteSelectionMI.setVisible(table.getSelectedRowCount() > 0);
                    saveSelectionMI.setVisible(table.getSelectedRowCount() > 0);

                    if (HostTable.this.getModel() instanceof HostInstallTableModel) {
                        refreshSeparator.setVisible(false);
                        configureSeparator.setVisible(false);
                        
                        configureMI.setVisible(false);

                        resolveSelectionMI.setVisible(false);
                        resolveAllMI.setVisible(false);
                    }

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
