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
package com.sun.grid.jgdi.util.shell;

import com.sun.grid.jgdi.configuration.EventClient;
import com.sun.grid.jgdi.JGDIException;
import com.sun.grid.jgdi.JGDIFactory;
import com.sun.grid.jgdi.configuration.ComplexEntry;
import com.sun.grid.jgdi.configuration.ComplexEntryImpl;
import com.sun.grid.jgdi.configuration.ExecHost;
import com.sun.grid.jgdi.configuration.GEObject;
import com.sun.grid.jgdi.configuration.Hostgroup;
import com.sun.grid.jgdi.configuration.JGDIAnswer;
import com.sun.grid.jgdi.configuration.ResourceQuotaSet;
import com.sun.grid.jgdi.configuration.ShareTree;
import com.sun.grid.jgdi.configuration.ShareTreeImpl;
import com.sun.grid.jgdi.configuration.UserSet;
import com.sun.grid.jgdi.util.OutputTable;
import com.sun.grid.jgdi.util.shell.editor.EditorParser;
import com.sun.grid.jgdi.util.shell.editor.GEObjectEditor;
import java.beans.IntrospectionException;
import java.io.IOException;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.LinkedList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import static com.sun.grid.jgdi.util.JGDIShell.getResourceString;
import static com.sun.grid.jgdi.util.shell.OptionAnnotation.MAX_ARG_VALUE;

/**
 *
 */
@CommandAnnotation(value = "qconf")
public class QConfCommand extends QConfCommandGenerated {

    public String getUsage() {
        return JGDIFactory.getJGDIVersion() + "\n" + getResourceString("usage.qconf");
    }

    public void run(String[] args) throws Exception {
        if (args.length == 0) {
            throw new IllegalArgumentException("Invalid number of arguments");
        }
        parseAndInvokeOptions(args);
    }

    //-help
    @OptionAnnotation(value = "-help", min = 0)
    public void printUsage(final OptionInfo oi) throws JGDIException {
        pw.println(getUsage());
    }

    String getTextFromFile(final List<String> args) {
        if (args.size() <= 0) {
            pw.println("no file argument given");
            return null;
        }
        if (args.size() != 1) {
            pw.println("error: invalid option argument \"" + args.get(1) + "\"");
            pw.println("Usage: qconf -help");
            return null;
        }
        String fileName = args.get(0);
        String inputText = null;
        try {
            inputText = readFile(fileName);
        } catch (IOException ex) {
            pw.println("error: error opening file \"" + fileName + "\" for reading: " + ex.getMessage());
            pw.println("error reading in file");
        }
        return inputText;
    }

    //SUBMITHOST
    @OptionAnnotation(value = "-as", extra = MAX_ARG_VALUE)
    public void addSubmitHost(final OptionInfo oi) throws JGDIException {
        final String hostName = oi.getFirstArg();
        List<JGDIAnswer> answers = new LinkedList<JGDIAnswer>();
        jgdi.addSubmitHostWithAnswer(hostName, answers);
        printAnswers(answers);
    }

    @OptionAnnotation(value = "-ds", extra = MAX_ARG_VALUE)
    public void deleteSubmitHost(final OptionInfo oi) throws JGDIException {
        List<JGDIAnswer> answers = new LinkedList<JGDIAnswer>();
        int size = oi.getArgs().size();
        final String[] hosts = oi.getArgs().toArray(new String[size]);
        oi.optionDone();
        jgdi.deleteSubmitHostsWithAnswer(hosts, answers);
        printAnswers(answers);
    }

    //ADMINHOST
    @OptionAnnotation(value = "-ah", extra = MAX_ARG_VALUE)
    public void addAdminHost(final OptionInfo oi) throws JGDIException {
        List<JGDIAnswer> answers = new LinkedList<JGDIAnswer>();
        String hostName = oi.getFirstArg();
        jgdi.addAdminHostWithAnswer(hostName, answers);
        printAnswers(answers);
    }

    @OptionAnnotation(value = "-dh", extra = MAX_ARG_VALUE)
    public void deleteAdminHost(final OptionInfo oi) throws JGDIException {
        List<JGDIAnswer> answers = new LinkedList<JGDIAnswer>();
        int size = oi.getArgs().size();
        final String[] hosts = oi.getArgs().toArray(new String[size]);
        oi.optionDone();
        jgdi.deleteAdminHostsWithAnswer(hosts, answers);
        printAnswers(answers);
    }

    //SHARETREE
    @OptionAnnotation(value = "-astree", min = 0)
    public void addShareTree(final OptionInfo oi) throws JGDIException {
        ShareTree stree = null;
        try {
            stree = jgdi.getShareTree("Root");
        } catch (JGDIException je) {
            // ignore no sharetree defined and fill in a template element instead
        }
        if (stree == null) {
            stree = new ShareTreeImpl("template");
        }
        String treeAsText = showShareTree(stree, "Root", true);
        String userTypedText = runEditor(treeAsText);
        if (userTypedText != null) {
            List<JGDIAnswer> answers = new LinkedList<JGDIAnswer>();
            ShareTreeImpl node = new ShareTreeImpl();
            node = (ShareTreeImpl) GEObjectEditor.updateObjectWithText(jgdi, node, userTypedText);
            // pw.println(node.dump());
            jgdi.updateShareTreeWithAnswer(node, answers);
            printAnswers(answers);
        }
        oi.optionDone();
    }

    @OptionAnnotation(value = "-Astree")
    public void addShareTreeFromFile(final OptionInfo oi) throws JGDIException {
        String inputText = getTextFromFile(oi.getArgs());
        if (inputText == null) {
            return;
        }
        pw.println("NOT IMPLEMENTED"); //TODO LP: Implement
    }

    // -mstree same as -astree
    @OptionAnnotation(value = "-mstree", min = 0)
    public void modifyShareTree(final OptionInfo oi) throws JGDIException {
        addShareTree(oi);
    }

    // -Mstree same as -Astree
    @OptionAnnotation(value = "-Mstree")
    public void modifyShareTreeFromFile(final OptionInfo oi) throws JGDIException {
        addShareTreeFromFile(oi);
    }

    @OptionAnnotation(value = "-sstree", min = 0)
    public void showShareTree(final OptionInfo oi) throws JGDIException {
        ShareTree stree = jgdi.getShareTree("Root");
        pw.println(showShareTree(stree, "Root", true));
    }

    @OptionAnnotation(value = "-dstree", min = 0)
    public void deleteShareTree(final OptionInfo oi) throws JGDIException {
        ShareTree empty = new ShareTreeImpl(true);
        List<JGDIAnswer> answers = new LinkedList<JGDIAnswer>();
        jgdi.deleteShareTreeWithAnswer(answers);
        printAnswers(answers);
    }

    /**
     * Show sharetree node
     * @param oi
     * @return
     * @throws com.sun.grid.jgdi.JGDIException
     */
    String showShareTreeNode(final OptionInfo oi) throws JGDIException {
        final String name = oi.getFirstArg();
        ShareTree stree = jgdi.getShareTree(name);
        return showShareTree(stree, name, false);
    }

    /*
     * Show sharetree node
     */
    private static String showShareTree(ShareTree node, final String name, final boolean showTree) throws JGDIException {
        StringBuilder sb = new StringBuilder();
        List<ShareTree> sharetree = new LinkedList<ShareTree>();
        List<ShareTree> childList;
        String childStr;
        String stName;
        ShareTree tempTree;
        sharetree.add(node);
        while (!sharetree.isEmpty()) {
            node = sharetree.remove(0);
            //Add children to queue
            childList = new LinkedList<ShareTree>();
            childList.addAll(node.getChildrenList());
            //Sort the list by ID
            Collections.sort(childList, new Comparator<ShareTree>() {

                public int compare(ShareTree a, ShareTree b) {
                    int na = a.getId();
                    int nb = b.getId();
                    return (na >= nb) ? ((na == nb) ? 0 : 1) : -1;
                }
            });
            childStr = "";
            for (ShareTree tree : childList) {
                sharetree.add(tree);
                childStr += tree.getId() + ",";
            }
            //For show node
            if (!showTree) {
                stName = node.getName();
                stName = stName.equals("Root") ? "" : stName;
                sb.append("/" + stName + "=" + node.getShares() + "\n");
                //For show tree
            } else {
                sb.append("id=" + node.getId() + "\n");
                sb.append("name=" + node.getName() + "\n");
                sb.append("type=" + node.getType() + "\n");
                sb.append("shares=" + node.getShares() + "\n");
                childStr = (childStr.length() > 0) ? childStr.substring(0, childStr.length() - 1) : "NONE";
                sb.append("childnodes=" + childStr + "\n");
            }
        }
        return sb.toString();
    }

    //-tsm
    @OptionAnnotation(value = "-tsm", min = 0)
    public void triggerSchedulerMonitoring(OptionInfo oi) throws JGDIException {
        List<JGDIAnswer> answers = new LinkedList<JGDIAnswer>();
        jgdi.triggerSchedulerMonitoringWithAnswer(answers);
        printAnswers(answers);
    }

    //-clearusage
    @OptionAnnotation(value = "-clearusage", min = 0)
    public void clearUsage(final OptionInfo oi) throws JGDIException {
        List<JGDIAnswer> answers = new LinkedList<JGDIAnswer>();
        jgdi.clearShareTreeUsageWithAnswer(answers);
        //TODO LP: Bug - got no answers
        printAnswers(answers);
    }

    //-cq
    @OptionAnnotation(value = "-cq", extra = MAX_ARG_VALUE)
    public void cleanQueue(final OptionInfo oi) throws JGDIException {
        final List<String> args = oi.getArgs();
        final String[] queues = args.toArray(new String[oi.getArgs().size()]);
        List<JGDIAnswer> answers = new LinkedList<JGDIAnswer>();
        jgdi.cleanQueuesWithAnswer(queues, answers);
        printAnswers(answers);
        oi.optionDone();
    }

    //-kec
    @OptionAnnotation(value = "-kec", extra = MAX_ARG_VALUE)
    public void killEventClient(final OptionInfo oi) throws JGDIException {
        final List<String> args = oi.getArgs();
        List<JGDIAnswer> answers = new LinkedList<JGDIAnswer>();
        int[] ids = new int[args.size()];
        int i = 0;
        for (String arg : args) {
            if (arg.equals("all")) {
                jgdi.killAllEventClientsWithAnswer(answers);
                printAnswers(answers);
                return;
            } else {
                try {
                    ids[i] = Integer.parseInt(arg);
                } catch (NumberFormatException nfe) {
                    throw new IllegalArgumentException(arg + " is not a valid event client id");
                }
            }
            i++;
        }
        jgdi.killEventClientsWithAnswer(ids, answers);
        printAnswers(answers);
        oi.optionDone();
    }

    //-km
    @OptionAnnotation(value = "-km", min = 0)
    public void killMaster(final OptionInfo oi) throws JGDIException {
        List<JGDIAnswer> answers = new LinkedList<JGDIAnswer>();
        jgdi.killMasterWithAnswer(answers);
        printAnswers(answers);
    }

    //-ks
    @OptionAnnotation(value = "-ks", min = 0)
    public void killScheduler(final OptionInfo oi) throws JGDIException {
        List<JGDIAnswer> answers = new LinkedList<JGDIAnswer>();
        jgdi.killSchedulerWithAnswer(answers);
        printAnswers(answers);
    }

    //-sds
    @OptionAnnotation(value = "-sds", min = 0)
    public void showDetachedSettings(final OptionInfo oi) throws JGDIException {
        String sds;
        sds = jgdi.showDetachedSettingsAll();
        pw.println(sds);
        oi.optionDone();
    }

    //-secl
    @OptionAnnotation(value = "-secl", min = 0)
    public void showEventClientList(final OptionInfo oi) throws JGDIException {
        List<EventClient> evcl = jgdi.getEventClientList();
        if (!evcl.isEmpty()) {
            OutputTable table = new OutputTable(com.sun.grid.jgdi.configuration.EventClient.class);
            try {
                table.addCol("id", "ID", 8, OutputTable.Column.RIGHT);
                table.addCol("name", "NAME", 15, OutputTable.Column.LEFT);
                table.addCol("host", "HOST", 24, OutputTable.Column.LEFT);
            } catch (IntrospectionException ex) {
                ex.printStackTrace();
            }
            table.printHeader(pw);
            pw.write("--------------------------------------------------\n");
            for (EventClient evc : evcl) {
                table.printRow(pw, (Object) evc);
            }
        } else {
            pw.println(getErrorMessage("NoObjectFound", oi.getOd().getOption(), "no event clients registered"));
        }
    }

    //-sep
    @OptionAnnotation(value = "-sep", min = 0)
    @SuppressWarnings(value = "unchecked")
    public void showProcessors(final OptionInfo oi) throws JGDIException {
        List<ExecHost> hosts;
        hosts = jgdi.getExecHostList();
        String name;
        String arch;
        int cpu;
        int totalCpu = 0;
        pw.println("HOST                      PROCESSOR        ARCH");
        pw.println("===============================================");
        //TODO LP: How about we sort the hosts by name first?
        for (ExecHost host : hosts) {
            name = host.getName();
            if (name.equals("global") || name.equals("template")) {
                continue;
            }
            cpu = host.getProcessors();
            totalCpu += cpu;
            if (host.isSetLoad()) {
                arch = host.getLoad("arch");
            } else {
                arch = "";
            }
            pw.printf("%-25.25s %9d %11s%n", name, cpu, arch);
        }
        pw.println("===============================================");
        pw.printf("%-25.25s %9d%n", "SUM", totalCpu);
    }

    //-sss
    @OptionAnnotation(value = "-sss", min = 0)
    public void showSchedulerState(final OptionInfo oi) throws JGDIException {
        pw.println(jgdi.getSchedulerHost());
    }

    //-ke
    @OptionAnnotation(value = "-ke", extra = MAX_ARG_VALUE)
    public void killExecd(final OptionInfo oi) throws JGDIException {
        final List<String> args = oi.getArgs();
        killExecd(false, args);
    }

    //-kej
    @OptionAnnotation(value = "-kej", extra = MAX_ARG_VALUE)
    public void killExecdWithJobs(final OptionInfo oi) throws JGDIException {
        final List<String> args = oi.getArgs();
        killExecd(true, args);
    }

    private void killExecd(final boolean terminateJobs, final List<String> args) throws JGDIException {
        List<JGDIAnswer> answers = new LinkedList<JGDIAnswer>();
        for (String host : args) {
            if (host.equals("all")) {
                jgdi.killAllExecdsWithAnswer(terminateJobs, answers);
                printAnswers(answers);
                return;
            }
        }
        jgdi.killExecdWithAnswer(args.toArray(new String[args.size()]), terminateJobs, answers);
        printAnswers(answers);
    }

    //HOSTGROUP
    //-shgrp_tree
    @OptionAnnotation(value = "-shgrp_tree")
    public void showHostgroupTree(final OptionInfo oi) throws JGDIException {
        final List<String> args = oi.getArgs();
        List<JGDIAnswer> answer = new LinkedList<JGDIAnswer>();
        int i = 0;
        while (i < args.size()) {
            //Prepare the hgroup
            String hgroup = args.get(i++);
            int level = 0;
            Hostgroup obj = null;
            try {
                obj = jgdi.getHostgroupWithAnswer(hgroup, answer);
                printAnswers(answer);
            } catch (JGDIException ex) {
                pw.println(ex.getMessage()); //TODO LP: Check if the message is a correct one
            }
            //Print the tree
            if (obj != null) {
                showHostgroupTree(obj, "", "   ");
            } else {
                pw.println(getErrorMessage("InvalidObjectArgument", oi.getOd().getOption(), "", "No such object found"));
            }
        }
    }

    //-shgrp_resolved
    @OptionAnnotation(value = "-shgrp_resolved")
    @SuppressWarnings(value = "unchecked")
    public void showHostgroupResolved(final OptionInfo oi) throws JGDIException {
        final List<String> args = oi.getArgs();
        List<JGDIAnswer> answer = new LinkedList<JGDIAnswer>();
        int i = 0;
        while (i < args.size()) {
            //Prepare the hgroup
            String hgroup = args.get(i++);
            int level = 0;
            Hostgroup obj = null;
            try {
                obj = jgdi.getHostgroupWithAnswer(hgroup, answer);
                printAnswers(answer);
            } catch (JGDIException ex) {
                pw.println(ex.getMessage()); //TODO LP: Check if the message is a correct one
            }
            //Print the tree
            if (obj != null) {
                List<String> hnames = new LinkedList<String>();
                printHostgroupResolved(hnames, obj);
                String out = "";
                for (String hname : hnames) {
                    out += hname + " ";
                }
                pw.println(out.substring(0, out.length() - 1));
            } else {
                pw.println(getErrorMessage("InvalidObjectArgument", oi.getOd().getOption(), "", "No object found"));
            }
        }
    }

    //TODO LP: Remove recursion in shgrp_tree
    private void showHostgroupTree(Hostgroup obj, String prefix, final String tab) {
        pw.println(prefix + obj.getName());
        prefix += tab;

        for (String hgroup : (List<String>) obj.getHostList()) {
            //Another hroup
            if (hgroup.startsWith("@")) {
                try {
                    obj = jgdi.getHostgroup(hgroup);
                } catch (JGDIException ex) {
                    pw.println(ex.getMessage());
                }
                showHostgroupTree(obj, prefix, tab);
            } else {
                pw.println(prefix + hgroup);
            }
        }
    }

    //TODO: Clients use postorder, better to use sort?
    private void printHostgroupResolved(List<String> result, Hostgroup obj) {
        LinkedList<Hostgroup> queue = new LinkedList<Hostgroup>();
        queue.add(obj);
        while (!queue.isEmpty()) {
            obj = queue.remove(0);
            for (String hgroup : obj.getHostList()) {
                //Another hroup
                if (hgroup.startsWith("@")) {
                    try {
                        obj = jgdi.getHostgroup(hgroup);
                        queue.add(obj);
                    } catch (JGDIException ex) {
                        pw.println(ex.getMessage());
                    }
                } else {
                    if (!result.contains(hgroup)) {
                        result.add(hgroup);
                    }
                }
            }
        }
        //Collections.sort(result);
    }

    //COMPLEXENTRY
    //-mc
    @OptionAnnotation(value = "-mc", min = 0)
    public void modifyComplexEntry(final OptionInfo oi) throws JGDIException {
        String text = runEditor(showComplexes());
        modifyComplexes(text);
    }

    //-Mc
    @OptionAnnotation(value = "-Mc")
    public void modifyComplexEntryFromFile(final OptionInfo oi) throws JGDIException {
        final List<String> args = oi.getArgs();
        String inputText = getTextFromFile(args);
        if (inputText == null) {
            return;
        }
        modifyComplexes(inputText);
    }

    //-sc
    @OptionAnnotation(value = "-sc", min = 0)
    public void showComplexEntry(final OptionInfo oi) throws JGDIException {
        pw.print(showComplexes());
    }

    /**
     * Updates the complex entry list based on the text
     * @param text
     */
    @SuppressWarnings(value = "unchecked")
    private void modifyComplexes(final String text) throws JGDIException {
        //Now parse lines and fields ignore whitespaces join lines on \
        List<List<String>> lineList = EditorParser.tokenizeToList(text);
        Map<String, ComplexEntry> newCEMap = new HashMap<String, ComplexEntry>();
        String elem;
        int val;
        for (List<String> singleLine : lineList) {
            elem = singleLine.get(0);

            //We check if the line starts with # and skip it
            if (elem.startsWith("#")) {
                continue;
            }
            if (singleLine.size() != 8) {
                throw new IllegalArgumentException("Expected 8 elements: name, shortcut, type, relop, requestable, consumable, default, urgency");
            }
            ComplexEntry ce = new ComplexEntryImpl(true);
            ce.setName(elem);
            ce.setShortcut(singleLine.get(1));

            val = ce.typeToInt(singleLine.get(2));
            //TODO LP: deny invalid
            ce.setValtype(val);

            val = ce.opToInt(singleLine.get(3));
            //TODO LP: deny invalid
            ce.setRelop(val);

            val = ce.reqToInt(singleLine.get(4));
            //TODO LP: deny invalid
            ce.setRequestable(val);

            ce.setConsumable(Util.isYes(singleLine.get(5)));

            ce.setDefault(singleLine.get(6));

            //val = Integer.parseInt((String) singleLine.get(7));
            ce.setUrgencyWeight(singleLine.get(7));

            newCEMap.put(elem, ce);
        }
        //TODO LP: Better to have jgdi.replaceCEList(List newList) and make the checks on qmaster side
        List<ComplexEntry> toModify = new LinkedList<ComplexEntry>();
        List<ComplexEntry> toDelete = new LinkedList<ComplexEntry>();
        List<ComplexEntry> origList = jgdi.getComplexEntryList();

        String name;
        String shortCut;
        for (ComplexEntry ce : origList) {
            name = ce.getName();
            //Check if existing value is modified
            if (newCEMap.containsKey(name)) {
                if (!ce.equalsCompletely(newCEMap.get(name))) {
                    toModify.add(newCEMap.get(name));
                }
                newCEMap.remove(name);
            } else {
                toDelete.add(ce);
            }
        }
        //Add new complexes
        for (String key : newCEMap.keySet()) {
            ComplexEntry ce = newCEMap.get(key);
            try {
                jgdi.addComplexEntry(ce);
            } catch (JGDIException ex) {
                pw.println(ex.getMessage());
            }
        }
        //Modify existing
        for (ComplexEntry ce : toModify) {
            try {
                jgdi.updateComplexEntry(ce);
            } catch (JGDIException ex) {
                pw.println(ex.getMessage());
            }
        }
        //Remove not defined anymore
        for (ComplexEntry ce : toDelete) {
            try {
                jgdi.deleteComplexEntry(ce);
            } catch (JGDIException ex) {
                pw.println(ex.getMessage());
            }
        }
    }

    private String showComplexes() {
        StringBuilder sb = new StringBuilder();
        List<ComplexEntry> cList = null;
        try {
            cList = jgdi.getComplexEntryList();
        } catch (JGDIException ex) {
            return ex.getMessage();
        }
        //Format is:nameLen+3 shortcutLen+3 typeLen+3 relopLen+3 requestableLen+8 consumableLen+7 defaultLen+3 urgencyLen+4
        int nameLen = 0;
        int shortcutLen = 0;
        int typeLen = 0;
        int relopLen = 2;
        int requestableLen = 0;
        int consumableLen = 0;
        int defaultLen = 0;
        int urgencyLen = 0;
        //Sort the list alphabetically
        List<ComplexEntry> complexList = sortListByName(cList);
        ComplexEntry ceDummy = new ComplexEntryImpl(true);
        //Need to first get the maximum column lengths
        for (ComplexEntry complex : complexList) {
            nameLen = Math.max(nameLen, complex.getName().length());
            shortcutLen = Math.max(shortcutLen, complex.getShortcut().length());
            typeLen = Math.max(typeLen, ceDummy.typeToString(complex.getValtype()).length());
            requestableLen = Math.max(requestableLen, (complex.getRequestable() == 1) ? 2 : 3); //length of YES, NO
            consumableLen = Math.max(consumableLen, (complex.isConsumable()) ? 3 : 2); //length of YES, NO);
            defaultLen = Math.max(defaultLen, complex.getDefault().length());
            urgencyLen = Math.max(urgencyLen, complex.getUrgencyWeight().length());
        }
        //Now format the columns
        String header0 = String.format("%-19.19s %-10.10s %-11.11s %-5.5s %-11.11s %-10.10s %-8.8s %-8s", "#name", "shortcut", "type", "relop", "requestable", "consumable", "default", "urgency");
        StringBuilder header1 = new StringBuilder("#");
        for (int j = 0; j < header0.length() - 1; j++) {
            header1.append("-");
        }
        sb.append(header0 + "\n");
        sb.append(header1 + "\n");
        String val;
        //And finally print the columns
        for (ComplexEntry complex : complexList) {
            val = String.format("%-19.19s %-10.10s %-11.11s %-5.5s %-11.11s %-10.10s %-8.8s %-8s", complex.getName(), complex.getShortcut(), ceDummy.typeToString(complex.getValtype()), ceDummy.opToString(complex.getRelop()), ceDummy.reqToString(complex.getRequestable()), (complex.isConsumable()) ? "YES" : "NO", complex.getDefault(), complex.getUrgencyWeight());
            sb.append(val + "\n");
        }
        sb.append("# >#< starts a comment but comments are not saved across edits --------\n");
        return sb.toString();
    }

    @SuppressWarnings(value = "unchecked")
    private static List sortListByName(final List list) {
        Collections.sort(list, new Comparator<GEObject>() {

            public int compare(GEObject o1, GEObject o2) {
                if (o1 == null && o2 == null) {
                    return 0;
                }
                if (o1 == null) {
                    return -1;
                }
                if (o2 == null) {
                    return 1;
                }
                return o1.getName().compareTo(o2.getName());
            }
        });
        return list;
    }

    //MANAGER
    @OptionAnnotation(value = "-am", extra = MAX_ARG_VALUE)
    public void addManager(final OptionInfo oi) throws JGDIException {
        final String name = oi.getFirstArg();
        List<JGDIAnswer> answer = new LinkedList<JGDIAnswer>();
        jgdi.addManagerWithAnswer(name, answer);
        printAnswers(answer);
    }

    @OptionAnnotation(value = "-dm", extra = MAX_ARG_VALUE)
    public void deleteManager(final OptionInfo oi) throws JGDIException {
        List<JGDIAnswer> answers = new LinkedList<JGDIAnswer>();
        String[] a = new String[0];
        int size = oi.getArgs().size();
        final String[] names = oi.getArgs().toArray(new String[size]);
        oi.optionDone();
        jgdi.deleteManagersWithAnswer(names, answers);
        printAnswers(answers);
    }

    //OPERATOR
    @OptionAnnotation(value = "-ao", extra = MAX_ARG_VALUE)
    public void addOperator(final OptionInfo oi) throws JGDIException {
        final String name = oi.getFirstArg();
        List<JGDIAnswer> answer = new LinkedList<JGDIAnswer>();
        jgdi.addOperatorWithAnswer(name, answer);
        printAnswers(answer);
    }

    @OptionAnnotation(value = "-do", extra = MAX_ARG_VALUE)
    public void deleteOperator(final OptionInfo oi) throws JGDIException {
        List<JGDIAnswer> answers = new LinkedList<JGDIAnswer>();
        String[] a = new String[0];
        int size = oi.getArgs().size();
        final String[] names = oi.getArgs().toArray(new String[size]);
        oi.optionDone();
        jgdi.deleteOperatorsWithAnswer(names, answers);
        printAnswers(answers);
    }

    //RESOURCE QUOTA SET
    /**
     *   Implements qconf -mrqs option
     *   @param  oi <b>OptionInfo</b> option enviroment object
     *   @throws JGDIException on any error on the GDI layer
     */
    @OptionAnnotation(value = "-mrqs", min = 0, extra = MAX_ARG_VALUE)
    public void modifyResourceQuotaSet(final OptionInfo oi) throws JGDIException {
        //TODO LP: Needs spceial editor handling, displays more objects at once
        ResourceQuotaSet obj;
        List<ResourceQuotaSet> rqsList = new LinkedList<ResourceQuotaSet>();
        List<JGDIAnswer> answer = new LinkedList<JGDIAnswer>();
        String textToEdit = "";


        if (oi.getArgs().size() == 0) {
            rqsList = jgdi.getResourceQuotaSetListWithAnswer(answer);
            printAnswers(answer);
        } else {
            for (String arg : oi.getArgs()) {
                obj = jgdi.getResourceQuotaSetWithAnswer(arg, answer);
                rqsList.add(obj);
                printAnswers(answer);
                answer.clear();
            }
        }
        //Create text to be displayed in the editor
        for (ResourceQuotaSet rqs : rqsList) {
            textToEdit += GEObjectEditor.getConfigurablePropertiesAsText(rqs);
        }

        String userTypedText = runEditor(textToEdit);
        if (userTypedText != null) {
            //TODO LP: Handle the multiple objects. Need special parser for the userTypedText
            //GEObjectEditor.updateObjectWithText(jgdi, obj, userTypedText);
            //jgdi.updateResourceQuotaSetWithAnswer(obj, answer);
            //printAnswers(answer);
            throw new IllegalStateException("NOT IMPLEMENTED");
        }
        oi.optionDone();
    }

    /**
     *   Implements qconf -srqs option
     *   @param  oi <b>OptionInfo</b> option enviroment object
     *   @throws JGDIException on any error on the GDI layer
     */
    @OptionAnnotation(value = "-srqs", min = 0, extra = MAX_ARG_VALUE)
    public void showResourceQuotaSet(final OptionInfo oi) throws JGDIException {
        ResourceQuotaSet obj;
        List<ResourceQuotaSet> rqsList = new LinkedList<ResourceQuotaSet>();
        List<JGDIAnswer> answer = new LinkedList<JGDIAnswer>();
        String text = "";


        if (oi.getArgs().size() == 0) {
            rqsList = jgdi.getResourceQuotaSetListWithAnswer(answer);
            oi.optionDone();
            printAnswers(answer);
        } else {
            for (String arg : oi.getArgs()) {
                obj = jgdi.getResourceQuotaSetWithAnswer(arg, answer);
                rqsList.add(obj);
                printAnswers(answer);
                answer.clear();
            }
        }
        //Show correct error message if list is empty
        if (rqsList.size() == 0) {
            pw.println(getErrorMessage("NoObjectFound", oi.getOd().getOption(), "", "No object found"));
            return;
        }
        //Create text to be displayed in the editor
        for (ResourceQuotaSet rqs : rqsList) {
            text += GEObjectEditor.getConfigurablePropertiesAsText(rqs);
        }
        pw.print(text);
    }

    //USERSET
    //-au
    @OptionAnnotation(value = "-au", min = 2, extra = MAX_ARG_VALUE)
    public void addUserSet(final OptionInfo oi) throws JGDIException {
        List<JGDIAnswer> answer = new LinkedList<JGDIAnswer>();
        final String userName = oi.getFirstArg();
        String setName = oi.showLastArg();
        UserSet obj = jgdi.getUserSet(setName);
        obj.addEntries(userName);
        jgdi.updateUserSetWithAnswer(obj, answer);
        printAnswers(answer);
    }

    //-du
    @OptionAnnotation(value = "-du", min = 2, extra = MAX_ARG_VALUE)
    public void deleteUserSet(final OptionInfo oi) throws JGDIException {
        List<JGDIAnswer> answer = new LinkedList<JGDIAnswer>();
        final String userName = oi.getFirstArg();
        String setName = oi.showLastArg();
        UserSet obj = jgdi.getUserSet(setName);
        if (obj == null) {
            String msg = getErrorMessage("InvalidObjectArgument", oi.getOd().getOption(), setName, null);
            pw.println(msg);
            return;
        }
        obj.removeEntries(userName);
        jgdi.updateUserSetWithAnswer(obj, answer);
        printAnswers(answer);
    }

    //-dul
    @OptionAnnotation(value = "-dul", min = 1, extra = MAX_ARG_VALUE)
    public void deleteUserSetList(final OptionInfo oi) throws JGDIException {
        List<JGDIAnswer> answer = new LinkedList<JGDIAnswer>();
        final String setName = oi.getFirstArg();
        UserSet obj = jgdi.getUserSet(setName);
        if (obj == null) {
            String msg = getErrorMessage("InvalidObjectArgument", oi.getOd().getOption(), setName, null);
            pw.println(msg);
            return;
        }
        obj.removeAllEntries();
        jgdi.updateUserSetWithAnswer(obj, answer);
        printAnswers(answer);
    }
    //OVERRIDES - special handling to have same behaviour as in clients
    //TODO: Fix the clients remove this special handling

    //CALENDAR
    @OptionAnnotation(value = "-acal", min = 0, extra = 1)
    @Override
    public void addCalendar(final OptionInfo oi) throws JGDIException {
        //Handling client inconsistancy. Does not open editor when no arg given
        if (hasNoArgument(oi)) {
            return;
        }
        //otherwise handle adding object
        super.addCalendar(oi);
    }

    //CHECKPOINT
    @OptionAnnotation(value = "-ackpt", min = 0, extra = 1)
    @Override
    public void addCheckpoint(final OptionInfo oi) throws JGDIException {
        //Handling client inconsistancy. Does not open editor when no arg given
        if (hasNoArgument(oi)) {
            return;
        }
        //otherwise handle adding of the object
        super.addCheckpoint(oi);
    }

    //PARALLEL ENVIRONMENT
    @OptionAnnotation(value = "-sp", min = 1, extra = 0)
    @Override
    public void showParallelEnvironment(final OptionInfo oi) throws JGDIException {
        super.showParallelEnvironment(oi);
    }

    //PROJECT
    @OptionAnnotation(value = "-sprj", min = 1, extra = 0)
    @Override
    public void showProject(final OptionInfo oi) throws JGDIException {
        super.showProject(oi);
    }

    //HOSTGROUP -  additional check if the argument starts with @ character
    @OptionAnnotation(value = "-ahgrp", min = 0, extra = 1)
    @Override
    public void addHostgroup(final OptionInfo oi) throws JGDIException {
        if (!hasValidHostgroupName(oi)) {
            return;
        }
        super.addHostgroup(oi);
    }

    /**
     * Helper method. Checks if option has an argument(s)
     * return hasNoArgument
     */
    private boolean hasNoArgument(OptionInfo oi) {
        String option = oi.getOd().getOption();
        if (oi.getArgs().size() == 0) {
            pw.println(getErrorMessage("NoArgument", option, "No argument provided to option " + option));
            return true;
        }
        return false;
    }

    /**
     * Helper method. Checks if option's next argument is a valid hostgroup name (starting with @).
     * return hasNoArgument
     */
    private boolean hasValidHostgroupName(OptionInfo oi) {
        List<String> args = oi.getArgs();
        OptionDescriptor od = oi.getOd();
        if (args != null && args.size() > 0) {
            String hgrp = args.get(0);
            if (!hgrp.startsWith("@")) {
                String msg = getErrorMessage("InvalidObjectArgument", od.getOption(), hgrp, "Hostgroup name \"" + hgrp + "\" is not valid");
                od.getPw().println(msg);
                return false;
            }
        }
        return true;
    }

    /**
     * Helper method. Checks if option's next argument is a reachable host
     * return isHostReachable
     */
    private boolean isHostReachable(OptionInfo oi) {
        List<String> args = oi.getArgs();
        OptionDescriptor od = oi.getOd();
        if (args != null && args.size() > 0) {
            String host = args.get(0);
            try {
                if (InetAddress.getByName(host) != null) {
                    return true;
                }
            } catch (UnknownHostException ex) {
                String msg = getErrorMessage("UnreachableHost", od.getOption(), host, "Host \"" + host + "\" is not reachable");
                od.getPw().println(msg);
                return false;
            }
        }
        return true;
    }
}