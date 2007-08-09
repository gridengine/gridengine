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
import com.sun.grid.jgdi.configuration.AbstractManager;
import com.sun.grid.jgdi.configuration.ComplexEntryImpl;
import com.sun.grid.jgdi.configuration.GEObject;
import com.sun.grid.jgdi.configuration.ComplexEntry;
import com.sun.grid.jgdi.configuration.Hostgroup;
import com.sun.grid.jgdi.configuration.ShareTree;
import com.sun.grid.jgdi.util.shell.editor.EditorParser;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.ResourceBundle;

/**
 *
 */
public class QConfCommand extends AbstractCommand {
   
   /*private static Map typeConvertor;
   static {
      typeConvertor = new HashMap();
      typeConvertor.put("checkpoint", "ckpt interface definition");
      typeConvertor.put("resourcequotaset", "resource quota set list");
      typeConvertor.put("project", "project list");
      typeConvertor.put("user", "user list");
   };
    
   private static Map addRemoveTypeConvertor;
   static {
       addRemoveTypeConvertor = new HashMap();
       addRemoveTypeConvertor.put("clusterqueue","cluster queue");
       addRemoveTypeConvertor.put("hostgroup","host group");
   }*/
   
   private static final Map optMap; /* Map holding all qconf options and method that should be invoked for it */
   
   /* Initialize the option map optMap
    * Only options in the map will be recognized as implemented */
   static {
      optMap = new HashMap(140);
      MapInit init = new MapInit();
      try {
         init.addSingleMethod("-help", 0, 0, init.CLS, init.CLS.getDeclaredMethod("printUsage", new Class[] { PrintWriter.class }));
         //Add default methods
         init.addDefaultMethodsForType("Calendar");
         init.addDefaultMethodsForType("Checkpoint");
         init.addDefaultMethodsForType("ClusterQueue");
         init.addDefaultMethodsForType("ExecHost");
         init.addDefaultMethodsForType("ParallelEnvironment");
         init.addDefaultMethodsForType("Project");
         init.addDefaultMethodsForType("ResourceQuotaSet"); //TODO LP: check if working
         init.addDefaultMethodsForType("User");
         
         //Special cases
         Class cls;
         Class[] argDefault = init.ARG_CLS;
         Class[] argJP = new Class[] { JGDI.class, PrintWriter.class };
         Class[] argJLP = new Class[] { JGDI.class, List.class, PrintWriter.class };
         
         //CONFIGURATION - no -Mconf option
         init.addDefaultMethodsForType("Configuration");
         optMap.remove("-Mconf"); //TODO LP: Why not allow it?
         
         //MANAGER
         cls = AbstractUserQConfOption.class;
         init.addSingleMethod("Manager", "-am", 1, Integer.MAX_VALUE, cls, cls.getDeclaredMethod("add", argDefault));
         init.addSingleMethod("Manager", "-dm", 1, Integer.MAX_VALUE, cls, cls.getDeclaredMethod("delete", argDefault));
         init.addSingleMethod("Manager", "-sm", 0, 0, init.CLS, init.CLS.getDeclaredMethod("showList", argDefault));
         //OPERATOR
         init.addSingleMethod("Operator", "-ao", 1, Integer.MAX_VALUE, cls, cls.getDeclaredMethod("add", argDefault));
         init.addSingleMethod("Operator", "-do", 1, Integer.MAX_VALUE, cls, cls.getDeclaredMethod("delete", argDefault));
         init.addSingleMethod("Operator", "-so", 0, 0, init.CLS, init.CLS.getDeclaredMethod("showList", argDefault));
         
         //COMPLEX ENTRY
         cls = ComplexEntryQConfOption.class;
         init.addSingleMethod("-mc", 0, 0, cls, cls.getDeclaredMethod("modify", argJLP));
         init.addSingleMethod("-Mc", 1, 0, cls, cls.getDeclaredMethod("modifyFromFile", argJLP));
         init.addSingleMethod("-sc", 0, 0, cls, cls.getDeclaredMethod("show", argJLP));
         //HOSTGROUP
         cls = HostgroupQConfOption.class;
         init.addDefaultMethodsForType("Hostgroup");
         init.addSingleMethod("-shgrp_tree", 1, 0, cls, cls.getDeclaredMethod("showTree", argJLP));
         init.addSingleMethod("-shgrp_resolved", 1, 0, cls, cls.getDeclaredMethod("showResolved", argJLP));
         
         //ADMIN HOST
         cls = AdminHostQConfOption.class;
         init.addSingleMethod("-ah", 1, Integer.MAX_VALUE, cls, cls.getDeclaredMethod("add", argJLP));
         init.addSingleMethod("-sh", 0, 0, cls, cls.getDeclaredMethod("showList", argJLP));
         init.addSingleMethod("-dh", 1, Integer.MAX_VALUE, cls, cls.getDeclaredMethod("delete", argJLP));         
         //SUBMIT HOST
         cls = SubmitHostQConfOption.class;
         init.addSingleMethod("-as", 1, Integer.MAX_VALUE, cls, cls.getDeclaredMethod("add", argJLP));
         init.addSingleMethod("-ss", 0, 0, cls, cls.getDeclaredMethod("showList", argJLP));
         init.addSingleMethod("-ds", 1, Integer.MAX_VALUE, cls, cls.getDeclaredMethod("delete", argJLP));
         
         //USER SET
         cls = UserSetQConfOption.class;
         init.addDefaultMethodsForType("UserSet");
         optMap.remove("-au");
         optMap.remove("-du");
         init.addSingleMethod("-au", 2, 0, cls, cls.getDeclaredMethod("add", argJLP));
         init.addSingleMethod("-du", 2, 0, cls, cls.getDeclaredMethod("deleteUser", argJLP));
         cls = init.CLS;
         init.addSingleMethod("UserSet", "-dul", 1, Integer.MAX_VALUE, cls, cls.getDeclaredMethod("delete", argDefault));
         //SCHEDULER CONFIGURATION
         init.addSingleMethod("SchedConf", init.MODIFY, 0, 0, cls, cls.getDeclaredMethod("modify", argDefault));
         init.addSingleMethod("SchedConf", init.MODIFY_FROM_FILE, 1, 0, cls, cls.getDeclaredMethod("modifyFromFile", argDefault));
         init.addSingleMethod("SchedConf", "-ssconf", 0, 0, cls, cls.getDeclaredMethod("showWithNoArgs", argDefault));
         
         //SHARETREE
         cls = ShareTreeQConfOption.class;
         init.addSingleMethod("-astree", 0, 0, cls, cls.getDeclaredMethod("add", argJP));
         init.addSingleMethod("-Astree", 1, 0, cls, cls.getDeclaredMethod("addFromFile", argJLP));
         init.addSingleMethod("-mstree", 0, 0, cls, cls.getDeclaredMethod("modify", argJP));
         init.addSingleMethod("-Mstree", 1, 0, cls, cls.getDeclaredMethod("modifyFromFile", argJLP));
         init.addSingleMethod("-sstree", 0, 0, cls, cls.getDeclaredMethod("show", argJP));
         init.addSingleMethod("-dstree", 0, 0, cls, cls.getDeclaredMethod("delete", argJP));
         
         //LP TODO: share tree node, *attr options
         
         //OTHER OPTIONS
         cls = OtherQConfOptions.class;
         init.addSingleMethod("-tsm", 0, 0, cls, cls.getDeclaredMethod("triggerSchedulerMonitoring", argJP));
         init.addSingleMethod("-clearusage", 0, 0, cls, cls.getDeclaredMethod("clearUsage", argJP));
         init.addSingleMethod("-cq", 1, Integer.MAX_VALUE, cls, cls.getDeclaredMethod("cleanQueue", argJLP));
         init.addSingleMethod("-kec", 1, Integer.MAX_VALUE, cls, cls.getDeclaredMethod("killEventClient", argJLP));
         init.addSingleMethod("-km", 0, 0, cls, cls.getDeclaredMethod("killMaster", argJP));
         init.addSingleMethod("-ks", 0, 0, cls, cls.getDeclaredMethod("killScheduler", argJP));
         init.addSingleMethod("-sds", 0, 0, cls, cls.getDeclaredMethod("showDetachedSettings", argJP));
         init.addSingleMethod("-secl", 0, 0, cls, cls.getDeclaredMethod("showEventClientList", argJP));
         init.addSingleMethod("-sep", 0, 0, cls, cls.getDeclaredMethod("showProcessors", argJP));
         init.addSingleMethod("-sss", 0, 0, cls, cls.getDeclaredMethod("showSchedulerState", argJP));
         init.addSingleMethod("-ke", 1, Integer.MAX_VALUE, cls, cls.getDeclaredMethod("killExecd", argJLP));
         init.addSingleMethod("-kej", 1, Integer.MAX_VALUE, cls, cls.getDeclaredMethod("killExecdWithJobs", argJLP));
      } catch (Exception ex) {
         throw new ExceptionInInitializerError(new Exception("<QConfCommand> failed: "+ex.getMessage()));
      }
   }
   
   
   /** Creates a new instance of QConfCommand */
   public QConfCommand(Shell shell, String name, ResourceBundle jgdiResource) {
      super(shell, name, jgdiResource);
   }
   
   public QConfCommand() {
      super(null,null,null);
   }
   
   public String getUsage() {
      //return getResourceString("qconf.usage");
      //TODO LP: Get qconf usage from resource bundle
      return "NOT IMPLEMENTED";
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
      
      List argList = new ArrayList();
      String arg;
      //Expand args to list of args, 'arg1,arg2 arg3' -> 3 args
      for (int i=0; i<args.length; i++) {
         arg = args[i];
         String[] subElems = arg.split("[,]");
         for (int j=0; j< subElems.length; j++) {
            arg = subElems[j].trim();
            if (arg.length() > 0) {
               argList.add(arg);
            }
         }
      }
      
      OptionInfo info ;
      try {
         while (!argList.isEmpty()) {
            //Get option info
            info = QConfOption.getOptionInfo(optMap, argList);
            info.invokeOption(jgdi, pw);
         }
      } catch (OptionFormatException ex) {
         pw.println(ex.getMessage());
         pw.flush();
      }
   }
   
    /*private String convertAddRemoveTypeToMessage(String type) {
        type = type.toLowerCase();
        if (addRemoveTypeConvertor.containsKey(type)) {
            return (String) addRemoveTypeConvertor.get(type);
        }
        return type;
    }*/
   
   /*
    * Helper class to initialize optMap.
    * optMap holds information what method should be called for a specific option string, among other things
    */
   static private class MapInit {
      private final int ADD = 0;
      private final int ADD_FROM_FILE = 1;
      private final int MODIFY = 2;
      private final int MODIFY_FROM_FILE = 3;
      private final int SHOW = 4;
      private final int SHOW_LIST = 5;
      private final int DELETE = 6;
      
      private static final Map nameToOpt; /* Helper map to convert name to option */
      static {
         nameToOpt = new HashMap();
         nameToOpt.put("Calendar","cal");
         nameToOpt.put("Checkpoint","ckpt");
         nameToOpt.put("ClusterQueue","q");
         nameToOpt.put("ExecHost","e");
         nameToOpt.put("Hostgroup","hgrp");
         nameToOpt.put("ParallelEnvironment","p");
         nameToOpt.put("Project","prj");
         nameToOpt.put("ResourceQuotaSet","rqs");
         nameToOpt.put("User","user");
         //Special cases
         nameToOpt.put("Configuration","conf");
         nameToOpt.put("SchedConf","sconf");
         nameToOpt.put("ShareTree","stree");
         nameToOpt.put("UserSet","u");
      }
      
      private final Class CLS = QConfOption.class;
      //Default class arguments - JGDI, Type, List of arguments, PrintWriter
      private final Class[] ARG_CLS = new Class[] {JGDI.class, String.class, List.class, PrintWriter.class};
      
      void addDefaultMethodsForType(String type) throws NoSuchMethodException {
         addSingleMethod(type, ADD, 0, 1, CLS, CLS.getDeclaredMethod("add", ARG_CLS));
         addSingleMethod(type, ADD_FROM_FILE, 1, 0, CLS, CLS.getDeclaredMethod("addFromFile", ARG_CLS));
         addSingleMethod(type, MODIFY, 1, 0, CLS, CLS.getDeclaredMethod("modify", ARG_CLS));
         addSingleMethod(type, MODIFY_FROM_FILE, 1, 0, CLS, CLS.getDeclaredMethod("modifyFromFile", ARG_CLS));
         addSingleMethod(type, SHOW, 1, Integer.MAX_VALUE, CLS, CLS.getDeclaredMethod("show", ARG_CLS));
         addSingleMethod(type, SHOW_LIST, 0, 0, CLS, CLS.getDeclaredMethod("showList", ARG_CLS));
         addSingleMethod(type, DELETE, 1, Integer.MAX_VALUE, CLS, CLS.getDeclaredMethod("delete", ARG_CLS));
      }
      
      void addSingleMethod(String optionStr, int mandatory, int optional, Class optionCls, Method method) {
         addSingleMethod("", optionStr , mandatory, optional, optionCls, method);
      }
      
      void addSingleMethod(String objectType, String optionStr, int mandatory, int optional, Class optionCls, Method method) {
         try {
            optMap.put(optionStr, new OptionDescriptor(objectType,  mandatory, optional, optionCls, method));
         } catch (Exception ex) {
            throw new ExceptionInInitializerError(new Exception("<QConfCommand> failed: "+ex.getMessage()));
         }
      }
      
      void addSingleMethod(String objectType, int methodType, int mandatory, int optional, Class optionCls, Method method) {
         if (!nameToOpt.containsKey(objectType)) {
            throw new ExceptionInInitializerError(new Exception("<QConfCommand> failed: Unknown option type "+objectType));
         }
         String opt = getOptionString(objectType, methodType);
         addSingleMethod(objectType, opt, mandatory, optional, optionCls, method);
      }
      
      private String getOptionString(String objectType, int methodType) {
         if (!nameToOpt.containsKey(objectType)) {
            throw new ExceptionInInitializerError(new Exception("<QConfCommand> failed: Unknown option type "+objectType));
         }
         String opt = (String) nameToOpt.get(objectType);
         switch (methodType) {
            case ADD: return "-a"+opt;
            case ADD_FROM_FILE: return "-A"+opt;
            case MODIFY: return "-m"+opt;
            case MODIFY_FROM_FILE: return "-M"+opt;
            case SHOW: return "-s"+opt;
            case SHOW_LIST: return "-s"+opt+"l";
            case DELETE: return "-d"+opt;
            default:
               throw new ExceptionInInitializerError(new Exception("<QConfCommand> failed: Unknown method type "+methodType));
         }
      }
   }
}