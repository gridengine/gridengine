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
 *   Copyright: 2006 by Sun Microsystems, Inc
 *
 *   All Rights Reserved.
 *
 ************************************************************************/
/*___INFO__MARK_END__*/
package com.sun.grid.jgdi.examples.jmxeventmonitor;

import com.sun.grid.jgdi.event.EventTypeEnum;
import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.text.DateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import javax.management.MBeanServerConnection;
import javax.management.Notification;
import javax.management.NotificationListener;
import javax.management.remote.JMXConnector;
import javax.swing.AbstractAction;
import javax.swing.BorderFactory;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JMenu;
import javax.swing.JMenuBar;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.JToolBar;
import javax.swing.SwingUtilities;
import javax.swing.table.AbstractTableModel;
import javax.swing.table.DefaultTableModel;

/**
 *
 */
public class MainFrame extends JFrame {
    
    //private final DefaultListModel subscriptionListModel = new DefaultListModel();
    //private final JList  subscriptionList = new JList(subscriptionListModel);
    
    private final ConnectionController controller = new ConnectionController();
    
    private final SubscriptionTableModel subscriptionTableModel = new SubscriptionTableModel(controller);
    private final JTable subscriptionTable = new JTable(subscriptionTableModel);
    
    private final DefaultTableModel eventTableModel = new DefaultTableModel();
    
    private final JTable eventTable = new JTable(eventTableModel);
    private final ConnectAction connectAction = new ConnectAction();
    private final DisConnectAction disconnectAction = new DisConnectAction();
    
    private MyNotificationListener listener = new MyNotificationListener();
    
    private JLabel statusLabel = new JLabel();
    
    /** Creates a new instance of MainFrame */
    public MainFrame() {
        
        setTitle("JGDI Event Monitor: Not connected");
        JPanel panel = new JPanel(new BorderLayout());
        getContentPane().add(panel);
        
        eventTableModel.setColumnIdentifiers(new Object[] { "time", "id", "type", "userdata" });
        
        eventTable.getColumnModel().getColumn(0).setMaxWidth(80);
        eventTable.getColumnModel().getColumn(1).setMaxWidth(30);
        eventTable.getColumnModel().getColumn(2).setMaxWidth(60);
        
        int SUB_COL_WIDTH = 40;
        int TYPE_WIDTH = 140;
        subscriptionTable.getColumnModel().getColumn(0).setMaxWidth(SUB_COL_WIDTH);
        
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        
        JScrollPane sp = new JScrollPane(subscriptionTable, JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED, JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
        sp.setMaximumSize(new Dimension(SUB_COL_WIDTH + TYPE_WIDTH + 20, Integer.MAX_VALUE));
        sp.setPreferredSize(new Dimension(SUB_COL_WIDTH + TYPE_WIDTH + 20, 300));
        sp.setBorder(
            BorderFactory.createCompoundBorder(
                BorderFactory.createTitledBorder("Subscription:"),
                BorderFactory.createLoweredBevelBorder()));
            
        panel.add(sp, BorderLayout.EAST);
        
        sp = new JScrollPane(eventTable);
        sp.setBorder(
            BorderFactory.createCompoundBorder(
                BorderFactory.createTitledBorder("Events:"),
                BorderFactory.createLoweredBevelBorder()));
        panel.add(sp, BorderLayout.CENTER);
        
        JPanel statusLabelPanel = new JPanel(new BorderLayout());
        
        statusLabel.setText("OK");
        statusLabelPanel.add(statusLabel, BorderLayout.CENTER);
        statusLabelPanel.setBorder(BorderFactory.createLoweredBevelBorder());
        panel.add(statusLabelPanel, BorderLayout.SOUTH);
        
        JToolBar toolBar = new JToolBar();
        toolBar.add(connectAction);
        toolBar.add(disconnectAction);
        getContentPane().add(toolBar, BorderLayout.NORTH);
        
        JMenuBar mb = new JMenuBar();
        
        JMenu menu = new JMenu("File");
        mb.add(menu);
        
        menu.add(connectAction);
        menu.add(disconnectAction);
        menu.addSeparator();
        menu.add(new ExitAction());
        setJMenuBar(mb);
        
        disconnectAction.setEnabled(false);
        subscriptionTable.setEnabled(false);
        
        controller.addNotificationListener(new MyNotificationListener(), null, null);
        controller.addListener(new ControllerListener());
    }
    
    
    private class ControllerListener implements ConnectionController.Listener {
        
        public void connected(final String host, final int port, final Set<EventTypeEnum> subscription) {
            if(SwingUtilities.isEventDispatchThread()) {
                setTitle("JGDI Event Monitor: " + host + ":" + port);
                connectAction.setEnabled(false);
                disconnectAction.setEnabled(true);
                subscriptionTable.setEnabled(true);
                statusLabel.setText("Connected to " + host + ":" + port);
            } else { 
                SwingUtilities.invokeLater(new Runnable() {
                    public void run() {
                        connected(host, port, subscription);
                    }
                });
            }
        }

        public void disconnected() {
            if(SwingUtilities.isEventDispatchThread()) {
                setTitle("JGDI Event Monitor: Not connected");
                connectAction.setEnabled(true);
                disconnectAction.setEnabled(false);
                subscriptionTable.setEnabled(false);
                statusLabel.setText("disconnected");
            } else {
                SwingUtilities.invokeLater(new Runnable() {
                    public void run() {
                        disconnected();
                    }
                });
            }
        }

        public void subscribed(final Set<EventTypeEnum> types) {
            if(SwingUtilities.isEventDispatchThread()) {
                statusLabel.setText(types + " subscribed");
            } else {
                SwingUtilities.invokeLater(new Runnable() {
                    public void run() {
                        subscribed(types);
                    }
                });
            }
        }

        public void unsubscribed(final Set<EventTypeEnum> types) {
            if(SwingUtilities.isEventDispatchThread()) {
                statusLabel.setText(types + " unsubscribed");
            } else {
                SwingUtilities.invokeLater(new Runnable() {
                    public void run() {
                        unsubscribed(types);
                    }
                });
            }
        }

        public void errorOccured(final Throwable ex) {
            if(SwingUtilities.isEventDispatchThread()) {
                ErrorDialog.showErrorDialog(MainFrame.this, ex);
            } else {
                SwingUtilities.invokeLater(new Runnable() {
                    public void run() {
                        errorOccured(ex);
                    }
                });
            }
        }
        
        
    }
    
    private class ConnectAction extends AbstractAction {
        
        public ConnectAction() {
            super("connect");
        }

        public void actionPerformed(ActionEvent e) {

            statusLabel.setText("Connecting");
            
            while(true) {
                String hostAndPort = JOptionPane.showInputDialog(MainFrame.this, "Enter <host>:<port>", "connection", JOptionPane.QUESTION_MESSAGE);

                if(hostAndPort == null) {
                    break;
                }
                String [] str = hostAndPort.split(":");
                if(str.length == 2) {
                    try {
                        String host = str[0];
                        int port = Integer.parseInt(str[1]);

                        statusLabel.setText("Connecting to " + host + ":" + port);
                        controller.connect(host, port);
                        break;
                    } catch(NumberFormatException nfe) {
                        JOptionPane.showMessageDialog(MainFrame.this, "Invalid port '" + str[1] + "'");
                        continue;
                    } catch(Exception ex) {
                        ErrorDialog.showErrorDialog(MainFrame.this, ex);
                        break;
                    }
                } else {
                    JOptionPane.showMessageDialog(MainFrame.this, "Please enter <host>:<port>");
                    continue;
                }
                
                
            }            
        }
    }
    
    private class ExitAction extends AbstractAction {
        
        public ExitAction() {
            super("Exit");
        }
        public void actionPerformed(ActionEvent e) {
            System.exit(0);
        }
    }
    
    private class DisConnectAction extends AbstractAction {
        
        public DisConnectAction() {
            super("disconnect");
        }

        public void actionPerformed(ActionEvent e) {
            statusLabel.setText("Disconnecting");
            controller.disconnect();
        }
    }
    
    public class MyNotificationListener implements NotificationListener {
        
        private DateFormat df = DateFormat.getTimeInstance();
        public void handleNotification(final Notification notification, final Object handback) {
            
            if(SwingUtilities.isEventDispatchThread()) {
                
                eventTableModel.addRow(new Object[] { df.format(new Date(notification.getTimeStamp())), 
                   notification.getSequenceNumber(), notification.getType(), notification.getUserData() } );
                if(eventTableModel.getRowCount() >= 1000) {
                    eventTableModel.removeRow(0);
                }
                
            } else {
                SwingUtilities.invokeLater(new Runnable() {
                    public void run() {
                        handleNotification(notification, handback);
                    }
                });
            }
        }
    }
    
    private final static int TYPE_COL = 1;
    private final static int SUB_COL = 0;
    
    private class SubscriptionTableModel extends AbstractTableModel implements ConnectionController.Listener {
        
        private final Map<EventTypeEnum,Boolean> subscription = new HashMap<EventTypeEnum,Boolean>();
        private final List<EventTypeEnum> subscriptionList;
        
        
        private final ConnectionController controller;
        
        public SubscriptionTableModel(ConnectionController controller) {
            
            this.controller = controller;
            subscriptionList = new ArrayList(Arrays.asList(EventTypeEnum.values()));
            
            Collections.sort(subscriptionList, new Comparator<EventTypeEnum>() {
                
                public int compare(EventTypeEnum o1, EventTypeEnum o2) {
                    return o1.toString().compareTo(o2.toString());
                }
                
            });
            
            for(EventTypeEnum type: subscriptionList) {
                subscription.put(type, false);
            }
            
            controller.addListener(this);
        }
        
        public int getRowCount() {
            return subscriptionList.size();
        }

        public int getColumnCount() {
            return 2;
        }

        public Object getValueAt(int rowIndex, int columnIndex) {
            switch(columnIndex) {
                case TYPE_COL:
                    return subscriptionList.get(rowIndex);
                case SUB_COL:
                    return subscription.get(subscriptionList.get(rowIndex));
                default:
                    return "Error";
            }
        }

        public boolean isCellEditable(int rowIndex, int columnIndex) {
            return columnIndex == SUB_COL;
        }

        public void setValueAt(Object aValue, int rowIndex, int columnIndex) {
            if(columnIndex == SUB_COL) {
                EventTypeEnum type = subscriptionList.get(rowIndex);
                boolean subscribe = (Boolean)aValue;
                
                if(subscribe) {
                    statusLabel.setText("subscribing " + type);
                    controller.subscribe(Collections.singleton(type));
                } else {
                    statusLabel.setText("unsubscribing " + type);
                    controller.unsubscribe(Collections.singleton(type));
                }
            }
        }
        
        public void clearSubscription() {
            for(Map.Entry<EventTypeEnum,Boolean> entry: subscription.entrySet()) {
                entry.setValue(false);
            }
        }
        
        public EventTypeEnum getType(int row) {
            return subscriptionList.get(row);
        }
        public void setSubscription(EventTypeEnum type, boolean subscribed) {
            subscription.put(type, subscribed);
        }
        
        public boolean isSubscribed(EventTypeEnum type) {
            return subscription.get(type);
        }
        

        public String getColumnName(int column) {
            switch(column) {
                case TYPE_COL: return "Type";
                case SUB_COL: return "";
                default: return "Error";
            }
        }

        public Class<?> getColumnClass(int columnIndex) {
            switch(columnIndex) {
                case TYPE_COL: return EventTypeEnum.class;
                case SUB_COL: return Boolean.class;
                default: return null;
            }
        }

        public void connected(String host, int port, Set<EventTypeEnum> subscription) {
            subscribed(subscription);
        }

        public void disconnected() {
            if(SwingUtilities.isEventDispatchThread()) {
                clearSubscription();
                fireTableDataChanged();
            } else {
                SwingUtilities.invokeLater(new Runnable() {
                    public void run() {
                       disconnected(); 
                    }
                });
            }
        }

        public void subscribed(final Set<EventTypeEnum> types) {
            if(SwingUtilities.isEventDispatchThread()) {
                for(EventTypeEnum type: types) {
                    setSubscription(type, true);
                }
                fireTableDataChanged();
            } else {
                SwingUtilities.invokeLater(new Runnable() {
                    public void run() {
                       subscribed(types); 
                    }
                });
            }
        }

        public void unsubscribed(final Set<EventTypeEnum> types) {
            if(SwingUtilities.isEventDispatchThread()) {
                for(EventTypeEnum type: types) {
                    setSubscription(type, false);
                }
                fireTableDataChanged();
            } else {
                SwingUtilities.invokeLater(new Runnable() {
                    public void run() {
                       unsubscribed(types); 
                    }
                });
            }
        }

        public void errorOccured(Throwable ex) {
        }
        
    }
    
}
