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
import com.izforge.izpack.rules.RulesEngine;
import com.izforge.izpack.util.Debug;
import com.izforge.izpack.util.VariableSubstitutor;
import com.sun.grid.installer.util.CommandExecutor;
import com.sun.grid.installer.util.Config;
import com.sun.grid.installer.util.Util;
import java.awt.Color;
import java.awt.Font;
import java.awt.FontMetrics;
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
import java.text.MessageFormat;

import java.util.Enumeration;
import java.util.Hashtable;
import javax.swing.AbstractButton;
import javax.swing.JButton;
import javax.swing.JFileChooser;
import javax.swing.JOptionPane;
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
    public static boolean installMode, checkMode = false;
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

        if (idata.langpack != null) {
            Set<String> keys = idata.langpack.keySet();
            for (Iterator<String> it = keys.iterator(); it.hasNext();) {
                String key = it.next();
                if (key.startsWith("msg.")) {
                    localizedMessages.put(key, idata.langpack.get(key));
                }
            }
        }

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
                if (e.getKeyChar() != '\n' && errorMessageVisible) {
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

            Properties tableLangProps = new Properties();
            tableLangProps.put("msg.qmasterhost.already.selected", getLabel("msg.qmasterhost.already.selected"));
            tableLangProps.put("msg.bdbhost.already.selected", getLabel("msg.bdbhost.already.selected"));
            tableLangProps.put("title.confirmation", getLabel("title.confirmation"));
            table.setModel(new HostSelectionTableModel(table, list, getSelectionHeaders(), getSelectionClassTypes(), tableLangProps));
            //table.setAutoResizeMode(JTable.AUTO_RESIZE_OFF);

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

        VariableSubstitutor vs = new VariableSubstitutor(idata.getVariables());

        // In some cases the execd spool dir does not get substituted
        idata.setVariable(VAR_EXECD_SPOOL_DIR, vs.substituteMultiple(idata.getVariable(VAR_EXECD_SPOOL_DIR), null));

        // cfg.db.spooling.dir
        String spoolingDir = "";
        if (idata.getVariable(VAR_SPOOLING_METHOD).equals("berkeleydb")) {
            spoolingDir = idata.getVariable(VAR_DB_SPOOLING_DIR_BDB);
        } else {
            spoolingDir = idata.getVariable(VAR_DB_SPOOLING_DIR_BDBSERVER);
        }
        idata.setVariable(VAR_DB_SPOOLING_DIR, vs.substituteMultiple(spoolingDir, null));

        // Substitute the variables in Host.State tooltips
        String key = "";
        String value = "";
        Host.State.localizedTexts.clear();
        if (idata.langpack != null) {
            Set<String> keys = idata.langpack.keySet();
            for (Iterator<String> it = keys.iterator(); it.hasNext();) {
                key = it.next();
                value = vs.substituteMultiple((String) idata.langpack.get(key), null);
                if (key.startsWith(LANGID_PREFIX_STATE)) {
                    Host.State.localizedTexts.put(key, value);
                }
            }

            // override the tooltip for perm_execd_spool_dir state dependig in the mode
            if (isExpressInst) {
                value = vs.substituteMultiple((String) idata.langpack.get("state.perm_execd_spool_dir.tooltip.global"), null);
            } else {
                value = vs.substituteMultiple((String) idata.langpack.get("state.perm_execd_spool_dir.tooltip.local"), null);
            }
            Host.State.localizedTexts.put("state.perm_execd_spool_dir.tooltip", value);
        }

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
                    resolveHosts(Host.Type.HOSTNAME, host, true, false, false, false, true, true, idata.getVariable(VAR_EXECD_SPOOL_DIR));

                    if (HostSelectionTableModel.getQmasterHost() == null) {
                        try {
                            arch = lists.get(0).get(0).getArchitecture();
                        } finally {
                            prop.setProperty(PARAMETER_1, arch);
                        }

                        if (!arch.equals("")) {
                            vs = new VariableSubstitutor(prop);
                            message = vs.substituteMultiple(idata.langpack.getString("warning.qmaster.arch.unsupported.message"), null);
                            emitWarning(idata.langpack.getString("installer.warning"), message);
                        }
                    }
                }
                host.clear();

                if (isBdbInst && !isBdbExist) {
                    if (idata.getVariable(VAR_DB_SPOOLING_SERVER).equals("")) {
                        host.add(Host.localHostName); // TODO localhost or qmaster host ?
                    } else {
                        host.add(idata.getVariable(VAR_DB_SPOOLING_SERVER));
                    }
                    resolveHosts(Host.Type.HOSTNAME, host, false, true, false, false, false, false, idata.getVariable(VAR_EXECD_SPOOL_DIR));

                    if (HostSelectionTableModel.getBdbHost() == null) {
                        try {
                            arch = lists.get(0).get(0).getArchitecture();
                        } finally {
                            prop.setProperty(PARAMETER_1, arch);
                        }

                        if (!arch.equals("")) {
                            vs = new VariableSubstitutor(prop);
                            message = vs.substituteMultiple(idata.langpack.getString("warning.bdbserver.arch.unsupported.message"), null);
                            emitWarning(idata.langpack.getString("installer.warning"), message);
                        }
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
    }

    @Override
    public void panelDeactivate() {
        enableInstallButton(true);
        triggerInstallButton(false);

        //Set the qmaster and bdb host values
        String qmasterHostName = "";
        if (HostSelectionTableModel.getQmasterHost() != null) {
            qmasterHostName = HostSelectionTableModel.getQmasterHost().getHostAsString();
        }
        idata.setVariable(VAR_QMASTER_HOST, qmasterHostName);
        String bdbHostName = "";
        if (HostSelectionTableModel.getBdbHost() != null) {
            bdbHostName = HostSelectionTableModel.getBdbHost().getHostAsString();
        }
        idata.setVariable(VAR_DB_SPOOLING_SERVER, bdbHostName);
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
        if (idata.langpack != null) {
            tooltip = idata.langpack.getString(key);
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

        if (idata.langpack != null) {
            label = idata.langpack.getString(key);
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

    private void addHostsFromTF() {
        try {
            final String pattern = hostTF.getText().trim();
            List<String> list = Util.parseHostPattern(pattern);
            resolveHosts(Host.Type.HOSTNAME, list, qmasterCB.isSelected(), bdbCB.isSelected(), shadowCB.isSelected(), execCB.isSelected(), adminCB.isSelected(), submitCB.isSelected(), idata.getVariable(VAR_EXECD_SPOOL_DIR));
        } catch (IllegalArgumentException ex) {
            statusBar.setVisible(true);
            statusBar.setText("Error: " + ex.getMessage());
            hostTF.setSelectionStart(0);
            hostTF.setSelectionEnd(hostTF.getText().length());
            errorMessageVisible = true;
        }
    }

	private void hostTFActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_hostTFActionPerformed

            addHostsFromTF();
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
                    List<String> tmp;
                    Host.Type type;
                    for (String host : list) {
                        type = Util.isIpPattern(host) ? Host.Type.IP : Host.Type.HOSTNAME;
                        tmp = new ArrayList<String>();
                        tmp.add(host);
                        resolveHosts(type, list, qmasterCB.isSelected(), bdbCB.isSelected(), shadowCB.isSelected(), execCB.isSelected(), adminCB.isSelected(), submitCB.isSelected(), idata.getVariable(VAR_EXECD_SPOOL_DIR));
                    }
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
            addHostsFromTF();
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
            if (waitingTasks.size() > 0) {
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
            try {
                tabbedPane.setTitleAt(i, tabTitles[i] + " (" + tables.get(i).getRowCount() + ")");
            } catch (Exception e) {
                // TODO find reason
            }
        }
    }

    public void resolveHosts(Host.Type type, List<String> list, boolean isQmasterHost, boolean isBDBHost, boolean isShadowHost, boolean isExecutionHost, boolean isAdminHost, boolean isSubmitHost, String execdSpoolDir) {
        //TODO: Add caching based on pattern as key (skip re-resolving of the same pattern)
        Host h;
        List<Host> hosts = new ArrayList<Host>(list.size());
        for (String hostname : list) {
            h = new Host(type, hostname, isQmasterHost, isBDBHost, isShadowHost, isExecutionHost, isAdminHost, isSubmitHost, execdSpoolDir, Host.State.NEW_UNKNOWN_HOST);
            hosts.add(h);
        }

        resolveHosts(hosts);
    }

    public void resolveHosts(List<Host> hosts) {
        HostTable table = tables.get(0);
        HostSelectionTableModel model = (HostSelectionTableModel) table.getModel();

        if (threadPool.isShutdown()) {
            threadPool = (ThreadPoolExecutor) Executors.newFixedThreadPool(Util.RESOLVE_THREAD_POOL_SIZE);
            threadPool.setThreadFactory(new InstallerThreadFactory());
        }

        progressTimer = new Timer("ResolveTimer");
        progressTimer.schedule(new UpdateInstallProgressTimerTask(this, threadPool, getLabel("column.state.label"), getLabel("progressbar.resolving.label")), 10);

        for (Host host : hosts) {
            host = model.addHost(host);

            try {
                threadPool.execute(new ResolveHostTask(threadPool, host, this));
            } catch (RejectedExecutionException e) {
                Debug.error(e);
                model.setHostState(host, Host.State.CANCELED);
                ((HostSelectionTableModel) tables.get(2).getModel()).addHost(host);
            }
        }
    }

    private void fixVariables() {
        VariableSubstitutor vs = new VariableSubstitutor(idata.getVariables());
        String variable = "";

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

        boolean isVarWinSuppSet = false;
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
//            if (!isVarHostResSet) {
//                if (!tmpDomainName.equals("") && !tmpDomainName.equals(Util.getDomainName(h.getHostname()))) {
//                    idata.setVariable(VAR_HOSTNAME_RESOLVING, "false");
//                    isVarHostResSet = true;
//                }
//
//                tmpDomainName = Util.getDomainName(h.getHostname());
//            }

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

    /**
     * Clears the selections in all of the tables in order to apply modifications
     */
    private void clearTableSelections() {
        for (JTable table : tables) {
            table.getSelectionModel().clearSelection();
        }
    }

    private void setTablesEnabled(boolean enabled) {
        for (JTable table : tables) {
            table.setEnabled(enabled);
        }
    }

    private void installButtonActionPerformed() {

        clearTableSelections();

        // Check end fix variables
        fixVariables();

        //Installation must be started in a new Thread
        new Thread() {

            @Override
            public void run() {
                checkHostsAndInstall(lists.get(1));
            }
        }.start();
    }

    private void checkHostsAndInstall(HostList hosts) {
        checkMode = true;
        //Disable the selecting host controls
        disableControls(true);
        if (parent != null) {
            parent.lockNextButton();
            parent.lockPrevButton();
        }

        setTablesEnabled(false);

        tabbedPane.setSelectedIndex(1);

        //Remove invalid components
        Host o;
        HostList tmpList = new HostList(); //in a copy of the hostlist
        for (Host h : hosts) {
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
                tmpList.addUnchecked(o);
            }
        }
        //And Check we have a components to install
        int numOfQmasterHost = 0;
        int numOfExecdHost = 0;
        int numOfShadowHost = 0;
        int numOfBdbHost = 0;
        for (Host h : tmpList) {
            if (h.isQmasterHost()) {
                numOfQmasterHost++;
            }
            if (h.isExecutionHost()) {
                numOfExecdHost++;
            }
            if (h.isShadowHost()) {
                numOfShadowHost++;
            }
            if (h.isBdbHost()) {
                numOfBdbHost++;
            }
        }
        if (numOfQmasterHost == 0 && numOfExecdHost == 0 && numOfShadowHost == 0 && numOfBdbHost == 0) {
            String message = new VariableSubstitutor(idata.getVariables()).substituteMultiple(idata.langpack.getString("warning.no.components.to.install.message"), null);
            JOptionPane.showOptionDialog(this, message, getLabel("installer.error"),
                    JOptionPane.CANCEL_OPTION, JOptionPane.ERROR_MESSAGE, null,
                    new Object[]{getLabel("installer.cancel")}, getLabel("installer.cancel"));
            //We don't proceed with the installation

            checkMode = false;
            disableControls(false);
            if (parent != null) {
                parent.unlockNextButton();
                parent.unlockPrevButton();
            }
            setTablesEnabled(true);

            return;
        }

        // Compare host selection with the component selection if does not match warn user
        String componentString = "";
        String hostTypeString = "";

        if (isQmasterInst && numOfQmasterHost == 0) {
            componentString = getLabel("install.qmaster.label");
            hostTypeString = getLabel("column.qmaster.label");
        } else if (isExecdInst && numOfExecdHost == 0) {
            componentString = getLabel("install.execd.label");
            hostTypeString = getLabel("column.execd.label");
        } else if (isShadowdInst && numOfShadowHost == 0) {
            componentString = getLabel("install.shadowd.label");
            hostTypeString = getLabel("column.shadowd.label");
        } else if (isBdbInst && numOfBdbHost == 0) {
            componentString = getLabel("install.bdb.label");
            hostTypeString = getLabel("column.bdb.label");
        }

        if (!componentString.equals("")) {
            Properties props = new Properties();
            props.put("component", componentString);
            props.put("host.type", hostTypeString);

            VariableSubstitutor vs = new VariableSubstitutor(props);

            if (JOptionPane.NO_OPTION == JOptionPane.showOptionDialog(this, vs.substituteMultiple(getLabel("warning.comp.selected.but.no.host"), null),
                    getLabel("installer.warning"), JOptionPane.YES_NO_OPTION, JOptionPane.WARNING_MESSAGE, null,
                    new Object[]{getLabel("installer.yes"), getLabel("installer.no")}, getLabel("installer.no"))) {
                
                checkMode = false;
                disableControls(false);
                if (parent != null) {
                    parent.unlockNextButton();
                    parent.unlockPrevButton();
                }
                setTablesEnabled(true);

                return;
            }
            vs = null; // Don't use it any more
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

        //Initialize new threadPool for the installation
        threadPool = (ThreadPoolExecutor) Executors.newFixedThreadPool(Util.INSTALL_THREAD_POOL_SIZE);
        threadPool.setThreadFactory(new InstallerThreadFactory());

        progressTimer = new Timer("CheckHostsTimer");
        progressTimer.schedule(new UpdateInstallProgressTimerTask(this, threadPool, getLabel("column.state.label"), getLabel("progressbar.checking.label")), 10);

        try {
            for (Host h : hosts) {
                threadPool.execute(new CheckHostTask(h, this));
            }

            while (threadPool.getCompletedTaskCount() < tmpList.size() && !threadPool.isTerminated()) {
                try {
                    Thread.sleep(500);
                } catch (InterruptedException ex) {
                }
            }

            long invalid = 0;
            for (Host h : hosts) {
                switch (h.getState()) {
                    case USED_QMASTER_PORT:
                    case USED_EXECD_PORT:
                    case USED_JMX_PORT:
                    case BDB_SPOOL_DIR_EXISTS:
                    case BDB_SPOOL_DIR_WRONG_FSTYPE:
                    case PERM_QMASTER_SPOOL_DIR:
                    case PERM_EXECD_SPOOL_DIR:
                    case PERM_BDB_SPOOL_DIR:
                    case PERM_JMX_KEYSTORE:
                    case ADMIN_USER_NOT_KNOWN:
                    case COPY_TIMEOUT_CHECK_HOST:
                    case COPY_FAILED_CHECK_HOST:
                    case UNKNOWN_ERROR: {
                        invalid++;
                    }
                }
            }

            String msg = MessageFormat.format(getLabel("warning.invalid.hosts.message"), invalid, hosts.size());
            if (invalid > 0 && JOptionPane.NO_OPTION == JOptionPane.showOptionDialog(this, msg,
                    getLabel("installer.warning"), JOptionPane.YES_NO_OPTION, JOptionPane.WARNING_MESSAGE, null,
                    new Object[]{getLabel("installer.yes"), getLabel("installer.no")}, getLabel("installer.no"))) {

                disableControls(false);
                if (parent != null) {
                    parent.unlockNextButton();
                    parent.unlockPrevButton();
                }
                setTablesEnabled(true);

                return;
            }

        } catch (Exception e) {
            Debug.error(e);
        } finally {
            try {
                threadPool.awaitTermination(100, TimeUnit.MILLISECONDS);
                threadPool.purge();
            } catch (InterruptedException ex) {
            }

            checkMode = false;
        }

        switchToInstallModeAndInstall(tmpList);
    }

    private void switchToInstallModeAndInstall(final HostList hosts) {
        Debug.trace("INSTALL");

        //set install mode
        installMode = true;

        // Change panel heading
        parent.changeActualHeading(getLabel("HostPanel.installing.headline"));

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

        //Remove hosts with no components
        HostList tmpList = new HostList();
        for (Host h : hosts) {
            if (h.getComponentString().length() > 0) {
                tmpList.add(h);
            }
        }

        //Create install list
        final HostList installList = new HostList();

        //Sort the hosts so that BDB is first and qmaster second
        //Find bdb and put it to the beggining
        Host h, o;
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
        //TODO: Need tmpList?
        if (isQmasterInst) {
            //Find qmaster and put it as next
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
        //TODO: Sort the rest of the list alphabetically

        //We cannot install shadowds in parallel!
        if (isShadowdInst) {
            //Find shadowds and put them as next
            for (int i = 0; i < tmpList.size(); i++) {
                h = tmpList.get(i);
                if (h.isShadowHost()) {
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
                    if (o.isQmasterHost()) {
                        o.setQmasterHost(false);
                        hasMore = true;
                    }
                    installList.addUnchecked(new Host(o));
                    if (hasMore) {
                        o.setAdminHost(false);
                        o.setSubmitHost(false);
                    }
                    tmpList.remove(o);
                }
            }
        }
        //Copy the rest
        for (Host host : tmpList) {
            installList.addUnchecked(new Host(host));
        }

        /**
         * Generate global auto_conf files depending on the different local execd spool dirs
         */
        // Seperate execution hosts depending on their execd spool dir
        Hashtable<String, ArrayList<Host>> hostSelection = new Hashtable<String, ArrayList<Host>>();
        String localExecdSpoolDir = "";
        for (Host host : installList) {
            if (host.isExecutionHost()) {
                localExecdSpoolDir = host.getSpoolDir();

                if (!hostSelection.containsKey(localExecdSpoolDir)) {
                    ArrayList<Host> hostList = new ArrayList<Host>();
                    hostList.add(host);
                    hostSelection.put(localExecdSpoolDir, hostList);
                } else {
                    hostSelection.get(localExecdSpoolDir).add(host);
                }
            }
        }

        // Construct the file names
        VariableSubstitutor vs = new VariableSubstitutor(idata.getVariables());

        String autoConfTempFile = vs.substituteMultiple(idata.getVariable(VAR_AUTO_CONF_TEMP_FILE), null);
        String autoConfFile = vs.substituteMultiple(idata.getVariable(VAR_AUTO_CONF_FILE), null);

        autoConfFile = autoConfFile + "_" + Util.generateTimeStamp();

        // Generate the auto_conf file for every different local execd spool dirs
        int index = 0;
        String outputFilePostfix = "";
        boolean firstRun = true; // Let a run even if there was no execution host selected
        for (Enumeration<String> enumer = hostSelection.keys(); firstRun || enumer.hasMoreElements();) {
            firstRun = false;

            if (enumer.hasMoreElements()) {
                localExecdSpoolDir = enumer.nextElement();
                idata.setVariable(VAR_EXEC_HOST_LIST, HostList.getHostNames(hostSelection.get(localExecdSpoolDir), " "));
                idata.setVariable(VAR_EXEC_HOST_LIST_RM, HostList.getHostNames(hostSelection.get(localExecdSpoolDir), " "));
            }

            // put the 
            if (!enumer.hasMoreElements()) {
                idata.setVariable(VAR_SHADOW_HOST_LIST, HostList.getHostNames(installList.getHosts(Host.HOST_TYPE_SHADOWD), " "));
                idata.setVariable(VAR_ADMIN_HOST_LIST, HostList.getHostNames(installList.getHosts(Host.HOST_TYPE_ADMIN), " "));
                idata.setVariable(VAR_SUBMIT_HOST_LIST, HostList.getHostNames(installList.getHosts(Host.HOST_TYPE_SUBMIT), " "));

                outputFilePostfix = "";
            } else {
                idata.setVariable(VAR_SHADOW_HOST_LIST, "");
                idata.setVariable(VAR_ADMIN_HOST_LIST, "");
                idata.setVariable(VAR_SUBMIT_HOST_LIST, "");

                outputFilePostfix = "_" + String.valueOf(index);
                index++;
            }

            if (localExecdSpoolDir.equals(idata.getVariable(VAR_EXECD_SPOOL_DIR))) {
                idata.setVariable(VAR_EXECD_SPOOL_DIR_LOCAL, "");
            } else {
                idata.setVariable(VAR_EXECD_SPOOL_DIR_LOCAL, localExecdSpoolDir);
            }

            outputFilePostfix = autoConfFile + outputFilePostfix + ".conf";
            try {
                Util.fillUpTemplate(autoConfTempFile, outputFilePostfix, idata.getVariables());
                Debug.trace("Generating auto_conf file: '" + outputFilePostfix + "'.");
            } catch (Exception ex) {
                Debug.error("Failed to generate auto_conf file: '" + outputFilePostfix + "'." + ex);
            }
        }

        /**
         * Build install table
         */

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
        //tabbedPane.updateUI();
        //tabbedPane.repaint();
        //setColumnsWidth();

        //Installation must be started in a new Thread
        new Thread() {

            @Override
            public void run() {
                startInstallation(installList);
            }
        }.start();
    }

    private void startInstallation(HostList installList) {
        //Initialize new threadPool for the installation
        threadPool = (ThreadPoolExecutor) Executors.newFixedThreadPool(Util.INSTALL_THREAD_POOL_SIZE);
        threadPool.setThreadFactory(new InstallerThreadFactory());
        //We need a new executor for shadowdTasks (only 1 task at single moment)
        ThreadPoolExecutor singleThreadPool = (ThreadPoolExecutor) Executors.newFixedThreadPool(1);
        singleThreadPool.setThreadFactory(new InstallerThreadFactory());

        progressTimer = new Timer("InstallTimer");
        progressTimer.schedule(new UpdateInstallProgressTimerTask(this, new ThreadPoolExecutor[]{threadPool, singleThreadPool}, getLabel("column.progress.label"), getLabel("progressbar.installing.label")), 10);

        boolean wait = false;
        long completed = 0, started = 0;
        try {
            for (Host h : installList) {
                Properties variablesCopy = new Properties();
                variablesCopy.putAll(idata.getVariables());

                h.setComponentVariables(variablesCopy);
                if (h.isBdbHost() || h.isQmasterHost()) {
                    wait = true;
                    completed = singleThreadPool.getCompletedTaskCount();
                }
                //TODO: Need to do the first task when no qmaster/execd

                //BDB, Qmaster, Shadowd instlalation go to special singlethreadPool
                if (h.isBdbHost() || h.isQmasterHost() || h.isShadowHost()) {
                    singleThreadPool.execute(new InstallTask(tables, h, variablesCopy, localizedMessages));
                } else if (started == 0) {
                    //This is a first execd/shadowd task and there were no other BDB or qmaster components
                    //Need to create a first task that will add all hosts as admin hosts
                    Properties vars = new Properties();
                    Host localhost = new Host(Host.Type.HOSTNAME, Host.localHostName, false, false, false, false, "");
                    vars.putAll(idata.getVariables());
                    List<String> allHosts = Util.getAllHosts((HostInstallTableModel) tables.get(0).getModel(), Host.localHostName);
                    vars.put(VAR_ALL_HOSTS, Util.listToString(allHosts));
                    vars.put(VAR_FIRST_TASK, "true");
                    //And execute the first task in the singleThreadPool
                    singleThreadPool.execute(new InstallTask(tables, localhost, vars, localizedMessages));
                    started++;
                    //Wait until it's finished
                    while (singleThreadPool.getCompletedTaskCount() < started && !singleThreadPool.isTerminated()) {
                        try {
                            Thread.sleep(100);
                        } catch (InterruptedException ex) {
                        }
                    }
                    threadPool.execute(new InstallTask(tables, h, variablesCopy, localizedMessages));
                } else {
                    //Only execd get installed in parallel
                    threadPool.execute(new InstallTask(tables, h, variablesCopy, localizedMessages));
                }
                started++;
                //In case the task is a BDB or qmaster host, we have to wait for sucessful finish!
                while (wait) {
                    if (singleThreadPool.getCompletedTaskCount() >= completed + 1) {
                        wait = false;
                        //If bdb or qmaster fails it's over!
                        if (h.getState() != Host.State.SUCCESS) {
                            HostInstallTableModel updateModel;
                            HostInstallTableModel installModel = (HostInstallTableModel) tables.get(0).getModel();
                            for (Host host : installList) {
                                updateModel = (HostInstallTableModel) tables.get(2).getModel();
                                installModel.setHostState(host, Host.State.FAILED_DEPENDENT_ON_PREVIOUS);
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
                        }
                    }
                }
            }
        } catch (Exception e) {
            Debug.error(e);
        } finally {
            //Wait until all tasks have finished
            while (threadPool.getCompletedTaskCount() + singleThreadPool.getCompletedTaskCount() < started && !(threadPool.isTerminated() && singleThreadPool.isTerminated())) {
                try {
                    Thread.sleep(100);
                } catch (InterruptedException ex) {
                }
            }
            //Execute final task to setup correct submit and remove invalid admin hosts silently on local host
            Properties vars = new Properties();
            Host localhost = new Host(Host.Type.HOSTNAME, Host.localHostName, false, false, false, false, "");
            vars.putAll(idata.getVariables());
            List<String> removeAdminHosts = Util.getAllHosts((HostInstallTableModel) tables.get(0).getModel(), Host.localHostName);
            List<String> submitHosts = new ArrayList<String>();
            for (Host h : installList) {
                if (!h.isAdminHost() && !removeAdminHosts.contains(h.getHostAsString())) {
                    removeAdminHosts.add(h.getHostAsString());
                }
                if (h.isSubmitHost() && !submitHosts.contains(h.getHostAsString())) {
                    submitHosts.add(h.getHostAsString());
                }
            }
            //Remove localhost and put as last host
            if (removeAdminHosts.contains(Host.localHostName)) {
                removeAdminHosts.remove(Host.localHostName);
            }
            removeAdminHosts.add(Host.localHostName);
            vars.put(VAR_REMOVE_ADMINHOSTS, Util.listToString(removeAdminHosts));
            vars.put(VAR_SUBMIT_HOST_LIST, Util.listToString(submitHosts));
            vars.put(VAR_LAST_TASK, "true");
            //localhost.setComponentVariables(vars);
            //And execute the last task in the singleThreadPool
            singleThreadPool.execute(new InstallTask(tables, localhost, vars, localizedMessages));
            started++;
            //Wait until it's finished
            while (threadPool.getCompletedTaskCount() + singleThreadPool.getCompletedTaskCount() < started && !singleThreadPool.isTerminated()) {
                try {
                    Thread.sleep(100);
                } catch (InterruptedException ex) {
                }
            }
            //Wait for all pending tasks to finish in all threadPools
            try {
                threadPool.awaitTermination(100, TimeUnit.MILLISECONDS);
                singleThreadPool.awaitTermination(100, TimeUnit.MILLISECONDS);
                threadPool.purge();
                singleThreadPool.purge();
            } catch (InterruptedException ex) {
            }
            //If we have failed hosts we go to the Failed tab
            if (((HostInstallTableModel) tables.get(2).getModel()).getRowCount() > 0) {
                tabbedPane.setSelectedIndex(2);
            } else { //Go to succeeded tab
                tabbedPane.setSelectedIndex(1);
            }
            threadPool.shutdown();
            singleThreadPool.shutdown();
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
    private boolean advancedMode = true;
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
        if (lists.size() > i) {
            return lists.get(i);
        } else {
            return null;
        }
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

    public void runTest() {
    }

    /**
     * @return the installData
     */
    public InstallData getInstallData() {
        return idata;
    }

    public RulesEngine getRuleEngine() {
        return parent.getRules();
    }
}

class ResolveHostTask extends TestableTask {

    private HostPanel panel;
    private Host host;
    private ThreadPoolExecutor tpe;
    private HostSelectionTableModel model;

    public ResolveHostTask(ThreadPoolExecutor tpe, Host h, HostPanel panel) {

        this.tpe = tpe;
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
            Debug.error("Unknown host: " + host);
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
            CommandExecutor archCmd = new CommandExecutor(panel.getInstallData().getVariables(), panel.getInstallData().getVariable(VAR_SHELL_NAME), host.getHostname(),
                    "'if [ ! -s " + SGE_ROOT + "/util/arch ]; then echo \"File " + SGE_ROOT + "/util/arch does not exist or empty.\"; exit " + CommandExecutor.EXITVAL_MISSING_FILE + "; else " + SGE_ROOT + "/util/arch ; fi'");
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
        } else if (exitValue == CommandExecutor.EXITVAL_MISSING_FILE) {
            state = Host.State.MISSING_FILE;
            tabPos = 2;
        } else if (exitValue == CommandExecutor.EXITVAL_INTERRUPTED) {
            state = Host.State.CANCELED;
            tabPos = 2;
        } else {
            state = Host.State.UNREACHABLE;
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

/**
 * Thread to check settings remotely on the specified host.
 */
class CheckHostTask extends TestableTask {

    private Host host = null;
    private HostPanel panel = null;
    private Properties variables = null;
    private Host.State prevState = null;

    public CheckHostTask(Host host, HostPanel panel) {
        setTaskName("CheckHostTask - " + host);

        this.host = host;
        this.prevState = host.getState();
        this.panel = panel;
        this.variables = panel.getInstallData().getVariables();
    }

    public void run() {
        int exitValue = getTestExitValue();
        long start = System.currentTimeMillis(), end;
        Host.State newState = prevState;

        setHostState(Host.State.CONTACTING);

        if (isIsTestMode()) {
            setHostState(prevState);
            return;
        }

        // Check port usage in java only if the host is the localhost
        // TODO check port usage in the script in case of remote host
        if (host.isLocalhost()) {
            if (host.isExecutionHost()) {
                // Check execd port usage
                if (!Util.isPortFree(host.getIp(), panel.getInstallData().getVariable(VAR_SGE_EXECD_PORT))) {
                    setHostState(Host.State.USED_EXECD_PORT);
                }
            }

            if (host.isQmasterHost()) {
                // Check jmx port usage
                if (panel.getRuleEngine().isConditionTrue(COND_JMX, variables) &&
                        !Util.isPortFree(host.getIp(), variables.getProperty(VAR_SGE_JMX_PORT))) {
                    setHostState(Host.State.USED_JMX_PORT);
                    return;
                }

                // Check qmaster port usage
                if (!Util.isPortFree(host.getIp(), variables.getProperty(VAR_SGE_QMASTER_PORT))) {
                    setHostState(Host.State.USED_QMASTER_PORT);
                    return;
                }
            }

            if (host.isBdbHost()) {
            }
        }

        VariableSubstitutor vs = new VariableSubstitutor(variables);

        // Fill up cfg.exec.spool.dir.local
        if (host.isExecutionHost() && !host.getSpoolDir().equals(variables.getProperty(VAR_EXECD_SPOOL_DIR))) {
            variables.setProperty(VAR_EXECD_SPOOL_DIR_LOCAL, host.getSpoolDir());
        } else {
            variables.setProperty(VAR_EXECD_SPOOL_DIR_LOCAL, "");
        }

        String checkHostTempFile = vs.substituteMultiple(variables.getProperty(VAR_CHECK_HOST_TEMP_FILE), null);
        String checkHostFile = vs.substituteMultiple(variables.getProperty(VAR_CHECK_HOST_FILE), null);

        checkHostFile = "/tmp/" + checkHostFile + "." + host.getHostAsString();
        Debug.trace("Generating check_host file: '" + checkHostFile + "'.");
        try {
            panel.getInstallData().getVariables().put("host.arch", host.getArchitecture());
            checkHostFile = Util.fillUpTemplate(checkHostTempFile, checkHostFile, panel.getInstallData().getVariables());

            Debug.trace("Copy auto_conf file to '" + host.getHostname() + ":" + checkHostFile + "'.");
            CommandExecutor cmd = new CommandExecutor(variables, variables.getProperty(VAR_COPY_COMMAND), checkHostFile,
                    host.getHostname() + ":" + checkHostFile + " && if [ ! -s " + checkHostFile + " ]; then echo 'File " + checkHostFile + " does not exist or empty.'; exit 1; fi");
            cmd.execute();
            exitValue = cmd.getExitValue();
            if (exitValue == CommandExecutor.EXITVAL_TERMINATED) {
                //Set the log content
                newState = Host.State.COPY_TIMEOUT_CHECK_HOST;
                Debug.error("Timeout while copying the " + checkHostFile + " script to host " + host.getHostname() + " via " + variables.getProperty(VAR_COPY_COMMAND) + " command!\nMaybe a password is expected. Try the command in the terminal first.");
            } else if (exitValue != 0) {
                newState = Host.State.COPY_FAILED_CHECK_HOST;
                Debug.error("Error when copying the file.");
            } else {
                new CommandExecutor(variables, variables.getProperty(VAR_SHELL_NAME), host.getHostAsString(), "chmod", "755", checkHostFile).execute();
                cmd = new CommandExecutor(variables, 10000, //Set install timeout to 60secs
                        variables.getProperty(VAR_SHELL_NAME),
                        host.getHostAsString(), checkHostFile, String.valueOf(host.isBdbHost()),
                        String.valueOf(host.isQmasterHost()), String.valueOf(host.isShadowHost()),
                        String.valueOf(host.isExecutionHost()));
                //TODO: Beta and later should delete the file as part of the command add - , ";", "rm", "-f", localScript);
                //Debug.trace("Start installation: "+cmd.getCommands());
                cmd.execute();
                int exitValueFromOutput = Util.getExitValue(cmd.getOutput());
                exitValue = cmd.getExitValue();
                //We don't trust the exit codes (rsh does not honor them)
                exitValue = (exitValueFromOutput != 0 && exitValue == 0) ? exitValueFromOutput : exitValue;

                // Set the new state of the host depending on the return value of the script
                switch (exitValue) {
                    case EXIT_VAL_SUCCESS: newState = Host.State.REACHABLE; break;
                    case EXIT_VAL_BDB_SERVER_SPOOL_DIR_PERM_DENIED: newState = Host.State.PERM_BDB_SPOOL_DIR; break;
                    case EXIT_VAL_QMASTER_SPOOL_DIR_PERM_DENIED: newState = Host.State.PERM_QMASTER_SPOOL_DIR; break;
                    case EXIT_VAL_EXECD_SPOOL_DIR_PERM_DENIED: newState = Host.State.PERM_EXECD_SPOOL_DIR; break;
                    case EXIT_VAL_JMX_KEYSTORE_PERM_DENIED: newState = Host.State.PERM_JMX_KEYSTORE; break;
                    case EXIT_VAL_BDB_SPOOL_DIR_PERM_DENIED: newState = Host.State.PERM_BDB_SPOOL_DIR; break;
                    case EXIT_VAL_EXECD_SPOOL_DIR_LOCAL_PERM_DENIED: newState = Host.State.PERM_EXECD_SPOOL_DIR; break;
                    case EXIT_VAL_BDB_SPOOL_DIR_EXISTS: newState = Host.State.BDB_SPOOL_DIR_EXISTS; break;
                    case EXIT_VAL_ADMIN_USER_NOT_KNOWN: newState = Host.State.ADMIN_USER_NOT_KNOWN; break;
                    case EXIT_VAL_BDB_SPOOL_WRONG_FSTYPE: newState = Host.State.BDB_SPOOL_DIR_WRONG_FSTYPE; break;
                    default: newState = Host.State.UNKNOWN_ERROR; break;
                }
            }
        } catch (InterruptedException e) {
            Debug.error(e);
        } catch (Exception e) {
            Debug.error("Failed to check host '" + host + "'. " + e);
        } finally {
            setHostState(newState);
        }

        end = System.currentTimeMillis();
        Debug.trace("Checking host took " + (end - start) + "ms");
    }

    /**
     * Sets the state of the host in the "All hosts" and in the "Reachable" tables
     * @param state The new state of the host
     */
    private void setHostState(Host.State state) {
        ((HostSelectionTableModel) panel.getHostTableAt(0).getModel()).setHostState(host, state);
        ((HostSelectionTableModel) panel.getHostTableAt(1).getModel()).setHostState(host, state);
    }
}

class UpdateInstallProgressTimerTask extends TimerTask {

    private HostPanel panel;
    private ThreadPoolExecutor[] tpes;
    private JProgressBar mainBar;
    private JButton cancelButton;
    private String colName;
    
    private static int instanceCounter = 0;

    public UpdateInstallProgressTimerTask(final HostPanel panel, ThreadPoolExecutor tpe, final String colName, final String prefix) {
        this(panel, new ThreadPoolExecutor[]{tpe}, colName, prefix);
    }

    public UpdateInstallProgressTimerTask(final HostPanel panel, ThreadPoolExecutor[] tpes, final String colName, final String prefix) {
        super();

        instanceCounter++;

        this.panel = panel;
        this.tpes = tpes;
        this.colName = colName;

        mainBar = panel.getProgressBar();
        mainBar.setMinimum(0);
        mainBar.setValue(getCompletedTaskCount());
        mainBar.setMaximum(getTaskCount());
        mainBar.setString(prefix);
        //mainBar.setString(prefix + " " + cur + " / " + max);
        mainBar.setStringPainted(true);
        mainBar.setVisible(true);

        cancelButton = panel.getCancelButton();
        cancelButton.setVisible(true);
    }

    @Override
    public void run() {
        Debug.trace("UpdateInstallProgressTimerTask - Start #" + instanceCounter);

        HostTable table;
        HostList list;
        DefaultTableModel model;
        Host.State state;
        Object obj;
        int i = 0, col = 0;
        boolean hasRunning = true;

        while (hasRunning) {
            hasRunning = false;
            i = 0;

            for (Iterator<HostTable> iter = panel.getHostTableIterator(); iter.hasNext();) {
                table = iter.next();
                list = panel.getHostListAt(i);
                col = table.getColumn(colName).getModelIndex();
                model = (DefaultTableModel) table.getModel();

                for (int row = 0; row < list.size(); row++) {
                    try {
                        obj = model.getValueAt(row, col);

                        if (obj instanceof Host.State) {
                            state = (Host.State) obj;

                            switch (state) {
                                case RESOLVING:
                                case INSTALLING:
                                case CONTACTING:
                                case READY_TO_INSTALL:
                                    hasRunning = true;
                            }
                        }

                        // if the state turns from RESOLVING/INSTALLING/CONTACTING to a final state
                        // the cell does not get updated and the progress bar remains shown
                        model.fireTableCellUpdated(row, col);
                    } catch (IndexOutOfBoundsException e) {
                        // if a host gets deleted during the progress: IndexOutOfBoundsException
                    }
                }
                i++;
            }

            //Update the main ProgressBar
            mainBar.setValue(getCompletedTaskCount());
            mainBar.setMaximum(getTaskCount());

            try {
                Thread.sleep(200);
            } catch (InterruptedException e) {
            }
        }

        //Last task should cancel the Timer and hide the progressBar
        mainBar.setVisible(false);
        cancelButton.setVisible(false);

        if (!HostPanel.checkMode && panel.getHostListAt(1) != null && panel.getHostListAt(1).size() > 0) {
            panel.enableInstallButton(true);
            if (HostPanel.installMode) {
                panel.triggerInstallButton(false);
            }
        }

        Debug.trace("UpdateInstallProgressTimerTask - End #" + instanceCounter);
    }

    /**
     * Returns the sum of completed tasks
     * @return the sum of completed tasks
     */
    private int getCompletedTaskCount() {
        int count = 0;

        for (ThreadPoolExecutor tpe : tpes) {
            if (tpe != null && !tpe.isTerminated()) {
                count += tpe.getCompletedTaskCount();
            }
        }

        return count;
    }

    /**
     * Returns the sum of tasks
     * @return the sum of tasks
     */
    private int getTaskCount() {
        int count = 0;

        for (ThreadPoolExecutor tpe : tpes) {
            if (tpe != null && !tpe.isTerminated()) {
                count += tpe.getTaskCount();
            }
        }

        return count;
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
        Debug.trace("[" + System.currentTimeMillis() + "] " + host.getHostAsString() + " Starting install task " + host.getComponentString());

        Host.State state;
        HostInstallTableModel updateModel;
        HostInstallTableModel installModel = (HostInstallTableModel) tables.get(0).getModel();
        installModel.setHostState(host, Host.State.INSTALLING);
        int tabPos = 0;

        String log = "";
        boolean prereqFailed = false;
        boolean isSpecialTask = Boolean.valueOf(variables.getProperty(VAR_FIRST_TASK, "false")) || Boolean.valueOf(variables.getProperty(VAR_LAST_TASK, "false"));

        try {
            String autoConfFile = null;
            int exitValue = getTestExitValue();
            Vector<String> output = getTestOutput();

            if (host.isBdbHost() || host.isQmasterHost() || host.isShadowHost() || host.isExecutionHost() || isSpecialTask) {
                if (!isSpecialTask) {
                    // Fill up cfg.exec.spool.dir.local
                    if (host.isExecutionHost() && !host.getSpoolDir().equals(variables.getProperty(VAR_EXECD_SPOOL_DIR))) {
                        variables.setProperty(VAR_EXECD_SPOOL_DIR_LOCAL, host.getSpoolDir());
                    } else {
                        variables.setProperty(VAR_EXECD_SPOOL_DIR_LOCAL, "");
                    }
                    //Fill up manually the cfg.allhosts in qmaster installation
                    String val = "";
                    if (host.isQmasterHost()) {
                        val = Util.listToString(Util.getAllHosts((HostInstallTableModel) tables.get(0).getModel(), Host.localHostName));
                        variables.setProperty(VAR_ALL_HOSTS, val);
                    }
                }

                String autoConfTempFile = vs.substituteMultiple(variables.getProperty(VAR_AUTO_INSTALL_COMPONENT_TEMP_FILE), null);
                autoConfFile = vs.substituteMultiple(variables.getProperty(VAR_AUTO_INSTALL_COMPONENT_FILE), null);

                //Appended CELL_NAME prevents a race in case of parallel multiple cell installations
                autoConfFile = "/tmp/" + autoConfFile + "." + host.getHostAsString() + "." + variables.getProperty(VAR_SGE_CELL_NAME);
                Debug.trace("Generating auto_conf file: '" + autoConfFile + "'.");
                autoConfFile = Util.fillUpTemplate(autoConfTempFile, autoConfFile, variables);
                new File(autoConfFile).deleteOnExit();
            }

            if (!isIsTestMode() && !prereqFailed) {
                //Need to copy the script to the final location first
                //TODO: Do this only when not on a shared FS (file is not yet accessible)
                Debug.trace("Copy auto_conf file to '" + host.getHostname() + ":" + autoConfFile + "'.");
                CommandExecutor cmd = new CommandExecutor(variables, variables.getProperty(VAR_COPY_COMMAND), autoConfFile, host.getHostname() + ":" + autoConfFile, " && if [ ! -s " + autoConfFile + " ]; then echo 'File " + autoConfFile + " does not exist or empty.'; exit 1; fi");
                cmd.execute();
                exitValue = cmd.getExitValue();
                if (exitValue == CommandExecutor.EXITVAL_TERMINATED) {
                    //Set the log content
                    setHostState(Host.State.COPY_TIMEOUT_INSTALL_COMPONENT);
                    log = "Timeout while copying the " + autoConfFile + " script to host " + host.getHostname() + " via " + variables.getProperty(VAR_COPY_COMMAND) + " command!\nMaybe a password is expected. Try the command in the terminal first.";
                    installModel.setHostLog(host, log);
                } else if (exitValue != 0) {
                    //TODO: polish output + error
                    setHostState(Host.State.COPY_TIMEOUT_INSTALL_COMPONENT);
                    log = "Error when copying the file " + autoConfFile + "to host " + host.getHostname() + " via " + variables.getProperty(VAR_COPY_COMMAND) + " command!\n";
                    log += "OUTPUT:\n" + cmd.getOutput();
                    log += "ERROR:\n" + cmd.getError();
                    installModel.setHostLog(host, log);
                } else {
                    new CommandExecutor(variables, variables.getProperty(VAR_SHELL_NAME), host.getHostAsString(), "chmod", "755", autoConfFile).execute();
                    int timeout = 60000; //Set install timeout to 60secs
                    if (host.isQmasterHost()) { //Qmaster to 2mins
                        timeout *= 2;
                    }
                    cmd = new CommandExecutor(variables, timeout,
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
            } else if (exitValue == EXIT_VAL_FAILED_ALREADY_INSTALLED_COMPONENT) {
                state = Host.State.FAILED_ALREADY_INSTALLED_COMPONENT;
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
        } finally {
            //Unset the special variables
            variables.remove(VAR_FIRST_TASK);
            variables.remove(VAR_LAST_TASK);
            variables.remove(VAR_ALL_HOSTS);
            variables.remove(VAR_ALL_SUBMIT_HOSTS);
            variables.remove(VAR_REMOVE_ADMINHOSTS);
        }

        if (!isSpecialTask) {
            updateModel = (HostInstallTableModel) tables.get(tabPos).getModel();
            installModel.setHostState(host, state);
            installModel.removeHost(host);
            updateModel.addHost(host);
        }

        //panel.getTabbedPane().setTitleAt(tabPos, HostPanel.INSTALL_TABS[tabPos] + " (" + updateModel.getRowCount() + ")");
        Debug.trace("[" + System.currentTimeMillis() + "] " + host.getHostAsString() + " Finished installtask " + host.getComponentString());
    }

    /**
     * Sets the state of the host in the "To Be Installed" and in the "Ok or Failed" tables
     * @param state The new state of the host
     */
    private void setHostState(Host.State state) {
        ((HostInstallTableModel) tables.get(0).getModel()).setHostState(host, state);
        ((HostInstallTableModel) tables.get((state == Host.State.SUCCESS) ? 2 : 1).getModel()).setHostState(host, state);
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
