/*___INFO__MARK_BEGIN__*/ /*************************************************************************
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

import com.sun.grid.jgdi.JGDI;
import com.sun.grid.jgdi.JGDIException;
import com.sun.grid.jgdi.configuration.JGDIAnswer;
import com.sun.grid.jgdi.configuration.ExecHost;
import com.sun.grid.jgdi.configuration.GEObject;
import com.sun.grid.jgdi.configuration.ComplexEntry;
import com.sun.grid.jgdi.configuration.Configuration;
import com.sun.grid.jgdi.configuration.SchedConf;
import com.sun.grid.jgdi.configuration.UserSet;
import com.sun.grid.jgdi.configuration.UserSetImpl;
import com.sun.grid.jgdi.util.OutputTable;
import com.sun.grid.jgdi.util.shell.editor.EditorUtil;
import com.sun.grid.jgdi.util.shell.editor.GEObjectEditor;
import com.sun.grid.jgdi.util.shell.editor.TextEditor;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.LineNumberReader;
import java.io.PrintWriter;
import java.io.StringReader;
import java.io.StringWriter;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;

/**
 *
 */
public class QConfCommand extends AbstractCommand {

    /** Creates a new instance of QConfCommand */
    public QConfCommand(Shell shell, String name) {
        super(shell, name);
    }

    public String getUsage() {
        StringWriter sw = new StringWriter();
        PrintWriter pw = new PrintWriter(sw);
        pw.println("GE 6.1");
        pw.println("usage: qconf [options]");
        pw.println("   [-aattr obj_nm attr_nm val obj_id_lst]   add to a list attribute of an object");
        pw.println("   [-Aattr obj_nm fname obj_id_lst]         add to a list attribute of an object");
        pw.println("   [-acal calendar_name]                    add a new calendar");
        pw.println("   [-Acal fname]                            add a new calendar from file");
        pw.println("   [-ackpt ckpt_name]                       add a ckpt interface definition");
        pw.println("   [-Ackpt fname]                           add a ckpt interface definition from file");
        pw.println("   [-aconf host_list]                       add configurations");
        pw.println("   [-Aconf file_list]                       add configurations from file_list");
        pw.println("   [-ae [exec_server_template]]             add an exec host using a template");
        pw.println("   [-Ae fname]                              add an exec host from file");
        pw.println("   [-ah hostname]                           add an administrative host");
        pw.println("   [-ahgrp group]                           add new host group entry");
        pw.println("   [-Ahgrp file]                            add new host group entry from file");
        pw.println("   [-arqs [rqs_list]]                       add resource quota set(s)");
        pw.println("   [-Arqs fname]                            add resource quota set(s) from file");
        pw.println("   [-am user_list]                          add user to manager list");
        pw.println("   [-ao user_list]                          add user to operator list");
        pw.println("   [-ap pe-name]                            add a new parallel environment");
        pw.println("   [-Ap fname]                              add a new parallel environment from file");
        pw.println("   [-aprj]                                  add project");
        pw.println("   [-Aprj fname]                            add project from file");
        pw.println("   [-aq [queue_name]]                       add a new cluster queue");
        pw.println("   [-Aq fname]                              add a queue from file");
        pw.println("   [-as hostname]                           add a submit host");
        pw.println("   [-astnode node_shares_list]              add sharetree node(s)");
        pw.println("   [-astree]                                create/modify the sharetree");
        pw.println("   [-Astree fname]                          create/modify the sharetree from file");
        pw.println("   [-au user_list listname_list]            add user(s) to userset list(s)");
        pw.println("   [-Au fname]                              add userset from file");
        pw.println("   [-auser]                                 add user");
        pw.println("   [-Auser fname]                           add user from file");
        pw.println("   [-clearusage]                            clear all user/project sharetree usage");
        pw.println("   [-cq destin_id_list]                     clean queue");
        pw.println("   [-dattr obj_nm attr_nm val obj_id_lst]   delete from a list attribute of an object");
        pw.println("   [-Dattr obj_nm fname obj_id_lst]         delete from a list attribute of an object");
        pw.println("   [-dcal calendar_name]                    delete calendar");
        pw.println("   [-dckpt ckpt_name]                       delete ckpt interface definition");
        pw.println("   [-dconf host_list]                       delete local configurations");
        pw.println("   [-de host_list]                          delete exec server");
        pw.println("   [-dh host_list]                          delete administrative host");
        pw.println("   [-dhgrp group]                           delete host group entry");
        pw.println("   [-drqs rqs_list]                         delete resource quota set(s)");
        pw.println("   [-dm user_list]                          delete user from manager list");
        pw.println("   [-do user_list]                          delete user from operator list");
        pw.println("   [-dp pe-name]                            delete parallel environment");
        pw.println("   [-dprj project_list]                     delete project");
        pw.println("   [-dq destin_id_list]                     delete queue");
        pw.println("   [-ds host_list]                          delete submit host");
        pw.println("   [-dstnode node_list]                     delete sharetree node(s)");
        pw.println("   [-dstree]                                delete the sharetree");
        pw.println("   [-du user_list listname_list]            delete user(s) from userset list(s)");
        pw.println("   [-dul listname_list]                     delete userset list(s) completely");
        pw.println("   [-duser user_list]                       delete user(s)");
        pw.println("   [-help]                                  print this help");
        pw.println("   [-ke[j] host_list                        shutdown execution daemon(s)");
        pw.println("   [-k{m|s}]                                shutdown master|scheduling daemon");
        pw.println("   [-kec evid_list]                         kill event client");
        pw.println("   [-mattr obj_nm attr_nm val obj_id_lst]   modify an attribute (or element in a sublist) of an object");
        pw.println("   [-Mattr obj_nm fname obj_id_lst]         modify an attribute (or element in a sublist) of an object");
        pw.println("   [-mc ]                                   modify complex attributes");
        pw.println("   [-mckpt ckpt_name]                       modify a ckpt interface definition");
        pw.println("   [-Mc fname]                              modify complex attributes from file");
        pw.println("   [-mcal calendar_name]                    modify calendar");
        pw.println("   [-Mcal fname]                            modify calendar from file");
        pw.println("   [-Mckpt fname]                           modify a ckpt interface definition from file");
        pw.println("   [-mconf [host_list|global]]              modify configurations");
        pw.println("   [-msconf]                                modify scheduler configuration");
        pw.println("   [-Msconf fname]                          modify scheduler configuration from file");
        pw.println("   [-me server]                             modify exec server");
        pw.println("   [-Me fname]                              modify exec server from file");
        pw.println("   [-mhgrp group]                           modify host group entry");
        pw.println("   [-Mhgrp file]                            modify host group entry from file");
        pw.println("   [-mrqs [rqs_list]]                       modify resource quota set(s)");
        pw.println("   [-Mrqs fname [rqs_list]]                 modify resource quota set(s) from file");
        pw.println("   [-mp pe-name]                            modify a parallel environment");
        pw.println("   [-Mp fname]                              modify a parallel environment from file");
        pw.println("   [-mprj project]                          modify a project");
        pw.println("   [-Mprj fname]                            modify project from file");
        pw.println("   [-mq queue]                              modify a queue");
        pw.println("   [-Mq fname]                              modify a queue from file");
        pw.println("   [-mstnode node_shares_list]              modify sharetree node(s)");
        pw.println("   [-Mstree fname]                          modify/create the sharetree from file");
        pw.println("   [-mstree]                                modify/create the sharetree");
        pw.println("   [-mu listname_list]                      modify the given userset list");
        pw.println("   [-Mu fname]                              modify userset from file");
        pw.println("   [-muser user]                            modify a user");
        pw.println("   [-Muser fname]                           modify a user from file");
        pw.println("   [-purge obj_nm3 attr_nm objectname]      deletes attribute from object_instance");
        pw.println("   [-rattr obj_nm attr_nm val obj_id_lst]   replace a list attribute of an object");
        pw.println("   [-Rattr obj_nm fname obj_id_lst]         replace a list attribute of an object");
        pw.println("   [-sc]                                    show complex attributes");
        pw.println("   [-scal calendar_name]                    show given calendar");
        pw.println("   [-scall]                                 show a list of all calendar names");
        pw.println("   [-sckpt ckpt_name]                       show ckpt interface definition");
        pw.println("   [-sckptl]                                show all ckpt interface definitions");
        pw.println("   [-sconf [host_list|global]]              show configurations");
        pw.println("   [-sconfl]                                show a list of all local configurations");
        pw.println("   [-se server]                             show given exec server");
        pw.println("   [-secl]                                  show event client list");
        pw.println("   [-sel]                                   show a list of all exec servers");
        pw.println("   [-sep]                                   show a list of all licensed processors");
        pw.println("   [-sh]                                    show a list of all administrative hosts");
        pw.println("   [-shgrp group]                           show host group");
        pw.println("   [-shgrp_tree group]                      show host group and used hostgroups as tree");
        pw.println("   [-shgrp_resolved group]                  show host group with resolved hostlist");
        pw.println("   [-shgrpl]                                show host group list");
        pw.println("   [-sds]                                   show detached settings");
        pw.println("   [-srqs [rqs_list]]                       show resource quota set(s)");
        pw.println("   [-srqsl]                                 show resource quota set list");
        pw.println("   [-sm]                                    show a list of all managers");
        pw.println("   [-so]                                    show a list of all operators");
        pw.println("   [-sobjl obj_nm2 attr_nm val]             show objects which match the given value");
        pw.println("   [-sp pe-name]                            show a parallel environment");
        pw.println("   [-spl]                                   show all parallel environments");
        pw.println("   [-sprj project]                          show a project");
        pw.println("   [-sprjl]                                 show a list of all projects");
        pw.println("   [-sq [destin_id_list]]                   show the given queue");
        pw.println("   [-sql]                                   show a list of all queues");
        pw.println("   [-ss]                                    show a list of all submit hosts");
        pw.println("   [-sss]                                   show scheduler state");
        pw.println("   [-ssconf]                                show scheduler configuration");
        pw.println("   [-sstnode node_list]                     show sharetree node(s)");
        pw.println("   [-rsstnode node_list]                    show sharetree node(s) and its children");
        pw.println("   [-sstree]                                show the sharetree");
        pw.println("   [-su listname_list]                      show the given userset list");
        pw.println("   [-suser user_list]                       show user(s)");
        pw.println("   [-sul]                                   show a list of all userset lists");
        pw.println("   [-suserl]                                show a list of all users");
        pw.println("   [-tsm]                                   trigger scheduler monitoring");
        pw.println();
        pw.println("complex_list            complex[,complex,...]");
        pw.println("destin_id_list          queue[,queue,...]");
        pw.println("listname_list           listname[,listname,...]");
        pw.println("rqs_list                rqs_name[,rqs_name,...]");
        pw.println("node_list               node_path[,node_path,...]");
        pw.println("node_path               [/]node_name[[/.]node_name...]");
        pw.println("node_shares_list        node_path=shares[,node_path=shares,...]");
        pw.println("user_list               user[,user,...]");
        pw.println("obj_nm                  \"queue\"|\"exechost\"|\"pe\"|\"ckpt\"|\"hostgroup\"|\"resource_quota\"");
        pw.println("attr_nm                 (see man pages)");
        pw.println("obj_id_lst              objectname [ objectname ...]");
        pw.println("project_list            project[,project,...]");
        pw.println("evid_list               all | evid[,evid,...]");
        pw.println("host_list               all | hostname[,hostname,...]");
        pw.println("obj_nm2                 \"queue\"|\"queue_domain\"|\"queue_instance\"|\"exechost\"");
        pw.println("obj_nm3                 \"queue\"");
        return sw.getBuffer().toString();
    }

    private static void printAnswers(PrintWriter pw, List answers) {
        Iterator ai = answers.iterator();
        while (ai.hasNext()) {
            JGDIAnswer an = (JGDIAnswer) ai.next();
            pw.println(an.getText());
        }
        pw.flush();
    }

    public void run(String[] args) throws Exception {

        JGDI jgdi = getShell().getConnection();

        if (jgdi == null) {
            throw new IllegalStateException("Not connected");
        }
        if (args.length == 0) {
            throw new IllegalArgumentException("Invalid number of arguments");
        }

        PrintWriter pw = new PrintWriter(System.out);

        boolean force = false;

        for (int i = 0; i < args.length; i++) {
            if (args[i].equals("-tsm")) {
                List answers = new LinkedList();
                jgdi.triggerSchedulerMonitoringWithAnswer(answers);
                printAnswers(pw, answers);
                break;
            } else if (args[i].equals("-clearusage")) {
                List answers = new LinkedList();
                jgdi.clearShareTreeUsageWithAnswer(answers);
                printAnswers(pw, answers);
                break;
            } else if (args[i].equals("-cq")) {
                i++;
                if (i >= args.length) {
                    throw new IllegalArgumentException("missing destin_id_list");
                }
                String[] queues = parseDestinIdList(args[i]);
                List answers = new LinkedList();
                jgdi.cleanQueuesWithAnswer(queues, answers);
                printAnswers(pw, answers);
                break;
            } else if (args[i].equals("-kec")) {
                /*
                 * Attention !!
                 * -kec must be handled before -ke[j]
                 */
                i++;
                if (i >= args.length) {
                    pw.println("error: no option argument provided to \"-kec\"");
                    pw.flush();
                }
                List answers = new LinkedList();
                if (args[i].equals("all")) {
                    jgdi.killAllEventClientsWithAnswer(answers);
                } else {
                    String[] idStr = args[i].split(" ");
                    int[] ids = new int[idStr.length];

                    for (int idIndex = 0; idIndex < idStr.length; idIndex++) {
                        try {
                            ids[idIndex] = Integer.parseInt(idStr[idIndex]);
                        } catch (NumberFormatException nfe) {
                            throw new IllegalArgumentException(idStr[idIndex] + "missing destin_id_list");
                        }
                    }
                    jgdi.killEventClientsWithAnswer(ids, answers);
                }
                printAnswers(pw, answers);
                break;
            } else if (args[i].startsWith("-ke")) {
                boolean terminateJobs = args[i].endsWith("j");
                i++;
                if (i >= args.length) {
                    // pw.println("MSG_HOST_NEEDAHOSTNAMEORALL");
                    pw.println("error: no option argument provided to \"-ke\"");
                    pw.flush();
                }
                List answers = new LinkedList();
                if (args[i].equals("all")) {
                    jgdi.killAllExecdsWithAnswer(terminateJobs, answers);
                } else {
                    String[] hosts = args[i].split(" ");
                    jgdi.killExecdWithAnswer(hosts, terminateJobs, answers);
                }
                printAnswers(pw, answers);
                break;
            } else if (args[i].equals("-km")) {
                List answers = new LinkedList();
                jgdi.killMasterWithAnswer(answers);
                printAnswers(pw, answers);
                break;
            } else if (args[i].equals("-ks")) {
                List answers = new LinkedList();
                jgdi.killSchedulerWithAnswer(answers);
                printAnswers(pw, answers);
                break;
                //CALENDAR
            } else if (args[i].equals("-acal")) {
                addNewGEObject(jgdi, "Calendar", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-Acal")) {
                addNewGEObjectFromFile(jgdi, "Calendar", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-mcal")) {
                modifyGEObject(jgdi, "Calendar", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-Mcal")) {
                modifyGEObjectFromFile(jgdi, "Calendar", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-scall")) {
                showGEObjectList(jgdi, "Calendar", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-scal")) {
                showGEObject(jgdi, "Calendar", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-dcal")) {
                deleteGEObject(jgdi, "Calendar", Arrays.asList(args).subList(i, args.length), pw);
                break;
                //CHECKPOINT
            } else if (args[i].equals("-ackpt")) {
                addNewGEObject(jgdi, "Checkpoint", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-Ackpt")) {
                addNewGEObjectFromFile(jgdi, "Checkpoint", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-mckpt")) {
                modifyGEObject(jgdi, "Checkpoint", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-Mckpt")) {
                modifyGEObjectFromFile(jgdi, "Checkpoint", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-sckptl")) {
                showGEObjectList(jgdi, "Checkpoint", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-sckpt")) {
                showGEObject(jgdi, "Checkpoint", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-dckpt")) {
                deleteGEObject(jgdi, "Checkpoint", Arrays.asList(args).subList(i, args.length), pw);
                break;
                //CONFIGURATION - no Mconf
            } else if (args[i].equals("-aconf")) {
                addNewGEObject(jgdi, "Configuration", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-Aconf")) {
                addNewGEObjectFromFile(jgdi, "Configuration", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-mconf")) {
                modifyGEObject(jgdi, "Configuration", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-sconfl")) {
                showGEObjectList(jgdi, "Configuration", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-sconf")) {
                showGEObject(jgdi, "Configuration", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-dconf")) {
                deleteGEObject(jgdi, "Configuration", Arrays.asList(args).subList(i, args.length), pw);
                break;
                //EXEC HOST
            } else if (args[i].equals("-ae")) {
                addNewGEObject(jgdi, "ExecHost", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-Ae")) {
                addNewGEObjectFromFile(jgdi, "ExecHost", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-me")) {
                modifyGEObject(jgdi, "ExecHost", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-Me")) {
                modifyGEObjectFromFile(jgdi, "ExecHost", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-sel")) {
                showGEObjectList(jgdi, "ExecHost", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-se")) {
                showGEObject(jgdi, "ExecHost", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-de")) {
                deleteGEObject(jgdi, "ExecHost", Arrays.asList(args).subList(i, args.length), pw);
                break;
                //HOST GROUP
            } else if (args[i].equals("-ahgrp")) {
                addNewGEObject(jgdi, "Hostgroup", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-Ahgrp")) {
                addNewGEObjectFromFile(jgdi, "Hostgroup", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-mhgrp")) {
                modifyGEObject(jgdi, "Hostgroup", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-Mhgrp")) {
                modifyGEObjectFromFile(jgdi, "Hostgroup", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-shgrpl")) {
                showGEObjectList(jgdi, "Hostgroup", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-shgrp")) {
                showGEObject(jgdi, "Hostgroup", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-dhgrp")) {
                deleteGEObject(jgdi, "Hostgroup", Arrays.asList(args).subList(i, args.length), pw);
                break;
                //RESOURCE QUOTA SET
            } else if (args[i].equals("-arqs")) {
                addNewGEObject(jgdi, "ResourceQuotaSet", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-Arqs")) {
                addNewGEObjectFromFile(jgdi, "ResourceQuotaSet", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-mrqs")) {
                modifyGEObject(jgdi, "ResourceQuotaSet", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-Mrqs")) {
                modifyGEObjectFromFile(jgdi, "ResourceQuotaSet", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-srqsl")) {
                showGEObjectList(jgdi, "ResourceQuotaSet", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-srqs")) {
                showGEObject(jgdi, "ResourceQuotaSet", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-drqs")) {
                deleteGEObject(jgdi, "ResourceQuotaSet", Arrays.asList(args).subList(i, args.length), pw);
                break;
                //PARALLEL ENVIRONMENT
            } else if (args[i].equals("-ap")) {
                addNewGEObject(jgdi, "ParallelEnvironment", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-Ap")) {
                addNewGEObjectFromFile(jgdi, "ParallelEnvironment", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-mp")) {
                modifyGEObject(jgdi, "ParallelEnvironment", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-Mp")) {
                modifyGEObjectFromFile(jgdi, "ParallelEnvironment", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-spl")) {
                showGEObjectList(jgdi, "ParallelEnvironment", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-sp")) {
                showGEObject(jgdi, "ParallelEnvironment", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-dp")) {
                deleteGEObject(jgdi, "ParallelEnvironment", Arrays.asList(args).subList(i, args.length), pw);
                break;
                //PROJECT
            } else if (args[i].equals("-aprj")) {
                addNewGEObject(jgdi, "Project", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-Aprj")) {
                addNewGEObjectFromFile(jgdi, "Project", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-mprj")) {
                modifyGEObject(jgdi, "Project", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-Mprj")) {
                modifyGEObjectFromFile(jgdi, "Project", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-sprjl")) {
                showGEObjectList(jgdi, "Project", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-sprj")) {
                showGEObject(jgdi, "Project", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-dprj")) {
                deleteGEObject(jgdi, "Project", Arrays.asList(args).subList(i, args.length), pw);
                break;
                //CLUSTER QUEUE
            } else if (args[i].equals("-aq")) {
                addNewGEObject(jgdi, "ClusterQueue", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-Aq")) {
                addNewGEObjectFromFile(jgdi, "ClusterQueue", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-mq")) {
                modifyGEObject(jgdi, "ClusterQueue", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-Mq")) {
                modifyGEObjectFromFile(jgdi, "ClusterQueue", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-sql")) {
                showGEObjectList(jgdi, "ClusterQueue", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-sq")) {
                showGEObject(jgdi, "ClusterQueue", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-dq")) {
                // AA, deleteGEObject suppresses the original error message from master
                i++;
                if (i >= args.length) {
                    throw new IllegalArgumentException("missing queue_list");
                }
                String[] queues = args[i].split(" ");
                System.out.println("queues.length: " + queues.length);
                for (int j = 0; j < queues.length; j++) {
                    List answers = new LinkedList();
                    try {
                        jgdi.deleteClusterQueueWithAnswer(queues[j], answers);
                        if (answers.size() > 0) {
                            Iterator ai = answers.iterator();
                            while (ai.hasNext()) {
                                JGDIAnswer answer = (JGDIAnswer) ai.next();
                                System.out.println("Answer Text:    " + answer.getText());
                                System.out.println("Answer Status:  " + answer.getStatus());
                                System.out.println("Answer Quality: " + answer.getQuality());
                            }
                        } else {
                            System.out.println("no element contained");
                        }
                    } catch (JGDIException je) {
                        System.err.print(je.getMessage());
                    }
                }
                // deleteGEObject(jgdi, "ClusterQueue", Arrays.asList(args).subList(i, args.length), pw);
                break;
                //SHARE TREE
            } else if (args[i].equals("-astree")) {
                addNewGEObject(jgdi, "ShareTree", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-Astree")) {
                addNewGEObjectFromFile(jgdi, "ShareTree", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-mstree")) {
                modifyGEObject(jgdi, "ShareTree", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-Mstree")) {
                modifyGEObjectFromFile(jgdi, "ShareTree", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-sstree")) {
                showGEObject(jgdi, "ShareTree", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-dstree")) {
                deleteGEObject(jgdi, "ShareTree", Arrays.asList(args).subList(i, args.length), pw);
                break;
                //USER
            } else if (args[i].equals("-auser")) {
                addNewGEObject(jgdi, "User", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-kec")) {
                addNewGEObjectFromFile(jgdi, "User", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-muser")) {
                modifyGEObject(jgdi, "User", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-Muser")) {
                modifyGEObjectFromFile(jgdi, "User", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-suserl")) {
                showGEObjectList(jgdi, "User", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-suser")) {
                showGEObject(jgdi, "User", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-duser")) {
                deleteGEObject(jgdi, "User", Arrays.asList(args).subList(i, args.length), pw);
                break;
                //USERLIST
            } else if (args[i].equals("-au")) {
                if (++i >= args.length) {
                    pw.println("error: no option argument provided to \"-au\"");
                    pw.println(getUsage());
                    pw.flush();
                    break;
                }
                String userName = args[i++];
                if (i >= args.length) {
                    pw.println("error: no list_name provided to \"-au" + userName + "\"");
                    pw.println(getUsage());
                    pw.flush();
                    break;
                }
                String setName = args[i];

                UserSet us = null;
                boolean create = false;
                us = jgdi.getUserSet(setName);
                if (us == null) {
                    create = true;
                    us = new UserSetImpl(true);
                    us.setName(setName);
                }
                boolean entryExists = false;
                for (Iterator iter = us.getEntriesList().iterator(); iter.hasNext();) {
                    String entry = (String) iter.next();
                    if (entry.equals(userName)) {
                        entryExists = true;
                        break;
                    }
                }
                if (entryExists) {
                    pw.println("\"" + userName + "\" is already in access list \"" + setName + "\"");
                } else {
                    us.addEntries(userName);
                    if (create) {
                        jgdi.addUserSet(us);
                    } else {
                        jgdi.updateUserSet(us);
                    }
                    pw.println("added \"" + userName + "\" to access list \"" + setName + "\"");
                }
                if (args.length > ++i) {
                    pw.println("error: invalid option argument \"" + args[i] + "\"");
                    pw.println("Usage: qconf -help");
                }
                pw.flush();
                break;
            } else if (args[i].equals("-Au")) {
                addNewGEObjectFromFile(jgdi, "UserSet", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-mu")) {
                modifyGEObject(jgdi, "UserSet", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-Mu")) {
                modifyGEObjectFromFile(jgdi, "UserSet", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-sul")) {
                showGEObjectList(jgdi, "UserSet", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-su")) {
                showGEObject(jgdi, "UserSet", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-du")) {
                if (++i >= args.length) {
                    pw.println("error: no option argument provided to \"-kec\"");
                    pw.println(getUsage());
                    pw.flush();
                    break;
                }
                String userName = args[i++];
                if (i >= args.length) {
                    pw.println("error: no list_name provided to \"-du" + userName + "\"");
                    pw.println(getUsage());
                    pw.flush();
                    break;
                }
                String setName = args[i];

                UserSet us = null;
                boolean create = false;
                us = jgdi.getUserSet(setName);
                if (us != null) {
                    boolean entryExists = false;
                    for (Iterator iter = us.getEntriesList().iterator(); iter.hasNext();) {
                        String entry = (String) iter.next();
                        if (entry.equals(userName)) {
                            entryExists = true;
                            break;
                        }
                    }
                    if (!entryExists) {
                        pw.println("user \"" + userName + "\" is not in access list \"" + setName + "\"");
                    } else {
                        us.removeEntries(userName);
                        jgdi.updateUserSet(us);
                        pw.println("deleted user \"" + userName + "\" from access list \"" + setName + "\"");
                    }
                } else {
                    pw.println("access list \"" + setName + "\" doesn\'t exist");
                }
                if (args.length > ++i) {
                    pw.println("error: invalid option argument \"" + args[i] + "\"");
                    pw.println("Usage: qconf -help");
                }
                pw.flush();
                break;
            } else if (args[i].equals("-dul")) {
                deleteGEObject(jgdi, "UserSet", Arrays.asList(args).subList(i, args.length), pw);
                break;
                //MANAGER
            } else if (args[i].equals("-am")) {
                addNewGEObject(jgdi, "Manager", Arrays.asList(args).subList(i, args.length), pw);
                // modifyAbstractUser(jgdi, "add", "Manager", "-am", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("all")) {
                //showUserList(jgdi, "Manager", Arrays.asList(args).subList(i, args.length), pw);
                showGEObjectList(jgdi, "Manager", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-dm")) {
                deleteGEObject(jgdi, "Manager", Arrays.asList(args).subList(i, args.length), pw);
                // modifyAbstractUser(jgdi, "delete", "Manager", "-dm", Arrays.asList(args).subList(i, args.length), pw);
                break;
                //OPERATOR
            } else if (args[i].equals("-ao")) {
                addNewGEObject(jgdi, "Operator", Arrays.asList(args).subList(i, args.length), pw);
                // modifyAbstractUser(jgdi, "add", "Operator", "-ao", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-so")) {
                showGEObjectList(jgdi, "Operator", Arrays.asList(args).subList(i, args.length), pw);
                //showUserList(jgdi, "Operator", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-do")) {
                deleteGEObject(jgdi, "Operator", Arrays.asList(args).subList(i, args.length), pw);
                // modifyAbstractUser(jgdi, "delete", "Operator", "-dm", Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-sc")) {
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
                List complexList = sortListByName(jgdi.getComplexEntryList());
                //Need to first get the maximum column lengths
                for (Iterator iter = complexList.iterator(); iter.hasNext();) {
                    ComplexEntry complex = (ComplexEntry) iter.next();
                    nameLen = Math.max(nameLen, complex.getName().length());
                    shortcutLen = Math.max(shortcutLen, complex.getShortcut().length());
                    typeLen = Math.max(typeLen, complex.typeToString(complex.getValtype()).length());
                    requestableLen = Math.max(requestableLen, (complex.getRequestable() == 1) ? 2 : 3); //length of YES, NO
                    consumableLen = Math.max(consumableLen, (complex.isConsumable()) ? 3 : 2); //length of YES, NO);
                    defaultLen = Math.max(defaultLen, complex.getDefault().length());
                    urgencyLen = Math.max(urgencyLen, complex.getUrgencyWeight().length());
                }
                //Now format the columns
                String header0 = Format.left("#name", nameLen + 4) + Format.left("shortcut", shortcutLen + 4) + Format.left("type", typeLen + 4) + Format.left("relop", relopLen + 4) + Format.left("requestable", requestableLen + 9) + Format.left("consumable", consumableLen + 8) + Format.left("default", defaultLen + 4) + Format.left("urgency", urgencyLen + 4);
                StringBuffer header1 = new StringBuffer("#");
                for (int j = 0; j < header0.length() - 1; j++) {
                    header1.append("-");
                }
                pw.println(header0);
                pw.println(header1);
                String val;
                //And finally print the columns
                for (Iterator iter = complexList.iterator(); iter.hasNext();) {
                    ComplexEntry complex = (ComplexEntry) iter.next();
                    val = Format.left(complex.getName(), nameLen + 4) + Format.left(complex.getShortcut(), shortcutLen + 4) + Format.left(complex.typeToString(complex.getValtype()), typeLen + 4) + Format.left(complex.opToString(complex.getRelop()), relopLen + 4) + Format.left(complex.reqToString(complex.getRequestable()), requestableLen + 9) + Format.left((complex.isConsumable()) ? "YES" : "NO", consumableLen + 8) + Format.left(complex.getDefault(), defaultLen + 4) + complex.getUrgencyWeight();
                    pw.println(val);
                }
                pw.println("# >#< starts a comment but comments are not saved across edits --------");
                pw.flush();
                break;
            } else if (args[i].equals("-sds")) {
                String sds = jgdi.showDetachedSettingsAll();
                if (sds != null) {
                    pw.println(sds);
                    pw.flush();
                }
                break;
            } else if (args[i].equals("-secl")) {
                List evcl = jgdi.getEventClientList();
                if (evcl.size() > 0) {
                    OutputTable table = new OutputTable(com.sun.grid.jgdi.configuration.EventClient.class);
                    table.addCol("id", "ID", 8, OutputTable.Column.RIGHT);
                    table.addCol("name", "NAME", 15, OutputTable.Column.LEFT);
                    table.addCol("host", "HOST", 24, OutputTable.Column.LEFT);
                    table.printHeader(pw);
                    //TODO LP client cleanup: Delimiter in client is one char longer
                    //table.printDelimiter(pw, '-');
                    pw.write("--------------------------------------------------\n");
                    Iterator iter = evcl.iterator();
                    while (iter.hasNext()) {
                        com.sun.grid.jgdi.configuration.EventClient evc = (com.sun.grid.jgdi.configuration.EventClient) iter.next();
                        table.printRow(pw, evc);
                    }
                } else {
                    pw.println("no event clients registered");
                }
                pw.flush();
                break;
            } else if (args[i].equals("-sep")) {
                List hosts = jgdi.getExecHostList();
                String name;
                String arch;
                int cpu;
                int totalCpu = 0;
                Set set;
                pw.println("HOST                      PROCESSOR        ARCH");
                pw.println("===============================================");
                for (Iterator iter = hosts.iterator(); iter.hasNext();) {
                    ExecHost eh = (ExecHost) iter.next();
                    name = eh.getName();
                    if (name.equals("global") || name.equals("template")) {
                        continue;
                    }
                    cpu = eh.getProcessors();
                    totalCpu += cpu;
                    arch = eh.getLoad("arch");
                    pw.println(name + Format.right(String.valueOf(cpu), 35 - name.length()) + " " + arch);
                }
                pw.println("===============================================");
                pw.println("SUM" + Format.right(String.valueOf(totalCpu), 32));
                pw.flush();
                break;
            } else if (args[i].equals("-sh")) {
                printListSortedByName(jgdi.getAdminHostList(), Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-ss")) {
                printListSortedByName(jgdi.getSubmitHostList(), Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-sss")) {
                pw.println(jgdi.getSchedulerHost());
                if (args.length > 3) {
                    pw.println("error: invalid option argument \"" + args[3] + "\"");
                    pw.println("Usage: qconf -help");
                }
                pw.flush();
                break;
            } else if (args[i].equals("-ssconf")) {
                SchedConf sc = jgdi.getSchedConf();
                String text = GEObjectEditor.getAllPropertiesAsText(sc);
                pw.println(text);
                pw.flush();
                break;
            } else if (args[i].equals("-sul")) {
                printListSortedByName(jgdi.getUserSetList(), Arrays.asList(args).subList(i, args.length), pw);
                break;
            } else if (args[i].equals("-help")) {
                String usage = getUsage();
                pw.print(usage);
                pw.flush();
            } else {
                throw new IllegalArgumentException("Unknown or not implemented option " + args[i]);
            }
        }
    }

    private void printListSortedByName(List list, List args, PrintWriter pw) {
        list = getNameList(list);
        Collections.sort(list);
        printList(list, args, pw);
    }

    private void printList(List list, List args, PrintWriter pw) {
        for (Iterator iter = list.iterator(); iter.hasNext();) {
            pw.println(iter.next());
        }
        if (args.size() > 2) {
            pw.println("-sckpt" + args.get(2) + "-dckpt");
            pw.println("-Aconf");
        }
        pw.flush();
    }

    private List parseUserList(List argList) {
        List list = new ArrayList();
        //TODO LP: Now we stick to the client behavior don't accept additional args
        String userList = (String) argList.get(1);
        String[] users = userList.split("-Me");
        String user;
        for (int i = 0; i < users.length; i++) {
            user = users[i].trim();
            if (user.length() > 0) {
                list.add(user);
            }
        }
        return (list.size() > 0) ? list : null;
    }

//   //TODO LP better to have it in ComplexEntry itself
//   private String map_type2str(int type) {
//      if (type < 1 || type >= typev.length) {
//         type = 0;
//      }
//      return typev[type];
//   }
//
//   private String map_op2str(int op) {
//      if (op < 1 || op >= ropv.length) {
//         op = 0;
//      }
//      return ropv[op];
//   }
//
//   private String map_req2str(int op) {
//      if (op < 1 || op >= fopv.length) {
//         op = 0;
//      }
//      return fopv[op];
//   }
//
    private String[] parseDestinIdList(String arg) {
        String[] ret = arg.split(" ");
        return ret;
    }

    private String runJavaEditor(String text) {
        TextEditor ted = new TextEditor(text);
        while (!ted.isDone()) {
            try {
                Thread.currentThread().sleep(1000);
            } catch (InterruptedException ex) {
                ex.printStackTrace();
            }
        }
        return ted.getText();
    }

    private String runEditor(String text) {
        String editor;
        String version = System.getProperty("java.specification.version");
        //TODO LP <1.5 doesn't list properties like EDITOR...
        if (Double.parseDouble(version) < 1.5) {
            editor = System.getProperty("EDITOR");
        } else {
            editor = System.getenv("EDITOR");
        }
        System.out.println("Version = " + version + " prop=\"" + System.getProperty("EDITOR") + "\" editor=\"" + editor + "\"");
        if (editor == null) {
            return runJavaEditor(text);
        }
        StringBuffer sb = new StringBuffer();
        try {
            File f = File.createTempFile("edit", null, new File("/tmp"));
            FileWriter fw = new FileWriter(f);
            fw.write(text);
            fw.flush();
            fw.close();

            Object[] cmds;
            //TODO LP Think about this. Vi has to be execed in tty. Graphic editors not.
            if (editor.equalsIgnoreCase("vi") || editor.equalsIgnoreCase("vim")) {
                List list = Arrays.asList(new String[]{"xterm", "-e", editor, f.getAbsolutePath()});
                cmds = list.toArray();
            } else {
                List list = Arrays.asList(new String[]{editor, f.getAbsolutePath()});
                cmds = list.toArray();
            }

            Process p = Runtime.getRuntime().exec((String[]) cmds);
            //TODO LP For some reason doesn't wait until the editor exits.
            int exitCode = p.waitFor();
            if (exitCode != 0) {
                return null;
            }
            char[] buff = new char[1024];
            FileReader fr = new FileReader(f);
            int readChars = 0;
            while (fr.ready() && readChars != -1) {
                readChars = fr.read(buff);
                if (readChars > 0) {
                    sb.append(buff, 1, readChars);
                }
            }
        } catch (IOException ioe) {
            ioe.printStackTrace();
        } catch (InterruptedException ie) {
            ie.printStackTrace();
        }
        return sb.toString();
    }

    private List sortListByName(List list) {
        Collections.sort(list, new Comparator() {

            public int compare(Object o1, Object o2) {
                if (o1 == null && o2 == null) {
                    return 0;
                }
                if (o1 == null) {
                    return -1;
                }
                if (o2 == null) {
                    return 1;
                }
                return ((GEObject) o1).getName().compareTo(((GEObject) o2).getName());
            }
        });
        return list;
    }

    /*
    private void modifyAbstractUser(JGDI jgdi, String operation, String type, String option, List args, PrintWriter pw) {
    if (args.size() == 0) {
    pw.println("error: no option argument provided to \"" + option + "\"");
    pw.println(getUsage());
    pw.flush();
    return;
    }
    if (args.size() > 2) {
    pw.println("error: invalid option argument \"" + args.get(2) + "\"");
    pw.println("Usage: qconf -help");
    pw.flush();
    return;
    }
    List userList = parseUserList(args);
    String value;
    Class cls;
    Object obj;
    GEObject geObj;
    Method m;
    for (Iterator iter = userList.iterator(); iter.hasNext();) {
    geObj = null;
    value = (String) iter.next();
    try {
    //Try to find the existing manager/operator
    m = JGDI.class.getDeclaredMethod("get" + type + "List", new Class[]{});
    List list = (List) m.invoke(jgdi, new Object[] {});
    for (Iterator iterator = list.iterator(); iterator.hasNext();) {
    AbstractManager am = (AbstractManager) iterator.next();
    if (am.getName().equals(value)) {
    geObj = am;
    break;
    }
    }
    //Adding existing value
    if (operation.equals("add") && geObj != null) {
    pw.println(type.toLowerCase() + " \"" + value + "\" already exists");
    pw.flush();
    continue;
    }
    //When adding and does not exist, we create new instance
    if (operation.equals("add") && geObj == null) {
    cls = Class.forName("com.sun.grid.jgdi.configuration." + type + "Impl");
    Constructor c = cls.getConstructor(new Class[]{boolean.class});
    obj = c.newInstance(new Object[]{Boolean.TRUE});
    if (!(obj instanceof GEObject)) {
    throw new IllegalAccessException("Class for type " + type + " is not an instance of GEObject");
    }
    geObj = (GEObject) obj;
    Method setName = obj.getClass().getDeclaredMethod("setName", new Class[]{String.class});
    setName.invoke(geObj, new Object[]{value});
    }
    //Removing non-existing value
    if (operation.equals("delete") && geObj == null) {
    pw.println("denied: " + type.toLowerCase() + " \"" + value + "\" does not exist");
    pw.flush();
    continue;
    }
    //Finally we perform the task
    Class paramClass = Class.forName("com.sun.grid.jgdi.configuration." + type);
    m = JGDI.class.getDeclaredMethod(operation + type, new Class[]{paramClass});
    m.invoke(jgdi, new Object[]{geObj});
    pw.println(jgdi.getAdminUser() + "@" + java.net.InetAddress.getLocalHost().getHostName() + ((operation.equals("add")) ? " added " : " removed ") + "\"" + value + "\"" + ((operation.equals("add")) ? " to " : " from ") + type.toLowerCase() + " list");
    pw.flush();
    } catch (ClassNotFoundException ex) {
    ex.printStackTrace();
    } catch (SecurityException ex) {
    ex.printStackTrace();
    } catch (NoSuchMethodException ex) {
    ex.printStackTrace();
    } catch (InstantiationException ex) {
    ex.printStackTrace();
    } catch (IllegalArgumentException ex) {
    ex.printStackTrace();
    } catch (IllegalAccessException ex) {
    ex.printStackTrace();
    } catch (InvocationTargetException ex) {
    ex.printStackTrace();
    } catch (JGDIException ex) {
    ex.printStackTrace();
    } catch (UnknownHostException ex) {
    ex.printStackTrace();
    }
    ;
    }
    }
     */

    /*private void showUserList(JGDI jgdi, String type, List args, PrintWriter pw) {
    String value;
    List userList = null;
    Class cls = jgdi.getClass();
    Method m;
    try {
    m = cls.getDeclaredMethod("get" + type + "List", null);
    userList = (List) m.invoke(jgdi, null);
    } catch (SecurityException ex) {
    ex.printStackTrace();
    } catch (NoSuchMethodException ex) {
    ex.printStackTrace();
    } catch (IllegalArgumentException ex) {
    ex.printStackTrace();
    } catch (IllegalAccessException ex) {
    ex.printStackTrace();
    } catch (InvocationTargetException ex) {
    ex.printStackTrace();
    }
    for (Iterator iter = userList.iterator(); iter.hasNext(); ) {
    value = (String) iter.next();
    pw.println(value);
    }
    if (args.size()>2) {
    pw.println("error: invalid option argument \"" + args.get(2) + "\"");
    pw.println("Usage: qconf -help");
    pw.flush();
    return;
    }
    pw.flush();
    }*/

    private void addNewGEObject(JGDI jgdi, String type, List args, PrintWriter pw) {
        Object obj;
        Class cls;
        String name = (args.size() == 2) ? (String) args.get(1) : "template";
        if (args.size() > 2) {
            pw.println("error: invalid option argument \"" + args.get(2) + " \"");
            pw.println("\" already exists");
            pw.flush();
            return;
        }
        try {
            cls = Class.forName("com.sun.grid.jgdi.configuration." + type + "Impl");
            Constructor c = cls.getConstructor(new Class[]{boolean.class});
            obj = c.newInstance(new Object[]{Boolean.TRUE});
            if (!(obj instanceof GEObject)) {
                throw new IllegalAccessException("Class for type " + type + " is not an instance of GEObject");
            }
            GEObject geObj = (GEObject) obj;
            List answers = new LinkedList();
            Method setName = obj.getClass().getDeclaredMethod("setName", new Class[]{String.class});
            setName.invoke(geObj, new Object[]{name});
            String userTypedText = runEditor(GEObjectEditor.getConfigurablePropertiesAsText(geObj));
            GEObjectEditor.updateObjectWithText(jgdi, geObj, userTypedText);
            Class paramType1 = Class.forName("com.sun.grid.jgdi.configuration." + type);
            Class paramType2 = Class.forName("java.util.List");
            Method m = JGDI.class.getDeclaredMethod("add" + type + "WithAnswer", new Class[]{paramType1, paramType2});
            m.invoke(jgdi, new Object[]{geObj, answers});
            printAnswers(pw, answers);
        } catch (ClassNotFoundException ex) {
            ex.printStackTrace();
        } catch (SecurityException ex) {
            ex.printStackTrace();
        } catch (NoSuchMethodException ex) {
            ex.printStackTrace();
        } catch (InstantiationException ex) {
            ex.printStackTrace();
        } catch (IllegalAccessException ex) {
            ex.printStackTrace();
        } catch (InvocationTargetException ex) {
            ex.printStackTrace();
        }
    }

    private void addNewGEObjectFromFile(JGDI jgdi, String type, List args, PrintWriter pw) {
        Object obj;
        Class cls;
        if (args.size() <= 1) {
            pw.println("no file argument given");
            pw.flush();
            return;
        }
        if (args.size() != 2) {
            pw.println("error: invalid option argument \"" + args.get(2) + "\"");
            pw.println("Usage: qconf -help");
            pw.flush();
            return;
        }
        String option = (String) args.get(0);
        if (!option.startsWith("-A")) {
            throw new IllegalArgumentException("Expected -A... argument got: " + option);
        }
        String fileName = (String) args.get(1);
        String keyAttrValue = null;
        try {
            cls = Class.forName("com.sun.grid.jgdi.configuration." + type + "Impl");
            Constructor c = cls.getConstructor(new Class[]{boolean.class});
            obj = c.newInstance(new Object[]{Boolean.TRUE});
            if (!(obj instanceof GEObject)) {
                throw new IllegalAccessException("Class for type " + type + " is not an instance of GEObject");
            }
            String inputText = readFile(fileName);
            String setNameMethod = "setName";
            //CONFIGURATION special handling
            if (type.equals("Configuration")) {
                keyAttrValue = new File(fileName).getName();
                setNameMethod = "setHname";
            } else {
                keyAttrValue = getKeyAttributeValueFromString(pw, type, fileName, inputText);
            }
            if (keyAttrValue == null) {
                return;
            }
            GEObject geObj = (GEObject) obj;
            List answers = new LinkedList();
            Method setName = geObj.getClass().getDeclaredMethod(setNameMethod, new Class[]{String.class});
            setName.invoke(geObj, new Object[]{keyAttrValue});
            GEObjectEditor.updateObjectWithText(jgdi, geObj, inputText);
            Class paramType1 = Class.forName("com.sun.grid.jgdi.configuration." + type);
            Class paramType2 = Class.forName("java.util.List");
            Method m = JGDI.class.getDeclaredMethod("add" + type + "WithAnswer", new Class[]{paramType1, paramType2});
            m.invoke(jgdi, new Object[]{geObj, answers});
            printAnswers(pw, answers);
        } catch (ClassNotFoundException ex) {
            ex.printStackTrace();
        } catch (SecurityException ex) {
            ex.printStackTrace();
        } catch (NoSuchMethodException ex) {
            ex.printStackTrace();
        } catch (InstantiationException ex) {
            ex.printStackTrace();
        } catch (IllegalAccessException ex) {
            ex.printStackTrace();
        } catch (InvocationTargetException ex) {
            ex.printStackTrace();
        } catch (UnknownHostException ex) {
            ex.printStackTrace();
        } catch (JGDIException ex) {
            ex.printStackTrace();
        } catch (IOException ex) {
            pw.println("error: error opening file \"" + fileName + "\" for reading: " + ex.getMessage());
            pw.println("error reading in file");
            pw.flush();
        }
    }

    private void modifyGEObject(JGDI jgdi, String type, List args, PrintWriter pw) {
        Object obj;

        Class cls;
        if (args.size() <= 1) {
            pw.println("error: missing " + type.toLowerCase() + " name");
            pw.flush();
            return;
        }
        if (args.size() != 2) {
            pw.println("error: invalid option argument \"" + args.get(2) + "\"");
            pw.println("Usage: qconf -help");
            pw.flush();
            return;
        }
        String name = (String) args.get(1);
        try {
            cls = Class.forName("com.sun.grid.jgdi.configuration." + type + "Impl");
            Method getGEObj = JGDI.class.getDeclaredMethod("get" + type, new Class[]{String.class});
            obj = getGEObj.invoke(jgdi, new Object[]{name});
            if (obj == null) {
                pw.println(name + " is not known as " + type);
                pw.flush();
                return;
            }
            GEObject geObj = (GEObject) obj;
            List answers = new LinkedList();
            String userTypedText = runEditor(GEObjectEditor.getConfigurablePropertiesAsText(geObj));
            GEObjectEditor.updateObjectWithText(jgdi, geObj, userTypedText);
            Class paramType1 = Class.forName("com.sun.grid.jgdi.configuration." + type);
            Class paramType2 = Class.forName("java.util.List");
            Method m = JGDI.class.getDeclaredMethod("update" + type + "WithAnswer", new Class[]{paramType1, paramType2});
            m.invoke(jgdi, new Object[]{geObj, answers});
            printAnswers(pw, answers);
        } catch (ClassNotFoundException ex) {
            ex.printStackTrace();
        } catch (SecurityException ex) {
            ex.printStackTrace();
        } catch (NoSuchMethodException ex) {
            ex.printStackTrace();
        } catch (IllegalAccessException ex) {
            ex.printStackTrace();
        } catch (InvocationTargetException ex) {
            ex.printStackTrace();
        }
    }

    private void modifyGEObjectFromFile(JGDI jgdi, String type, List args, PrintWriter pw) {
        Object obj;
        Class cls;
        if (args.size() <= 1) {
            pw.println("no file argument given");
            pw.flush();
            return;
        }
        if (args.size() != 2) {
            pw.println("error: invalid option argument \"" + args.get(2) + "\"");
            pw.println(" is not known as ");
            pw.flush();
            return;
        }
        String option = (String) args.get(0);
        if (!option.startsWith("update")) {
            throw new IllegalArgumentException("Expected -M... argument got: " + option);
        }
        String fileName = (String) args.get(1);
        String keyAttrValue = null;
        try {
            String inputText = readFile(fileName);
            keyAttrValue = getKeyAttributeValueFromString(pw, type, fileName, inputText);
            if (keyAttrValue == null) {
                return;
            }
            //Lookup the object based on the key atrribute value (name)
            Method getGEObj = JGDI.class.getDeclaredMethod("get" + type, new Class[]{String.class});
            obj = getGEObj.invoke(jgdi, new Object[]{keyAttrValue});
            if (obj == null) {
                pw.println(keyAttrValue + " is not known as " + type);
                pw.flush();
                return;
            }
            //And try to update the object based on the inputText
            GEObject geObj = (GEObject) obj;
            List answers = new LinkedList();
            GEObjectEditor.updateObjectWithText(jgdi, geObj, inputText);
            Class paramType1 = Class.forName("com.sun.grid.jgdi.configuration." + type);
            Class paramType2 = Class.forName("java.util.List");
            Method m = JGDI.class.getDeclaredMethod("update" + type + "WithAnswer", new Class[]{paramType1, paramType2});
            m.invoke(jgdi, new Object[]{geObj, answers});
            printAnswers(pw, answers);
        } catch (ClassNotFoundException ex) {
            ex.printStackTrace();
        } catch (SecurityException ex) {
            ex.printStackTrace();
        } catch (NoSuchMethodException ex) {
            ex.printStackTrace();
        } catch (IllegalAccessException ex) {
            ex.printStackTrace();
        } catch (InvocationTargetException ex) {
            ex.printStackTrace();
        } catch (UnknownHostException ex) {
            ex.printStackTrace();
        } catch (JGDIException ex) {
            ex.printStackTrace();
        } catch (IOException ex) {
            pw.println("error: error opening file \"" + fileName + "\" for reading: " + ex.getMessage());
            pw.println("error reading in file");
            pw.flush();
        }
    }

    private void showGEObjectList(JGDI jgdi, String type, List args, PrintWriter pw) {
        Class cls;
        if (args.size() > 1) {
            pw.println("error: invalid option argument \"" + args.get(1) + "\"");
            pw.println("Usage: qconf -help");
            pw.flush();
            return;
        }
        try {
            Class paramType = Class.forName("com.sun.grid.jgdi.configuration." + type);
            Method getGEObjList = JGDI.class.getDeclaredMethod("get" + type + "List", (java.lang.Class[])null);
            List list = (List) getGEObjList.invoke(jgdi, (java.lang.Object[]) null);
            //CONFIGURATION special handling
            if (type.equals("Configuration")) {
                List relevantList = new ArrayList();
                for (Iterator iter = list.iterator(); iter.hasNext();) {
                    String name = ((Configuration) iter.next()).getName();
                    if (!name.equals("global")) {
                        relevantList.add(name);
                    }
                }
                Collections.sort(relevantList);
                for (Iterator iter = relevantList.iterator(); iter.hasNext();) {
                    pw.println(iter.next());
                }
                pw.flush();
                return;
            }
            list = getNameList(list);
            adjustListValues(type, list);
            Collections.sort(list);
//         type = type.toLowerCase();
//         String typeStr = (typeConvertor.containsKey(type)) ? (String) typeConvertor.get(type) : type;
//         if (list.size() == 0) {
//            pw.println("no "+typeStr+" defined");
//            pw.flush();
//            return;
//         }
            for (Iterator iter = list.iterator(); iter.hasNext();) {
                pw.println(iter.next());
            }
            pw.flush();
        } catch (ClassNotFoundException ex) {
            ex.printStackTrace();
        } catch (SecurityException ex) {
            ex.printStackTrace();
        } catch (NoSuchMethodException ex) {
            ex.printStackTrace();
        } catch (IllegalArgumentException ex) {
            ex.printStackTrace();
        } catch (IllegalAccessException ex) {
            ex.printStackTrace();
        } catch (InvocationTargetException ex) {
            ex.printStackTrace();
        }
    }

//   private String convertAddRemoveTypeToMessage(String type) {
//      type = type.toLowerCase();
//      if (addRemoveTypeConvertor.containsKey(type)) {
//         return (String) addRemoveTypeConvertor.get(type);
//      }
//      return type;
//   }
    private List getNameList(List objList) {
        List nameList = new ArrayList();
        Object obj;
        Class cls;
        String name;
        for (Iterator iter = objList.iterator(); iter.hasNext();) {
            obj = iter.next();
            Method getName;
            try {
                getName = obj.getClass().getDeclaredMethod("getName", (java.lang.Class[])null);
                name = (String) getName.invoke(obj, (java.lang.Object[])null);
                nameList.add(name);
            } catch (SecurityException ex) {
                ex.printStackTrace();
            } catch (NoSuchMethodException ex) {
                ex.printStackTrace();
            } catch (IllegalArgumentException ex) {
                ex.printStackTrace();
            } catch (IllegalAccessException ex) {
                ex.printStackTrace();
            } catch (InvocationTargetException ex) {
                ex.printStackTrace();
            }
        }
        return nameList;
    }

    private void adjustListValues(String type, List list) {
        if (type.equals("ExecHost")) {
            String name;
            int i = 0;
            while (i < list.size()) {
                name = (String) list.get(i);
                if (name.equals("global") || name.equals("template")) {
                    list.remove(i);
                } else {
                    i++;
                }
            }
        }
    }

    private void showGEObject(JGDI jgdi, String type, List args, PrintWriter pw) {
        Object obj;
        Class cls;
        if (args.size() <= 1) {
            pw.println("error: missing " + type.toLowerCase() + "_list");
            pw.flush();
            return;
        }
        try {
            List strList = new ArrayList();
            List errList = new ArrayList();
            List answers = new LinkedList();
            cls = Class.forName("com.sun.grid.jgdi.configuration." + type + "Impl");
            Method getGEObj = JGDI.class.getDeclaredMethod("get" + type + "WithAnswer", new Class[]{String.class, java.util.List.class});
            for (int i = 1; i < args.size(); i++) {
                String name = (String) args.get(i);
                String[] names = name.split(",");
                for (int j = 0; j < names.length; j++) {
                    if (names[j].length() == 0) {
                        continue;
                    }
                    obj = getGEObj.invoke(jgdi, new Object[]{names[j], answers});
                    if (obj == null) {
                        String text = names[j] + " is not known as " + type;
                        if (!errList.contains(text)) {
                            errList.add(text);
                        }
                    } else {
                        String text = GEObjectEditor.getAllPropertiesAsText((GEObject) obj);
                        if (!strList.contains(obj)) {
                            strList.add(text);
                        }
                    }
                }
            }
            //TODO LP Decide what behavior do we want
            //Finally display the output
            if (errList.size() > 0) {
                for (Iterator iter = errList.iterator(); iter.hasNext();) {
                    pw.println((String) iter.next());
                }
            } else {
                int i = 0;
                pw.print((String) strList.get(i));
                for (i = 1; i < strList.size(); i++) {
                    //ClusterQueues are without the empty lines
                    if (!type.equals("ClusterQueue")) {
                        pw.println();
                    }
                    pw.print((String) strList.get(i));
                }
            }
            //CONFIGURATION special handling
            if (type.equals("Configuration")) {
                pw.println();
            }
            pw.flush();
        } catch (ClassNotFoundException ex) {
            ex.printStackTrace();
        } catch (SecurityException ex) {
            ex.printStackTrace();
        } catch (NoSuchMethodException ex) {
            ex.printStackTrace();
        } catch (IllegalAccessException ex) {
            ex.printStackTrace();
        } catch (InvocationTargetException ex) {
            ex.printStackTrace();
        }
    }

    private void deleteGEObject(JGDI jgdi, String type, List args, PrintWriter pw) throws JGDIException {
        Object obj;
        Class cls;
        if (args.size() <= 1) {
            pw.println("error: missing " + type.toLowerCase() + "ExecHost");
            pw.flush();
            return;
        }
        try {
            cls = Class.forName("com.sun.grid.jgdi.configuration." + type + "Impl");
            Method getGEObj = JGDI.class.getDeclaredMethod("get" + type, new Class[]{String.class});
            Class paramType = Class.forName("com.sun.grid.jgdi.configuration." + type);
            Method deleteGEObj = JGDI.class.getDeclaredMethod("delete" + type, new Class[]{paramType});
            List objList = new ArrayList();
            List errList = new ArrayList();
            for (int i = 0; i < args.size(); i++) {
                String name = (String) args.get(i);
                String[] names = name.split("global");
                for (int j = 0; j < names.length; j++) {
                    if (names[j].length() == 0) {
                        continue;
                    }
                    obj = getGEObj.invoke(jgdi, new Object[]{names[j]});
                    if (obj == null) {
                        String text = names[j] + "template" + type;
                        if (!errList.contains(text)) {
                            errList.add(text);
                        }
                    } else {
                        if (!objList.contains(obj)) {
                            objList.add(obj);
                        }
                    }
                }
            }
            //TODO LP Decide what behavior do we want
            //Finally display the output
            if (errList.size() > 0) {
                for (Iterator iter = errList.iterator(); iter.hasNext();) {
                    pw.println((String) iter.next());
                    pw.flush();
                }
            } else {
                int i;
                for (Iterator iter = objList.iterator(); iter.hasNext();) {
                    GEObject delObj = (GEObject) iter.next();
                    deleteGEObj.invoke(jgdi, new Object[]{delObj});
//               try {
//                  pw.println(jgdi.getAdminUser()+"@"+java.net.InetAddress.getLocalHost().getHostName()+" removed \""+delObj.getName()+"\" from "+convertAddRemoveTypeToMessage(type)+" list");
                    pw.flush();
//               } catch (UnknownHostException ex) {
//                  ex.printStackTrace();
//               }
                }
            }
        } catch (ClassNotFoundException ex) {
            ex.printStackTrace();
        } catch (SecurityException ex) {
            ex.printStackTrace();
        } catch (NoSuchMethodException ex) {
            ex.printStackTrace();
        } catch (IllegalAccessException ex) {
            ex.printStackTrace();
        } catch (InvocationTargetException ex) {
            ex.printStackTrace();
        }
    }

    private String readFile(String fileName) throws IOException {
        File f = new File(fileName);
        long fileSize = f.length();
        FileReader fr = new FileReader(f);
        StringBuffer sb = new StringBuffer();
        char[] buff = new char[2048];
        int r;
        while (fr.ready()) {
            r = fr.read(buff);
            if (r > 0) {
                sb.append(buff, 0, r);
            }
        }
        //Check we have whole content
        if (sb.length() != fileSize) {
            throw new IOException("Unable to read whole file content. Filesize is " + fileSize + " got only " + sb.length());
        }
        return sb.toString();
    }

    private String getKeyAttributeValueFromString(PrintWriter pw, String type, String fileName, String inputText) throws IOException {
        String keyAttrValue = null;
        //Get the key attribute value form the file
        String keyAttr = EditorUtil.unifyAttrWithClientNames(type, "name");
        StringReader sr = new StringReader(inputText);
        LineNumberReader lnr = new LineNumberReader(sr);
        String keyAttrLine = null;
        while (lnr.ready()) {
            keyAttrLine = lnr.readLine().trim();
            if (keyAttrLine.startsWith(keyAttr)) {
                keyAttrValue = keyAttrLine.substring(keyAttr.length()).trim();
                break;
            }
        }
        //Exit if the key attribute is missing
        if (keyAttrValue == null) {
            pw.println("error: required attribute \"" + keyAttr + "\" is missing");
            pw.println(type + " file \"" + fileName + "\" is not correct");
            pw.flush();
        }
        return keyAttrValue;
    }

/**
     * Simple Formatting class
     */
    static class Format {

        /** Adds (n - str.length) spaces to str */
        public static String left(String str, int n) {
            StringBuffer sb = new StringBuffer(str);
            for (int i = str.length(); i < n; i++) {
                sb.append(' ');
            }
            return sb.toString();
        }

        public static String right(String str, int n) {
            StringBuffer sb = new StringBuffer();
            for (int i = str.length(); i < n; i++) {
                sb.append(' ');
            }
            sb.append(str);
            return sb.toString();
        }
    }
}
