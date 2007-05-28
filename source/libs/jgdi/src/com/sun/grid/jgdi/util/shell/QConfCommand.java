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

import com.sun.grid.jgdi.JGDI;
import com.sun.grid.jgdi.JGDIException;
import com.sun.grid.jgdi.configuration.ClusterQueue;
import com.sun.grid.jgdi.configuration.GEObject;
import com.sun.grid.jgdi.configuration.Project;
import com.sun.grid.jgdi.configuration.ProjectImpl;
import com.sun.grid.jgdi.configuration.User;
import com.sun.grid.jgdi.configuration.ComplexEntry;
import com.sun.grid.jgdi.configuration.UserImpl;
import com.sun.grid.jgdi.configuration.Util;
import com.sun.grid.jgdi.configuration.reflect.PropertyDescriptor;
import com.sun.grid.jgdi.util.OutputTable;
import com.sun.grid.jgdi.util.shell.editor.EditorUtil;
import com.sun.grid.jgdi.util.shell.editor.GEObjectEditor;
import com.sun.grid.jgdi.util.shell.editor.TextEditor;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.LineNumberReader;
import java.io.PrintStream;
import java.io.PrintWriter;
import java.io.StringReader;
import java.io.StringWriter;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.net.UnknownHostException;
import java.nio.ByteOrder;
import java.nio.CharBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;

/**
 *
 */
public class QConfCommand extends AbstractCommand {
   
   //TODO LP better to have it in ComplexEntry itself, Used only for -sc
   private static String typev[] = {
      "??????",
      "INT",     /* TYPE_INT */
      "STRING",  /* TYPE_STR */
      "TIME",    /* TYPE_TIM */
      "MEMORY",  /* TYPE_MEM */
      "BOOL",    /* TYPE_BOO */
      "CSTRING", /* TYPE_CSTR */
      "HOST",    /* TYPE_HOST */
      "DOUBLE",  /* TYPE_DOUBLE */
      "RESTRING", /* TYPE_RESTR */
      
      "TYPE_ACC",/* TYPE_ACC */
      "TYPE_LOG",/* TYPE_LOG */
      "TYPE_LOF" /* TYPE_LOF */
   };
   private static String ropv[] = {
      "??",
      "==", /* CMPLXEQ_OP */
      ">=", /* CMPLXGE_OP */
      ">",  /* CMPLXGT_OP */
      "<",  /* CMPLXLT_OP */
      "<=", /* CMPLXLE_OP */
      "!="  /* CMPLXNE_OP */
    };
    private static String fopv[] = {
      "??",
      "NO",       /* REQU_NO */
      "YES",      /* REQU_YES */
      "FORCED"    /* REQU_FORCED */
    };
    
   
   
   
   /** Creates a new instance of QConfCommand */
   public QConfCommand(Shell shell, String name) {
      super(shell, name);
   }
    
    public String getUsage() {
        StringWriter sw = new StringWriter();
        PrintWriter pw = new PrintWriter(sw);
        pw.println("usage: qconf [options]");
//      pw.println("   [-aattr obj_nm attr_nm val obj_id_lst]   add to a list attribute of an object");
//      pw.println("   [-Aattr obj_nm fname obj_id_lst]         add to a list attribute of an object");
        pw.println("   [-acal calendar_name]                    add a new calendar");
        pw.println("   [-Acal fname]                            add a new calendar from file");
        pw.println("   [-ackpt ckpt_name]                       add a ckpt interface definition");
        pw.println("   [-Ackpt fname]                           add a ckpt interface definition from file");
//      pw.println("   [-aconf host_list]                       add configurations");
//      pw.println("   [-Aconf file_list]                       add configurations from file_list");
        pw.println("   [-ae [exec_server_template]]             add an exec host using a template");
        pw.println("   [-Ae fname]                              add an exec host from file");
//      pw.println("   [-ah hostname]                           add an administrative host");
        pw.println("   [-ahgrp group]                           add new host group entry");
        pw.println("   [-Ahgrp file]                            add new host group entry from file");
        pw.println("   [-arqs [rqs_list]]                       add resource quota set(s)");
        pw.println("   [-Arqs fname]                            add resource quota set(s) from file");
//      pw.println("   [-am user_list]                          add user to manager list");
//      pw.println("   [-ao user_list]                          add user to operator list");
        pw.println("   [-ap pe-name]                            add a new parallel environment");
        pw.println("   [-Ap fname]                              add a new parallel environment from file");
        pw.println("   [-aprj]                                  add project");
        pw.println("   [-Aprj fname]                            add project from file");
        pw.println("   [-aq ]                                   add a new cluster queue");
        pw.println("   [-Aq fname]                              add a queue from file");
//      pw.println("   [-as hostname]                           add a submit host");
//      pw.println("   [-astnode node_shares_list]              add sharetree node(s)");
//      pw.println("   [-astree]                                create/modify the sharetree");
//      pw.println("   [-Astree fname]                          create/modify the sharetree from file");
//      pw.println("   [-au user_list listname_list]            add user(s) to userset list(s)");
//      pw.println("   [-Au fname]                              add userset from file");
        pw.println("   [-auser]                                 add user");
        pw.println("   [-Auser fname]                           add user from file");
        pw.println("   [-clearusage]                            clear all user/project sharetree usage");
        pw.println("   [-cq destin_id_list]                     clean queue");
//      pw.println("   [-dattr obj_nm attr_nm val obj_id_lst]   delete from a list attribute of an object");
//      pw.println("   [-Dattr obj_nm fname obj_id_lst]         delete from a list attribute of an object");
        pw.println("   [-dcal calendar_name]                    remove a calendar");
        pw.println("   [-dckpt ckpt_name]                       remove a ckpt interface definition");
//      pw.println("   [-dconf host_list]                       delete local configurations");
        pw.println("   [-de host_list]                          remove an exec server");
        pw.println("   [-dh host_list]                          remove an administrative host");
        pw.println("   [-dhgrp group]                           delete host group entry");
        pw.println("   [-drqs rqs_list]                         delete resource quota set(s)");
        pw.println("   [-dm user_list]                          remove user from manager list");
        pw.println("   [-do user_list]                          remove user from operator list");
        pw.println("   [-dp pe-name]                            remove a parallel environment");
        pw.println("   [-dprj project_list]                     delete project");
        pw.println("   [-dq destin_id_list]                     remove a queue");
        pw.println("   [-ds host_list]                          remove submit host");
//      pw.println("   [-dstnode node_list]                     remove sharetree node(s)");
        pw.println("   [-dstree]                                delete the sharetree");
//      pw.println("   [-du user_list listname_list]            remove user(s) from userset list(s)");
//      pw.println("   [-dul listname_list]                     remove userset list(s) completely");
        pw.println("   [-duser user_list]                       delete user");
        pw.println("   [-help]                                  print this help");
        pw.println("   [-ke[j] host_list                        shutdown execution daemon(s)");
        pw.println("   [-k{m|s}]                                shutdown master|scheduling daemon");
        pw.println("   [-kec evid_list]                         kill event client");
//      pw.println("   [-mattr obj_nm attr_nm val obj_id_lst]   modify an attribute (or element in a sublist) of an object");
//      pw.println("   [-Mattr obj_nm fname obj_id_lst]         modify an attribute (or element in a sublist) of an object");
//      pw.println("   [-mc ]                                   modify complex attributes");
        pw.println("   [-mckpt ckpt_name]                       modify a ckpt interface definition");
//      pw.println("   [-Mc fname]                              modify complex attributes from file");
        pw.println("   [-mcal calendar_name]                    modify calendar");
        pw.println("   [-Mcal fname]                            modify calendar from file");
        pw.println("   [-Mckpt fname]                           modify a ckpt interface definition from file");
//      pw.println("   [-mconf [host_list|global]]              modify configurations");
//      pw.println("   [-msconf]                                modify scheduler configuration");
//      pw.println("   [-Msconf fname]                          modify scheduler configuration from file");
//      pw.println("   [-me server]                             modify exec server");
//      pw.println("   [-Me fname]                              modify exec server from file");
        pw.println("   [-mhgrp group]                           modify host group entry");
        pw.println("   [-Mhgrp file]                            modify host group entry from file");
        pw.println("   [-mrqs [rqs_list]]                       modify resource quota set(s)");
        pw.println("   [-Mrqs fname]                            modify resource quota set(s) from file");
        pw.println("   [-mp pe-name]                            modify a parallel environment");
        pw.println("   [-Mp fname]                              modify a parallel environment from file");
        pw.println("   [-mprj project]                          modify a project");
        pw.println("   [-Mprj fname]                            modify project from file");
        pw.println("   [-mq queue]                              modify a queue");
        pw.println("   [-Mq fname]                              modify a queue from file");
//      pw.println("   [-mstnode node_shares_list]              modify sharetree node(s)");
//      pw.println("   [-Mstree fname]                          modify/create the sharetree from file");
//      pw.println("   [-mstree]                                modify/create the sharetree");
//      pw.println("   [-mu listname_list]                      modify the given userset list");
//      pw.println("   [-Mu fname]                              modify userset from file");
        pw.println("   [-muser user]                            modify a user");
        pw.println("   [-Muser fname]                           modify a user from file");
//      pw.println("   [-purge obj_nm3 attr_nm objectname]      removes attribute from object_instance");
//      pw.println("   [-rattr obj_nm attr_nm val obj_id_lst]   replace a list attribute of an object");
//      pw.println("   [-Rattr obj_nm fname obj_id_lst]         replace a list attribute of an object");
        pw.println("   [-sc ]                                   show complex attributes");
        pw.println("   [-scal calendar_name]                    show given calendar");
        pw.println("   [-scall]                                 show a list of all calendar names");
        pw.println("   [-sckpt ckpt_name]                       show ckpt interface definition");
        pw.println("   [-sckptl]                                show all ckpt interface definitions");
//      pw.println("   [-sconf [host_list|global]]              show configurations");
//      pw.println("   [-sconfl]                                show a list of all local configurations");
//      pw.println("   [-se server]                             show given exec server");
//      pw.println("   [-secl]                                  show event client list");
//      pw.println("   [-sel]                                   show a list of all exec servers");
//      pw.println("   [-sep]                                   show a list of all licensed processors");
//      pw.println("   [-sh]                                    show a list of all administrative hosts");
        pw.println("   [-shgrp group]                           show host group");
        pw.println("   [-shgrp_tree group]                      show host group and used hostgroups as tree");
        pw.println("   [-shgrp_resolved group]                  show host group with resolved hostlist");
        pw.println("   [-shgrpl]                                show host group list");
        pw.println("   [-sds]                                   show detached settings");
        pw.println("   [-srqs [rqs_list]]                       show resource quota set(s)");
        pw.println("   [-srqsl]                                 show resource quota set list");
        pw.println("   [-sm]                                    show a list of all managers");
        pw.println("   [-so]                                    show a list of all operators");
//      pw.println("   [-sobjl obj_nm2 attr_nm val]             show objects which match the given value");
        pw.println("   [-sp pe-name]                            show a parallel environment");
        pw.println("   [-spl]                                   show all parallel environments");
        pw.println("   [-sprj project]                          show a project");
        pw.println("   [-sprjl]                                 show a list of all projects");
        pw.println("   [-sq [destin_id_list]]                   show the given queue");
        pw.println("   [-sql]                                   show a list of all queues");
        pw.println("   [-ss]                                    show a list of all submit hosts");
//      pw.println("   [-sss]                                   show scheduler state");
//      pw.println("   [-ssconf]                                show scheduler configuration");
//      pw.println("   [-sstnode node_list]                     show sharetree node(s)");
//      pw.println("   [-rsstnode node_list]                    show sharetree node(s) and its children");
//      pw.println("   [-sstree]                                show the sharetree");
//      pw.println("   [-su listname_list]                      show the given userset list");
        pw.println("   [-suser user_list]                       show user(s)");
        pw.println("   [-sul]                                   show a list of all userset lists");
        pw.println("   [-suserl]                                show a list of all users");
        pw.println("   [-tsm]                                   trigger scheduler monitoring");
        pw.println("complex_list            complex[,complex,...]");
        pw.println("destin_id_list          queue[ queue ...]");
        pw.println("listname_list           listname[,listname,...]");
        pw.println("rqs_list               rqs_name[,rqs_name,...]");        
        pw.println("node_list               node_path[,node_path,...]");
        pw.println("node_path               [/]node_name[[/.]node_name...]");
        pw.println("node_shares_list        node_path=shares[,node_path=shares,...]");  
        pw.println("user_list               user|pattern[,user|pattern,...]");
        pw.println("obj_nm                  \"queue\"|\"exechost\"|\"pe\"|\"ckpt\"|\"hostgroup\"");
        pw.println("attr_nm                 (see man pages)");
        pw.println("obj_id_lst              objectname [ objectname ...]");
        pw.println("project_list            project[,project,...]");
        pw.println("evid_list               all | evid[,evid,...]");
        pw.println("host_list               all | hostname[,hostname,...]");
        pw.println("obj_nm2                 \"queue\"|\"queue_domain\"|\"queue_instance\"|\"exechost\"");
        pw.println("obj_nm3                 \"queue\"");
        return sw.getBuffer().toString();
    }
    
    
    public void run(String[] args) throws Exception {
        
        JGDI jgdi = getShell().getConnection();
        
        if (jgdi == null) {
            throw new IllegalStateException("Not connected");
        }
        if(args.length == 0) {
            throw new IllegalArgumentException("Invalid number of arguments");
        }
        
        PrintWriter pw = new PrintWriter(System.out);
        
        boolean force = false;
        
        for(int i = 0; i < args.length; i++) {
            if (args[i].equals("-tsm")) {
                jgdi.triggerSchedulerMonitoring();
                break;
            } else if (args[i].equals("-clearusage")) {
                jgdi.clearShareTreeUsage();
                break;
            } else if (args[i].equals("-cq")) {
                i++;
                if(i>= args.length) {
                    throw new IllegalArgumentException("missing destin_id_list");
                }
                String [] queues = parseDestinIdList(args[i]);
                jgdi.cleanQueues(queues);
                break;
            } else if (args[i].equals("-kec")) {
                
                i++;
                if(i>= args.length) {
                    throw new IllegalArgumentException("missing evid_list");
                }
                if(args[i].equals("all")) {
                    jgdi.killAllEventClients();
                } else {
                    String idStr [] = args[i].split(" ");
                    int [] ids = new int[idStr.length];
                    
                    for(int idIndex = 0; idIndex < idStr.length; idIndex++) {
                        try {
                            ids[idIndex] = Integer.parseInt(idStr[idIndex]);
                        } catch(NumberFormatException nfe) {
                            throw new IllegalArgumentException(idStr[idIndex] + " is not a valid event client id");
                        }
                    }
                    
                    jgdi.killEventClients(ids);
                }
                break;
            } else if (args[i].equals("-km")) {
                jgdi.killMaster();
                break;
            } else if (args[i].equals("-ks")) {
                jgdi.killScheduler();
                break;
            //CALENDAR
            } else if (args[i].equals("-acal")) {
                addNewGEObject(jgdi, "Calendar", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-Acal")) {
                addNewGEObjectFromFile(jgdi, "Calendar", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-mcal")) {
                modifyGEObject(jgdi, "Calendar", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-Mcal")) {
                modifyGEObjectFromFile(jgdi, "Calendar", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-scall")) {
                showGEObjectList(jgdi, "Calendar", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-scal")) {
                showGEObject(jgdi, "Calendar", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-dcal")) {
                deleteGEObject(jgdi, "Calendar", Arrays.asList(args).subList(i,args.length), pw);
                break;
            //CHECKPOINT
            } else if (args[i].equals("-ackpt")) {
                addNewGEObject(jgdi, "Checkpoint", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-Ackpt")) {
                addNewGEObjectFromFile(jgdi, "Checkpoint", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-mckpt")) {
                modifyGEObject(jgdi, "Checkpoint", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-Mckpt")) {
                modifyGEObjectFromFile(jgdi, "Checkpoint", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-sckptl")) {
                showGEObjectList(jgdi, "Checkpoint", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-sckpt")) {
                showGEObject(jgdi, "Checkpoint", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-dckpt")) {
                deleteGEObject(jgdi, "Checkpoint", Arrays.asList(args).subList(i,args.length), pw);
                break;
            //EXEC HOST
            } else if (args[i].equals("-ae")) {
                addNewGEObject(jgdi, "ExecHost", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-Ae")) {
                addNewGEObjectFromFile(jgdi, "ExecHost", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-me")) {
                modifyGEObject(jgdi, "ExecHost", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-Me")) {
                modifyGEObjectFromFile(jgdi, "ExecHost", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-sel")) {
                showGEObjectList(jgdi, "ExecHost", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-se")) {
                showGEObject(jgdi, "ExecHost", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-de")) {
                deleteGEObject(jgdi, "ExecHost", Arrays.asList(args).subList(i,args.length), pw);
                break;
            //HOST GROUP
            } else if (args[i].equals("-ahgrp")) {
                addNewGEObject(jgdi, "Hostgroup", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-Ahgrp")) {
                addNewGEObjectFromFile(jgdi, "Hostgroup", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-mhgrp")) {
                modifyGEObject(jgdi, "Hostgroup", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-Mhgrp")) {
                modifyGEObjectFromFile(jgdi, "Hostgroup", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-shgrpl")) {
                showGEObjectList(jgdi, "Hostgroup", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-shgrp")) {
                showGEObject(jgdi, "Hostgroup", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-dhgrp")) {
                deleteGEObject(jgdi, "Hostgroup", Arrays.asList(args).subList(i,args.length), pw);
                break;
            //RESOURCE QUOTA SET
            } else if (args[i].equals("-arqs")) {
                addNewGEObject(jgdi, "ResourceQuotaSet", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-Arqs")) {
                addNewGEObjectFromFile(jgdi, "ResourceQuotaSet", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-mrqs")) {
                modifyGEObject(jgdi, "ResourceQuotaSet", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-Mrqs")) {
                modifyGEObjectFromFile(jgdi, "ResourceQuotaSet", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-srqsl")) {
                showGEObjectList(jgdi, "ResourceQuotaSet", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-srqs")) {
                showGEObject(jgdi, "ResourceQuotaSet", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-drqs")) {
                deleteGEObject(jgdi, "ResourceQuotaSet", Arrays.asList(args).subList(i,args.length), pw);
                break;
            //PARALLEL ENVIRONMENT
            } else if (args[i].equals("-ap")) {
                addNewGEObject(jgdi, "ParallelEnvironment", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-Ap")) {
                addNewGEObjectFromFile(jgdi, "ParallelEnvironment", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-mp")) {
                modifyGEObject(jgdi, "ParallelEnvironment", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-Mp")) {
                modifyGEObjectFromFile(jgdi, "ParallelEnvironment", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-spl")) {
                showGEObjectList(jgdi, "ParallelEnvironment", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-sp")) {
                showGEObject(jgdi, "ParallelEnvironment", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-dp")) {
                deleteGEObject(jgdi, "ParallelEnvironment", Arrays.asList(args).subList(i,args.length), pw);
                break;
            //PROJECT
            } else if (args[i].equals("-aprj")) {
                addNewGEObject(jgdi, "Project", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-Aprj")) {
                addNewGEObjectFromFile(jgdi, "Project", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-mprj")) {
                modifyGEObject(jgdi, "Project", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-Mprj")) {
                modifyGEObjectFromFile(jgdi, "Project", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-sprjl")) {
                showGEObjectList(jgdi, "Project", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-sprj")) {
                showGEObject(jgdi, "Project", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-dprj")) {
                deleteGEObject(jgdi, "Project", Arrays.asList(args).subList(i,args.length), pw);
                break;
            //CLUSTER QUEUE
            } else if (args[i].equals("-aq")) {
                addNewGEObject(jgdi, "ClusterQueue", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-Aq")) {
                addNewGEObjectFromFile(jgdi, "ClusterQueue", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-mq")) {
                modifyGEObject(jgdi, "ClusterQueue", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-Mq")) {
                modifyGEObjectFromFile(jgdi, "ClusterQueue", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-sql")) {
                showGEObjectList(jgdi, "ClusterQueue", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-sq")) {
                showGEObject(jgdi, "ClusterQueue", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-dq")) {
                deleteGEObject(jgdi, "ClusterQueue", Arrays.asList(args).subList(i,args.length), pw);
                break;
            //SHARE TREE
            } else if (args[i].equals("-astree")) {
                addNewGEObject(jgdi, "ShareTree", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-Astree")) {
                addNewGEObjectFromFile(jgdi, "ShareTree", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-mstree")) {
                modifyGEObject(jgdi, "ShareTree", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-Mstree")) {
                modifyGEObjectFromFile(jgdi, "ShareTree", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-sstree")) {
                showGEObject(jgdi, "ShareTree", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-dstree")) {
                deleteGEObject(jgdi, "ShareTree", Arrays.asList(args).subList(i,args.length), pw);
                break;
            //USER
            } else if (args[i].equals("-auser")) {
                addNewGEObject(jgdi, "User", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-Auser")) {
                addNewGEObjectFromFile(jgdi, "User", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-muser")) {
                modifyGEObject(jgdi, "User", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-Muser")) {
                modifyGEObjectFromFile(jgdi, "User", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-suserl")) {
                showGEObjectList(jgdi, "User", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-suser")) {
                showGEObject(jgdi, "User", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-duser")) {
                deleteGEObject(jgdi, "User", Arrays.asList(args).subList(i,args.length), pw);
                break;
            } else if (args[i].equals("-sc")) {
                //Format is:nameLen+3 shortcutLen+3 typeLen+3 relopLen+3 requestableLen+8 consumableLen+7 defaultLen+3 urgencyLen+4
                int nameLen = 0, shortcutLen = 0, typeLen = 0, relopLen = 2,
                    requestableLen = 0, consumableLen = 0, defaultLen = 0, urgencyLen = 0;
                //Sort the list alphabetically
                List complexList = sortListByName(jgdi.getComplexEntryList());
                //Need to first get the maximum column lengths
                for (Iterator iter=complexList.iterator(); iter.hasNext();) {
                    ComplexEntry complex = (ComplexEntry)iter.next();
                    nameLen = Math.max(nameLen, complex.getName().length());
                    shortcutLen = Math.max(shortcutLen, complex.getShortcut().length());
                    typeLen = Math.max(typeLen, map_type2str(complex.getValtype()).length());
                    requestableLen = Math.max(requestableLen, (complex.getRequestable()==1) ? 2 : 3); //length of YES, NO
                    consumableLen = Math.max(consumableLen, (complex.isConsumable()) ? 3 : 2); //length of YES, NO);
                    defaultLen = Math.max(defaultLen, complex.getDefault().length());
                    urgencyLen = Math.max(urgencyLen, complex.getUrgencyWeight().length());
                }
                //Now format the columns
                String header0 = Format.left("#name", nameLen+4) + Format.left("shortcut", shortcutLen+4) +
                                 Format.left("type", typeLen+4) + Format.left("relop", relopLen+4) +
                                 Format.left("requestable", requestableLen+9) + Format.left("consumable", consumableLen+8) +
                                 Format.left("default", defaultLen+4) + Format.left("urgency", urgencyLen+4);
                StringBuffer header1 = new StringBuffer("#");
                for (int j=0; j<header0.length()-1; j++) {
                    header1.append("-");
                }
                pw.println(header0);
                pw.println(header1);
                String val;
                //And finally print the columns
                for (Iterator iter=complexList.iterator(); iter.hasNext();) {
                    ComplexEntry complex = (ComplexEntry)iter.next();
                    val = Format.left(complex.getName(), nameLen+4) + Format.left(complex.getShortcut(), shortcutLen+4) +
                          Format.left(map_type2str(complex.getValtype()),typeLen+4) + 
                          Format.left(map_op2str(complex.getRelop()), relopLen+4) +
                          Format.left(map_req2str(complex.getRequestable()), requestableLen+9) +  //1 is NO
                          Format.left((complex.isConsumable()) ? "YES" : "NO", consumableLen+8) +
                          Format.left(complex.getDefault(), defaultLen+4) + complex.getUrgencyWeight();
                    pw.println(val);
                }
                pw.println("# >#< starts a comment but comments are not saved across edits --------");
                pw.flush();
                break;
            } else if (args[i].equals("-sds")) {
                pw.println(jgdi.showDetachedSettingsAll());
                pw.flush();
                break;
            } else if (args[i].equals("-secl")) {
                List evcl = jgdi.getEventClientList();
                if (evcl.size() > 0) {
                    OutputTable table = new OutputTable(com.sun.grid.jgdi.configuration.EventClient.class);
                    table.addCol("id", "ID", 8, OutputTable.Column.RIGHT);
                    table.addCol("name", "Name", 15, OutputTable.Column.LEFT);
                    table.addCol("host", "HOST", 25, OutputTable.Column.LEFT);
                    table.printHeader(pw);
                    table.printDelimiter(pw, '-');
                    Iterator iter = evcl.iterator();
                    while (iter.hasNext()) {
                        com.sun.grid.jgdi.configuration.EventClient evc = (com.sun.grid.jgdi.configuration.EventClient)iter.next();
                        table.printRow(pw, evc);
                    }
                } else {
                    pw.println("no event clients registered");
                }
                pw.flush();
                break;
            } else if (args[i].equals("-help")) {
                System.out.println(getUsage());
            } else if (args[i].startsWith("-ke") ) {
                boolean terminateJobs = args[i].endsWith("j");
                i++;
                if(i>= args.length) {
                    throw new IllegalArgumentException("missing host_list");
                }
                if(args[i].equals("all")) {
                    jgdi.killAllExecds(terminateJobs);
                } else {
                    String hosts [] = args[i].split(" ");
                    jgdi.killExecd(hosts, terminateJobs);
                }
                break;
            } else {
                throw new IllegalArgumentException("Unknown or not implemented option " + args[i]);
            }
        }
    }
    
    //TODO LP better to have it in ComplexEntry itself
    private String map_type2str(int type) {
       if (type < 1 || type >= typev.length) {
          type = 0;
       }
       return typev[type];
    }
    
    private String map_op2str(int op) {    
       if (op < 1 || op >= ropv.length) {
          op = 0;
       }
       return ropv[op];
    }
    private String map_req2str(int op) {
       if (op < 1 || op >= fopv.length) {
          op = 0;
       }
       return fopv[op];
    }


    private String [] parseDestinIdList(String arg) {
        String [] ret = arg.split(" ");
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
       }
       else {
          editor = System.getenv("EDITOR");
       }
       System.out.println("Version = "+version+" prop=\""+System.getProperty("EDITOR")+"\" editor=\""+editor+"\"");
       if (editor ==null) {
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
             List list = Arrays.asList( new String[] {"xterm", "-e", editor, f.getAbsolutePath()});
             cmds = list.toArray();
          } else {
             List list = Arrays.asList(new String[] {editor, f.getAbsolutePath()});
             cmds = list.toArray();
          }
          
          Process p = Runtime.getRuntime().exec((String[])cmds);
          //TODO LP For some reason doesn't wait until the editor exits.
          int exitCode = p.waitFor();
          if (exitCode != 0) {
             return null;
          }
          char[] buff = new char[1024];
          FileReader fr = new FileReader(f);
          int readChars=0;
          while (fr.ready() && readChars != -1) {
             readChars = fr.read(buff);
             if (readChars > 0) {
                sb.append(buff,0,readChars);
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
             if (o1 ==null) {
                return -1;
             }
             if (o2 == null) {
                return 1;
             }
             return ((GEObject)o1).getName().compareTo(((GEObject)o2).getName());
          }
       });
       return list;
    }
    
    private void addNewGEObject(JGDI jgdi, String type, List args, PrintWriter pw) {
       Object obj;
       Class cls;
       String name = (args.size() == 2) ? (String)args.get(1) : "template";
       if (args.size()>2) {
          pw.println("error: invalid option argument \"" + args.get(2) + "\"");
          pw.println("Usage: qconf -help");
          pw.flush();
          return;
       }
       try {
          cls = Class.forName("com.sun.grid.jgdi.configuration." + type + "Impl");
          Constructor c = cls.getConstructor(new Class[] {boolean.class});
          obj = c.newInstance(new Object[] {Boolean.TRUE});
          if (!(obj instanceof GEObject)) {
             throw new IllegalAccessException("Class for type "+type+" is not an instance of GEObject");
          }
          GEObject geObj = (GEObject)obj;
          Method setName = obj.getClass().getDeclaredMethod("setName", new Class[] {String.class});
          setName.invoke(geObj, new Object[] {name});
          String userTypedText = runEditor(GEObjectEditor.getConfigurablePropertiesAsText(geObj));
          GEObjectEditor.updateObjectWithText(jgdi, geObj, userTypedText);
          Class paramType = Class.forName("com.sun.grid.jgdi.configuration." + type);
          Method m = JGDI.class.getDeclaredMethod("add"+type, new Class[] {paramType});
          m.invoke(jgdi, new Object[] {geObj});
          pw.println(jgdi.getAdminUser()+"@"+java.net.InetAddress.getLocalHost().getHostName()+" added \""+geObj.getName()+"\" to "+type.toLowerCase()+" list");
          pw.flush();
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
       }
       pw.flush();
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
       String option = (String)args.get(0);
       if (!option.startsWith("-A")) {
          throw new IllegalArgumentException("Expected -A... argument got: "+option);
       }
       String fileName = (String)args.get(1);
       String keyAttrValue = null;
       try {
          cls = Class.forName("com.sun.grid.jgdi.configuration." + type + "Impl");
          Constructor c = cls.getConstructor(new Class[] {boolean.class});
          obj = c.newInstance(new Object[] {Boolean.TRUE});
          if (!(obj instanceof GEObject)) {
             throw new IllegalAccessException("Class for type "+type+" is not an instance of GEObject");
          }
          String inputText = readFile(fileName);
          keyAttrValue = getKeyAttributeValueFromString(pw, type, fileName, inputText);
          if (keyAttrValue == null) {
             return;
          }
          GEObject geObj = (GEObject)obj;
          Method setName = obj.getClass().getDeclaredMethod("setName", new Class[] {String.class});
          setName.invoke(obj, new Object[] {keyAttrValue});
          GEObjectEditor.updateObjectWithText(jgdi, geObj, inputText);
          Class paramType = Class.forName("com.sun.grid.jgdi.configuration." + type);
          Method m = JGDI.class.getDeclaredMethod("add"+type, new Class[] {paramType});
          m.invoke(jgdi, new Object[] {obj});
          pw.println(jgdi.getAdminUser()+"@"+java.net.InetAddress.getLocalHost().getHostName()+" added \""+geObj.getName()+"\" to "+type.toLowerCase()+" list");
          pw.flush();
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
          pw.println("error: error opening file \""+fileName+"\" for reading: "+ex.getMessage());
          pw.println("error reading in file");
          pw.flush();
       }
    }
       
    private void modifyGEObject(JGDI jgdi, String type, List args, PrintWriter pw) {
       Object obj;
             
       Class cls;
       if (args.size() <= 1) {
          pw.println("error: missing "+type.toLowerCase()+" name");
          pw.flush();
          return;
       }
       if (args.size() != 2) {
          pw.println("error: invalid option argument \"" + args.get(2) + "\"");
          pw.println("Usage: qconf -help");
          pw.flush();
          return;
       }
       String name = (String)args.get(1);
       try {
          cls = Class.forName("com.sun.grid.jgdi.configuration." + type + "Impl");
          Method getGEObj = JGDI.class.getDeclaredMethod("get"+type, new Class[] {String.class});
          obj = getGEObj.invoke(jgdi, new Object[] {name});
          if (obj == null) {
              pw.println(name+" is not known as "+type);
              pw.flush();
              return;
          }
          GEObject geObj = (GEObject)obj;
          String userTypedText = runEditor(GEObjectEditor.getConfigurablePropertiesAsText(geObj));
          GEObjectEditor.updateObjectWithText(jgdi, geObj, userTypedText);
          Class paramType = Class.forName("com.sun.grid.jgdi.configuration." + type);
          Method m = JGDI.class.getDeclaredMethod("update"+type, new Class[] {paramType});
          m.invoke(jgdi, new Object[] {geObj});
          pw.println(jgdi.getAdminUser()+"@"+java.net.InetAddress.getLocalHost().getHostName()+" modified \""+geObj.getName()+"\" in "+type.toLowerCase()+" list");
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
       } catch (UnknownHostException ex) {
          ex.printStackTrace();
       } catch (JGDIException ex) {
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
          pw.println("Usage: qconf -help");
          pw.flush();
          return;
       }
       String option = (String)args.get(0);
       if (!option.startsWith("-M")) {
          throw new IllegalArgumentException("Expected -M... argument got: "+option);
       }
       String fileName = (String)args.get(1);
       String keyAttrValue = null;
       try {
          String inputText = readFile(fileName);
          keyAttrValue = getKeyAttributeValueFromString(pw, type, fileName, inputText);
          if (keyAttrValue == null) {
             return;
          }
          //Lookup the object based on the key atrribute value (name)
          Method getGEObj = JGDI.class.getDeclaredMethod("get"+type, new Class[] {String.class});
          obj = getGEObj.invoke(jgdi, new Object[] {keyAttrValue});
          if (obj == null) {
             pw.println(keyAttrValue+" is not known as "+type);
             pw.flush();
             return;
          }
          //And try to update the object based on the inputText
          GEObject geObj = (GEObject)obj;
          GEObjectEditor.updateObjectWithText(jgdi, (GEObject) obj, inputText);
          Class paramType = Class.forName("com.sun.grid.jgdi.configuration." + type);
          Method m = JGDI.class.getDeclaredMethod("update"+type, new Class[] {paramType});
          m.invoke(jgdi, new Object[] {obj});
          pw.println(jgdi.getAdminUser()+"@"+java.net.InetAddress.getLocalHost().getHostName()+" modified \""+geObj.getName()+"\" in "+type.toLowerCase()+" list");
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
       } catch (UnknownHostException ex) {
          ex.printStackTrace();
       } catch (JGDIException ex) {
          ex.printStackTrace();
       } catch (IOException ex) {
          pw.println("error: error opening file \""+fileName+"\" for reading: "+ex.getMessage());
          pw.println("error reading in file");
          pw.flush();
       }
    }
    
    private void showGEObjectList(JGDI jgdi, String type, List args, PrintWriter pw) {
       Object obj;
       Class cls;
       if (args.size() > 1) {
          pw.println("error: invalid option argument \"" + args.get(1) + "\"");
          pw.println("Usage: qconf -help");
          pw.flush();
          return;
       }
       try {
          Class paramType = Class.forName("com.sun.grid.jgdi.configuration." + type);
          Method getGEObjList = JGDI.class.getDeclaredMethod("get"+type+"List", null);
          List list = sortListByName((List)getGEObjList.invoke(jgdi, null));
          if (list.size() == 0) {
             pw.println("no "+type.toLowerCase()+" defined");
             pw.flush();
             return;
          }
          for (Iterator iter = list.iterator(); iter.hasNext(); ) {
             obj = iter.next();
             Method getName = obj.getClass().getDeclaredMethod("getName", null);
             pw.println(getName.invoke(obj, null));
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
    
    private void showGEObject(JGDI jgdi, String type, List args, PrintWriter pw) {
       Object obj;
       Class cls;
       if (args.size() <= 1) {
          pw.println("error: missing "+type.toLowerCase()+"_list");
          pw.flush();
          return;
       }
       try {
          List strList = new ArrayList();
          List errList = new ArrayList();
          cls = Class.forName("com.sun.grid.jgdi.configuration." + type + "Impl");
          Method getGEObj = JGDI.class.getDeclaredMethod("get"+type, new Class[] {String.class});
          for (int i=1; i < args.size(); i++) {
             String name = (String)args.get(i);
             String[] names = name.split(",");
             for (int j=0; j < names.length; j++) {
                if (names[j].length()==0) {
                   continue;
                }
                obj = getGEObj.invoke(jgdi, new Object[] {names[j]});
                if (obj == null) {
                   String text = names[j]+" is not known as "+type;
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
          if (errList.size()>0) {
             for (Iterator iter = errList.iterator(); iter.hasNext();) {
                pw.println((String)iter.next());
             }
          } else {
             int i=0;
             pw.print((String)strList.get(i));
             for (i=1; i < strList.size(); i++) {
                pw.println();
                pw.print((String)strList.get(i));
             }
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
          pw.println("error: missing "+type.toLowerCase()+"_list");
          pw.flush();
          return;
       }
       try {
          cls = Class.forName("com.sun.grid.jgdi.configuration." + type + "Impl");
          Method getGEObj = JGDI.class.getDeclaredMethod("get"+type, new Class[] {String.class});
          Class paramType = Class.forName("com.sun.grid.jgdi.configuration." + type);
          Method deleteGEObj = JGDI.class.getDeclaredMethod("delete"+type, new Class[] {paramType});
          List objList = new ArrayList();
          List errList = new ArrayList();
          for (int i=1; i < args.size(); i++) {
             String name = (String)args.get(i);
             String[] names = name.split(",");
             for (int j=0; j < names.length; j++) {
                if (names[j].length()==0) {
                   continue;
                }
                obj = getGEObj.invoke(jgdi, new Object[] {names[j]});
                if (obj == null) {
                   String text = names[j]+" is not known as "+type;
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
          if (errList.size()>0) {
             for (Iterator iter = errList.iterator(); iter.hasNext();) {
                pw.println((String)iter.next());
                pw.flush();
             }
          } else {
             int i;
             for (Iterator iter = objList.iterator(); iter.hasNext();) {
                GEObject delObj = (GEObject) iter.next();
                deleteGEObj.invoke(jgdi, new Object[] {delObj});
                try {
                   pw.println(jgdi.getAdminUser()+"@"+java.net.InetAddress.getLocalHost().getHostName()+" removed \""+delObj.getName()+"\" from "+type.toLowerCase()+" list");
                   pw.flush();
                } catch (UnknownHostException ex) {
                   ex.printStackTrace();
                }
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
          throw new IOException("Unable to read whole file content. Filesize is "+fileSize+" got only "+sb.length());
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
          pw.println("error: required attribute \""+keyAttr+"\" is missing");
          pw.println(type+" file \""+fileName+"\" is not correct");
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
          for (int i=str.length(); i<n ; i++) {
             sb.append(' ');
          }
          return sb.toString();
       }
    }
}