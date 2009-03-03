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

import javax.swing.event.TableModelEvent;

public class HostSelectionTableModel extends SortedTableModel {

    final private String[] headers;
    final private Class[] types;

    private HostList hostList;

    // To ensure the qmaster host singularity among diff. tables, default: no qmaster
    private static Host qmasterHost = null;

    // To ensure the Berkeley db host singularity among diff. tables, default: no bdb server
    private static Host bdbHost = null;

    public HostSelectionTableModel(HostList hostList, String [] headers, Class[] types) {
        super(new Object[][]{}, headers);
        this.headers = headers;
        this.types = types;
        this.hostList = hostList;
    }

    @Override
    public int getColumnCount() {
        return headers.length;
    }

    @Override
    public int getRowCount() {
        return (hostList == null) ? 0 : hostList.size();
    }

    public static Host getQmasterHost() {
        return qmasterHost;
    }

    public static Host getBdbHost() {
        return bdbHost;
    }

    @Override
    public Object getValueAt(int row, int col) {
        row = getSortedRowIndex(row);
        if (row >= hostList.size()) {
            return null;
        }

        Host h = hostList.get(row);

        switch (col) {
            case 0: return h.getHostname();
            case 1: return h.getIp();
            case 2: return h.getArchitecture();
            case 3: return h.isQmasterHost();
            case 4: return h.isShadowHost();
            case 5: return h.isExecutionHost();
            case 6: { // Show only if it is an ececution host
                if (h.isExecutionHost()) {
                    return h.getSpoolDir();
                } else {
                    return "";
                }
            }
            case 7: return h.isAdminHost();
            case 8: return h.isSubmitHost();
            case 9: return h.isBdbHost();
            case 10: return h.getState();
            default: throw new IndexOutOfBoundsException("Invalid index rowIndex="+row+" columnIndex="+col);
        }
    }
    
    @Override
    public void setValueAt(Object aValue, int row, int col) {
        row = getSortedRowIndex(row);

        Host h = hostList.get(row);

        switch (col) {
            case 0: h.setHostname((String)aValue); break;
            case 1: h.setIp((String) aValue); break;
            case 2: h.setArchitecture((String) aValue); break;
            case 3: {
                /* Enable to set only if a Qmaster host hasn't been selected yet or
                 * that's itself.
                 */
                boolean bValue = ((Boolean)aValue).booleanValue();
                if (qmasterHost == null || qmasterHost.equals(h)) {
                    h.setQmasterHost(bValue);
                    if (!bValue) {
                        qmasterHost = null;
                    } else {
                        qmasterHost = h;
                    }
                }
                break;
            }
            case 4: h.setShadowHost(((Boolean)aValue).booleanValue()); break;
            case 5: h.setExecutionHost(((Boolean)aValue).booleanValue()); break;
            case 6: h.setSpoolDir((String)aValue); break;
            case 7: h.setAdminHost(((Boolean)aValue).booleanValue()); break;
            case 8: h.setSubmitHost(((Boolean)aValue).booleanValue()); break;
            case 9: {
                /* Enable to set only if a Berkeley db host hasn't been selected yet or
                 * that's itself.
                 */
                boolean bValue = ((Boolean)aValue).booleanValue();
                if (bdbHost == null || bdbHost.equals(h)) {
                    h.setBdbHost(bValue);
                    if (!bValue) {
                        bdbHost = null;
                    } else {
                        bdbHost = h;
                    }
                }
                break;
            }
            case 10: h.setState((Host.State) aValue); break;
            default: throw new IndexOutOfBoundsException("Invalid index rowIndex="+row+" columnIndex="+col);
        }

        fireTableCellUpdated(row, col);

        // Update execd spool dir column too regarding to execd component selection
        if (col == 5) {
            fireTableCellUpdated(row, 6);
        }
    }

    @Override
    public Class getColumnClass(int columnIndex) {
        return types[columnIndex];
    }

    @Override
    public boolean isCellEditable(int row, int col) {
        row = getSortedRowIndex(row);
        Host h = hostList.get(row);
        
        switch (col) {
            case 3: return (qmasterHost == null || qmasterHost.equals(h));
            case 4:
            case 5:
            case 6:
            case 7:
            case 8: return true;
            case 9: return (bdbHost == null || bdbHost.equals(h));
            default: return false;
        }
    }    

    public boolean removeAll() {
        boolean result = hostList.removeAll(hostList);

        sortedIndexes = null;

        qmasterHost = null;
        bdbHost = null;
        
        fireTableDataChanged();
        return result;
    }

    public void setHostState(Host h, Host.State state) {
        h.setState(state);
        
        int row = getRowIndex(hostList.indexOf(h));
        if (row == -1 || row >= hostList.size()) {
            return;
        }

        fireTableCellUpdated(row, 4);
    }

    public boolean addHost(Host h) {
        int row = hostList.size();

        if (h.isQmasterHost()) {
            if (qmasterHost != null && !qmasterHost.equals(h)) {
                h.setQmasterHost(false);
            } else {
                qmasterHost = h;
            }
        }

        if (h.isBdbHost()) {
            if (bdbHost != null && !bdbHost.equals(h)) {
                h.setBdbHost(false);
            } else {
                bdbHost = h;
            }
        }

        if (!hostList.add(h)) {
            return false;
        }
        fireTableRowsInserted(row, row);
        return true;
    }

    public void removeHost(Host h) {
        int row = getRowIndex(hostList.indexOf(h));
        if (row == -1 || row >= hostList.size()) {
            return;
        }
        
        if (!hostList.remove(h)) {
            return;
        }

        if (h.equals(qmasterHost)) {
            qmasterHost = null;
        }

        if (h.equals(bdbHost)) {
            bdbHost = null;
        }

        reSort();
        
        fireTableRowsDeleted(row, row);
        fireTableChanged(new TableModelEvent(this));
    }
}
