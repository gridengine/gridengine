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
package com.sun.grid.jgdi.examples.jmxeventmonitor;

import com.sun.grid.jgdi.configuration.GEObject;
import com.sun.grid.jgdi.configuration.xml.XMLUtil;
import com.sun.grid.jgdi.event.ChangedObjectEvent;
import com.sun.grid.jgdi.event.Event;
import java.awt.BorderLayout;
import java.awt.Component;
import java.io.StringWriter;
import java.util.Date;
import javax.swing.BorderFactory;
import javax.swing.JDialog;
import javax.swing.JEditorPane;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.JTextField;
import javax.swing.table.AbstractTableModel;
import javax.swing.table.TableCellRenderer;

/**
 *
 */
public class EventDetailsDialog extends JDialog {
    private final static long serialVersionUID = -2008010901L;
    
    private final EventTableModel eventTableModel = new EventTableModel();
    private final JTable eventTable = new JTable(eventTableModel);
    private final JEditorPane detailsTextField = new JEditorPane();
    
    public EventDetailsDialog(JFrame frame) {
        super(frame, false);

        eventTable.getColumnModel().getColumn(0).setPreferredWidth(80);
        eventTable.getColumnModel().getColumn(0).setMaxWidth(80);
        eventTable.getColumnModel().getColumn(0).setMinWidth(80);
        eventTable.getColumnModel().getColumn(1).setPreferredWidth(320);
        eventTable.getColumnModel().getColumn(1).setMaxWidth(Integer.MAX_VALUE);
        eventTable.setDefaultRenderer(String.class, new MyCellRenderer());
        eventTable.setShowGrid(false);
        eventTable.setOpaque(false);
        getContentPane().add(eventTable, BorderLayout.NORTH);
        getContentPane().add(new JScrollPane(detailsTextField), BorderLayout.CENTER);
        
        setDefaultCloseOperation(HIDE_ON_CLOSE);
        setSize(400,300);
    }
    
    public void setEvent(Event event) {
        eventTableModel.setEvent(event);
        detailsTextField.setText(eventTableModel.getEventDetails());
    }
    
    
    private class MyCellRenderer implements TableCellRenderer {

        private JLabel label = new JLabel();
        private JTextField textField = new JTextField();
        
        public MyCellRenderer() {
            label.setHorizontalAlignment(JLabel.RIGHT);
            label.setBorder(BorderFactory.createEmptyBorder(10, 2, 10, 2));
            label.setOpaque(false);
            textField.setEditable(false);
            textField.setBorder(BorderFactory.createEmptyBorder(10, 2, 10, 2));
            textField.setOpaque(false);
        }
        
        public Component getTableCellRendererComponent(JTable table, Object value, boolean isSelected, 
                                                       boolean hasFocus, int row, int column) {
            if(column == 0) {
                label.setText(value == null ? "" : value.toString());
                return label;
            } else {
                textField.setText(value == null ? "" : value.toString());
                return textField;
            }
        }
        
    }
    
    private static class EventTableModel extends AbstractTableModel {
        
        private final static long serialVersionUID = -2008010901L;
        private Event event;
        
        public int getRowCount() {
            return 4;
        }

        public int getColumnCount() {
            return 2;
        }

        public void setEvent(Event evt) {
            this.event = evt;
            fireTableDataChanged();
        }

        @Override
        public Class<?> getColumnClass(int columnIndex) {
            return String.class;
        }

        @Override
        public String getColumnName(int column) {
            return "";
        }

        @Override
        public boolean isCellEditable(int rowIndex, int columnIndex) {
            return false;
        }
        
        public Object getValueAt(int rowIndex, int columnIndex) {
            if(columnIndex == 0) {
                switch(rowIndex) {
                    case 0:  return "EventId:";
                    case 1:  return "Timestamp:";
                    case 2:  return "Type:";
                    case 3:  return "Class:";
                    default:
                        throw new IllegalArgumentException("unknown row " + rowIndex);
                }
            } else {
                switch(rowIndex) {
                    case 0:  return event == null ? "" : Integer.toString(event.getEventId());
                    case 1:  return event == null ? "" : new Date(event.getTimestamp()).toString();
                    case 2:  return event == null ? "" : event.getType().toString();
                    case 3:  return event == null ? "" : event.getClass().getName();
                    default:
                        throw new IllegalArgumentException("unknown row " + rowIndex);
                }
            }
        }
        
        private String getEventDetails() {
            if(event == null) {
                return "";
            } else if (event instanceof ChangedObjectEvent) {

                String ret = null;
                Object obj = ((ChangedObjectEvent)event).getChangedObject();
                if(obj instanceof GEObject) {
                    StringWriter sw = new StringWriter();
                    XMLUtil.Context ctx = new XMLUtil.Context(sw);
                    ctx.setHideConfigurable(false);
                    ctx.setHideReadOnly(false);
                    XMLUtil.write((GEObject) obj, ctx);
                    ret = sw.getBuffer().toString();
                } else if (obj != null) {
                    ret = obj.toString();
                } else {
                    ret = "";
                }

                return ret;
            } else {
                return event.toString();
            }
        }
        
        
    }
    
    
}
