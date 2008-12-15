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

import com.izforge.izpack.installer.InstallData;
import com.izforge.izpack.installer.InstallerFrame;
import com.izforge.izpack.installer.IzPanel;
import com.izforge.izpack.util.Debug;
import com.izforge.izpack.util.VariableSubstitutor;
import com.sun.grid.installer.util.CommandExecutor;
import com.sun.grid.installer.util.Config;
import com.sun.grid.installer.util.ExtendedFile;
import com.sun.grid.installer.util.Util;
import java.awt.Color;
import java.awt.Font;
import java.awt.FontMetrics;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.FocusAdapter;
import java.awt.event.FocusEvent;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.io.File;
import java.io.FileNotFoundException;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import java.util.Timer;
import java.util.TimerTask;
import java.util.Vector;
import java.util.Set;
import java.util.concurrent.Executors;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.text.MessageFormat;

import javax.swing.AbstractButton;
import javax.swing.JButton;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JProgressBar;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.JTextField;
import javax.swing.SwingUtilities;
import javax.swing.event.TableModelEvent;
import javax.swing.event.TableModelListener;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.JTableHeader;
import javax.swing.table.TableColumn;
import net.n3.nanoxml.XMLElement;

public class HostPanel extends IzPanel implements Config {
    private ActionListener[] nextButtonListeners = null;
    private boolean isQmasterInst = true;
    private boolean isShadowdInst = true;
    private boolean isExecdInst = true;
    private boolean isBdbInst = true;
    private boolean isExpressInst = true;
    public static String[] SELECTION_TABS = null;
    public static String[] SELECTION_TABS_TOOLTIPS = null;
    public static String[] INSTALL_TABS = null;
    public static String[] INSTALL_TABS_TOOLTIPS = null;

    public static boolean installMode = false;

    private static Properties localizedMessages = new Properties();

    /** Creates new HostPanel */
    public HostPanel(InstallerFrame parent, InstallData idata) {
        super(parent, idata);
        //TODO: verify that shell name is OK

        SELECTION_TABS = new String[]{
                    getLabel("tab.all.label"),
                    getLabel("tab.reachable.label"),
                    getLabel("tab.unreachable.label")};
        SELECTION_TABS_TOOLTIPS = new String[]{
                    getLabel("tab.all.label.tooltip"),
                    getLabel("tab.reachable.label.tooltip"),
                    getLabel("tab.unreachable.label.tooltip")};
        INSTALL_TABS = new String[]{
                    getLabel("tab.installing.label"),
                    getLabel("tab.done.label"),
                    getLabel("tab.failed.label")};
        INSTALL_TABS_TOOLTIPS = new String[]{
                    getLabel("tab.installing.label.tooltip"),
                    getLabel("tab.done.label.tooltip"),
                    getLabel("tab.failed.label.tooltip")};

        Properties stateLocalizedTexts = new Properties();        
        if (parent.langpack != null) {
            Set<String> keys = parent.langpack.keySet();
            for (Iterator<String> it = keys.iterator(); it.hasNext();) {
                String key = it.next();
                if (key.startsWith(LANGID_PREFIX_STATE)) {
                    stateLocalizedTexts.put(key, parent.langpack.get(key));
                } else if (key.startsWith("msg.")) {
                    localizedMessages.put(key, parent.langpack.get(key));
                }
            }
        }

        Host.State.localizedTexts = stateLocalizedTexts;

        validList = new HostList();
        invalidList = new HostList();
        initComponents();
        defaultSelectionColor = hostTF.getSelectionColor();
        setComponentSelectionVisible(advancedMode);
        progressBar.setVisible(false);
        cancelB.setVisible(false);
        statusBar.setVisible(false);

        hostTF.addMouseListener(new MouseAdapter() {
            @Override
            public void mouseClicked(MouseEvent e) {
                super.mouseClicked(e);
                if (errorMessageVisible) {
                    statusBar.setVisible(false);
                }
            }
        });
        
        hostTF.addKeyListener(new KeyAdapter() {
            @Override
            public void keyTyped(KeyEvent e) {
                if (e.getKeyChar() == '\n') {
                    buttonActionPerformed(new ActionEvent(e.getSource(), e.getKeyCode(), e.paramString()));
                } else if (errorMessageVisible) {
                    statusBar.setVisible(false);
                }
            }

        });

        hostRB.addFocusListener(new FocusAdapter() {
            @Override
            public void focusGained(FocusEvent e) {
                if (!hostRB.isSelected() && errorMessageVisible) {
                    statusBar.setVisible(false);
                }
            }
        });

        fileRB.addFocusListener(new FocusAdapter() {
            @Override
            public void focusGained(FocusEvent e) {
                if (!fileRB.isSelected() && errorMessageVisible) {
                    statusBar.setVisible(false);
                }
            }
        });

        String[] selectionHeaders = getSelectionLabelVars();
        String[] headerToltips = new String[selectionHeaders.length];
        for (int i = 0; i < selectionHeaders.length; i++) {
            headerToltips[i] = getTooltip(selectionHeaders[i]);
        }

        lists = new Vector<HostList>();
        tables = new Vector<HostTable>();
        HostList list;
        HostTable table;
        tabbedPane.removeAll();
        for (int i = 0; i < SELECTION_TABS.length; i++) {
            list = new HostList();
            table = new HostTable(this, i);
            lists.add(list);
            tables.add(table);

            table.setModel(new HostSelectionTableModel(list, getSelectionHeaders(), getSelectionClassTypes()));
//            table.setAutoResizeMode(JTable.AUTO_RESIZE_OFF);

            table.setTableHeader(new TooltipTableHeader(table.getColumnModel(), headerToltips));
            JTableHeader header = table.getTableHeader();
            SortedColumnHeaderRenderer headerRenderer = new SortedColumnHeaderRenderer(header, parent.icons.getImageIcon("columns.sorted.asc"), parent.icons.getImageIcon("columns.sorted.desc"));
            for (int col = 0; col < table.getColumnCount(); col++) {
                table.getColumnModel().getColumn(col).setHeaderRenderer(headerRenderer);
            }

            table.getColumn(getLabel("column.state.label")).setCellRenderer(new StateCellRenderer());

            header.addMouseListener(new TableHeaderListener(header, headerRenderer));

            table.getModel().addTableModelListener(new TableModelListener() {
                public void tableChanged(TableModelEvent e) {
                    HostPanel.this.tableChanged();
                }
            });

            tabbedPane.addTab(SELECTION_TABS[i] + " (" + lists.get(i).size() + ")", new JScrollPane(tables.get(i)));
            tabbedPane.setToolTipTextAt(i, SELECTION_TABS_TOOLTIPS[i]);
        }

        hostTF.requestFocus();
        this.repaint();

        threadPool = (ThreadPoolExecutor) Executors.newFixedThreadPool(Util.RESOLVE_THREAD_POOL_SIZE);
        threadPool.setThreadFactory(new InstallerThreadFactory());
    }

    private String[] getSelectionHeaders() {
        String[] selectionLabels = getSelectionLabelVars();

        for (int i = 0; i < selectionLabels.length; i++) {
            selectionLabels[i] = getLabel(selectionLabels[i]);
        }

        return selectionLabels;
    }

    private String[] getSelectionLabelVars() {
        return new String[]{
                    "column.hostname.label",
                    "column.ip.address.label",
                    "column.arch.label",
                    "column.qmaster.label",
                    "column.shadowd.label",
                    "column.execd.label",
                    "column.exec.spool.dir.label",
                    "column.admin.label",
                    "column.submit.label",
                    "column.bdb.label",
                    "column.state.label"};
    }

    private String[] getInstallHeaders() {
        String[] installLabels = getInstallLabelVars();

        for (int i = 0; i < installLabels.length; i++) {
            installLabels[i] = getLabel(installLabels[i]);
        }

        return installLabels;
    }

    private String[] getInstallLabelVars() {
        return new String[]{
                    "column.hostname.label",
                    "column.ip.address.label",
                    "column.component.label",
                    "column.arch.label",
                    "column.progress.label",
                    "column.log.label"};
    }

    private Class[] getSelectionClassTypes() {
        List<Class> list = new ArrayList<Class>();

        list.add(String.class);
        list.add(String.class);
        list.add(String.class);
        list.add(Boolean.class);
        list.add(Boolean.class);
        list.add(Boolean.class);
        list.add(String.class);
        list.add(Boolean.class);
        list.add(Boolean.class);
        list.add(Boolean.class);
        list.add(Host.State.class);

        Class[] a = new Class[list.size()];
        return list.toArray(a);
    }

    private Class[] getInstallClassTypes() {
        List<Class> list = new ArrayList<Class>();

        list.add(String.class);
        list.add(String.class);
        list.add(String.class);
        list.add(String.class);
        list.add(Host.State.class);
        list.add(String.class);

        Class[] a = new Class[list.size()];
        return list.toArray(a);
    }

    /** This method is called from within the constructor to
     * initialize the form.
     * WARNING: Do NOT modify this code. The content of this method is
     * always regenerated by the Form Editor.
     */
    @SuppressWarnings("unchecked")
    // <editor-fold defaultstate="collapsed" desc="Generated Code">//GEN-BEGIN:initComponents
    private void initComponents() {

        hostButtonGroup = new javax.swing.ButtonGroup();
        tabbedPane = new javax.swing.JTabbedPane();
        allHostsScrollPane = new javax.swing.JScrollPane();
        progressBar = new javax.swing.JProgressBar();
        hostRB = new javax.swing.JRadioButton();
        hostTF = new javax.swing.JTextField();
        addB = new javax.swing.JButton();
        fileB = new javax.swing.JButton();
        fileRB = new javax.swing.JRadioButton();
        componentSelectionPanel = new javax.swing.JPanel();
        shadowCB = new javax.swing.JCheckBox();
        execCB = new javax.swing.JCheckBox();
        adminCB = new javax.swing.JCheckBox();
        submitCB = new javax.swing.JCheckBox();
        qmasterCB = new javax.swing.JCheckBox();
        bdbCB = new javax.swing.JCheckBox();
        statusBar = new javax.swing.JLabel();
        cancelB = new javax.swing.JButton();

        hostButtonGroup.add(hostRB);
        hostButtonGroup.add(fileRB);

        allHostsScrollPane.setBackground(new java.awt.Color(255, 255, 255));
        tabbedPane.addTab("All hosts (0)", allHostsScrollPane);

        progressBar.setMaximum(10);
        progressBar.setMinimum(1);

        hostButtonGroup.add(hostRB);
        hostRB.setSelected(true);
        hostRB.setText(getLabel("hostinput.label"));
        hostRB.setToolTipText(getTooltip("hostinput.label.tooltip"));
        hostRB.addFocusListener(new java.awt.event.FocusAdapter() {
            public void focusGained(java.awt.event.FocusEvent evt) {
                hostRBFocusGained(evt);
            }
        });

        hostTF.setText(idata.getVariable("add.hostinput"));
        hostTF.setToolTipText(getTooltip("hostinput.label.tooltip"));
        hostTF.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                hostTFActionPerformed(evt);
            }
        });
        hostTF.addFocusListener(new java.awt.event.FocusAdapter() {
            public void focusGained(java.awt.event.FocusEvent evt) {
                hostTFFocusGained(evt);
            }
        });

        addB.setText(getLabel("button.add.label"));
        addB.setToolTipText(getTooltip("button.add.label.tooltip"));
        addB.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                addBActionPerformed(evt);
            }
        });

        fileB.setText(getLabel("UserInputPanel.search.browse"));
        fileB.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                fileBActionPerformed(evt);
            }
        });
        fileB.addFocusListener(new java.awt.event.FocusAdapter() {
            public void focusGained(java.awt.event.FocusEvent evt) {
                fileBFocusGained(evt);
            }
        });

        hostButtonGroup.add(fileRB);
        fileRB.setText(getLabel("fromfile.label"));
        fileRB.setToolTipText(getTooltip("fromfile.label.tooltip"));
        fileRB.addFocusListener(new java.awt.event.FocusAdapter() {
            public void focusGained(java.awt.event.FocusEvent evt) {
                fileRBFocusGained(evt);
            }
        });

        shadowCB.setText(getLabel("component.shadow.host.label"));
        shadowCB.setToolTipText(getTooltip("component.shadow.host.label.tooltip"));

        execCB.setSelected(true);
        execCB.setText(getLabel("component.execd.host.label"));
        execCB.setToolTipText(getTooltip("component.execd.host.label.tooltip"));

        adminCB.setSelected(true);
        adminCB.setText(getLabel("component.admin.host.label"));
        adminCB.setToolTipText(getTooltip("component.admin.host.label.tooltip"));

        submitCB.setSelected(true);
        submitCB.setText(getLabel("component.submit.host.label"));
        submitCB.setToolTipText(getTooltip("component.submit.host.label.tooltip"));

        qmasterCB.setText(getLabel("component.qmaster.host.label"));
        qmasterCB.setToolTipText(getTooltip("component.shadow.host.label.tooltip"));

        bdbCB.setText(getLabel("component.bdb.host.label"));
        bdbCB.setToolTipText(getTooltip("component.bdb.host.label.tooltip"));

        org.jdesktop.layout.GroupLayout componentSelectionPanelLayout = new org.jdesktop.layout.GroupLayout(componentSelectionPanel);
        componentSelectionPanel.setLayout(componentSelectionPanelLayout);
        componentSelectionPanelLayout.setHorizontalGroup(
            componentSelectionPanelLayout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
            .add(0, 666, Short.MAX_VALUE)
            .add(componentSelectionPanelLayout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
                .add(componentSelectionPanelLayout.createSequentialGroup()
                    .addContainerGap()
                    .add(qmasterCB)
                    .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)
                    .add(shadowCB)
                    .addPreferredGap(org.jdesktop.layout.LayoutStyle.UNRELATED)
                    .add(execCB)
                    .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)
                    .add(bdbCB)
                    .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)
                    .add(adminCB)
                    .addPreferredGap(org.jdesktop.layout.LayoutStyle.UNRELATED)
                    .add(submitCB)
                    .addContainerGap(62, Short.MAX_VALUE)))
        );
        componentSelectionPanelLayout.setVerticalGroup(
            componentSelectionPanelLayout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
            .add(0, 34, Short.MAX_VALUE)
            .add(componentSelectionPanelLayout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
                .add(componentSelectionPanelLayout.createSequentialGroup()
                    .addContainerGap()
                    .add(componentSelectionPanelLayout.createParallelGroup(org.jdesktop.layout.GroupLayout.BASELINE)
                        .add(shadowCB)
                        .add(execCB)
                        .add(qmasterCB)
                        .add(bdbCB)
                        .add(adminCB)
                        .add(submitCB))
                    .addContainerGap(org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE)))
        );

        statusBar.setForeground(new java.awt.Color(255, 0, 0));
        statusBar.setIcon(parent.icons.getImageIcon("error.small"));

        cancelB.setText(getLabel("button.cancel.label"));
        cancelB.setToolTipText(getTooltip("button.cancel.label.tooltip"));
        cancelB.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                cancelBActionPerformed(evt);
            }
        });

        org.jdesktop.layout.GroupLayout layout = new org.jdesktop.layout.GroupLayout(this);
        this.setLayout(layout);
        layout.setHorizontalGroup(
            layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
            .add(layout.createSequentialGroup()
                .addContainerGap()
                .add(layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
                    .add(layout.createSequentialGroup()
                        .add(tabbedPane, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, 1300, Short.MAX_VALUE)
                        .addContainerGap())
                    .add(layout.createSequentialGroup()
                        .add(progressBar, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, 1184, Short.MAX_VALUE)
                        .add(9, 9, 9)
                        .add(cancelB)
                        .addContainerGap())
                    .add(layout.createSequentialGroup()
                        .add(layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
                            .add(hostRB)
                            .add(fileRB))
                        .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)
                        .add(layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
                            .add(fileB)
                            .add(layout.createSequentialGroup()
                                .add(hostTF, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, 815, Short.MAX_VALUE)
                                .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)))
                        .add(addB)
                        .add(284, 284, 284))
                    .add(layout.createSequentialGroup()
                        .add(componentSelectionPanel, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                        .addContainerGap(646, Short.MAX_VALUE))
                    .add(layout.createSequentialGroup()
                        .add(statusBar, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, 413, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                        .addContainerGap())))
        );
        layout.setVerticalGroup(
            layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
            .add(org.jdesktop.layout.GroupLayout.TRAILING, layout.createSequentialGroup()
                .add(layout.createParallelGroup(org.jdesktop.layout.GroupLayout.BASELINE)
                    .add(hostRB)
                    .add(addB)
                    .add(hostTF, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE))
                .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)
                .add(layout.createParallelGroup(org.jdesktop.layout.GroupLayout.BASELINE)
                    .add(fileRB)
                    .add(fileB))
                .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)
                .add(statusBar, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, 23, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)
                .add(componentSelectionPanel, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, 34, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)
                .add(tabbedPane, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, 330, Short.MAX_VALUE)
                .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)
                .add(layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
                    .add(cancelB, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, 20, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                    .add(progressBar, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, 20, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE))
                .addContainerGap())
        );
    }// </editor-fold>//GEN-END:initComponents

    @Override
    public void panelActivate() {
        isQmasterInst = parent.getRules().isConditionTrue(COND_INSTALL_QMASTER, idata.getVariables());
        isShadowdInst = parent.getRules().isConditionTrue(COND_INSTALL_SHADOWD, idata.getVariables());
        isExecdInst = parent.getRules().isConditionTrue(COND_INSTALL_EXECD, idata.getVariables());
        isBdbInst = parent.getRules().isConditionTrue(COND_INSTALL_BDB, idata.getVariables()) ||
                parent.getRules().isConditionTrue(COND_SPOOLING_BDBSERVER, idata.getVariables());
        isExpressInst = parent.getRules().isConditionTrue(COND_EXPRESS_INSTALL, idata.getVariables());

        enableInstallButton(false);
        triggerInstallButton(true);
        setupComponentSelectionPanel();
        setColumnsWidth();

        VariableSubstitutor vs = new VariableSubstitutor(idata.getVariables());;

        // In some cases the execd spool dir does not get substituted
        idata.setVariable(VAR_EXECD_SPOOL_DIR, vs.substituteMultiple(idata.getVariable(VAR_EXECD_SPOOL_DIR), null));

        // Initialize the table(s) with the qmaster and the Berkeley DB host if it's necessary
        if (true) {
            try {
                boolean isQmasterExist = HostSelectionTableModel.getQmasterHost() != null;
                boolean isBdbExist = HostSelectionTableModel.getBdbHost() != null;
                parent.lockPrevButton();

                ArrayList<String> host = new ArrayList<String>();
                Properties prop = new Properties();
                String arch = "";
                String message = "";
                if (isQmasterInst && !isQmasterExist) {
                    host.add(idata.getVariable(VAR_QMASTER_HOST));
                    resolveHosts(Host.Type.HOSTNAME, idata.getVariable(VAR_QMASTER_HOST), host, true, false, false, false, true, true, idata.getVariable(VAR_EXECD_SPOOL_DIR));

                    if (HostSelectionTableModel.getQmasterHost() == null) {
                        try {
                            arch = lists.get(0).get(0).getArchitecture();
                        } finally {
                            prop.setProperty(PARAMETER_1, arch);
                        }
                        
                        vs = new VariableSubstitutor(prop);
                        message = vs.substituteMultiple(parent.langpack.getString("warning.qmaster.arch.unsupported.message"), null);
                        emitWarning(parent.langpack.getString("installer.warning"), message);
                    }
                }
                host.clear();

                if (isBdbInst && !isBdbExist) {
                    host.add(idata.getVariable(VAR_DB_SPOOLING_SERVER));
                    resolveHosts(Host.Type.HOSTNAME, idata.getVariable(VAR_DB_SPOOLING_SERVER), host, false, true, false, false, false, false, idata.getVariable(VAR_EXECD_SPOOL_DIR));

                    if (HostSelectionTableModel.getBdbHost() == null) {
                        try {
                            arch = lists.get(0).get(0).getArchitecture();
                        } finally {
                            prop.setProperty(PARAMETER_1, arch);
                        }

                        vs = new VariableSubstitutor(prop);
                        message = vs.substituteMultiple(parent.langpack.getString("warning.bdbserver.arch.unsupported.message"), null);
                        emitWarning(parent.langpack.getString("installer.warning"), message);
                    }
                }

                if (tables.get(1).getRowCount() > 0) {
                    enableInstallButton(true);
                }
            } catch (Exception e) {
                Debug.error(e);
            } finally {
                vs = null;
                parent.unlockPrevButton();
            }
        }

        /**
         * Check directories in express mode
         */
        if (isExpressInst) {
            // JMX keystore path (do it only one time) in case of JMX and JMX SSL enabled
            if (parent.getRules().isConditionTrue(COND_JMX, idata.getVariables()) &&
                    parent.getRules().isConditionTrue(COND_JMX_SSL, idata.getVariables()) &&
                    !idata.getVariable(VAR_JMX_SSL_KEYSTORE).equals(idata.getVariable(VAR_JMX_SSL_KEYSTORE_DEF))) {
                ExtendedFile ef = new ExtendedFile(idata.getVariable(VAR_JMX_SSL_KEYSTORE)).getFirstExistingParent();
                
                if (!ef.hasWritePermission(idata.getVariable(VAR_USER_NAME), idata.getVariable(VAR_USER_GROUP))) {
                    idata.setVariable(VAR_JMX_SSL_KEYSTORE, idata.getVariable(VAR_JMX_SSL_KEYSTORE_DEF));
                }
            }

            // Spooling dir
            if (!isBdbInst && !idata.getVariable(VAR_DB_SPOOLING_DIR_BDB).equals(VAR_DB_SPOOLING_DIR_BDB_DEF)) {
                String fstype = Util.getDirFSType(idata.getVariables(), idata.getVariable(VAR_DB_SPOOLING_DIR_BDB));

                if (fstype.equals("nfs")) {
                    idata.setVariable(VAR_DB_SPOOLING_DIR_BDB, idata.getVariable(VAR_DB_SPOOLING_DIR_BDB_DEF));
                }

                Debug.trace("add.db.spooling.dir.bdb='" + idata.getVariable(VAR_DB_SPOOLING_DIR_BDB) + "'");
            }
        }
    }

    @Override
    public void panelDeactivate() {
        enableInstallButton(true);
        triggerInstallButton(false);
    }

    public void triggerInstallButton(boolean show) {
        JButton nextButton = parent.getNextButton();

        if (show) {
            nextButton.setText(getLabel("InstallPanel.install"));
            nextButton.setIcon(parent.icons.getImageIcon("install"));
            nextButtonListeners = removeListeners(nextButton);

            nextButton.addActionListener(new java.awt.event.ActionListener() {

                public void actionPerformed(java.awt.event.ActionEvent evt) {
                    installButtonActionPerformed();
                }
            });
        } else {
            nextButton.setText(getLabel("installer.next"));
            nextButton.setIcon(parent.icons.getImageIcon("stepforward"));
            removeListeners(nextButton);

            for (int i = 0; i < nextButtonListeners.length; i++) {
                nextButton.addActionListener(nextButtonListeners[i]);
            }
        }
    }

    private ActionListener[] removeListeners(AbstractButton button) {
        ActionListener[] listeners = button.getActionListeners();

        for (int i = 0; i < listeners.length; i++) {
            button.removeActionListener(listeners[i]);
        }

        return listeners;
    }

    public String getTooltip(String key) {
        if (!key.endsWith(TOOLTIP)) {
            key = key + "." + TOOLTIP;
        }

        String tooltip = null;
        if (parent.langpack != null) {
            tooltip = parent.langpack.getString(key);
        }

        VariableSubstitutor vs = new VariableSubstitutor(idata.getVariables());
        tooltip = vs.substituteMultiple(tooltip, null);

        if (tooltip.equals(key)) {
            tooltip = null;
        }

        return tooltip;
    }

    public String getLabel(String key) {
        String label = "";

        if (parent.langpack != null) {
            label = parent.langpack.getString(key);
        }

        return label;
    }

    private void setupComponentSelectionPanel() {
        componentSelectionPanel.setVisible(!isExpressInst);
        if (!isExecdInst) {
            execCB.setSelected(false);
            execCB.setVisible(false);
        } else {
            execCB.setSelected(true);
            execCB.setVisible(true);
        }
        if (!isShadowdInst) {
            shadowCB.setSelected(false);
            shadowCB.setVisible(false);
        } else {
            shadowCB.setSelected(true);
            shadowCB.setVisible(true);
        }
        qmasterCB.setSelected(false);
        qmasterCB.setVisible(false);
        bdbCB.setSelected(false);
        bdbCB.setVisible(false);
    }

    private void setColumnsWidth() {
        int minWidth;
        TableColumn column;

        for (HostTable table : tables) {
            try {
                for (int col = 0; col < table.getColumnModel().getColumnCount() - 1; col++) {
                    column = table.getColumnModel().getColumn(col);
                    String header = (String) column.getIdentifier();
                    minWidth = getColumnTextWidth(table, header) + 30;

                    if (header.equals(getLabel("column.qmaster.label"))) {
                        if (!isQmasterInst) {
                            column.setPreferredWidth(0);
                            column.setMinWidth(0);
                            column.setMaxWidth(0);
                        } else {
                            column.setMinWidth(minWidth);
                            column.setPreferredWidth(minWidth);
                            column.setMaxWidth(minWidth);
                        }
                    } else if (header.equals(getLabel("column.shadowd.label"))) {
                        if (!isShadowdInst) {
                            column.setPreferredWidth(0);
                            column.setMinWidth(0);
                            column.setMaxWidth(0);
                        } else {
                            column.setMinWidth(minWidth);
                            column.setPreferredWidth(minWidth);
                            column.setMaxWidth(minWidth);
                        }
                    } else if (header.equals(getLabel("column.execd.label"))) {
                        if (!isExecdInst) {
                            column.setPreferredWidth(0);
                            column.setMinWidth(0);
                            column.setMaxWidth(0);
                        } else {
                            column.setMinWidth(minWidth);
                            column.setPreferredWidth(minWidth);
                            column.setMaxWidth(minWidth);
                        }
                    } else if (header.equals(getLabel("column.exec.spool.dir.label"))) {
                        if (isExecdInst && !isExpressInst) {
                            column.setMinWidth(15);
                            column.setMaxWidth(Integer.MAX_VALUE);
                            column.setPreferredWidth(minWidth);
                            column.setWidth(minWidth);
                        } else {
                            column.setPreferredWidth(0);
                            column.setMinWidth(0);
                            column.setMaxWidth(0);
                        }
                    } else if (header.equals(getLabel("column.admin.label"))) {
                        column.setMinWidth(minWidth);
                        column.setPreferredWidth(minWidth);
                        column.setMaxWidth(minWidth);
                    } else if (header.equals(getLabel("column.submit.label"))) {
                        column.setMinWidth(minWidth);
                        column.setPreferredWidth(minWidth);
                        column.setMaxWidth(minWidth);
                    } else if (header.equals(getLabel("column.bdb.label"))) {
                        if (!isBdbInst) {
                            column.setPreferredWidth(0);
                            column.setMinWidth(0);
                            column.setMaxWidth(0);
                        } else {
                            column.setMinWidth(minWidth);
                            column.setPreferredWidth(minWidth);
                            column.setMaxWidth(minWidth);
                        }
                    } else {
                        column.setMinWidth(minWidth);
                        column.setPreferredWidth(minWidth);
                        column.setMaxWidth(Integer.MAX_VALUE);
                        column.setWidth(minWidth);
                    }
                }

            } catch (IllegalArgumentException e) {
            }
        }
    }

    private int getColumnTextWidth(JTable table, String columnHeader) {
        JButton defButton = new JButton(columnHeader);
        Font font = table.getTableHeader().getFont();
        FontMetrics metrics = defButton.getFontMetrics(font);

        return SwingUtilities.computeStringWidth(metrics, columnHeader);
    }

    @Override
    public void makeXMLData(XMLElement arg0) {
    }

    private void addHostFocusGained() {
        hostTF.setSelectionEnd(0);
        hostTF.setSelectionColor(defaultSelectionColor);
    }

	private void hostTFActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_hostTFActionPerformed
		try {
			final String pattern = hostTF.getText().trim();
			List<String> list = Util.parseHostPattern(pattern);
			resolveHosts(Host.Type.HOSTNAME, pattern, list, qmasterCB.isSelected(), bdbCB.isSelected(), shadowCB.isSelected(), execCB.isSelected(), adminCB.isSelected(), submitCB.isSelected(), idata.getVariable(VAR_EXECD_SPOOL_DIR));
		} catch (IllegalArgumentException ex) {
			statusBar.setVisible(true);
			statusBar.setText("Error: " + ex.getMessage());
			hostTF.setSelectionStart(0);
			hostTF.setSelectionEnd(hostTF.getText().length());
			errorMessageVisible = true;
		}
	}//GEN-LAST:event_hostTFActionPerformed

	private void hostTFFocusGained(java.awt.event.FocusEvent evt) {//GEN-FIRST:event_hostTFFocusGained
        hostRB.setSelected(true);
        hostTF.setEditable(true);
        //addHostFocusGained();
        selectTextField(hostTF);
	}//GEN-LAST:event_hostTFFocusGained

	private void fileBFocusGained(java.awt.event.FocusEvent evt) {//GEN-FIRST:event_fileBFocusGained
        fileRB.setSelected(true);
        hostTF.setEditable(false);
	}//GEN-LAST:event_fileBFocusGained

	private void fileBActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_fileBActionPerformed
		final JFileChooser fc = new JFileChooser();
		int ret = fc.showOpenDialog(this.getParent());
		if (ret == JFileChooser.APPROVE_OPTION) {
			File f = fc.getSelectedFile();
			try {
				List<String> list = Util.parseFileList(f);
				resolveHosts(Host.Type.HOSTNAME, "/"+f.getAbsolutePath(), list, qmasterCB.isSelected(), bdbCB.isSelected(), shadowCB.isSelected(), execCB.isSelected(), adminCB.isSelected(), submitCB.isSelected(), idata.getVariable(VAR_EXECD_SPOOL_DIR));
			} catch (IllegalArgumentException ex) {
				statusBar.setVisible(true);
				statusBar.setText("Error: " + ex.getMessage());
				errorMessageVisible = true;
			} catch (FileNotFoundException ex) {
				statusBar.setVisible(true);
				statusBar.setText("Error: " + ex.getMessage());
				errorMessageVisible = true;
			}
		}
	}//GEN-LAST:event_fileBActionPerformed

    private void selectTextField(JTextField tf) {
        tf.requestFocus();
        tf.setSelectionStart(0);
        tf.setSelectionEnd(tf.getText().length());
        lastSelectedTF = tf;
    }

	private void hostRBFocusGained(java.awt.event.FocusEvent evt) {//GEN-FIRST:event_hostRBFocusGained
        hostTF.setEditable(true);
        selectTextField(hostTF);
	}//GEN-LAST:event_hostRBFocusGained

	private void fileRBFocusGained(java.awt.event.FocusEvent evt) {//GEN-FIRST:event_fileRBFocusGained
        hostTF.setEditable(false);
        lastSelectedTF.setSelectionEnd(0);
	}//GEN-LAST:event_fileRBFocusGained

    private void buttonActionPerformed(java.awt.event.ActionEvent evt) {
        if (hostRB.isSelected()) {
            hostTFActionPerformed(evt);
            hostRB.requestFocus();
        }
    }

    private void setComponentSelectionVisible(boolean b) {
        componentSelectionPanel.setVisible(b);
    }

	private void addBActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_addBActionPerformed
        buttonActionPerformed(evt);
	}//GEN-LAST:event_addBActionPerformed

    private void cancelBActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_cancelBActionPerformed
        cancelActions();
    }//GEN-LAST:event_cancelBActionPerformed

    private void cancelActions() {
        //Cancel host selection thread pool
        List<Runnable> waitingTasks = threadPool.shutdownNow();

        try {
            threadPool.awaitTermination(200, TimeUnit.MILLISECONDS);
            threadPool.purge();
        } catch (InterruptedException e) {
            Debug.error(e);
        }

        try {
            ThreadPoolExecutor tmp = (ThreadPoolExecutor) Executors.newFixedThreadPool(waitingTasks.size() * 2);
            tmp.setThreadFactory(new InstallerThreadFactory());

            for (Iterator<Runnable> it = waitingTasks.iterator(); it.hasNext();) {
                TestableTask runnable = (TestableTask) it.next();
                runnable.setTestMode(true);
                runnable.setTestExitValue(CommandExecutor.EXITVAL_INTERRUPTED);
                runnable.setTestOutput(new Vector<String>());
                try {
                    Debug.trace("Cancel task: " + runnable.getTaskName());
                    tmp.execute(runnable);
                    tmp.remove(runnable);
                } catch (Exception e) {
                    Debug.error("Failed to cancel task: " + runnable.getTaskName());
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        //Stop updateTab thread
        /*if (progressTimer != null) {
            progressTimer.cancel();
            progressTimer = null;
        }*/

        progressBar.setVisible(false);
        cancelB.setVisible(false);
    }

    public synchronized void tableChanged() {
        String[] tabTitles = (installMode ? HostPanel.INSTALL_TABS : HostPanel.SELECTION_TABS);
        for (int i = 0; i < tables.size(); i++) {
            tabbedPane.setTitleAt(i, tabTitles[i] + " (" + tables.get(i).getRowCount() + ")");
        }
    }

    private void resolveHosts(Host.Type type, String pattern, List<String> list, boolean isQmasterHost, boolean isBDBHost, boolean isShadowHost, boolean isExecutionHost, boolean isAdminHost, boolean isSubmitHost, String execdSpoolDir) {
        HostList hostList = lists.get(0);
        HostTable table = tables.get(0);
        HostSelectionTableModel model = (HostSelectionTableModel) table.getModel();

        //Periodically update the tabs and progressbars, while resolving
        if (progressTimer == null) {
            progressTimer = new Timer("ResolveTimer");
            progressTimer.schedule(new UpdateInstallProgressTimerTask(this, threadPool, progressTimer, getLabel("column.state.label"), SELECTION_TABS, getLabel("progressbar.resolving.label")), 0, 200);
        }

        //Display progressBar if hidden
        if (!progressBar.isVisible()) {
            progressBar.setVisible(true);
            cancelB.setVisible(true);
        }

        //TODO: Add caching based on pattern as key (skip re-resolving of the same pattern)
        Host h;
        for (String hostname : list) {
            h = new Host(type, hostname, isQmasterHost, isBDBHost, isShadowHost, isExecutionHost, isAdminHost, isSubmitHost, execdSpoolDir, Host.State.NEW_UNKNOWN_HOST);
            model.addHost(h);
            
            try {
                threadPool.execute(new ResolveHostTask(threadPool, progressTimer, h, this));
            } catch (RejectedExecutionException e) {
                Debug.error(e);
                model.setHostState(h, Host.State.CANCELED);
                ((HostSelectionTableModel) tables.get(2).getModel()).addHost(h);
            }
        }
    //Update table
    //model.fireTableChanged(new TableModelEvent(model));
    }

    private void fixVariables() {
        VariableSubstitutor vs = new VariableSubstitutor(idata.getVariables());
        String variable = "";

        // cfg.db.spooling.dir
        String spoolingDir = "";
        if ( idata.getVariable(VAR_SPOOLING_METHOD).equals("berkeleydb")) {
            spoolingDir = idata.getVariable(VAR_DB_SPOOLING_DIR_BDB);
        } else {
            spoolingDir = idata.getVariable(VAR_DB_SPOOLING_DIR_BDBSERVER);
        }
        idata.setVariable(VAR_DB_SPOOLING_DIR, vs.substituteMultiple(spoolingDir, null));

        // cfg.spooling.method.berkeleydbserver
        variable = idata.getVariable(VAR_SPOOLING_METHOD_BERKELEYDBSERVER);
        if (variable.equals("none")) {
            idata.setVariable(VAR_SPOOLING_METHOD_BERKELEYDBSERVER, "berkeleydb");
        }
        
        // add.spooling.method
        variable = idata.getVariable(VAR_SPOOLING_METHOD);
        if (variable.equals("none")) {
            idata.setVariable(VAR_SPOOLING_METHOD, "berkeleydb");
        }

        // cfg.db.spooling.server
        idata.setVariable(VAR_DB_SPOOLING_SERVER, "none");

        // add.qmaster.host
        if (isQmasterInst) {
            idata.setVariable(VAR_QMASTER_HOST, "none");
        }

        String tmpDomainName = "";
        boolean isVarWinSuppSet = false;
        boolean isVarHostResSet = false;
        HostList list = lists.get(1);
        Host h = null;
        for (int i = 0; i < list.size(); i++) {
            h = list.get(i);
            // cfg.windows.support
            if (!isVarWinSuppSet && h.getArchitecture().startsWith("win")) {
                idata.setVariable(VAR_WINDOWS_SUPPORT, "true");
                isVarWinSuppSet = true;
            }

            // cfg.hostname.resolving
            if (!isVarHostResSet) {
                if (!tmpDomainName.equals("") && !tmpDomainName.equals(Util.getDomainName(h.getHostname()))) {
                    idata.setVariable(VAR_HOSTNAME_RESOLVING, "false");
                    isVarHostResSet = true;
                }
                
                tmpDomainName = Util.getDomainName(h.getHostname());
            }

            // cfg.db.spooling.server
            if (h.isBdbHost()) {
                idata.setVariable(VAR_DB_SPOOLING_SERVER, h.getHostAsString());
            }

            // add.qmaster.host
            if (h.isQmasterHost()) {
                idata.setVariable(VAR_QMASTER_HOST, h.getHostAsString());
            }
        }
    }

    private void installButtonActionPerformed() {
        Debug.trace("INSTALL");
        //lists.get(1).printList();

        //Remove invalid components
        Host o;

        validList = lists.get(1);          //sort the current valid list
        HostList tmpList = new HostList(); //in a copy of the hostlist
        for (Host h : validList) {
            o = new Host(h);
            //Remove invalid components. They can't be installed!
            if (o.isShadowHost() && !isShadowdInst) {
                o.setShadowHost(false);
            }
            if (o.isExecutionHost() && !isExecdInst) {
                o.setExecutionHost(false);
            }
            if (o.isQmasterHost() && !isQmasterInst) {
               o.setQmasterHost(false);
            }
            if (o.isBdbHost() && !isBdbInst) {
                o.setBdbHost(false);
            }
            //Set new state
            o.setState(Host.State.READY_TO_INSTALL);
            if (o.getComponentString().length() > 0) {
               tmpList.add(o);
            }
        }
        //And Check we have a components to install
        boolean haveComponents = false;
        for (Host h : tmpList) {
            if (h.getComponentString().length() > 0) {
                haveComponents = true;
                break;
            }
        }
        if (haveComponents == false) {
            String message = new VariableSubstitutor(idata.getVariables()).substituteMultiple(parent.langpack.getString("warning.no.components.to.install.message"), null);
            emitWarning(parent.langpack.getString("installer.warning"), message);
            //We don't proceed with the installation
            return;
        }

        //set install mode
        installMode = true;

        // Change panel heading
        parent.changeActualHeading(getLabel("HostPanel.installing.headline"));

        // Check end fix variables
        fixVariables();

        //Disable the selecting host controls
        disableControls(true);
        if (parent != null) {
            parent.lockNextButton();
            parent.lockPrevButton();
        }
        //Stop updateTab thread
        if (progressTimer != null) {
            progressTimer.cancel();
            progressTimer = null;
        }
        //Cancel host selection thread pool
        threadPool.shutdownNow();
        try {
            threadPool.awaitTermination(200, TimeUnit.MILLISECONDS);
        } catch (InterruptedException e) {
           Debug.error(e);
        }
        //Create install list
        //installList = new InstallHostList();
        final HostList installList = new HostList();

        //Sort the hosts so that BDB is first and qmaster second
        //Find bdb and put it to the beggining
        Host h;
        if (isBdbInst) {
            for (int i = 0; i < tmpList.size(); i++) {
                h = tmpList.get(i);
                if (h.isBdbHost()) {
                    o = new Host(h);
                    boolean hasMore = false;
                    //Clear selection
                    if (o.isQmasterHost()) {
                       o.setQmasterHost(false);
                       hasMore = true;
                    }
                    if (o.isExecutionHost()) {
                       o.setExecutionHost(false);
                       hasMore = true;
                    }
                    if (o.isShadowHost()) {
                       o.setShadowHost(false);
                       hasMore = true;
                    }
                    installList.addUnchecked(new Host(o));
                    if (hasMore) {
                       o.setAdminHost(false);
                       o.setSubmitHost(false);
                    }
                    tmpList.remove(o);
                    break;
                }
            }
        }
        if (isQmasterInst) {
            //Find qmaster and put it to the beginning
            for (int i = 0; i < tmpList.size(); i++) {
                h = tmpList.get(i);
                if (h.isQmasterHost()) {
                    o = new Host(h);
                    boolean hasMore = false;
                    //Clear selection
                    if (o.isBdbHost()) {
                       o.setBdbHost(false);
                       hasMore = true;
                    }
                    if (o.isExecutionHost()) {
                       o.setExecutionHost(false);
                       hasMore = true;
                    }
                    if (o.isShadowHost()) {
                       o.setShadowHost(false);
                       hasMore = true;
                    }
                    installList.addUnchecked(new Host(o));
                    if (hasMore) {
                       o.setAdminHost(false);
                       o.setSubmitHost(false);
                    }
                    tmpList.remove(o);
                    break;
                }
            }
        }
        //Copy the rest
        for (Host host : tmpList) {
            installList.addUnchecked(new Host(host));
        }

        //installList.appendHostList(validList, isBdbInst, isQmasterInst, isShadowdInst, isExecdInst);

        // Save host list
        idata.setVariable(VAR_EXEC_HOST_LIST, HostList.getHostNames(installList.getHosts(Host.HOST_TYPE_EXECD), " "));
        idata.setVariable(VAR_SHADOW_HOST_LIST, HostList.getHostNames(installList.getHosts(Host.HOST_TYPE_SHADOWD), " "));
        idata.setVariable(VAR_ADMIN_HOST_LIST, HostList.getHostNames(installList.getHosts(Host.HOST_TYPE_ADMIN), " "));
        idata.setVariable(VAR_SUBMIT_HOST_LIST, HostList.getHostNames(installList.getHosts(Host.HOST_TYPE_SUBMIT), " "));

        // Initialize column tooltips
        String[] installHeaders = getInstallLabelVars();
        String[] headerToltips = new String[installHeaders.length];
        for (int i = 0; i < installHeaders.length; i++) {
            headerToltips[i] = getTooltip(installHeaders[i]);
        }

        //TODO: Do not overwrite old values tabbedpane, lists, hosts, etc..
        //Create a new tabbed pane for installation
        tabbedPane.invalidate();
        tabbedPane.removeAll();
        lists = new Vector<HostList>();
        tables = new Vector<HostTable>();
        HostList list;
        HostTable table;
        for (int i = 0; i < INSTALL_TABS.length; i++) {
            list = (i == 0) ? installList : new HostList();
            table = new HostTable(this, i);
            lists.add(list);
            tables.add(table);
            table.setModel(new HostInstallTableModel(list, getInstallHeaders(), getInstallClassTypes()));

            table.setTableHeader(new TooltipTableHeader(table.getColumnModel(), headerToltips));
            JTableHeader header = table.getTableHeader();
            SortedColumnHeaderRenderer headerRenderer = new SortedColumnHeaderRenderer(
                    header,
                    parent.icons.getImageIcon("columns.sorted.asc"),
                    parent.icons.getImageIcon("columns.sorted.desc"));
            for (int col = 0; col < table.getColumnCount(); col++) {
                table.getColumnModel().getColumn(col).setHeaderRenderer(headerRenderer);
            }

            table.getColumn(getLabel("column.progress.label")).setCellRenderer(new StateCellRenderer());
            table.getColumn(getLabel("column.log.label")).setCellRenderer(new LogButtonCellRenderer(getLabel("cell.log.label"), getLabel("cell.nolog.label")));
            table.getColumn(getLabel("column.log.label")).setCellEditor(new LogButtonCellEditor(getLabel("cell.log.label")));

            header.addMouseListener(new TableHeaderListener(header, headerRenderer));

            table.getModel().addTableModelListener(new TableModelListener() {
                public void tableChanged(TableModelEvent e) {
                    HostPanel.this.tableChanged();
                }
            });

            tabbedPane.addTab(INSTALL_TABS[i] + " (" + lists.get(i).size() + ")", new JScrollPane(tables.get(i)));
            tabbedPane.setToolTipTextAt(i, INSTALL_TABS_TOOLTIPS[i]);
        }
        tabbedPane.validate();
        tabbedPane.updateUI();
        //tabbedPane.repaint();

        setColumnsWidth();

        //Installation must be started in a new Thread
        new Thread() {
            public void run() {
                startInstallation(installList);
            }
        }.start();
    }

    private void startInstallation(HostList installList) {
        progressTimer = new Timer("InstallTimer");
        progressTimer.schedule(new UpdateInstallProgressTimerTask(this, threadPool, progressTimer, getLabel("column.progress.label"), INSTALL_TABS,  getLabel("progressbar.installing.label")), 0, 200);

		//Initialize new threadPool for the installation
		threadPool = (ThreadPoolExecutor) Executors.newFixedThreadPool(Util.INSTALL_THREAD_POOL_SIZE);
		threadPool.setThreadFactory(new InstallerThreadFactory());

        String SGE_ROOT = idata.getVariable(VAR_SGE_ROOT);

        boolean wait = false;
        long completed = 0, started = 0;
        try {
            for (Host h : installList) {
                Properties variablesCopy = new Properties();
                variablesCopy.putAll(idata.getVariables());
                
                h.setComponentVariables(variablesCopy);
                if (h.isBdbHost() || h.isQmasterHost()) {
                    wait = true;
                    completed = threadPool.getCompletedTaskCount();
                }
                threadPool.execute(new InstallTask(tables, h, variablesCopy, localizedMessages));
                started++;
                //In case the task is a BDB or qmaster host, we have to wait for sucessful finish!
                while (wait) {
                    if (threadPool.getCompletedTaskCount() >= completed + 1) {
                        wait = false;
                        //If bdb or qmaster fails it's over!
                        if (h.getState() != Host.State.SUCCESS) {
                            HostInstallTableModel updateModel;
                            HostInstallTableModel installModel = (HostInstallTableModel) tables.get(0).getModel();
                            for (Host host : installList) {
                                updateModel = (HostInstallTableModel) tables.get(2).getModel();
                                installModel.setHostState(host, Host.State.FAILED);
                                installModel.setHostLog(host, MessageFormat.format(localizedMessages.getProperty("msg.previous.dependent.install.failed"), h.getComponentString()));
                                installModel.removeHost(host);
                                updateModel.addHost(host);
                            }
                            return;
                        }
                        completed++;
                    } else {
                        try {
                            Thread.sleep(100);
                        } catch (InterruptedException ex) {
                            Logger.getLogger(HostPanel.class.getName()).log(Level.SEVERE, null, ex);
                        }
                    }
                }                
            }
        } catch(Exception e) {
            Debug.error(e);
        } finally {
            //Wait for all pending tasks to finish
            while (threadPool.getCompletedTaskCount() < started && !threadPool.isTerminated()) {
                try {
                    Thread.sleep(100);
                } catch (InterruptedException ex) {
                }
            }
            try {
                threadPool.awaitTermination(100, TimeUnit.MILLISECONDS);
                threadPool.purge();
            } catch (InterruptedException ex) {
            }
            //If we have failed hosts we go to the Failed tab
            if (((HostInstallTableModel)tables.get(2).getModel()).getRowCount() > 0) {
                tabbedPane.setSelectedIndex(2);
            } else { //Go to succeeded tab
                tabbedPane.setSelectedIndex(1);
            }
            threadPool.shutdown();
        }
    }

    private void disableControls(boolean b) {
        hostRB.setEnabled(!b);
        hostTF.setEnabled(!b);
        fileRB.setEnabled(!b);
        fileB.setEnabled(!b);
        shadowCB.setEnabled(!b);
        execCB.setEnabled(!b);
        adminCB.setEnabled(!b);
        submitCB.setEnabled(!b);
        addB.setEnabled(!b);
        qmasterCB.setEnabled(!b);
        bdbCB.setEnabled(!b);
    }
    // Variables declaration - do not modify//GEN-BEGIN:variables
    private javax.swing.JButton addB;
    private javax.swing.JCheckBox adminCB;
    private javax.swing.JScrollPane allHostsScrollPane;
    private javax.swing.JCheckBox bdbCB;
    private javax.swing.JButton cancelB;
    private javax.swing.JPanel componentSelectionPanel;
    private javax.swing.JCheckBox execCB;
    private javax.swing.JButton fileB;
    private javax.swing.JRadioButton fileRB;
    private javax.swing.ButtonGroup hostButtonGroup;
    private javax.swing.JRadioButton hostRB;
    private javax.swing.JTextField hostTF;
    private javax.swing.JProgressBar progressBar;
    private javax.swing.JCheckBox qmasterCB;
    private javax.swing.JCheckBox shadowCB;
    private javax.swing.JLabel statusBar;
    private javax.swing.JCheckBox submitCB;
    private javax.swing.JTabbedPane tabbedPane;
    // End of variables declaration//GEN-END:variables
    private JTextField lastSelectedTF;
    private Color defaultSelectionColor;
    private boolean adding = true;
    private boolean advancedMode = true;
    private HostList validList, invalidList;
    private Vector<HostTable> tables;
    private Vector<HostList> lists;
    private Host qmasterHost;
    private Host bdbHost;
    private boolean errorMessageVisible = false;
    private ThreadPoolExecutor threadPool;
    private Timer progressTimer = null;

    /**
     * @return the hostTabbedPane
     */
    public javax.swing.JTabbedPane getHostTabbedPane() {
        return tabbedPane;
    }

    /**
     * @return the progressBar
     */
    public javax.swing.JProgressBar getProgressBar() {
        return progressBar;
    }

    /**
     * @param progressBar the progressBar to set
     */
    public void setProgressBar(javax.swing.JProgressBar progressBar) {
        this.progressBar = progressBar;
    }

    public JButton getCancelButton() {
        return cancelB;
    }

    /**
     * @return the invalidHostsScrollPane
     */
    public javax.swing.JLabel getStatusBar() {
        return statusBar;
    }

    /**
     * @return the invalidHostsScrollPane
     */
//    public javax.swing.JTabbedPane getTabbedPane() {
//        return tabbedPane;
//    }

    public Iterator<HostList> getHostListIterator() {
        return lists.iterator();
    }

    public HostList getHostListAt(int i) {
        return lists.get(i);
    }

    public HostTable getHostTableAt(int i) {
        return tables.get(i);
    }

    public Iterator<HostTable> getHostTableIterator() {
        return tables.iterator();
    }

    /**
     * @return the bdbHost
     */
    public Host getBdbHost() {
        return bdbHost;
    }

    /**
     * @return the qmasterHost
     */
    public Host getQmasterHost() {
        return qmasterHost;
    }

    public void enableInstallButton(boolean b) {
        if (parent == null) {
            return;
        }
        JButton nextButton = parent.getNextButton();
        if (b && !nextButton.isEnabled()) {
            parent.unlockNextButton();
        } else if (!b && nextButton.isEnabled()) {
            parent.lockNextButton();
        }
    }

    public static void main(String[] args) {
        JFrame selectFrame = new JFrame("Select Hosts Panel");
        InstallData data = new InstallData() {
        };
        data.setVariable("cfg.sge.root", "<PATH_TO_SGE_ROOT>");
        data.setVariable("cond.install.shadowd", "false");
        data.setVariable("cond.install.execd", "false");
        data.setVariable("cond.install.qmaster", "true");
        data.setVariable("cond.install.express.mode", "true");
        final HostPanel hostPanel = new HostPanel(null, data);
        selectFrame.add(hostPanel);
        selectFrame.pack();
        selectFrame.setVisible(true);
        selectFrame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
    }

    public void runTest() {
        
    }

    /**
     * @return the installData
     */
    public InstallData getInstallData() {
        return idata;
    }
}

class ResolveHostTask extends TestableTask {

    private HostPanel panel;
    private Host host;
    private ThreadPoolExecutor tpe;
    private Timer timer;
    private HostSelectionTableModel model;

    public ResolveHostTask(ThreadPoolExecutor tpe, Timer timer, Host h, HostPanel panel) {

        this.tpe = tpe;
        this.timer = timer;
        host = h;
        this.panel = panel;
        model = (HostSelectionTableModel) panel.getHostTableAt(0).getModel();
    }

    public void run() {
        InetAddress inetAddr;
        String name, ip;
        model.setHostState(host, Host.State.RESOLVING);
        String value = host.getHostAsString();
        //TODO: Update table model
        long start = System.currentTimeMillis(), end;
        try {
            inetAddr = InetAddress.getByName(value);
            host.setInetAddr(inetAddr);
            name = inetAddr.getHostName(); //If IP can't be resolved takes took long on windows
            ip = inetAddr.getHostAddress();
            if (ip.equals(name)) {
                throw new UnknownHostException(ip);
            }
            model.setHostState(host, Host.State.RESOLVABLE);
            host.setHostname(name);
            host.setIp(ip);

            GetArchTask getArchTask = new GetArchTask(host, panel);
            getArchTask.setTestMode(isIsTestMode());
            getArchTask.setTestExitValue(getTestExitValue());
            getArchTask.setTestOutput(getTestOutput());

            tpe.execute(getArchTask);
        } catch (UnknownHostException e) {
            Debug.error(e);
            end = System.currentTimeMillis();
            host.setState(Host.State.UNKNOWN_HOST);
            //Add host to unreachable hosts tab
            model = (HostSelectionTableModel) panel.getHostTableAt(2).getModel();
            model.addHost(host);
        } catch (RejectedExecutionException e) {
            Debug.error(e);
            end = System.currentTimeMillis();
            host.setState(Host.State.CANCELED);
            //Add host to unreachable hosts tab
            model = (HostSelectionTableModel) panel.getHostTableAt(2).getModel();
            model.addHost(host);
        } finally {
            end = System.currentTimeMillis();
            Debug.trace("Resolving " + value + " took " + (end - start) + "ms");
        }
    }
}

class GetArchTask extends TestableTask {

    private Host host;
    private HostPanel panel;
    private HostSelectionTableModel model;
    private final String SGE_ROOT;

    public GetArchTask(Host h, HostPanel panel) {
        setTaskName("GetArchTask - " + h);
        host = h;
        this.panel = panel;
        model = (HostSelectionTableModel) panel.getHostTableAt(0).getModel();
        SGE_ROOT = panel.getInstallData().getVariable(VAR_SGE_ROOT);
    }

    public void run() {
        //Done only for RESOLVABLE hosts
        if (host.getState() != Host.State.RESOLVABLE) {
            return;
        }

        model.setHostState(host, Host.State.CONTACTING);
        //TODO:Need to remove the CONNECTING state in case of failure.

        int exitValue = getTestExitValue();
        Vector<String> output = getTestOutput();
        if (!isIsTestMode()) {
            CommandExecutor archCmd = new CommandExecutor(panel.getInstallData().getVariables(), panel.getInstallData().getVariable(VAR_SHELL_NAME), host.getHostname(), SGE_ROOT + "/util/arch");
            archCmd.execute();
            exitValue = archCmd.getExitValue();
            output = archCmd.getOutput();
        }
//        JTabbedPane tab = panel.getTabbedPane();
        int pos;
        Host.State state;
        int tabPos = 0;
        if (exitValue == 0 && output.size() > 0) {
            host.setArchitecture(output.get(0));
            state = Host.State.REACHABLE;
            tabPos = 1;
        } else if (exitValue == CommandExecutor.EXITVAL_INTERRUPTED) {
            state = Host.State.CANCELED;
            tabPos = 2;
        } else {
            state = Host.State.RESOLVABLE;
            tabPos = 2;
        }
        model.setHostState(host, state);
        //Update other tabs
        HostTable table = panel.getHostTableAt(tabPos);
        model = (HostSelectionTableModel) table.getModel();
        model.addHost(host);

        //tab.setTitleAt(tabPos, HostPanel.SELECTION_TABS[tabPos] + " (" + model.getRowCount() + ")");
        //TODO: Can update just the exact row?
        table.tableChanged(new TableModelEvent(model));
    //TODO: Show that r/ssh failed and why
    }
}

class UpdateInstallProgressTimerTask extends TimerTask {

    private HostPanel panel;
    private ThreadPoolExecutor tpe;
    private JProgressBar mainBar;
    private JButton cancelButton;
    private String colName;
    private String[] TABS;
    private Timer timer;
    private String prefix;

    public UpdateInstallProgressTimerTask(final HostPanel panel, ThreadPoolExecutor tpe, Timer timer, final String colName, final String[] TABS, final String prefix) {
        super();
        this.panel = panel;
        this.tpe = tpe;
        this.timer = timer;
        this.colName = colName;
        this.TABS = TABS;
        this.prefix = prefix;
        mainBar = panel.getProgressBar();
        mainBar.setMinimum(0);
        int cur = (int) tpe.getCompletedTaskCount();
        int max = (int) tpe.getTaskCount();
        mainBar.setValue(cur);
        mainBar.setMaximum(max);
        mainBar.setString(prefix);
        //mainBar.setString(prefix + " " + cur + " / " + max);
        mainBar.setStringPainted(true);
        mainBar.setVisible(true);
        cancelButton = panel.getCancelButton();
        cancelButton.setVisible(true);
    }

    @Override
    public void run() {
        HostTable table;
        HostList list;
        DefaultTableModel model;
        //Update individual install progress bars
//        JTabbedPane tab = panel.getTabbedPane();
        Host.State tmpState;
        Object obj;
        int i = 0;

        for (Iterator<HostTable> iter = panel.getHostTableIterator(); iter.hasNext();) {
            table = iter.next();
            list = panel.getHostListAt(i);
            int col = table.getColumn(colName).getModelIndex();
            model = (DefaultTableModel) table.getModel();
            //Update tabs
            //tab.setTitleAt(i, TABS[i] + " (" + list.size() + ")");
            for (int row = 0; row < list.size(); row++) {
                // if a host gets deleted during the progress: IndexOutOfBoundsException
                try {
                    obj = model.getValueAt(row, col);

                    if (obj instanceof Host.State) {
                        tmpState = (Host.State) obj;

                        switch (tmpState) {
                            case RESOLVING:
                            case INSTALLING:
                            case CONTACTING:
                                model.fireTableCellUpdated(row, col);
                                break;
                        }
                    }
                } catch (IndexOutOfBoundsException e) {
                }
            }
            i++;
        }
        //Update the main ProgressBar
        int cur = (int) tpe.getCompletedTaskCount();
        int max = (int) tpe.getTaskCount();
        mainBar.setValue(cur);
        mainBar.setMaximum(max);

        //Last task should cancel the Timer and hide the progressBar
        if (cur == max && cur > 0) {
            mainBar.setVisible(false);
            cancelButton.setVisible(false);

            if (panel.getHostListAt(1).size() > 0) {
                panel.enableInstallButton(true);
                if (HostPanel.installMode) {
                    panel.triggerInstallButton(false);
                }
            }

            // Jump to the Done tab if it's not empty. Otherwise jump to Failed tab.
//            if (panel.getHostListAt(1).size() > 1) {
//                tab.setSelectedIndex(1);
//            } else {
//                tab.setSelectedIndex(2);
//            }

            //timer.cancel();
        }
    }
}

class InstallTask extends TestableTask {
    private Vector<HostTable> tables;
    private Host host;
    private Properties variables, msgs;
    private VariableSubstitutor vs;

    public InstallTask(Vector<HostTable> tables, Host h, Properties variables, final Properties msgs) {
        setTaskName("InstallTask - " + h);
        this.tables = tables;
        host = h;
        this.variables = variables;
        this.msgs = msgs;
        vs = new VariableSubstitutor(variables);
    }

    public void run() {
        Debug.trace("["+System.currentTimeMillis()+"] "+host.getHostAsString()+" Starting install task "+host.getComponentString());

        Host.State state;
        HostInstallTableModel updateModel;
        HostInstallTableModel installModel = (HostInstallTableModel) tables.get(0).getModel();
        installModel.setHostState(host, Host.State.INSTALLING);
        int tabPos = 0;

        String log = "";
        boolean prereqFailed = false;

        try {
            String autoConfFile = null;
            int exitValue = getTestExitValue();
            Vector<String> output = getTestOutput();

            if (host.isBdbHost() || host.isQmasterHost() || host.isShadowHost() || host.isExecutionHost()) {
                String autoConfTempFile = vs.substituteMultiple(variables.getProperty(VAR_AUTO_INSTALL_COMPONENT_TEMP_FILE), null);
                autoConfFile = vs.substituteMultiple(variables.getProperty(VAR_AUTO_INSTALL_COMPONENT_FILE), null);

                //Appended CELL_NAME prevents a race in case of parallel multiple cell installations
                autoConfFile = "/tmp/" + autoConfFile + "." + host.getHostAsString() + "." + variables.getProperty(VAR_SGE_CELL_NAME);
                Debug.trace("Generating auto_conf file: '" + autoConfFile + "'.");
                autoConfFile = Util.fillUpTemplate(autoConfTempFile, autoConfFile, variables);
                new File(autoConfFile).deleteOnExit();
            }
            
            if (!isIsTestMode() && ! prereqFailed) {
                //Need to copy the script to the final location first
                //TODO: Do this only when not on a shared FS (file is not yet accessible)
                Debug.trace("Copy auto_conf file to '" + host.getHostname() + ":" + autoConfFile + "'.");
                CommandExecutor cmd = new CommandExecutor(variables, variables.getProperty(VAR_COPY_COMMAND), autoConfFile, host.getHostname() + ":" + autoConfFile, " && if [ ! -s "+autoConfFile+" ]; then echo 'File "+autoConfFile+" is empty.'; exit 1; fi");
                cmd.execute();
                exitValue = cmd.getExitValue();
                if (exitValue == CommandExecutor.EXITVAL_TERMINATED) {
                    //Set the log content
                    log = "Timeout while copying the " + autoConfFile + " script to host " + host.getHostname() + " via " + variables.getProperty(VAR_COPY_COMMAND) + " command!\nMaybe a password is expected. Try the command in the terminal first.";
                    installModel.setHostLog(host, log);
                } else if (exitValue != 0) {
                    //TODO: output + error
                    log = "Error when copying the file.";
                    installModel.setHostLog(host, log);
                } else {
                    new CommandExecutor(variables, variables.getProperty(VAR_SHELL_NAME),host.getHostAsString(), "chmod", "755", autoConfFile).execute();
                    cmd = new CommandExecutor(variables, 60000, //Set install timeout to 60secs
                            variables.getProperty(VAR_SHELL_NAME),
                            host.getHostAsString(), autoConfFile, String.valueOf(host.isBdbHost()),
                            String.valueOf(host.isQmasterHost()), String.valueOf(host.isShadowHost()),
                            String.valueOf(host.isExecutionHost()), String.valueOf(host.isAdminHost()),
                            String.valueOf(host.isSubmitHost()));
                            //TODO: Beta and later should delete the file as part of the command add - , ";", "rm", "-f", localScript);
                    //Debug.trace("Start installation: "+cmd.getCommands());
                    cmd.execute();
                    //Set the log content
                    int tmpExitValue = 0;
                    String tmpNum = "";
                    String SEP = System.getProperty("line.separator");
                    String message;
                    log += "OUTPUT:" + SEP;
                    for (String d : cmd.getOutput()) {
                        if (d.matches("___EXIT_CODE_[1-9]?[0-9]___")) {
                            tmpNum = d.substring("___EXIT_CODE_".length());
                            tmpNum = tmpNum.substring(0, tmpNum.indexOf("_"));
                            tmpExitValue = Integer.valueOf(tmpNum);
                            message = (String) msgs.get("msg.exit." + tmpNum);
                            if (message == null || message.length() == 0) {
                                message = MessageFormat.format((String) msgs.get("msg.exit.fail.general"), tmpNum);
                            }
                            log += "FAILED:" + message + SEP;
                        } else {
                            log += d + SEP;
                        }
                    }
                    log += SEP + "ERROR:" + SEP;
                    for (String d : cmd.getError()) {
                        log += d + SEP;
                    }
                    exitValue = cmd.getExitValue();
                    //Cleanup if timeout or interrupt
                    //TODO: Temporary do not delete for testing
                    /*if (exitValue < -1) {
                        cmd = new CommandExecutor("rm", "-f", localScript);
                        cmd.execute();
                    }*/
                    //We don't trust the exit codes (rsh does not honor them)
                    exitValue = (tmpExitValue != 0 && exitValue == 0) ? tmpExitValue : exitValue;

                    // Try to reach the host
                    /*Debug.trace("Ping host(s): " + host.getHostAsString());
                    if (exitValue == 0 && host.isQmasterHost()) {
                        if (!Util.pingHost(variables, host, "qmaster", 5)) {
                            log = "The installation finished but can not reach the 'qmaster' on host '" + host.getHostAsString() + "' at port '" + variables.getProperty(VAR_SGE_QMASTER_PORT) + "'.";
                            exitValue = CommandExecutor.EXITVAL_OTHER;
                        }
                    }

                    if (exitValue == 0 && host.isExecutionHost()) {
                        if (!Util.pingHost(variables, host, "execd", 5)) {
                            log = "The installation finished but can not reach the 'execd' on host '" + host.getHostAsString() + "' at port '" + variables.getProperty(VAR_SGE_EXECD_PORT) + "'.";
                            exitValue = CommandExecutor.EXITVAL_OTHER;
                        }
                    }*/
                    installModel.setHostLog(host, log);
                }
            }
            if (exitValue == 0) {
                state = Host.State.SUCCESS;
                tabPos = 1;
            } else if (exitValue == CommandExecutor.EXITVAL_INTERRUPTED) {
                state = Host.State.CANCELED;
                tabPos = 2;
            } else {
                state = Host.State.FAILED;
                tabPos = 2;
            }
        } catch (InterruptedException e) {
            Debug.error(e);
            state = Host.State.CANCELED;
            tabPos = 2;
        } catch (Exception e) {
            Debug.error(e);
            state = Host.State.FAILED;
            tabPos = 2;
            }

        updateModel = (HostInstallTableModel) tables.get(tabPos).getModel();

        installModel.setHostState(host, state);
        installModel.removeHost(host);
        updateModel.addHost(host);

        //panel.getTabbedPane().setTitleAt(tabPos, HostPanel.INSTALL_TABS[tabPos] + " (" + updateModel.getRowCount() + ")");
        Debug.trace("["+System.currentTimeMillis()+"] "+host.getHostAsString()+" Finished installtask "+host.getComponentString());
	}
}


abstract class TestableTask implements Runnable, Config {
    private boolean testMode = false;
    private int testExitValue = 0;
    private Vector<String> testOutput = new Vector<String>();
    private String taskName = "";

    public String getTaskName() {
        return taskName;
    }

    public void setTaskName(String taskName) {
        this.taskName = taskName;
    }

    public boolean isIsTestMode() {
        return testMode;
    }

    public void setTestMode(boolean isTestMode) {
        this.testMode = isTestMode;
    }

    public int getTestExitValue() {
        return testExitValue;
    }

    public void setTestExitValue(int testExitValue) {
        this.testExitValue = testExitValue;
    }

    public Vector<String> getTestOutput() {
        return testOutput;
    }

    public void setTestOutput(Vector<String> testOutput) {
        this.testOutput = testOutput;
    }
}

class InstallerThreadFactory implements ThreadFactory {

    static final AtomicInteger poolNumber = new AtomicInteger(1);
    final ThreadGroup group;
    final AtomicInteger threadNumber = new AtomicInteger(1);
    final String namePrefix;

    InstallerThreadFactory() {
        SecurityManager s = System.getSecurityManager();
        group = (s != null) ? s.getThreadGroup() : Thread.currentThread().getThreadGroup();
        namePrefix = "install_task_pool-" +
                poolNumber.getAndIncrement() +
                "-thread-";
    }

    public Thread newThread(Runnable r) {
        Thread t = new Thread(group, r,
                namePrefix + threadNumber.getAndIncrement(),
                0);
        if (t.isDaemon()) {
            t.setDaemon(false);
        }
        if (t.getPriority() != Thread.MIN_PRIORITY) {
            t.setPriority(Thread.MIN_PRIORITY);
        }
        return t;
    }
}
