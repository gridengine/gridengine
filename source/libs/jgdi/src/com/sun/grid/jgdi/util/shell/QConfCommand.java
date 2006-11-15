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
import com.sun.grid.jgdi.util.OutputTable;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.Iterator;
import java.util.List;

/**
 *
 * @author rh150277
 */
public class QConfCommand extends AbstractCommand {
    
    
    /** Creates a new instance of QModCommand */
    public QConfCommand(Shell shell, String name) {
        super(shell, name);
    }
    
    public String getUsage() {
        StringWriter sw = new StringWriter();
        PrintWriter pw = new PrintWriter(sw);
        pw.println("usage: qconf [options]");
//      pw.println("   [-aattr obj_nm attr_nm val obj_id_lst]   add to a list attribute of an object");
//      pw.println("   [-Aattr obj_nm fname obj_id_lst]         add to a list attribute of an object");
//      pw.println("   [-acal calendar_name]                    add a new calendar");
//      pw.println("   [-Acal fname]                            add a new calendar from file");
//      pw.println("   [-ackpt ckpt_name]                       add a ckpt interface definition");
//      pw.println("   [-Ackpt fname]                           add a ckpt interface definition from file");
//      pw.println("   [-aconf host_list]                       add configurations");
//      pw.println("   [-Aconf file_list]                       add configurations from file_list");
//      pw.println("   [-ae [exec_server_template]]             add an exec host using a template");
//      pw.println("   [-Ae fname]                              add an exec host from file");
//      pw.println("   [-ah hostname]                           add an administrative host");
//      pw.println("   [-ahgrp group]                           add new host group entry");
//      pw.println("   [-Ahgrp file]                            add new host group entry from file");
//      pw.println("   [-alrs [lirs_list]]                      add limitation rule set(s)");
//      pw.println("   [-Alrs fname]                            add limitation rule set(s) from file");
//      pw.println("   [-am user_list]                          add user to manager list");
//      pw.println("   [-ao user_list]                          add user to operator list");
//      pw.println("   [-ap pe-name]                            add a new parallel environment");
//      pw.println("   [-Ap fname]                              add a new parallel environment from file");
//      pw.println("   [-aprj]                                  add project");
//      pw.println("   [-Aprj fname]                            add project from file");
//      pw.println("   [-aq ]                                   add a new cluster queue");
//      pw.println("   [-Aq fname]                              add a queue from file");
//      pw.println("   [-as hostname]                           add a submit host");
//      pw.println("   [-astnode node_shares_list]              add sharetree node(s)");
//      pw.println("   [-astree]                                create/modify the sharetree");
//      pw.println("   [-Astree fname]                          create/modify the sharetree from file");
//      pw.println("   [-au user_list listname_list]            add user(s) to userset list(s)");
//      pw.println("   [-Au fname]                              add userset from file");
//      pw.println("   [-auser]                                 add user");
//      pw.println("   [-Auser fname]                           add user from file");
        pw.println("   [-clearusage]                            clear all user/project sharetree usage");
        pw.println("   [-cq destin_id_list]                     clean queue");
//      pw.println("   [-dattr obj_nm attr_nm val obj_id_lst]   delete from a list attribute of an object");
//      pw.println("   [-Dattr obj_nm fname obj_id_lst]         delete from a list attribute of an object");
//      pw.println("   [-dcal calendar_name]                    remove a calendar");
//      pw.println("   [-dckpt ckpt_name]                       remove a ckpt interface definition");
//      pw.println("   [-dconf host_list]                       delete local configurations");
//      pw.println("   [-de host_list]                          remove an exec server");
//      pw.println("   [-dh host_list]                          remove an administrative host");
//      pw.println("   [-dhgrp group]                           delete host group entry");
//      pw.println("   [-dlrs lirs_list]                        delete limitation rule set(s)");
//      pw.println("   [-dm user_list]                          remove user from manager list");
//      pw.println("   [-do user_list]                          remove user from operator list");
//      pw.println("   [-dp pe-name]                            remove a parallel environment");
//      pw.println("   [-dprj project_list]                     delete project");
//      pw.println("   [-dq destin_id_list]                     remove a queue");
//      pw.println("   [-ds host_list]                          remove submit host");
//      pw.println("   [-dstnode node_list]                     remove sharetree node(s)");
//      pw.println("   [-dstree]                                delete the sharetree");
//      pw.println("   [-du user_list listname_list]            remove user(s) from userset list(s)");
//      pw.println("   [-dul listname_list]                     remove userset list(s) completely");
//      pw.println("   [-duser user_list]                       delete user");
//      pw.println("   [-help]                                  print this help");
        pw.println("   [-ke[j] host_list                        shutdown execution daemon(s)");
        pw.println("   [-k{m|s}]                                shutdown master|scheduling daemon");
        pw.println("   [-kec evid_list]                         kill event client");
//      pw.println("   [-mattr obj_nm attr_nm val obj_id_lst]   modify an attribute (or element in a sublist) of an object");
//      pw.println("   [-Mattr obj_nm fname obj_id_lst]         modify an attribute (or element in a sublist) of an object");
//      pw.println("   [-mc ]                                   modify complex attributes");
//      pw.println("   [-mckpt ckpt_name]                       modify a ckpt interface definition");
//      pw.println("   [-Mc fname]                              modify complex attributes from file");
//      pw.println("   [-mcal calendar_name]                    modify calendar");
//      pw.println("   [-Mcal fname]                            modify calendar from file");
//      pw.println("   [-Mckpt fname]                           modify a ckpt interface definition from file");
//      pw.println("   [-mconf [host_list|global]]              modify configurations");
//      pw.println("   [-msconf]                                modify scheduler configuration");
//      pw.println("   [-Msconf fname]                          modify scheduler configuration from file");
//      pw.println("   [-me server]                             modify exec server");
//      pw.println("   [-Me fname]                              modify exec server from file");
//      pw.println("   [-mhgrp group]                           modify host group entry");
//      pw.println("   [-Mhgrp file]                            modify host group entry from file");
//      pw.println("   [-mlrs [lirs_list]]                      modify limitation rule set(s)");
//      pw.println("   [-Mlrs fname]                            modify limitation rule set(s) from file");
//      pw.println("   [-mp pe-name]                            modify a parallel environment");
//      pw.println("   [-Mp fname]                              modify a parallel environment from file");
//      pw.println("   [-mprj project]                          modify a project");
//      pw.println("   [-Mprj fname]                            modify project from file");
//      pw.println("   [-mq queue]                              modify a queue");
//      pw.println("   [-Mq fname]                              modify a queue from file");
//      pw.println("   [-mstnode node_shares_list]              modify sharetree node(s)");
//      pw.println("   [-Mstree fname]                          modify/create the sharetree from file");
//      pw.println("   [-mstree]                                modify/create the sharetree");
//      pw.println("   [-mu listname_list]                      modify the given userset list");
//      pw.println("   [-Mu fname]                              modify userset from file");
//      pw.println("   [-muser user]                            modify a user");
//      pw.println("   [-Muser fname]                           modify a user from file");
//      pw.println("   [-purge obj_nm3 attr_nm objectname]      removes attribute from object_instance");
//      pw.println("   [-rattr obj_nm attr_nm val obj_id_lst]   replace a list attribute of an object");
//      pw.println("   [-Rattr obj_nm fname obj_id_lst]         replace a list attribute of an object");
//      pw.println("   [-sc ]                                   show complex attributes");
//      pw.println("   [-scal calendar_name]                    show given calendar");
//      pw.println("   [-scall]                                 show a list of all calendar names");
//      pw.println("   [-sckpt ckpt_name]                       show ckpt interface definition");
//      pw.println("   [-sckptl]                                show all ckpt interface definitions");
//      pw.println("   [-sconf [host_list|global]]              show configurations");
//      pw.println("   [-sconfl]                                show a list of all local configurations");
//      pw.println("   [-se server]                             show given exec server");
//      pw.println("   [-secl]                                  show event client list");
//      pw.println("   [-sel]                                   show a list of all exec servers");
//      pw.println("   [-sep]                                   show a list of all licensed processors");
//      pw.println("   [-sh]                                    show a list of all administrative hosts");
//      pw.println("   [-shgrp group]                           show host group");
//      pw.println("   [-shgrp_tree group]                      show host group and used hostgroups as tree");
//      pw.println("   [-shgrp_resolved group]                  show host group with resolved hostlist");
//      pw.println("   [-shgrpl]                                show host group list");
        pw.println("   [-sds]                                   show detached settings");
//      pw.println("   [-slrs [lirs_list]]                      show limitation rule set(s)");
//      pw.println("   [-slrsl]                                 show limitation rule set list");
//      pw.println("   [-sm]                                    show a list of all managers");
//      pw.println("   [-so]                                    show a list of all operators");
//      pw.println("   [-sobjl obj_nm2 attr_nm val]             show objects which match the given value");
//      pw.println("   [-sp pe-name]                            show a parallel environment");
//      pw.println("   [-spl]                                   show all parallel environments");
//      pw.println("   [-sprj project]                          show a project");
//      pw.println("   [-sprjl]                                 show a list of all projects");
//      pw.println("   [-sq [destin_id_list]]                   show the given queue");
//      pw.println("   [-sql]                                   show a list of all queues");
//      pw.println("   [-ss]                                    show a list of all submit hosts");
//      pw.println("   [-sss]                                   show scheduler state");
//      pw.println("   [-ssconf]                                show scheduler configuration");
//      pw.println("   [-sstnode node_list]                     show sharetree node(s)");
//      pw.println("   [-rsstnode node_list]                    show sharetree node(s) and its children");
//      pw.println("   [-sstree]                                show the sharetree");
//      pw.println("   [-su listname_list]                      show the given userset list");
//      pw.println("   [-suser user_list]                       show user(s)");
//      pw.println("   [-sul]                                   show a list of all userset lists");
//      pw.println("   [-suserl]                                show a list of all users");
        pw.println("   [-tsm]                                   trigger scheduler monitoring");
//      pw.println("complex_list            complex[,complex,...]");
        pw.println("destin_id_list          queue[ queue ...]");
//      pw.println("listname_list           listname[,listname,...]");
//      pw.println("lirs_list               lirs_name[,lirs_name,...]");        
//      pw.println("node_list               node_path[,node_path,...]");
//      pw.println("node_path               [/]node_name[[/.]node_name...]");
//      pw.println("node_shares_list        node_path=shares[,node_path=shares,...]");
//      pw.println("user_list               user|pattern[,user|pattern,...]");
//      pw.println("obj_nm                  \"queue\"|\"exechost\"|\"pe\"|\"ckpt\"|\"hostgroup\"");
//      pw.println("attr_nm                 (see man pages)");
//      pw.println("obj_id_lst              objectname [ objectname ...]");
//      pw.println("project_list            project[,project,...]");
        pw.println("evid_list               all | evid[,evid,...]");
        pw.println("host_list               all | hostname[,hostname,...]");
//      pw.println("obj_nm2                 \"queue\"|\"queue_domain\"|\"queue_instance\"|\"exechost\"");
//      pw.println("obj_nm3                 \"queue\"");
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
    
    
    private String [] parseDestinIdList(String arg) {
        String [] ret = arg.split(" ");
        return ret;
    }
    
    
    
}
