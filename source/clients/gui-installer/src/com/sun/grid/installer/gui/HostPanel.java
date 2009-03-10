/*___INFO__MARK_BEGIN__*///GEN-LINE:variables
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

import com.sun.grid.installer.gui.Host.State;
import com.sun.grid.installer.task.ThreadPoolObserver.ThreadPoolEvent;
import com.sun.grid.installer.task.TestableTask;
import com.sun.grid.installer.task.ValidateHostTask;
import com.sun.grid.installer.task.GetArchitectureTask;
import com.sun.grid.installer.task.InstallTask;
import com.izforge.izpack.installer.InstallData;
import com.izforge.izpack.installer.InstallerFrame;
import com.izforge.izpack.installer.IzPanel;
import com.izforge.izpack.util.Debug;
import com.izforge.izpack.util.VariableSubstitutor;
import com.sun.grid.installer.task.TaskHandler;
import com.sun.grid.installer.task.TaskThreadFactory;
import com.sun.grid.installer.task.ThreadPoolObserver;
import com.sun.grid.installer.util.cmd.CmdExec;
import com.sun.grid.installer.util.Config;
import com.sun.grid.installer.util.Util;
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
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Properties;
import java.util.Vector;
import java.util.Set;
import java.util.concurrent.Executors;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;
import java.text.MessageFormat;

import java.util.Enumeration;
import java.util.Hashtable;
import javax.swing.AbstractButton;
import javax.swing.JButton;
import javax.swing.JFileChooser;
import javax.swing.JOptionPane;
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

public class HostPanel extends IzPanel implements Config, ThreadPoolObserver.ThreadPoolListener, TaskHandler {

    private ActionListener[] nextButtonListeners = null;

    private boolean isQmasterInst = true;
    private boolean isShadowdInst = true;
    private boolean isExecdInst = true;
    private boolean isBdbInst = true;
    private boolean isExpressInst = true;
    private boolean advancedMode = true;
    private boolean errorMessageVisible = false;
    public static boolean installMode, checkMode = false;

    public static String[] SELECTION_TABS = null;
    public static String[] SELECTION_TABS_TOOLTIPS = null;
    public static String[] INSTALL_TABS = null;
    public static String[] INSTALL_TABS_TOOLTIPS = null;
    
    private static Properties localizedMessages = new Properties();

    private static List<String> additionalAdminHosts, additionalSubmitHosts;
    private Host firstTaskHost, lastTaskHost;

    private Vector<HostTable> tables;
    private Vector<HostList> lists;
    
    private ThreadPoolExecutor threadPool = null;
    private ThreadPoolExecutor singleThreadPool = null;
    private ThreadPoolObserver observer = null;

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
        setComponentSelectionVisible(advancedMode);
        
        progressBar.setVisible(false);
        progressBar.setMinimum(0);
        progressBar.setStringPainted(true);
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
            table = new HostTable(this);
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

            // To the success panel add listener which handles install/next button
            // visibility
            if (i == 1) {
                table.getModel().addTableModelListener(new TableModelListener() {
                    public void tableChanged(TableModelEvent e) {
                        if (((DefaultTableModel)e.getSource()).getRowCount() > 0) {
                            enableInstallButton(true);
                        } else {
                            enableInstallButton(false);
                        }
                    }
                });
            }

            tabbedPane.addTab(SELECTION_TABS[i] + " (" + lists.get(i).size() + ")", new JScrollPane(tables.get(i)));
            tabbedPane.setToolTipTextAt(i, SELECTION_TABS_TOOLTIPS[i]);
        }

        hostTF.requestFocus();
        this.repaint();
    }

    /**
     * Returns with the localized header texts for Selection table
     * @return the localized header texts
     */
    private String[] getSelectionHeaders() {
        String[] selectionLabels = getSelectionLabelVars();

        for (int i = 0; i < selectionLabels.length; i++) {
            selectionLabels[i] = getLabel(selectionLabels[i]);
        }

        return selectionLabels;
    }

    /**
     * Returns with the key of the selection table column headers
     * @return The keys of the selection table column headers
     */
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

    /**
     * Returns with the localized header texts for Install table
     * @return the localized header texts
     */
    private String[] getInstallHeaders() {
        String[] installLabels = getInstallLabelVars();

        for (int i = 0; i < installLabels.length; i++) {
            installLabels[i] = getLabel(installLabels[i]);
        }

        return installLabels;
    }

    /**
     * Returns with the key of the install table column headers
     * @return The keys of the install table column headers
     */
    private String[] getInstallLabelVars() {
        return new String[]{
                    "column.component.label",
                    "column.hostname.label",
                    "column.ip.address.label",
                    "column.arch.label",
                    "column.progress.label",
                    "column.log.label"};
    }

    /**
     * Returns with the class types of the selection table columns
     * @return the class types of the selection table columns
     */
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

    /**
     * Returns with the class types of the install table columns
     * @return the class types of the intall table columns
     */
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
    // <editor-fold defaultstate="collapsed" desc="Generated Code">
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
   }// </editor-fold> 

    @Override
    public void panelActivate() {
        isQmasterInst = parent.getRules().isConditionTrue(COND_INSTALL_QMASTER, idata.getVariables());
        isShadowdInst = parent.getRules().isConditionTrue(COND_INSTALL_SHADOWD, idata.getVariables());
        isExecdInst = parent.getRules().isConditionTrue(COND_INSTALL_EXECD, idata.getVariables());
        isBdbInst = parent.getRules().isConditionTrue(COND_INSTALL_BDB, idata.getVariables()) ||
                parent.getRules().isConditionTrue(COND_SPOOLING_BDBSERVER, idata.getVariables());
        isExpressInst = parent.getRules().isConditionTrue(COND_EXPRESS_INSTALL, idata.getVariables());

        //Disable only is we have no reachable host!
        if (((HostSelectionTableModel)tables.get(1).getModel()).getRowCount() == 0) {
           enableInstallButton(false);
        }

        triggerInstallButton(true);
        setupComponentSelectionPanel();
        setColumnsWidth();

        if (threadPool == null) {
            threadPool = (ThreadPoolExecutor) Executors.newFixedThreadPool(Util.RESOLVE_THREAD_POOL_SIZE);
            threadPool.setThreadFactory(new TaskThreadFactory());
        }

        if (observer == null) {
            observer = new ThreadPoolObserver(threadPool);
            observer.addThreadPoolListener(this);
        }

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
                    if (!idata.getVariable(VAR_QMASTER_HOST).equals("")) {
                        host.add(idata.getVariable(VAR_QMASTER_HOST));

                        resolveHosts(Host.Type.HOSTNAME, host, true, false, (isShadowdInst == true) ? true : false, false, true, true, idata.getVariable(VAR_EXECD_SPOOL_DIR));

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
            qmasterHostName = HostSelectionTableModel.getQmasterHost().getHostname();
        }
        idata.setVariable(VAR_QMASTER_HOST, qmasterHostName);
        String bdbHostName = "";
        if (HostSelectionTableModel.getBdbHost() != null) {
            bdbHostName = HostSelectionTableModel.getBdbHost().getHostname();
        }
        idata.setVariable(VAR_DB_SPOOLING_SERVER, bdbHostName);
    }

    /**
     * Switches between the Next and Install button
     * @param show if true the Install button will be shown, otherwise the Next button.
     */
    private synchronized void triggerInstallButton(boolean show) {
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

    /**
     * Removes the listeners from the given {@link AbstractButton}
     * @param button The button which listeners have to be removed
     * @return The removed listeners
     */
    private ActionListener[] removeListeners(AbstractButton button) {
        ActionListener[] listeners = button.getActionListeners();

        for (int i = 0; i < listeners.length; i++) {
            button.removeActionListener(listeners[i]);
        }

        return listeners;
    }

    /**
     * Returns with localized tooltip for the given key
     * @param key The key which identifies the localized tooltip
     * @return The localized tooltip if there is any. Empty string otherwise.
     */
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

    /**
     * Returns with localized text for the given key
     * @param key The key which identifies the localized text
     * @return The localized text if there is any. Empty string otherwise.
     */
    public String getLabel(String key) {
        String label = "";

        if (idata.langpack != null) {
            label = idata.langpack.getString(key);
        }

        return label;
    }

    /**
     * Sets the visibility on the Component Selection panel regarding to component
     * selection and instalall mode
     */
    private void setupComponentSelectionPanel() {
        SwingUtilities.invokeLater(new Runnable() {

            public void run() {
                componentSelectionPanel.setVisible(!isExpressInst);
                if (!isExecdInst) {
                    execCB.setSelected(false);
                    execCB.setVisible(false);
                } else {
                    execCB.setSelected(true);
                    execCB.setVisible(true);
                }
                // Shadow component is not default on
                if (!isShadowdInst) {
                   shadowCB.setSelected(false);
                   shadowCB.setVisible(false);
                } else {
                   shadowCB.setSelected(false);
                   shadowCB.setVisible(true);
                }
                qmasterCB.setSelected(false);
                qmasterCB.setVisible(false);
                bdbCB.setSelected(false);
                bdbCB.setVisible(false);

                // Admin component is not default on
                adminCB.setSelected(false);
            }
        });
    }

    /**
     * Sets the column widths in the tables regards to the component selection and
     * instalall mode
     */
    private void setColumnsWidth() {
        SwingUtilities.invokeLater(new Runnable() {

            public void run() {
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
        });
    }

    /**
     * Returns with the minimum cloumns width for the given header text
     * @param table The table which contains the clomn
     * @param columnHeader The header text of the column
     * @return The minimum column with for the given header text
     */
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
            Host.Type type = Util.isIpPattern(pattern) ? Host.Type.IP : Host.Type.HOSTNAME;
            List<String> list = Util.parsePattern(pattern, type);
            resolveHosts(type, list, qmasterCB.isSelected(), bdbCB.isSelected(), shadowCB.isSelected(), execCB.isSelected(), adminCB.isSelected(), submitCB.isSelected(), idata.getVariable(VAR_EXECD_SPOOL_DIR));
        } catch (IllegalArgumentException ex) {
            statusBar.setVisible(true);
            statusBar.setText("Error: " + ex.getMessage());
            hostTF.setSelectionStart(0);
            hostTF.setSelectionEnd(hostTF.getText().length());
            errorMessageVisible = true;
        }
    }

    private void hostTFActionPerformed(java.awt.event.ActionEvent evt) {
        addHostsFromTF();
    }

    private void hostTFFocusGained(java.awt.event.FocusEvent evt) {
        hostRB.setSelected(true);
        hostTF.setEditable(true);
        //addHostFocusGained();
        selectTextField(hostTF);
    }

    private void fileBFocusGained(java.awt.event.FocusEvent evt) {
        fileRB.setSelected(true);
        hostTF.setEditable(false);
    }

    private void fileBActionPerformed(java.awt.event.ActionEvent evt) {
        final JFileChooser fc = new JFileChooser();
        int ret = fc.showOpenDialog(this.getParent());
        if (ret == JFileChooser.APPROVE_OPTION) {
            File f = fc.getSelectedFile();
            try {
                // TODO improve
                List<String> list = Util.parseFileList(f);
                List<String> tmp = new ArrayList<String>();
                Host.Type type = Host.Type.HOSTNAME;
                for (String host : list) {
                    type = Util.isIpPattern(host) ? Host.Type.IP : Host.Type.HOSTNAME; //Using last value as type
                    //List<String> list = Util.parsePattern(host, type);
                    tmp.add(host);
                }
                resolveHosts(type, tmp, qmasterCB.isSelected(), bdbCB.isSelected(), shadowCB.isSelected(), execCB.isSelected(), adminCB.isSelected(), submitCB.isSelected(), idata.getVariable(VAR_EXECD_SPOOL_DIR));
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
    }

    private void selectTextField(JTextField tf) {
        tf.requestFocus();
        tf.setSelectionStart(0);
        tf.setSelectionEnd(tf.getText().length());
        lastSelectedTF = tf;
    }

    private void hostRBFocusGained(java.awt.event.FocusEvent evt) {
        hostTF.setEditable(true);
        selectTextField(hostTF);
    }

    private void fileRBFocusGained(java.awt.event.FocusEvent evt) {
        hostTF.setEditable(false);
        lastSelectedTF.setSelectionEnd(0);
    }

    private void buttonActionPerformed(java.awt.event.ActionEvent evt) {
        if (hostRB.isSelected()) {
            addHostsFromTF();
            hostRB.requestFocus();
        }
    }

    private void setComponentSelectionVisible(boolean b) {
        componentSelectionPanel.setVisible(b);
    }

    private void addBActionPerformed(java.awt.event.ActionEvent evt) {
        buttonActionPerformed(evt);
    }

    private void cancelBActionPerformed(java.awt.event.ActionEvent evt) {                                        
        cancelActions();
    }                                       

    /**
     * Cancels the running tasks
     */
    private void cancelActions() {
        ThreadPoolExecutor[] tpes = new ThreadPoolExecutor[]{singleThreadPool, threadPool};

        for (ThreadPoolExecutor tpe : tpes) {
            if (tpe == null) {
                continue;
            }

            //Cancel host selection thread pool
            List<Runnable> waitingTasks = tpe.shutdownNow();

            try {
                tpe.awaitTermination(200, TimeUnit.MILLISECONDS);
                tpe.purge();
            } catch (InterruptedException e) {
                Debug.error(e);
            }

            try {
                if (waitingTasks.size() > 0) {
                    ThreadPoolExecutor tmp = (ThreadPoolExecutor) Executors.newFixedThreadPool(waitingTasks.size() * 2);
                    tmp.setThreadFactory(new TaskThreadFactory());

                    for (Iterator<Runnable> it = waitingTasks.iterator(); it.hasNext();) {
                        TestableTask runnable = (TestableTask) it.next();
                        runnable.setTestMode(true);
                        runnable.setTestExitValue(CmdExec.EXITVAL_INTERRUPTED);
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
        }

        progressBar.setVisible(false);
        cancelB.setVisible(false);
    }

    /**
     * Sets the tabbed panes title depending on the
     */
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

    /**
     * Method to capture {@link ThreadPoolEvent}s
     * @param threadPoolEvent The {@link ThreadPoolEvent} has been fired.
     */
    public void threadPoolActionPerformed(ThreadPoolEvent threadPoolEvent) {
        final int type = threadPoolEvent.getType();
        final ThreadPoolObserver obs = (ThreadPoolObserver) threadPoolEvent.getSource();

        SwingUtilities.invokeLater(new Runnable() {

            public void run() {

                /**
                 * Update main progress bar
                 */
                int cur = obs.getLastRunCompletedTaskCount();
                int max = obs.getLastRunTaskCount();

                String text = getLabel("progressbar.resolving.label");
                if (checkMode) {
                    text = getLabel("progressbar.checking.label");
                } else if (installMode) {
                    text = getLabel("progressbar.installing.label");
                }

                switch (type) {
                    case ThreadPoolEvent.EVENT_THREAD_POOL_STARTED: {
                        progressBar.setValue(cur);
                        progressBar.setMaximum(max);
                        progressBar.setString(text + " " + cur + " / " + max);
                        progressBar.setVisible(true);
                        cancelB.setVisible(true);
                        break;
                    }
                    case ThreadPoolEvent.EVENT_THREAD_POOL_FINISHED: {
                        progressBar.setVisible(false);
                        cancelB.setVisible(false);
                        break;
                    }
                    case ThreadPoolEvent.EVENT_THREAD_POOL_UPDATED: {
                        progressBar.setValue(obs.getLastRunCompletedTaskCount());
                        progressBar.setMaximum(obs.getLastRunTaskCount());
                        progressBar.setString(text + " " + cur + " / " + max);
                        break;
                    }
                }
            }
        });
    }

    /**
     * Resolves the given hosts
     * @param type The {@link Host.Type} type of the host ID list: HOSTNAME or IP
     * @param list The host ID list
     * @param isQmasterHost Indicates whether the host is a qmaster host
     * @param isBDBHost Indicates whether the host is a BDB host
     * @param isShadowHost Indicates whether the host is a shadow host
     * @param isExecutionHost Indicates whether the host is a execution host
     * @param isAdminHost  Indicates whether the host is a admin host
     * @param isSubmitHost Indicates whether the host is a submit host
     * @param execdSpoolDir The execution spool dir for the host
     */
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

    /**
     * Resolves the given hosts
     * @param hosts The host list
     */
    public void resolveHosts(List<Host> hosts) {
        HostTable table = tables.get(0);
        HostSelectionTableModel model = (HostSelectionTableModel) table.getModel();

        if (threadPool.isShutdown()) {
            threadPool = (ThreadPoolExecutor) Executors.newFixedThreadPool(Util.RESOLVE_THREAD_POOL_SIZE);
            threadPool.setThreadFactory(new TaskThreadFactory());

            observer = new ThreadPoolObserver(threadPool);
            observer.addThreadPoolListener(this);
        }

        observer.observe();
        
        for (Host host : hosts) {
            host = model.addHost(host);

            try {
                threadPool.execute(new GetArchitectureTask(host, this, idata.getVariable(VAR_SHELL_NAME), idata.getVariable(VAR_SGE_ROOT),
                        idata.getVariable(VAR_SGE_QMASTER_PORT), idata.getVariable(VAR_EXECD_SPOOL_DIR)));
            } catch (RejectedExecutionException e) {
                setHostState(host, State.CANCELED);
            }
        }
    }

    /**
     * Makes some fixes on the variables. Should be used right before the installation.
     */
    private void fixVariables() {
        VariableSubstitutor vs = new VariableSubstitutor(idata.getVariables());
        String variable = "";

        // cfg.spooling.method.berkeleydbserver
        variable = idata.getVariable(VAR_SPOOLING_METHOD_BERKELEYDBSERVER);
        if (variable.equals("none")) {
            idata.setVariable(VAR_SPOOLING_METHOD_BERKELEYDBSERVER, "berkeleydb");
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
                idata.setVariable(VAR_DB_SPOOLING_SERVER, h.getHostname());
            }

            // add.qmaster.host
            if (h.isQmasterHost()) {
                idata.setVariable(VAR_QMASTER_HOST, h.getHostname());
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

    /**
     * Sets the tables enabled property
     * @param enabled if true the tables will be enabled, otherwise the tables will
     * be disbaled.
     */
    private void setTablesEnabled(final boolean enabled) {
        SwingUtilities.invokeLater(new Runnable() {

            public void run() {
                for (JTable table : tables) {
                    table.setEnabled(enabled);
                }
            }
        });
    }

        /**
     * Sets the install/next button's enabled property
     * @param b if true button will be enabled else the button will be disabled
     */
    private synchronized void enableInstallButton(boolean b) {
        if (parent == null) {
            return;
        }
        
        JButton nextButton = parent.getNextButton();
        
        if (b && !nextButton.isEnabled()) {
            SwingUtilities.invokeLater(new Runnable(){
                public void run() {
                    parent.unlockNextButton();
                }
            });
        } else if (!b && nextButton.isEnabled()) {
            SwingUtilities.invokeLater(new Runnable(){
                public void run() {
                    parent.lockNextButton();
                }
            });
        }
    }

    /**
     * Sets the sate of the given host
     * @param host The host
     * @param state The state to set
     */
    public void setHostState(Host host, State state) {

        /**
         * Set host state on the table(s)
         */
        ((HostTableModel)tables.get(0).getModel()).setHostState(host, state);

        if (checkMode) {
            ((HostTableModel)tables.get(1).getModel()).setHostState(host, state);
        }

        int targetTable = -1;
        switch (state) {
            //Move nowhere
            case NEW_UNKNOWN_HOST:
            case RESOLVING:
            case RESOLVABLE:
            case CONTACTING:
            case VALIDATING:
            case COPY_TIMEOUT_CHECK_HOST:
            case COPY_FAILED_CHECK_HOST:
            case PERM_QMASTER_SPOOL_DIR:
            case PERM_EXECD_SPOOL_DIR:
            case PERM_BDB_SPOOL_DIR:
            case BDB_SPOOL_DIR_EXISTS:
            case BDB_SPOOL_DIR_WRONG_FSTYPE:
            case ADMIN_USER_NOT_KNOWN:
            case PERM_JMX_KEYSTORE:
            case USED_QMASTER_PORT:
            case USED_EXECD_PORT:
            case USED_JMX_PORT:
            case UNKNOWN_ERROR:
            case READY_TO_INSTALL:
            case PROCESSING: break;

            //Success. Move to the success table
            case REACHABLE: {
                // At successful validation move nowhere
                if (checkMode) {
                    break;
                }
            }
            case OK:
            case SUCCESS: targetTable = 1; break;

            // Failed! Move to failed table.
            case MISSING_FILE:
            case UNKNOWN_HOST:
            case UNREACHABLE:
            case OPERATION_TIMEOUT:
            case COPY_TIMEOUT_INSTALL_COMPONENT:
            case COPY_FAILED_INSTALL_COMPONENT:
            case CANCELED: {
                // At canceled validation move nowhere
                if (checkMode) {
                    break;
                }
            }
            case FAILED:
            case FAILED_ALREADY_INSTALLED_COMPONENT: //not used
            case FAILED_DEPENDENT_ON_PREVIOUS: targetTable = 2; break;

            //Unknown state
            default: throw new IllegalArgumentException("Unknown state: "+state);
        }

        /**
         * Move host to the proper panel
         */
        if (targetTable > -1) {
            ((HostTableModel)tables.get(targetTable).getModel()).addHost(host);

            if (installMode) {
                ((HostTableModel) tables.get(0).getModel()).removeHost(host);
            }
        }
    }

    /**
     * Sets the log of the given host
     * @param host The host
     * @param log The log to set
     */
    public void setHostLog(Host host, String log) {
        ((HostTableModel)tables.get(0).getModel()).setHostLog(host, log);
    }

    /**
     * Removes the given host
     * @param host The host to remove
     */
    public void removeHost(Host host) {
        for (JTable table : tables) {
            ((HostTableModel)table.getModel()).removeHost(host);
        }
    }

    /**
     * Adds the given hosts
     * @param hosts Hosts to add to the data
     */
    public void addHosts(List<Host> hosts) {
        resolveHosts(hosts);
    }

    /**
     * Install button's action. Main etry point of the installation procedure
     */
    private void installButtonActionPerformed() {

        clearTableSelections();

        // Check end fix variables
        fixVariables();

        //Installation must be started in a new Thread
        new Thread() {

            @Override
            public void run() {
                validateHostsAndInstall(lists.get(1));
            }
        }.start();
    }

    /**
     * Validates the given host list then starts the installation
     * @param hosts The hosts have to be installed
     */
    private void validateHostsAndInstall(HostList hosts) {
        checkMode = true;
        //Disable the selecting host controls
        disableControls(true);
        if (parent != null) {
            SwingUtilities.invokeLater(new Runnable() {

                public void run() {
                    parent.lockNextButton();
                    parent.lockPrevButton();
                }
            });
        }

        setTablesEnabled(false);

        SwingUtilities.invokeLater(new Runnable() {

            public void run() {
                tabbedPane.setSelectedIndex(1);
            }
        });
        
        //Remove invalid components
        additionalAdminHosts = new ArrayList<String>();
        additionalSubmitHosts = new ArrayList<String>();
        Host o;
        List<Host> tmpList = new ArrayList<Host>(); //need a copy of the hostlist
        List<Integer> indexList = new ArrayList<Integer>();
        for (int i = 0; i < hosts.size(); i++) {
            o = new Host(hosts.get(i));
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
                //tmpList.addUnchecked(o);
                tmpList.add(o);
                indexList.add(i);
            } else {
                if (o.isAdminHost() && !(o.isBdbHost() || o.isQmasterHost() || o.isExecutionHost() || o.isShadowHost())) {
                    if (!additionalAdminHosts.contains(o.getHostname())) {
                        additionalAdminHosts.add(o.getHostname());
                    }
                }
                if (o.isSubmitHost() && !(o.isBdbHost() || o.isQmasterHost() || o.isExecutionHost() || o.isShadowHost())) {
                    if (!additionalSubmitHosts.contains(o.getHostname())) {
                        additionalSubmitHosts.add(o.getHostname());
                    }
                }
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
            String message;
            int res=JOptionPane.YES_OPTION;
            if (additionalAdminHosts.size() > 0 || additionalSubmitHosts.size() > 0) {
                message = new VariableSubstitutor(idata.getVariables()).substituteMultiple(idata.langpack.getString("warning.only.admin.submit.to.install.message"), null);
                res=JOptionPane.showOptionDialog(this, message, getLabel("installer.warning"), JOptionPane.YES_NO_OPTION, JOptionPane.WARNING_MESSAGE, null,
                                                 new Object[]{getLabel("installer.yes"), getLabel("installer.no")}, getLabel("installer.no"));
            } else {
                message = new VariableSubstitutor(idata.getVariables()).substituteMultiple(idata.langpack.getString("warning.no.components.to.install.message"), null);
                JOptionPane.showOptionDialog(this, message, getLabel("installer.error"),
                    JOptionPane.CANCEL_OPTION, JOptionPane.ERROR_MESSAGE, null,
                    new Object[]{getLabel("installer.cancel")}, getLabel("installer.cancel"));
                res = JOptionPane.NO_OPTION;
            }

            //If Cancel or No, we don't proceed with the installation
            if (res == JOptionPane.NO_OPTION) {
                checkMode = false;
                disableControls(false);
                if (parent != null) {
                    SwingUtilities.invokeLater(new Runnable() {

                        public void run() {
                            parent.unlockNextButton();
                            parent.unlockPrevButton();
                        }
                    });
                }
                setTablesEnabled(true);
                return;
            }
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
                    SwingUtilities.invokeLater(new Runnable() {

                        public void run() {
                            parent.unlockNextButton();
                            parent.unlockPrevButton();
                        }
                    });
                }
                setTablesEnabled(true);

                return;
            }
            vs = null; // Don't use it any more
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
        threadPool.setThreadFactory(new TaskThreadFactory());

        observer = new ThreadPoolObserver(threadPool);
        observer.addThreadPoolListener(this);
        observer.observe();

        try {
            long started = 0;
            Host h;
            for (int i : indexList) {
                h = hosts.get(i);
                //Check only hosts that have real components
                if (h.isExecutionHost() || h.isShadowHost() || h.isQmasterHost() || h.isBdbHost()) {
                    try {
                       threadPool.execute(new ValidateHostTask(h, this, idata.getVariables()));
                       started++;
                    } catch (RejectedExecutionException e) {
                        setHostState(h, State.CANCELED);
                    }
                }
            }

            while (threadPool.getCompletedTaskCount() < started && !threadPool.isTerminated()) {
                try {
                    Thread.sleep(500);
                } catch (InterruptedException ex) {
                }
            }

            int warningState = 0, noLocalSpoolWindows = 0;
            for (int i : indexList) {
                h = hosts.get(i);
                switch (h.getState()) {
                    case REACHABLE: break;
                    default: {
                        warningState++;
                    }
                }

                //If windows host, need to set a LOCAL_SPOOL_DIR if empty!
                if (!isExpressInst && h.getArchitecture().startsWith("win") &&
                        h.getSpoolDir().equals(idata.getVariable(VAR_EXECD_SPOOL_DIR))) {
                    noLocalSpoolWindows++;
                }
            }

            String msg = MessageFormat.format(getLabel("warning.invalid.hosts.message"), warningState, hosts.size());
            if (warningState > 0 && JOptionPane.NO_OPTION == JOptionPane.showOptionDialog(this, msg,
                    getLabel("installer.warning"), JOptionPane.YES_NO_OPTION, JOptionPane.WARNING_MESSAGE, null,
                    new Object[]{getLabel("installer.yes"), getLabel("installer.no")}, getLabel("installer.no"))) {

                disableControls(false);
                if (parent != null) {
                    SwingUtilities.invokeLater(new Runnable() {

                        public void run() {
                            parent.unlockNextButton();
                            parent.unlockPrevButton();
                        }
                    });
                }
                setTablesEnabled(true);

                return;
            }
            
            if (noLocalSpoolWindows > 0 && JOptionPane.NO_OPTION == JOptionPane.showOptionDialog(this, getLabel("warning.windows.needs.local.spooling.message"),
                    getLabel("installer.warning"), JOptionPane.YES_NO_OPTION, JOptionPane.WARNING_MESSAGE, null,
                    new Object[]{getLabel("installer.yes"), getLabel("installer.no")}, getLabel("installer.no"))) {

                disableControls(false);
                if (parent != null) {
                    SwingUtilities.invokeLater(new Runnable() {

                        public void run() {
                            parent.unlockNextButton();
                            parent.unlockPrevButton();
                        }
                    });
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

    private void switchToInstallModeAndInstall(final List<Host> hosts) {
        Debug.trace("INSTALL");

        //set install mode
        installMode = true;

        // Change panel heading
        parent.changeActualHeading(getLabel("HostPanel.installing.headline"));

        // add.spooling.method
        if (idata.getVariable(VAR_SPOOLING_METHOD).equals("none")) {
            idata.setVariable(VAR_SPOOLING_METHOD, "berkeleydb");
        }


        //Disable the selecting host controls
        disableControls(true);
        if (parent != null) {
            SwingUtilities.invokeLater(new Runnable() {

                public void run() {
                    parent.lockNextButton();
                    parent.lockPrevButton();
                }
            });
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

        firstTaskHost = new Host(Host.Type.HOSTNAME, Host.localHostName, Host.HOST_TYPE_ALL, true, false);
        firstTaskHost.setIp("all");
        firstTaskHost.setArchitecture("all");

        //Create install list
        final HostList installList = new HostList();
        boolean haveFirstTask = false;
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
                    //We add additional prerequisite task (first task)
                    installList.addUnchecked(firstTaskHost);
                    haveFirstTask = true;
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
        //Add first task if none added so far
        if (!haveFirstTask) {
            //We add additional prerequisite task (first task)
            installList.addUnchecked(firstTaskHost);
        }
        //Copy the rest (execds)
        for (Host host : tmpList) {
            installList.addUnchecked(new Host(host));
        }
        //Add the final task
        lastTaskHost = new Host(Host.Type.HOSTNAME, Host.localHostName, Host.HOST_TYPE_ALL, false, true);
        lastTaskHost.setIp("all");
        lastTaskHost.setArchitecture("all");
        installList.addUnchecked(lastTaskHost);

        //Standalone bdb, qmaster installations do not need the first task (for others we might need to copy CSP certs)
        if (installList.size() == 3) {
            Host firstHost = installList.get(0);
            if (firstHost.isBdbHost() || firstHost.isQmasterHost()) {
                installList.removeUnchecked(firstTaskHost);
            } else {
                firstTaskHost.setDisplayName(firstHost.getHostname());
                firstTaskHost.setIp(firstHost.getIp());
                firstTaskHost.setArchitecture(firstHost.getArchitecture());
            }
            lastTaskHost = installList.get(installList.size() - 1);
            lastTaskHost.setDisplayName(firstHost.getHostname());
            lastTaskHost.setIp(firstHost.getIp());
            lastTaskHost.setArchitecture(firstHost.getArchitecture());
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
                idata.setVariable(VAR_ADMIN_HOST_LIST, HostList.getHostNames(installList.getHosts(Host.HOST_TYPE_ADMIN), additionalAdminHosts, " "));
                idata.setVariable(VAR_SUBMIT_HOST_LIST, HostList.getHostNames(installList.getHosts(Host.HOST_TYPE_SUBMIT), additionalSubmitHosts, " "));

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
         * Hide the input panel, there is no way back anyway!
         */
        SwingUtilities.invokeLater(new Runnable() {

            public void run() {
                hostRB.setVisible(false);
                hostTF.setVisible(false);
                addB.setVisible(false);
                fileB.setVisible(false);
                fileRB.setVisible(false);
                componentSelectionPanel.setVisible(false);
                statusBar.setVisible(false);
            }
        });
        

        /**
         * Build install table
         */

        // Initialize column tooltips
        String[] installHeaders = getInstallLabelVars();
        String[] headerTooltips = new String[installHeaders.length];
        for (int i = 0; i < installHeaders.length; i++) {
            headerTooltips[i] = getTooltip(installHeaders[i]);
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
            table = new HostTable(this);
            lists.add(list);
            tables.add(table);
            table.setModel(new HostInstallTableModel(list, getInstallHeaders(), getInstallClassTypes()));

            table.setTableHeader(new TooltipTableHeader(table.getColumnModel(), headerTooltips));
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

            // To the success panel add listener which handles install/next button
            // visibility
            if (i == 1) {
                table.getModel().addTableModelListener(new TableModelListener() {
                    public void tableChanged(TableModelEvent e) {
                        if (((DefaultTableModel)e.getSource()).getRowCount() > 0) {
                            enableInstallButton(true);
                            triggerInstallButton(false);
                        } else {
                            enableInstallButton(false);
                        }
                    }
                });
            }

            tabbedPane.addTab(INSTALL_TABS[i] + " (" + lists.get(i).size() + ")", new JScrollPane(tables.get(i)));
            tabbedPane.setToolTipTextAt(i, INSTALL_TABS_TOOLTIPS[i]);
        }
        tabbedPane.validate();

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
        threadPool.setThreadFactory(new TaskThreadFactory());
        //We need a new executor for shadowdTasks (only 1 task at single moment)
        singleThreadPool = (ThreadPoolExecutor) Executors.newFixedThreadPool(1);
        singleThreadPool.setThreadFactory(new TaskThreadFactory());

        //progressTimer = new Timer("InstallTimer");
        observer = new ThreadPoolObserver(new ThreadPoolExecutor[]{threadPool, singleThreadPool});
        observer.addThreadPoolListener(this);
        observer.setTaskCount(installList.size());
        observer.observe();

        //Create a copy of the install list for the last_task
        HostList initialInstallList = new HostList();
        for (Host h: installList) {
            initialInstallList.addUnchecked(new Host(h));
        }

        boolean wait = false;
        long completed = 0, started = 0;
        try {
            for (Host h : installList) {
                //Skip the last task is done in finally
                if (h.isLastTask()) {
                    continue;
                }
                Properties variablesCopy = new Properties();
                variablesCopy.putAll(idata.getVariables());

                h.setComponentVariables(variablesCopy);
                if (h.isBdbHost() || h.isQmasterHost()) {
                    wait = true;
                    completed = singleThreadPool.getCompletedTaskCount();
                }

                //All windows need windows specified varibles to be set as well
                if (h.getArchitecture().startsWith("win")) {
                    variablesCopy.setProperty(VAR_WINDOWS_SUPPORT, "true");
                    variablesCopy.setProperty(VAR_WIN_DOMAIN_ACCESS, "true");
                    //TODO: What about WIN_ADMIN_USER?
                }

                //BDB, Qmaster, Shadowd instlalation go to special singlethreadPool
                if (h.isBdbHost() || h.isQmasterHost() || h.isShadowHost()) {
                    try {
                        singleThreadPool.execute(new InstallTask(h, this, variablesCopy, localizedMessages));
                    } catch (RejectedExecutionException e) {
                        setHostState(h, State.CANCELED);
                    }
                } else if (h.isFirstTask()) {
                    //This is a first execd/shadowd task and there were no other BDB or qmaster components
                    //Need to create a first task that will add all hosts as admin hosts
                    Properties vars = new Properties();
                    vars.putAll(idata.getVariables());
                    List<String> allHosts = Util.getAllHosts((HostInstallTableModel) tables.get(0).getModel(), Host.localHostName);
                    if (additionalAdminHosts.size() > 0) {
                       allHosts.addAll(additionalAdminHosts); //Don't need to check if in the list since they can't be there
                    }
                    vars.put(VAR_ALL_ADMIN_HOSTS, Util.listToString(allHosts));
                    //Only for CSP mode
                    if (vars.getProperty("add.product.mode").equals("csp")) {
                        List<String> cspList = Util.getAllHosts((HostInstallTableModel) tables.get(0).getModel(), "");
                        //Add additional admin hosts if any
                        if (additionalAdminHosts.size() > 0) {
                            for (String ah: additionalAdminHosts) {
                                if (!cspList.contains(ah)) {
                                    cspList.add(ah);
                                }
                            }
                        }
                        //Add additional submit hosts if any
                        if (additionalSubmitHosts.size() > 0) {
                            for (String sh: additionalSubmitHosts) {
                                if (!cspList.contains(sh)) {
                                    cspList.add(sh);
                                }
                            }
                        }
                        String val = Util.listToString(cspList);
                        String[] vals = val.split(h.getHostname());
                        String val2 = "";
                        for (String s : vals) {
                            val2 += s.trim() + " ";
                        }
                        vars.setProperty(VAR_ALL_CSPHOSTS, val2.trim());
                    }
                    vars.put(VAR_FIRST_TASK, "true");
                    wait = true;

                    try {
                        //And execute the first task in the singleThreadPool
                        singleThreadPool.execute(new InstallTask(h, this, vars, localizedMessages));
                    } catch (RejectedExecutionException e) {
                        setHostState(h, State.CANCELED);
                    }
                } else {
                    try {
                        //Only execd get installed in parallel
                        threadPool.execute(new InstallTask(h, this, variablesCopy, localizedMessages));
                    } catch (RejectedExecutionException e) {
                        setHostState(h, State.CANCELED);
                    }
                }

                started++;
                //In case the task is a BDB or qmaster host, we have to wait for sucessful finish!
                while (wait) {
                    if (singleThreadPool.getCompletedTaskCount() >= completed + 1 && !singleThreadPool.isTerminated()) {
                        wait = false;
                        //If bdb, qmaster, prereq tasks fail => it's over!
                        if (h.getState() != Host.State.SUCCESS) {
                            HostInstallTableModel updateModel;
                            HostInstallTableModel installModel = (HostInstallTableModel) tables.get(0).getModel();
                            for (Host host : installList) {
                                updateModel = (HostInstallTableModel) tables.get(2).getModel();
                                installModel.setHostState(host, Host.State.FAILED_DEPENDENT_ON_PREVIOUS);
                                installModel.setHostLog(host, "FAILED: " + MessageFormat.format(localizedMessages.getProperty("msg.previous.dependent.install.failed"), h.getComponentString()));
                                installModel.removeHost(host);
                                updateModel.addHost(host);
                            }
                            return;
                        }
                        completed++;
                    } else if (singleThreadPool.isTerminated()){
                        //There is not guarantee that the currently executing task wil lbe interrupted. It may also finish as SUCCESS or FAILED!
                        HostInstallTableModel updateModel;
                        HostInstallTableModel installModel = (HostInstallTableModel) tables.get(0).getModel();
                        for (Host host : installList) {
                            updateModel = (HostInstallTableModel) tables.get(2).getModel();
                            installModel.setHostState(host, Host.State.CANCELED);
                            installModel.setHostLog(host, "CANCELED: " + MessageFormat.format(localizedMessages.getProperty("msg.install.canceled"), ""));
                            installModel.removeHost(host);
                            updateModel.addHost(host);
                        }
                        return;
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
            if (lastTaskHost == null) {
                Debug.error("lastTaskHost not set!");
                throw new IllegalArgumentException("lastTaskHost not set!");
            } else if (lastTaskHost.getState() != Host.State.FAILED_DEPENDENT_ON_PREVIOUS) {
                //Run the last task only  if we have the lastTaskHost and we are not in FAILED_DEPENDENT_ON_PREVIOUS state
                vars.putAll(idata.getVariables());
                List<String> adminHosts = new ArrayList<String>();
                List<String> submitHosts = new ArrayList<String>();
                List<String> allHosts = new ArrayList<String>();
                Host localHost = null;
                for (Host h : initialInstallList) {
                    if (h.isAdminHost() && !adminHosts.contains(h.getHostname())) {
                        adminHosts.add(h.getHostname());
                        if (h.getHostname().equals(Host.localHostName)) {
                            localHost = h;
                        }
                    }
                    if (h.isSubmitHost() && !submitHosts.contains(h.getHostname())) {
                        submitHosts.add(h.getHostname());
                    }
                    //We need to remove all and add only submit hosts, since previous installation could already make some hosts submit hosts. They are now marked as not submit => need to remove
                    if (!allHosts.contains(h.getHostname())) {
                        allHosts.add(h.getHostname());
                    }
                }
                //Add additional admin/submit hosts if any
                if (additionalAdminHosts.size() > 0) {
                    adminHosts.addAll(additionalAdminHosts);
                }
                if (additionalSubmitHosts.size() > 0) {
                    submitHosts.addAll(additionalSubmitHosts);
                }
                //Remove localhost and put as last host
                if (adminHosts.contains(Host.localHostName) && !localHost.isAdminHost()) {
                    adminHosts.remove(Host.localHostName);
                }
                vars.put(VAR_ALL_ADMIN_HOSTS, Util.listToString(adminHosts));
                vars.put(VAR_ALL_HOSTS, Util.listToString(allHosts));
                vars.put(VAR_ALL_SUBMIT_HOSTS, Util.listToString(submitHosts));
                vars.put(VAR_LAST_TASK, "true");
                //And execute the last task in the singleThreadPool
                try {
                    singleThreadPool.execute(new InstallTask(lastTaskHost, this, vars, localizedMessages));
                } catch (RejectedExecutionException e) {
                    setHostState(localHost, State.CANCELED);
                }
                started++;
                //Wait until it's finished
                while (threadPool.getCompletedTaskCount() + singleThreadPool.getCompletedTaskCount() < started && !singleThreadPool.isTerminated()) {
                    try {
                        Thread.sleep(100);
                    } catch (InterruptedException ex) {
                    }
                }
            }
            //If we have failed hosts we go to the Failed tab
            if (((HostInstallTableModel) tables.get(2).getModel()).getRowCount() > 0) {
                tabbedPane.setSelectedIndex(2);
            } else { //Go to succeeded tab
                tabbedPane.setSelectedIndex(1);
            }

            //Let's cleanup
            try {
                threadPool.awaitTermination(100, TimeUnit.MILLISECONDS);
                singleThreadPool.awaitTermination(100, TimeUnit.MILLISECONDS);
                threadPool.purge();
                singleThreadPool.purge();
            } catch (InterruptedException ex) {
            }
            threadPool.shutdown();
            singleThreadPool.shutdown();
        }
    }

    private void disableControls(final boolean b) {
        SwingUtilities.invokeLater(new Runnable() {

            public void run() {
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
        });
    }
    // Variables declaration - do not modify                     
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
    // End of variables declaration                   
    private JTextField lastSelectedTF;
}