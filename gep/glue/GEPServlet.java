/*___INFO__MARK_BEGIN__*/
/*
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
 */
/*___INFO__MARK_END__*/

package com.sun.gep;

import java.io.*;
import java.util.*;
import java.net.*;
import javax.servlet.*;
import javax.servlet.http.*;

import com.iplanet.sso.*;
import com.iplanet.am.sdk.*;

/*
 * The source code, object code, and documentation in the com.oreilly.servlet
 * package is copyright and owned by Jason Hunter.
 *
 * Copyright (C) 2001 by Jason Hunter <jhunter@servlets.com>.
 * All rights reserved. 
 *
 * Please read http://www.servlets.com/cos/license.html.
 */

import com.oreilly.servlet.MultipartRequest;

/**
 * Grid Engine Portal servlet
 * Sun Technical Computing Portal user servlet
 *
 * @author Frederic Pariente, Sun Microsystems
 * @author Dan Fraser, Sun Microsystems (modifications)
 * @author Eric Sharakan, Sun Microsystems (X11 support; SGE cells)
 * @version 0.97, 2001/11/28, removed directory dependency on "/home",
 *                            generalized the "export" commands
 * @version 0.96, 2001/07/27, added support for X11 Apps, SGE cells
 * @version 0.95, 2001/07/24, removed requirement for SGE MPI queues,
 *                            added capability for dynamic home dir location
 * @version 0.94, 2001/05/15, added usage of $HOME for export variables
 * @version 0.93, 2001/05/15, added support for access list per application
 * @version 0.92, 2001/05/09, exported CODINE_ROOT with user q-commands
 * @version 0.91, 2001/04/26, synchronized access to project list
 * @version 0.90, 2001/04/25
 */

public class GEPServlet extends HttpServlet {
   private String APP_HOME_DIR = "/export/apps/";
   private String SGE_MPRUN = "mpi/MPRUN";
   private String SGE_ROOT = "/export/gridengine/";
   private String SGE_CELL = "default";
   private String SGE_ACCT = "/common/accounting";
   private String COMMD_PORT = "667";
   private String[] SGE_EXPORT = {
      "SGE_ROOT=/export/gridengine",
      "SGE_CELL=default",
      "COMMD_PORT=667",
      "LD_LIBRARY_PATH=/export/gridengine",
      "GEP_ROOT=/gridware/Tools/SGP",
      "PATH=/bin:/usr/bin:/usr/openwin/bin"
   };
   private String SGE_ARCH;
   private String GEP_SU_SCRIPT = ".gep-su";
   private String GEP_LIST = ".gep-list";
   private String GEP_APP = ".gep-app";
   private String GEP_APP_FORM = ".gep-form";
   private String GEP_QSUB_SCRIPT = ".gep-qsub";
   private String GEP_PROJECT_DIR = "/gep/";
   private String GEP_PROJECT = ".gep-project";
   private String GEP_DESKTOP_ATTR = "xdesktop";
   private String X_NETLET = "Xvnc";
   private String X_SERVER;
   private String VNCSERVER = "vncserver";
   private String GEP_ROOT = "/gridware/Tools/SGP";
   private String GETWORKSPACE = "/gridware/Tools/SGP/bin/gethomedir";
   private String ADMINRUN = "adminrun";
   private String xDisplayNum = null;
   private RequestDispatcher reqDisp;
   private LogMonitor monitor;

   // Inner class for receiving iPlanet SSOToken chenge events.  Used to
   // shutdown vncserver when SSOToken no longer valid.
   class SSOTokenChange implements SSOTokenListener {
      public void ssoTokenChanged(SSOTokenEvent se) {
         try {
            SSOToken token = se.getToken();
            String uid;
            int type = se.getType();
            // If SSOToken is history, shutdown vncserver
            if (type == SSOTokenEvent.SSO_TOKEN_DESTROY ||
                type == SSOTokenEvent.SSO_TOKEN_MAX_TIMEOUT ||
                type == SSOTokenEvent.SSO_TOKEN_IDLE_TIMEOUT) {
               AMStoreConnection amConn = new AMStoreConnection(token);
               String userDN = token.getPrincipal().getName();
               AMUser amUser = amConn.getUser(userDN);
               uid = amUser.getStringAttribute("uid");
               startOrStopVNCServer(false, token, null, uid);
            }   
         } catch (Exception sex) {
            // Should never happen when called with a null SSOToken parameter
         }
      }
   }

   // Inner class to log events in TCP
   class LogMonitor {

      protected PrintWriter pw;
      protected FileOutputStream os;
      protected String log_status;
      private boolean echolog = false;

      public LogMonitor() {
         log_status = "Creating logging file.";
         try {
            // create file output stream for appending
            os = new FileOutputStream("/tmp/transactions" , true);
            pw = new PrintWriter(os);
         } catch (Exception e) {
            log_status = "Logging file failed.";
            System.out.println("Unable to open log file.");
         }
      }

      public void setecho(boolean state) { 
         echolog = state; 
      }

      public String status() {
         return log_status;
      }

      public void log(PrintWriter echo, String uid, String type, String event) {
         if ( echolog )
            echo.println(type+":"+uid+":"+event);
         this.log(type,uid,event);
      }

      public void log(String type, String uid, String event) {
         String date;
         Date now = new Date();
         //if ( type.equals("DEBUG") )
         //   return;
         date = now.toString();
         if ( pw == null )
            log_status = "No print writer to log to.";
         else {
            pw.println(date + ":Logging event:"+type+":"+uid+":"+event);
            pw.flush();
         }
      }

      public void log(String event) {
         String date;
         Date now = new Date();
         date = now.toString();
         pw.println(date + ": " + event);
         pw.flush();
      }
   }

   private static String getUser(HttpServletRequest request)
      throws SSOException, AMException
   {
      String uid = null;
      SSOTokenManager manager = SSOTokenManager.getInstance();
      SSOToken token = manager.createSSOToken(request);
      if (manager.getInstance().isValidToken(token)) {
         AMStoreConnection amConn = new AMStoreConnection(token);
         String userDN = token.getPrincipal().getName();
         AMUser amUser = amConn.getUser(userDN);
         uid = amUser.getStringAttribute("uid");
      }
      return uid;
   }   

   private static String getDomain(HttpServletRequest request)
      throws SSOException, AMException
   {
      String domain_name = null;
   /*   
      SSOTokenManager manager = SSOTokenManager.getInstance();
      SSOToken token = manager.createSSOToken(request);
      if (manager.getInstance().isValidToken(token)) {
         AMStoreConnection amConn = new AMStoreConnection(token);
         String userDN = token.getPrincipal().getName();
         AMUser amUser = amConn.getUser(userDN);
         uid = amUser.getStringAttribute("uid");
      }
   */   
      return domain_name;
   }   

   private static void getRoles(HttpServletRequest request, HttpServletResponse response)
      throws SSOException, AMException, IOException
   {
      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
      SSOTokenManager manager = SSOTokenManager.getInstance();
      SSOToken token = manager.createSSOToken(request);
      if (manager.getInstance().isValidToken(token)) {
         AMStoreConnection amConn = new AMStoreConnection(token);
         String userDN = token.getPrincipal().getName();
         AMUser amUser = amConn.getUser(userDN);
         Set roles = amUser.getRoleDNs();
/*          out.println("roles: " + roles); */
      }
   }

   // Inner class for extracting and storing info necessary
   // to submit a job for launch
   class JobInfo {
      // Members directly retrieved from servlet request
      String project;
      String email;
      String input;
      final String output;
      final String error;
      String cpu;

      // Members whose values are derived from the above
      final String projectDirectory;
      String jobName;
      String appName;
      String binary;
      String xApp;
      ArrayList envVars;
      String parallel;

      // Private members used for deriving the above
      private BufferedReader projectInfo;
      private BufferedReader appInfo;

      public JobInfo(HttpServletRequest request, String home, String uid)
         throws FileNotFoundException, IOException {

         project = request.getParameter("project");
         email   = request.getParameter("email");
         input   = request.getParameter("input");
         output  = request.getParameter("output");
         error   = request.getParameter("error");
         cpu     = request.getParameter("cpu");

         //
         // Compute derived values from the project information
         //
         projectDirectory = home + project;
         // ensure readability in place
         // if it fails, let execution continue to show exception
         try {
           chmodFile(uid,"o+rx", projectDirectory);
         }
         catch (Exception e) {}

         projectInfo = new BufferedReader(new FileReader(new File(projectDirectory, GEP_PROJECT)));
         jobName = projectInfo.readLine().replace(' ', '_').replace('\'', '_');

         appName = projectInfo.readLine();
         appInfo = new BufferedReader(new FileReader(new File(APP_HOME_DIR + appName, GEP_APP)));
         appInfo.readLine();
         binary = APP_HOME_DIR + appName + "/" + appInfo.readLine();
         parallel = appInfo.readLine();
         xApp = appInfo.readLine();

         // Build ArrayList of environment variables from application- and
         // project-specific info.
         String line;
         envVars = new ArrayList();
         appInfo.readLine(); appInfo.readLine();
         while ((line = appInfo.readLine()) != null) {
            envVars.add(line);
         }   
         appInfo.close();
         while ((line = projectInfo.readLine()) != null) {
            envVars.add(line);
         }   
         projectInfo.close();
         // enable following line if desire is for added security
         //chmodFile(uid,"o-rx", projectDirectory);
      }
   }

   private void openWorkspace(String uid, String home)
           throws Exception
   // method opens home directory for world read access
   // also opens project workspace to world read access
   {
      chmodFile(uid,"ugo+rx", home+"../");
      chmodFile(uid,"ugo+rx", home);
   }

   private void closeWorkspace(String uid, String home)
           throws Exception
   // method closes home directory to world access
   {
      chmodFile(uid,"go-rx", home+"../");
   }
   

   private void updateProjectList(String home, String name, 
                                  String project, String uid, 
                                  String addordelete)
           throws Exception
   // name is the portal supplied id
   // project is the user defined description
   //
   // update the list of projects for the user
   //
   {
      // copy existing file to /tmp location
      // update file
      // writeback updated file
      monitor.log("MONITOR " + uid + "updateProjectList "+home+" "+name+" "+project+" "+addordelete); 

      String listfile = home + GEP_LIST;    // original list file
      String editlistfile = "/tmp/"+name+GEP_LIST; // temp copy
      String reply = null;

      // create copy in /tmp owned by root for updates
      // make changes
      // copy updated file back to user area owned by user

      try { 
         Vector projects = new Vector();
         chmodFile(uid,"o+r",listfile); // ensure readability
         int status = Runtime.getRuntime().exec("/usr/bin/cp " + 
                              listfile +" "+editlistfile).waitFor();
         if ( status != 0 && !addordelete.equals("ADD")) {
            monitor.log("ERROR "+ uid + "update project - failed copy project list");
            throw new Exception("Failed copy original project list");
         }   

         // omit this statement for now
         // chmodFile(uid,"o-r",listfile);

         // remove the specified project
         // read all but the deleted project into a list
         String line;
         // read from the home location into a vector
         // add only the undeleted projects
         BufferedReader read = new BufferedReader(new FileReader(editlistfile));
         while ((line = read.readLine()) != null) {
            if ( addordelete.equals("DELETE") || addordelete.equals("UPDATE")) {
               if (!((new StringTokenizer(line, "\t")).nextToken().equals(name))) {
                  projects.addElement(line);
               }   
            } else {   
               projects.addElement(line);
            }   
         }
         read.close();

         if ( addordelete.equals("ADD")|| addordelete.equals("UPDATE")) {
            projects.addElement(name+"\t"+project);
         }
         monitor.log("MONITOR" + uid + "projects: " + projects.toString());

         // overwrite back the vector list to the edit list file
         PrintWriter newlist = new PrintWriter(new BufferedWriter(new FileWriter(editlistfile, false)));
         for (int i = 0 ; i < projects.size() ; i++) {
            newlist.println((String)projects.get(i));
         }
         newlist.close();

         // copy back updated results
         // make readable by the portal user 
         Runtime.getRuntime().exec("/usr/bin/chmod  o+r "+editlistfile).waitFor();
         // ensure file is writable for update by user
         chmodFile(uid,"u+w",listfile);

         // copy the updated file
         reply = reply + copyFileAsUser(listfile, editlistfile, uid, false);

         // return the file to protected
         chmodFile(uid,"u-w",listfile);

         // remove the temporary copy
/*          removeFile(null,editlistfile); */
      } catch (Exception e ) {
         monitor.log("ERROR" + uid + "Exception updating project list "+ e);
         throw new Exception("Error updating project list: "+e.toString());
      }
   }
       
   private void removeFile(String uid, String file)
           throws Exception
   // attempt to remove a file as user id (unforced)
   {
      Process remove;

      if ( uid == null )  {
        monitor.log("MONITOR " + uid + "remove "+file +" as root");
        Runtime.getRuntime().exec("/usr/bin/rm "+ file).waitFor();
      }
      else {
        execAsUser(uid,"/usr/bin/rm "+file);
      }
       
   }
   private void execAsUser(String uid, String command)
           throws Exception
   // general method to invoke a task (in command) as a specific userid
   // accomplished by writing task to a file and then starting
   // a process to execute the task.
   {
        monitor.log("DEBUG " + uid + "Executing "+command);

        Process taskp;
        String taskfile = "/tmp/"+uid+"task"+GEP_SU_SCRIPT;
        PrintWriter task;
        // create the copy task
        task = new PrintWriter(new BufferedWriter(new FileWriter(taskfile)));
        task.println("#!/bin/ksh");
        task.println(command);
        task.close();
        // prep and run the task
        Runtime.getRuntime().exec("/usr/bin/chmod 755 "+taskfile).waitFor();

        taskp = Runtime.getRuntime().exec("su - "+uid+" -c "+taskfile);
        int status = taskp.waitFor(); // wait for completion

        //
        // recover any error messages and send to the log file
        //
        if ( status != 0 )  {
           BufferedReader error =
            new BufferedReader(new InputStreamReader(taskp.getErrorStream()));
           String line;
           while ( (line = error.readLine()) != null )
              monitor.log("ERROR " + uid + "failed execute:"+line);
        }

        // cleanup
        Runtime.getRuntime().exec("/usr/bin/rm "+taskfile).waitFor(); 
   }
   private void chmodFile(String uid, String chmod, String file)
                throws Exception
   {
      Process chmodp;

      if ( uid == null )  {
        monitor.log("MONITOR " + uid + "chmod "+chmod+" "+file +" as root");
        Runtime.getRuntime().exec("/usr/bin/chmod "+ chmod+" "+file).waitFor();
      }
      else {
        execAsUser(uid, "/usr/bin/chmod "+chmod+" "+file);
      }
   }
   private String copyFileAsUser( String target, String source, String uid, boolean delete)
           throws Exception
   //
   // method to copy a File at a remote location to a target location
   // with uid ownership
   // delete attempted as root
   {
   // create commands to perform task

      execAsUser(uid,"/usr/bin/cp "+source+ " " + target);

      if ( delete ) {
        Runtime.getRuntime().exec("/usr/bin/rm -f "+source).waitFor(); 
        monitor.log("MONITOR " + uid + "Copy file removing "+source);
      }
      return "Copy complete";
   }
   private void createProjectDescriptor(String uid, String home, String name, 
                                        String project, String app, String export)
       throws Exception
  //
  // create a project descriptor for the project in the appropriate locations
  // uid : userid to own target file
  // home : project home directory
  // name : portal generated id of the project
  // project : user provided name of the project
  // app : application currently associated with the project
  // export ; the text environment variables specified (will be added to descriptor)
  //
  {
     // location of temporary copy to be created
     String descriptorfile="/tmp/"+name+GEP_PROJECT;

     // location to place target descriptor
     String path = home+name;

     // create the new descriptor file, completing the first two lines
     PrintWriter ressource =
          new PrintWriter(new BufferedWriter(new FileWriter(new File(descriptorfile))));
     ressource.println(project);
     ressource.println(app);
      
     // parse the export text string for environment variables
     try {
             String value;
             StringTokenizer st_equal, st_newline = new StringTokenizer(export, "\n\r\f");
               while (st_newline.hasMoreTokens()) {
                  st_equal =
                     new StringTokenizer(st_newline.nextToken(), "= \t");
                  ressource.print(st_equal.nextToken() + "=");
                  value = st_equal.nextToken();
                     // if $HOME is specified, here is used
                  // otherwise use the value defined
                  if (value.equals("$HOME"))
                     ressource.println(path);
                  else
                     ressource.println(value);
               }
               ressource.close();
               //
               // copy the new descriptor to the project structure
               // to ensure user owns the file
               //
               // NOTE: 
               // compatibility with prior versions doable here by
               // copying file as root instead of user
               // also, having root chmod to writable enables overwrite
               //
               String reply = copyFileAsUser(path+"/"+GEP_PROJECT, 
                                 descriptorfile, uid, true);
         } 
         catch (NoSuchElementException e) {
               ressource.close();
            throw new Exception("Error creating project descriptor");
         }
  } 


   public void init(ServletConfig config) throws ServletException {
      String s;


      super.init(config);

      monitor = new LogMonitor();

      s = getInitParameter("app_home");
      if (s != null) {
         APP_HOME_DIR = s;
      }  
      s = getInitParameter("gep_su_script");
      if (s != null) {
         GEP_SU_SCRIPT = s;
      }  
      s = getInitParameter("gep_list");
      if (s != null) {
         GEP_LIST = s;
      }  
      s = getInitParameter("gep_app");
      if (s != null) {
         GEP_APP = s;
      }  
      s = getInitParameter("gep_app_form");
      if (s != null) {
         GEP_APP_FORM = s;
      }  
      s = getInitParameter("gep_qsub_script");
      if (s != null) {
         GEP_QSUB_SCRIPT = s;
      }  
      s = getInitParameter("gep_project_dir");
      if (s != null) {
         GEP_PROJECT_DIR = s;
      }  
      s = getInitParameter("gep_project");
      if (s != null) {
         GEP_PROJECT = s;
      }  
      s = getInitParameter("gep_root");
      if (s != null) {
         GEP_ROOT = s;
         SGE_EXPORT[4] = "GEP_ROOT=" + s;
      }  
      s = getInitParameter("sge_root");
      if (s != null) {
         SGE_ROOT = s;
         SGE_EXPORT[0] = "SGE_ROOT=" + s;
      }
      s = getInitParameter("sge_cell");
      if (s != null) {
         SGE_CELL = s;
         SGE_EXPORT[1] = "SGE_CELL=" + s;
      }
      s = getInitParameter("commd_port");
      if (s != null) {
         COMMD_PORT = s;
         SGE_EXPORT[2] = "COMMD_PORT=" + s;
      }
      // Determine SGE architecture
      try {
         Process sgeArch = 
            Runtime.getRuntime().exec(SGE_ROOT + "util/arch", SGE_EXPORT);
         BufferedReader archStdout =
            new BufferedReader(new InputStreamReader(sgeArch.getInputStream()));
      
         SGE_ARCH = archStdout.readLine();
         archStdout.close();
      } catch (IOException ioe) {
         // Just assume Solaris
         SGE_ARCH = "solaris";
      }
    
      // Parameters for X tunneling and vncserver script
      s = getInitParameter("x_netlet");
      if (s != null) 
         X_NETLET = s;
      s = getInitParameter("x_server");
      if (s != null) {
         X_SERVER = s;
      } else {
         try {
            X_SERVER = InetAddress.getLocalHost().getHostName();
         } catch (UnknownHostException uhe) {
            X_SERVER = "localhost";
         }
      }
      s = getInitParameter("vncroot");
      if (s != null) {
         VNCSERVER = s + "/vncserver";
      }

      SGE_ACCT = SGE_CELL + SGE_ACCT;

      SGE_EXPORT[3] = "LD_LIBRARY_PATH="+ SGE_ROOT + "lib/" + SGE_ARCH;

      GETWORKSPACE = GEP_ROOT + "bin/gethomedir " + GEP_ROOT + " ";
      ADMINRUN = SGE_ROOT + "utilbin/" + SGE_ARCH + "/adminrun ";

      monitor.log("MONITOR: APP_HOME_DIR:"+APP_HOME_DIR);
      monitor.log("MONITOR: GEP_ROOT:"+GEP_ROOT);
      monitor.log("MONITOR: SGE_ROOT:"+SGE_ROOT);
      monitor.log("MONITOR: SGE_CELL:"+SGE_CELL);
      monitor.log("MONITOR: SGE_ARCH:"+SGE_ARCH);
      monitor.log("MONITOR: COMMD_PORT:"+COMMD_PORT);
      monitor.log("MONITOR: GETWORKSPACE:"+GETWORKSPACE);
      monitor.log("MONITOR: ADMINRUN:"+ADMINRUN);
      monitor.log("MONITOR: X_NETLET:"+X_NETLET);
      monitor.log("MONITOR: X_SERVER:"+X_SERVER);
      monitor.log("MONITOR: VNCSERVER:"+VNCSERVER);
   }


   private String getWorkspace(String uid, String mapped_uid)
      throws IOException, InterruptedException {
      String workdir;

      Process test_user = Runtime.getRuntime().exec(GETWORKSPACE + uid);
      test_user.waitFor();
      if ( test_user.exitValue() == 0 ) {
         BufferedReader test_home = new BufferedReader(new InputStreamReader(test_user.getInputStream())); 
         workdir = test_home.readLine() + GEP_PROJECT_DIR;
      } else {
         Process test_user2 = Runtime.getRuntime().exec(GETWORKSPACE + 
                                                         mapped_uid);
         test_user2.waitFor();
         BufferedReader test_home2 = new BufferedReader(new InputStreamReader(test_user2.getInputStream())); 
         workdir = test_home2.readLine() + uid + GEP_PROJECT_DIR;

      }
      monitor.log("MONITOR " + uid + "Working directory "+workdir);
      return workdir;
   }

   public void doGet(HttpServletRequest request, HttpServletResponse response)
      throws ServletException, IOException {
      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
      monitor.log("MONITOR undefined Begin doget");
      try {
         String uid = getUser(request);
         String domain_name = getDomain(request);
         String submit_uid = uid;

         getRoles(request, response);
         String home = getWorkspace(uid, domain_name);

         String action = request.getParameter("action");

         monitor.log("MONITOR " + uid + "doGet home:"+home);
         monitor.log("MONITOR " + uid + "doGet action:"+action);
         monitor.log("MONITOR " + uid + "doGet domain_name:"+domain_name);
      
         if (action.equals("projectList"))
            projectList(uid, home, response);
         else if (action.equals("projectInfo"))
            projectInfo(uid, home, request, response);
         else if (action.equals("newProjectForm"))
            newProjectForm(uid, home, response);
         else if (action.equals("editProjectForm"))
            editProjectForm(uid, home, request, response);
         else if (action.equals("deleteProject"))
            deleteProject(uid, home, request, response);
         else if (action.equals("deleteFile"))
            deleteFile(uid, home, request, response);
         else if (action.equals("jobList"))
            jobList(uid, submit_uid, home, response);
         else if (action.equals("jobInfo"))
            jobInfo(request, response);
         else if (action.equals("newJobForm"))
            newJobForm(uid, home, request, response);
         else if (action.equals("newJobCustomForm"))
            newJobCustomForm(uid, home, request, response);
         else if (action.equals("submitNewJob"))
            submitNewJob(uid, submit_uid, home, domain_name, request, response);
         else if (action.equals("getxDisplayNum"))
            getxDisplayNum(uid, home, request, response);
         else if (action.equals("killJob"))
            killJob(uid, home, request, response);
         else if (action.equals("applicationList"))
            applicationList(uid, response);
         else if (action.equals("applicationInfo"))
            applicationInfo(request, response);
         else if (action.equals("viewFile"))
            viewFile(uid, home, request, response);
         else if (action.equals("downloadFile"))
            downloadFile(uid, home, request, response);
         else if (action.equals("launchvncserver"))
            launchVNCServer(uid, home, domain_name, request, response);
         else if (action.equals("debug"))
            installationInfo(request, response);
         else if (action.equals("applicationAdminList"))
            applicationAdminList(response);
         else if (action.equals("applicationAdminInfo"))
            applicationAdminInfo(request, response);
         else if (action.equals("newAdminApplicationForm"))
            newAdminApplicationForm(response);
         else if (action.equals("editAdminApplicationForm"))
            editAdminApplicationForm(request, response);
         else if (action.equals("updateAdminApplication"))
            updateAdminApplication(request, response);
         else if (action.equals("deleteAdminApplication"))
            deleteAdminApplication(request, response);
         else if (action.equals("jobAdminList"))
            jobAdminList(response);
         else if (action.equals("jobAdminAccounting"))
            jobAdminAccounting(response);
         else if (action.equals("viewAdminFile"))
            viewAdminFile(request, response);
         else
            unknownAction(uid, response);
      } catch (Exception e) {
         monitor.log("MONITOR undefined doGet Error:"+e);
         out.println("<html><body><pre>");
         e.printStackTrace(out);
         out.println("</pre></body></html>");
      }
   }
  
   public void doPost(HttpServletRequest request, HttpServletResponse response)
      throws ServletException, IOException {
      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
      try {  
         String uid = getUser(request);
         String domain_name = getDomain(request);
         String submit_uid = uid;
         String home = getWorkspace(uid, domain_name);

      
         // create a location to upload files from the desktop
         String name = "T" + System.currentTimeMillis();
         String tmpdir = "/tmp/" + name;
         Runtime.getRuntime().exec("mkdir -m 777 -p " + tmpdir).waitFor();

         PrintWriter sume;
         MultipartRequest multi = 
            new MultipartRequest(request, tmpdir, 100*1024*1024); // 100MB

         out.println("<pre>");
         Enumeration params = multi.getParameterNames();
         while (params.hasMoreElements()) {
            String pname = (String) params.nextElement();
            String pval = (String) multi.getParameter(pname);
            out.println(pname + ": " + pval);
         }   
         out.println("</pre>");
         out.flush();
         String action = multi.getParameter("action");

         if (action.equals("createNewAdminApplication")) {
            /*      
               Profile prof = sess.getUserProfile();
               if (!prof.isAllowed("iwtAdmin-execute")) throw new AccessControlException("User " + uid + " does not have Admin privileges");
            */      
            createNewAdminApplication(multi, out);
         } else {   
            String project = multi.getParameter("project");
            if (project != null)
               project.trim();
            String app = multi.getParameter("app");
            String export = multi.getParameter("export");
            String filename = null;
       
            Enumeration files = multi.getFileNames();
            if (files.hasMoreElements()) {
               filename = multi.getFilesystemName((String)files.nextElement());
               monitor.log("MONITOR " + uid + "FileUpload as "+name+ " "+project+" using " + app );
               Runtime.getRuntime().exec("/usr/bin/chmod 644 " + tmpdir + "/" + filename).waitFor();
               //
               // NOTE: enable other upload options here for multipart files
               //
               if (multi.getParameter("tar") != null) {
                  sume = new PrintWriter(new BufferedWriter(new FileWriter(new File(tmpdir, GEP_SU_SCRIPT))));
                  sume.println("cd " + tmpdir);
                  sume.println("gunzip -c " + filename + " | tar xf -");
                  sume.println("chmod -R 644 .");
                  sume.close();
                  Runtime.getRuntime().exec("/usr/bin/chmod 755 " + tmpdir + "/" + GEP_SU_SCRIPT).waitFor();
                  Process zip = Runtime.getRuntime().exec("su - " + submit_uid + " -c " + tmpdir + "/" + GEP_SU_SCRIPT);
               }
            }
            if (action.equals("createNewProject")) {
               createNewProject(uid, submit_uid, home, tmpdir, 
                                filename, project, app, export, request, out);
            } else if (action.equals("updateProject"))  {
               String id = multi.getParameter("id");
               updateProject(uid, submit_uid, home, id, tmpdir, 
                             filename, project, app, export, request, response);  
            } else {
               unknownAction(uid, response);
            }   
         }      
      } catch (Exception e) {
         out.println("<html><body><pre>");
         e.printStackTrace(out);
         out.println("</pre></body></html>");
      }
   }

   private void projectList(String uid, String home, HttpServletResponse response) throws Exception {
      //
      // display the project list
      //
      StringTokenizer st;
      String line, project;
      String directory = home;

      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
 
      monitor.log("DEBUG " + uid + "Project List "+directory);

      String projectlist = directory+GEP_LIST;

      // make workspace visible by root
      openWorkspace(uid, home);

      // make project list visible by root
      chmodFile(uid,"o+r",projectlist);
    
      // read through the project list file
      out.println("<html><body>");    
      try {
         synchronized (Class.forName("com.sun.gep.GEPServlet")) {
            monitor.log("DEBUG " + uid + "Project List "+projectlist);
            BufferedReader list = new BufferedReader(new FileReader(projectlist));
            out.println("<table width=100%>");
            while ((line = list.readLine()) != null) {
               st = new StringTokenizer(line, "\t");
               project = st.nextToken();
               out.println("<tr><td><li><a href=\"GEPServlet?action=projectInfo&project="
                    + project + "\" target=GEPServletProject>");
               out.println(st.nextToken() + "</a></li></td>");
               out.println("<td><a href=\"/Edit Project\" onClick=\"window.open(\'GEPServlet?action=editProjectForm&project=" + project + "\', \'GEPServletProject\'); return false\">");
               out.println("edit</a></td>");
               out.println("<td><a href=URL onClick=\"window.open(\'GEPServlet?action=deleteProject&project=" + project + "\', \'GEPServletProject\'); return false\">");
               out.println("delete</a></td></tr>");
            }
            out.println("</table>");
            list.close();
         }
      } catch (FileNotFoundException e) {
         monitor.log("MONITOR " + uid + " No projects found");
         out.println("You have no available projects to view.");      
      }

      // close down the visibility of the project to others
      chmodFile(uid,"o-r",projectlist);
      // close the workspace
      closeWorkspace(uid,home);

      out.println("<table width=100%><tr><td align=right>");
      out.println("<a href=URL onClick=\"window.open(\'GEPServlet?action=newProjectForm\', \'GEPServletProject\'); return false\">");
      out.println("Create new project...</a>");
      out.println("</td></tr></table>");
      out.println("</body></html>");
   }     

   private void projectInfo(String uid, String home, HttpServletRequest request, HttpServletResponse response)
      throws Exception {
      String project = request.getParameter("project");
      String line;
      String path = home + project;

      response.setContentType("text/html");
      PrintWriter out = response.getWriter();

      openWorkspace(uid,home);
    
      monitor.log("MONITOR " + uid + " Viewing project "+path);
      htmlHeader(out, "Project Information", "");
      try {
         BufferedReader info =
            new BufferedReader(new FileReader(new File(path, GEP_PROJECT)));
    
         out.println("<table>");
         out.println("<tr><td><a target=GEPServletFiles> Project name:</td><td>" + info.readLine() + "</a></td></tr>");
         out.println("<tr><td>Application:</td><td>" + info.readLine() + "</td></tr>");

         out.println("<tr><td valign=top>Exports:</td>");
         if ((line = info.readLine()) != null)
            out.println("<td>" + line + "</td>");
         out.println("</tr>");
         while ((line = info.readLine()) != null)
            out.println("<tr><td>&nbsp;</td><td>" + line + "</td></tr>");
      
         File directory = new File(path);
         String[] files = directory.list();
      
         out.println("<tr><td valign=top>Files:</td>");
         if (files.length > 0) {
            out.println("<td><a href=\"GEPServlet?action=downloadFile&project="
                    + project + "&file=" + files[0] + "\" target=GEPServletFile1>" + files[0] + "</a></td>");
            File temp = new File (path, files[0]);
            out.println("<td>&nbsp;</td><td>(" + temp.length() + " bytes) </td>");
            out.println("<td>&nbsp;</td><td><a href=URL onClick=\"window.open(\'GEPServlet?action=viewFile&view=view&project=" + project + "&file=" + files[0] + "\', \'GEPServletFile1\'); return false\">");
            out.println("view</a></td>");   
            out.println("<td>&nbsp;</td><td><a href=URL onClick=\"window.open(\'GEPServlet?action=viewFile&view=head&project=" + project + "&file=" + files[0] + "\', \'GEPServletFile1\'); return false\">");
            out.println("head</a></td>");   
            out.println("<td>&nbsp;</td><td><a href=URL onClick=\"window.open(\'GEPServlet?action=viewFile&view=tail&project=" + project + "&file=" + files[0] + "\', \'GEPServletFile1\'); return false\">");
            out.println("tail</a></td>");   
            if (!files[0].equals(".gep-project"))  {
               out.println("<td>&nbsp;</td><td><a href=URL onClick=\"window.open(\'GEPServlet?action=deleteFile&project=" + project + "&file=" + files[0] + "\', \'GEPServletFile1\'); return false\">");
               out.println("delete</a></td>");   
            }
         }
         out.println("</tr>");
         for (int i = 1; i < files.length; i++) {
            out.println("<td>&nbsp;</td><td><a href=\"GEPServlet?action=downloadFile&project="
                    + project + "&file=" + files[i] + "\" target=GEPServletFile2>" + files[i] + "</a></td>");
            File temp = new File (path, files[i]);
            out.println("<td>&nbsp;</td><td>(" + temp.length() + " bytes) </td>");
            out.println("<td>&nbsp;</td><td><a href=URL onClick=\"window.open(\'GEPServlet?action=viewFile&view=view&project=" + project + "&file=" + files[i] + "\', \'GEPServletFile2\'); return false\">");
            out.println("view</a></td>");   
            out.println("<td>&nbsp;</td><td><a href=URL onClick=\"window.open(\'GEPServlet?action=viewFile&view=head&project=" + project + "&file=" + files[i] + "\', \'GEPServletFile2\'); return false\">");
            out.println("head</a></td>");   
            out.println("<td>&nbsp;</td><td><a href=URL onClick=\"window.open(\'GEPServlet?action=viewFile&view=tail&project=" + project + "&file=" + files[i] + "\', \'GEPServletFile2\'); return false\">");
            out.println("tail</a></td>");   
            if (!files[i].equals(".gep-project"))  {
               out.println("<td>&nbsp;</td><td><a href=URL onClick=\"window.open(\'GEPServlet?action=deleteFile&project=" + project + "&file=" + files[i] + "\', \'GEPServletFile2\'); return false\">");
               out.println("delete</a></td>"); 
            }
            out.println("</tr>");
         }
      
         out.println("</table>");
      } catch (FileNotFoundException e) {
         monitor.log("ERROR" + uid + " Error accessing this project: "+path);
         out.println("Error accessing this project.");
      }
      closeWorkspace(uid,home);
      out.println("<center><form><input type=button value=Continue onClick=window.close()></form></center>");   
      htmlFooter(out);
   }     

   private void newProjectForm(String uid, String home, HttpServletResponse response) throws Exception {
      StringTokenizer st;
      String line, app, text;
      boolean dismiss = false;

      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
    
      htmlHeader(out, "New Project Form", "");
      monitor.log("MONITOR " + uid + "Creating new Project Form");
      try {
      monitor.log("DEBUG " + uid + " Creating List of Applications from " + APP_HOME_DIR+GEP_LIST);
         BufferedReader list =
            new BufferedReader(new FileReader(APP_HOME_DIR + GEP_LIST));
    
         out.println("<form method=post action=GEPServlet enctype=multipart/form-data>");
         out.println("<input type=hidden name=action value=createNewProject>");
         out.println("<table>");
         out.println("<tr><td>Project name:</td>");
         out.println("<td><input type=text name=project size=25></td></tr>");
         out.println("<tr><td>Application:</td>");
         out.println("<td><select name=app>");

         while ((line = list.readLine()) != null) {
            st = new StringTokenizer(line, "\t");
            app = st.nextToken();
            text = st.nextToken();
            if (st.hasMoreTokens()) {
               dismiss = true;
               while (st.hasMoreTokens()) {
                  if (st.nextToken(" \t").equals(uid)) {
                     dismiss = false;
                     break;
                  }
               }
               if (dismiss) 
                  continue;
            }
            out.println("<option value=" + app + ">");
            out.println(text);
         }
    
         out.println("</select></td></tr>");
         out.println("<tr><td>Input file:</td>");
         out.println("<td><input type=file name=input size=25></td></tr>");
         out.println("<tr><td colspan=2><input type=checkbox name=tar value=yes>");
         out.println("check here if this file is a compressed tar archive.</td></tr>");
         out.println("<tr><td valign=top>Environment variables:</td>");
         out.println("<td><textarea name=export rows=5 cols=25></textarea></td></tr>");
         out.println("</table>");
         out.println("<center><input type=submit value=Submit></center>");
         out.println("</form>");
      } catch (FileNotFoundException e) {
         monitor.log("ERROR " + uid + " No Applications. " + e.toString());
         out.println("There are no applications available.");      
      }
      htmlFooter(out);
   }     

   private void createNewProject(String uid, String submit_uid, String home, String tmpdir, 
      String filename, String project, String app, String export, HttpServletRequest request, PrintWriter out)
      throws Exception {
      
      PrintWriter sume;
      String name = "P" + System.currentTimeMillis();
      String path = home + name;

      monitor.log("MONITOR" + uid + "Begin creating new project "+name);

      openWorkspace(uid,home);

      htmlHeader(out, "New Project Status", "");
      out.println("Pathname: " + path);
    
      // create the new project area
      execAsUser(uid,"mkdir -m 755 -p "+path);

      // copy uploaded file to new project area
      if (filename != null) {
         String reply = copyFileAsUser( path, tmpdir+"/*", submit_uid, true);
         monitor.log("MONITOR " + submit_uid + " Copy to project "+reply);
         Runtime.getRuntime().exec("/usr/bin/rm -rf " + tmpdir).waitFor();  
      }
    
      if (project.length() > 0) {
         synchronized (Class.forName("com.sun.gep.GEPServlet")) {
            //
            // update the list of projects
            //
            updateProjectList(home, name, project, submit_uid, "ADD");
            //
            // create project descriptor in the project directory
            //
            try {
               createProjectDescriptor(submit_uid, home, name, project, app, export);
               out.println("<p>Project <i>" + project + 
                                        "</i> was created successfully.");
            }
            catch (Exception e ) {
               out.println("<p>Error parsing export variables. You can "+
                "add/correct variables by editing the newly created project.");
            }

            out.println("<center><form><input type=button value=Continue "+
                         "onClick=\"opener.location.reload(); window.close()\"></form></center>");
         } // synchronized
      } else {
         Runtime.getRuntime().exec("/usr/bin/rm -rf " + path);     
         out.println("<p>Go back and enter a non-empty project name.");
         out.println("<center><form><input type=button value=Back onClick=window.back()></form></center>");
      }
      closeWorkspace(uid, home);
      monitor.log("MONITOR " + submit_uid + " End Create new project.");
      htmlFooter(out);
   }

   private void editProjectForm(String uid, String home, HttpServletRequest request, HttpServletResponse response) 
      throws Exception {
      StringTokenizer st;
      String line, app, text;
      boolean dismiss = false;
      String id = request.getParameter("project");
      String directory = home + id;

      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
    
      openWorkspace(uid,home);

      htmlHeader(out, "Project Form", "");
      try {
         BufferedReader read =
            new BufferedReader(new FileReader(new File(directory, GEP_PROJECT)));
    
         String project = read.readLine();
         String application = read.readLine();
         Vector export = new Vector();
         while ((line = read.readLine()) != null)
            export.addElement(line);
         read.close();

         BufferedReader list =
            new BufferedReader(new FileReader(APP_HOME_DIR + GEP_LIST));
    
         out.println("<form method=post action=GEPServlet enctype=multipart/form-data>");
         out.println("<input type=hidden name=action value=updateProject>");
         out.println("<input type=hidden name=id value=" + id + ">");
         out.println("<table>");
         out.println("<tr><td>Project name:</td>");
         out.println("<td><input type=text name=project value=\""
                  + project + "\" size=25></td></tr>");
         out.println("<tr><td>Application:</td>");
         out.println("<td><select name=app>");

         while ((line = list.readLine()) != null) {
            st = new StringTokenizer(line, "\t");
            app = st.nextToken();
            text = st.nextToken();
            if (st.hasMoreTokens()) {
               dismiss = true;
               while (st.hasMoreTokens()) {
                  if (st.nextToken(" \t").equals(uid)) {
                     dismiss = false;
                     break;
                  }
               }
               if (dismiss) 
                  continue;
            }
            if (app.equals(application))
               out.println("<option selected value=" + app + ">");
            else
               out.println("<option value=" + app + ">");
            out.println(text);
         }
    
         out.println("</select></td></tr>");
         out.println("<tr><td>Input file:</td>");
         out.println("<td><input type=file name=input size=25></td></tr>");
         out.println("<tr><td colspan=2><input type=checkbox name=tar value=yes>");
         out.println("check here if this file is a compressed tar archive.</td></tr>");
         out.println("<tr><td valign=top>Environment variables:</td>");
         out.println("<td><textarea name=export rows=5 cols=25>");
         for (int i = 0; i < export.size(); i++)
            out.println((String)export.get(i));
         out.println("</textarea></td></tr>");
         out.println("</table>");
         out.println("<center><input type=submit value=Submit></center>");
         out.println("</form>");
      } catch (FileNotFoundException e) {
         out.println("There is no application available.");      
      }
      closeWorkspace(uid,home);
      htmlFooter(out);
   }     

   private void updateProject(String uid, String submit_uid, String home, 
                                 String id, String tmpdir, String filename, 
                                String project, String app, String export, 
                                HttpServletRequest request, 
                                HttpServletResponse response)
      throws Exception {
      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
    
      String directory = home + id; // project to update

      openWorkspace(uid,home);

      // open the project directory
      chmodFile(submit_uid,"o+rxw",directory);

      // update any updated files
      if (filename != null) {
         execAsUser(submit_uid,"cd "+tmpdir+"; /usr/bin/cp * " + directory);
         Runtime.getRuntime().exec("/usr/bin/rm -rf " + tmpdir).waitFor();  
      }
    
      htmlHeader(out, "Project Status", "");

      if (project.length() > 0) {
         synchronized (Class.forName("com.sun.gep.GEPServlet")) {
            String path = home;
      
            updateProjectList(home, id, project, submit_uid,"UPDATE");

            try {
               createProjectDescriptor(submit_uid, home, id, project, app, export);
               out.println("<p>Project <i>" + project + "</i> was updated successfully.");
            } catch (NoSuchElementException e) {
               out.println("<p>Error parsing export variables. You can add/correct variables by editing again the project.");
            }
            out.println("<center><form><input type=button value=Continue onClick=\"opener.location.reload(); window.close()\"></form></center>");
         } // synchronized
      } else {
         out.println("<p>Go back and enter a non-empty project name.");
         out.println("<center><form><input type=button value=Back onClick=window.back()></form></center>");
      }
      closeWorkspace(submit_uid,home);
      htmlFooter(out);
   }

   private void deleteProject(String uid, String home, HttpServletRequest request, HttpServletResponse response)
      throws Exception {
      String projectid = request.getParameter("project");
      String line;
      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
    
      htmlHeader(out, "Project Status", "");
      monitor.log("MONITOR " + uid + " Deleting project "+ projectid);
      openWorkspace(uid,home);
      try {
         synchronized (Class.forName("com.sun.gep.GEPServlet")) {
            
            String directory = home;
            updateProjectList(home, projectid, null, uid,"DELETE");

            // now remove the project file directory
            // change protection on projects to delete
            //
              chmodFile(uid, "700", directory+projectid);

              execAsUser(uid,"/usr/bin/rm -rf "+directory+projectid);

            out.println("The project was successfully deleted.");
            monitor.log("MONITOR " + uid + " Project deletion complete for "+projectid);
         } // synchronized
      } catch (Exception e) {
         monitor.log("MONITOR " + uid + " Error deleting project "+projectid);
         out.println("Error accessing this project.");
      }
      closeWorkspace(uid,home);
      out.println("<center><form><input type=button value=Continue onClick=\"opener.location.reload(); window.close()\"></form></center>");
      htmlFooter(out);
   }

   private void jobList(String uid, String submit_uid, String home, HttpServletResponse response) 
      throws Exception {
      String line, id, name;
      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
    
      out.println("<html><body>");
      try {
         String qstattask=SGE_ROOT+"bin/"+SGE_ARCH+"/qstat -u " + submit_uid;
         monitor.log("MONITOR " + uid + " QSTAT:"+qstattask);
         
         Process qstat = Runtime.getRuntime().exec(qstattask, SGE_EXPORT);

         BufferedReader jobs =
            new BufferedReader(new InputStreamReader(qstat.getInputStream()));
         BufferedReader error =
            new BufferedReader(new InputStreamReader(qstat.getErrorStream()));
      
         if ((jobs.readLine()) != null) {
            jobs.readLine(); // 2 header lines
            line = jobs.readLine();
            out.println("<table width=100%>");
            do {
               id = line.substring(0, 7).trim();
               if (id.length() > 0) {
               String partial = line.substring(14, 17);
               if (!partial.equals("tmp")) {
                  out.println("<tr><td><li><a href=\"GEPServlet?action=jobInfo&id="
                        + id + "\" target=GEPServletJob>");
                  name = line.substring(13, 24).trim();
                  if (name.length() == 10) 
                     name = name.concat("...");
                  name = name.replace('_', ' ');
                  out.println(name + "</a></li></td>");
                  out.println("<td align=right><a href=URL onCLick=\"window.open(\'GEPServlet?action=killJob&id="
                        + id + "\', \'GEPServletJob\'); return false\">kill</a></td></tr>");
               }
               }
            } while ((line = jobs.readLine()) != null);
            out.println("</table>");
         } else {
            out.println("You have no running jobs.");
            out.println("<pre>");
            while ((line = error.readLine()) != null) 
               out.println(line);
            out.println("</pre>");
         }
      } catch (Exception e) {
         e.printStackTrace(out);      
      }
      out.println("<table width=100%><tr><td align=right>");
      out.println("<a href=URL onCLick=\"window.open(\'GEPServlet?action=newJobForm\', \'GEPServletJob\'); return false\">");
      out.println("Submit new job...</a>");
      out.println("</td></tr></table>");
      out.println("</body></html>");
   }     

   private void jobAdminList(HttpServletResponse response) throws Exception {
      String line, id, name;
      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
    
      out.println("<html><body>");
      try {
         Process qstat = Runtime.getRuntime().exec(SGE_ROOT + "bin/" + SGE_ARCH + "/qstat", SGE_EXPORT);
         BufferedReader jobs = new BufferedReader(new InputStreamReader(qstat.getInputStream()));
         BufferedReader error = new BufferedReader(new InputStreamReader(qstat.getErrorStream()));
      
         if ((jobs.readLine()) != null) {
            jobs.readLine(); // 2 header lines
            line = jobs.readLine();
            out.println("<table width=100%>");
            do {
               id = line.substring(0, 6).trim();
               if (id.length() > 0) {
                  out.println("<tr><td><li><a href=\"GEPServlet?action=jobInfo&id="
                        + id + "\" target=GEPServletJob>");
                  name = line.substring(13, 24).trim();
                  if (name.length() == 10) 
                     name = name.concat("...");
                  name = name.replace('_', ' ');
                  out.println(name + "</a></li></td>");
                  out.println("<td align=right><a href=URL onCLick=\"window.open(\'GEPServlet?action=killJob&id="
                        + id + "\', \'GEPServletJob\'); return false\">kill</a></td></tr>");
               }
            } while ((line = jobs.readLine()) != null);
            out.println("</table>");
         } else {
            out.println("There are no running jobs.");
            out.println("<pre>");
            while ((line = error.readLine()) != null) 
               out.println(line);
            out.println("</pre>");
         }
      } catch (Exception e) {
         e.printStackTrace(out);      
      }
      out.println("<table width=100%><tr><td align=right>");
      out.println("<a href=URL onCLick=\"window.open(\'GEPServlet?action=jobAdminAccounting\', \'GEPServletJob\'); return false\">");
      out.println("Perform accounting...</a>");
      out.println("</td></tr></table>");
      out.println("</body></html>");
   }     

   private void jobInfo(HttpServletRequest request, HttpServletResponse response)
      throws Exception {
      String id = request.getParameter("id");
      String line;
      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
    
      htmlHeader(out, "Job Information", "");
      out.println("<pre>");
      try {
         Process qstat =
         Runtime.getRuntime().exec(SGE_ROOT + "bin/" + SGE_ARCH + "/qstat -j " + id,
                                  SGE_EXPORT);
         BufferedReader jobs =
            new BufferedReader(new InputStreamReader(qstat.getInputStream()));
      
         while ((line = jobs.readLine()) != null) 
            out.println(line);
      } catch (Exception e) {
         e.printStackTrace(out);      
      }
      out.println("</pre>");
      out.println("<center><form><input type=button value=Continue onClick=window.close()></form></center>");
      htmlFooter(out);
   }     

   private void newJobForm(String uid, String home, HttpServletRequest request, HttpServletResponse response) 
      throws Exception {
      StringTokenizer st;
      String line, project, path, application;
      String project_name;
      BufferedReader projectinfo, appinfo;
      Vector all, yes;
      int i;

      openWorkspace(uid,home);

      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
    
      htmlHeader(out, "New Job Form", "");
      monitor.log("MONITOR " + uid + " Submitting new job");
      try {
         String listfile = home + GEP_LIST;
         chmodFile(uid,"o+r",listfile); // ensure readability
         monitor.log("DEBUG " + uid + " Current list file " + listfile);
         BufferedReader list = new BufferedReader(new FileReader(listfile));

         all = new Vector();
         yes = new Vector();
         while ((line = list.readLine()) != null) {
            st = new StringTokenizer(line, "\t");
            project = st.nextToken();
            project_name = st.nextToken();
            path = home + project;
            projectinfo = new BufferedReader(new FileReader(new File(path, GEP_PROJECT)));
            projectinfo.readLine();
            monitor.log("DEBUG " + uid + " Project info reader created.");
            application = projectinfo.readLine();
            monitor.log("DEBUG " + uid + " Application reader being created for "+ application);
            try {
            appinfo = new BufferedReader(new FileReader(new File(APP_HOME_DIR + application, GEP_APP)));

           // success this far to reading project information and associated
           // application information
           all.addElement(project);
           all.addElement(project_name);
           monitor.log("DEBUG " + uid + " Adding submission choice "+project+" "+project_name);
           // read through application lines to find line 5
           appinfo.readLine(); appinfo.readLine(); 
           appinfo.readLine(); appinfo.readLine();
           // check to see if project has a defined form
            if (appinfo.readLine().equals("yes")) {
               yes.addElement(project);
               yes.addElement(application);
            }
            projectinfo.close();
            appinfo.close();
         }
         catch ( FileNotFoundException e)
         { 
           monitor.log("ERROR " + uid + " Project "+project_name+" missing Appfile. file Not Found Exception " + e.toString() );
         }
         projectinfo.close();
       }


         int length = yes.size();
         if (length > 0) {
            // externalize javascript code as iplanet rewrites location tags
            out.println("<script language=javascript>"); 
            out.println("function loadCustom() {");
            out.println("  var myselect = document.forms[0].elements[1];");
            out.println("  var project = myselect.options[myselect.selectedIndex].value;");
            out.println("  var p = new Array(" + length + ");");
            for (i = 0; i < length; i++)
               out.println("  p[" + i + "] = \"" + (String)yes.get(i) + "\";");
            out.println("  for (var i = 0; i < p.length; i += 2) {");
            out.println("    if (project == p[i]) {");
            out.println("      window.open(\"GEPServlet?action=newJobCustomForm&project=\" + p[i] + \"&app=\" + p[i + 1], p[i], \"menubar=no,toolbar=no,resizable=yes,location=no,status=no\");");
            out.println("      return false;");
            out.println("    }");
            out.println("  }");
            out.println("}");
            out.println("</script>");
         }
         
         out.println("<form method=get action=GEPServlet>");
         out.println("<input type=hidden name=action value=submitNewJob>");  
         out.println("<table>");
         
         project = request.getParameter("project");
         out.println("<tr><td>Project name:</td>");
         out.println("<td><select name=project onChange=loadCustom()>");
         if (project == null) {
            out.println("<option value=null>Select project first.");
            for (i = 0; i < all.size(); i += 2)
               out.println("<option value=" + (String)all.get(i) + ">" + (String)all.get(i+1));
         } else {
            for (i = 0; i < all.size(); i += 2) {
               if (project.equals((String)all.get(i)))
                  out.println("<option value=" + (String)all.get(i) + " selected>" + (String)all.get(i+1));
               else
                  out.println("<option value=" + (String)all.get(i) + ">" + (String)all.get(i+1));
            }
         }
         out.println("</select></td></tr>");
      
         out.println("<tr><td>Email notification:</td>");
         String email = request.getParameter("email");
         if (email == null)
            out.println("<td><input type=text name=email value=" + uid + " size=25></td></tr>");
         else
            out.println("<td><input type=text name=email value=" + email + " size=25></td></tr>");
         out.println("<tr><td>Input arguments:</td>");
         String input = request.getParameter("input");
         if (input == null)
            out.println("<td><input type=text name=input size=25></td></tr>");
         else
            out.println("<td><input type=text name=input size=25 value=\"" + input + "\"></td></tr>");
         out.println("<tr><td>Output file name:</td>");
         String output = request.getParameter("output");
         if (output == null)
            out.println("<td><input type=text name=output value=output.txt size=25></td></tr>");
         else
            out.println("<td><input type=text name=output value=\"" + output + "\" size=25></td></tr>");
         out.println("<tr><td>Error file name:</td>");
         String error = request.getParameter("error");
         if (error == null)
            out.println("<td><input type=text name=error value=error.txt size=25></td></tr>");
         else
            out.println("<td><input type=text name=error value=\"" + error + "\" size=25></td></tr>");
         out.println("<tr><td>Number of CPUs:</td>");
         String cpu = request.getParameter("cpu");
         if (cpu == null)
            out.println("<td><input type=text name=cpu value=1 size=25></td></tr>");
         else
            out.println("<td><input type=text name=cpu value=" + cpu + " size=25></td></tr>");
         out.println("</table>");
         out.println("<center><input type=submit value=Submit></center>");
         out.println("</form>");
      } catch (FileNotFoundException e) {
         monitor.log("ERROR " + uid + " No Application Available. " + e.toString());
         out.println("There are no projects available.");      
      }
      closeWorkspace(uid,home);
      htmlFooter(out);
   }     

   private void newJobCustomForm(String uid, String home, HttpServletRequest request, HttpServletResponse response) 
      throws Exception {
      String line;
      String project = request.getParameter("project");
      String application = request.getParameter("app");
      BufferedReader form = new BufferedReader(new FileReader(new File(APP_HOME_DIR + application, GEP_APP_FORM)));
  
      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
    
      out.println("<html><body><form>");
      //
      // copy the javascript form to the window
      //
      out.println("<input type=hidden name=project value=" + project + ">");
      while ((line = form.readLine()) != null)
         out.println(line);
      out.println("</form></body></html>");
   }

   private boolean isxDisplayNumValid() {
      if (xDisplayNum == null)
         return false;
      return(!xDisplayNum.equals("::"));
   }

   private void startOrStopVNCServer(boolean start, SSOToken token, PrintWriter out, String xvncUser) 
      throws Exception {
      String line;
      // Handle error cases first
      if (start && isxDisplayNumValid()) {
/*          String jsLaunchXURL = "/portal/NetletConfig?func=" + X_NETLET + ":" + xDisplayNum + "&machine=" + X_SERVER; */
         String jsLaunchXURL = "/portal/NetletConfig?func=" + X_NETLET + ":" + xDisplayNum + "&machine=" + X_SERVER;

         out.println("vncserver already launched for this SSOToken.");
         out.println("<a href=\"javascript:window.open(\'" + jsLaunchXURL + "\', \'xAppWindow\', \'width=1024,height=800\');window.close()\"> Click here</a> to re-open vncviewer window if inadvertently closed");
         return;
      } else if (!start && !isxDisplayNumValid()) {
         if (out != null)
         out.println("No vncserver registered; nothing to kill");
         return;
      }

      if (!isxDisplayNumValid()) {
         // Start up new vncserver for user.  Get X desktop number
         // from output of vncserver script.  If desktop number not
         // in range of known netlet rules, tell user to try again
         // later.
         // TODO: Support dynamic netlet rule generation?
/*          try { */
            out.println("Output of vncserver launch:");
            out.println("xvncUser: " + xvncUser);
            out.println("VNCSERVER: " + VNCSERVER);
            Process vncServer =
               Runtime.getRuntime().exec("su - " + xvncUser + " -c " + VNCSERVER, SGE_EXPORT);
            BufferedReader vncStderr =
               new BufferedReader(new InputStreamReader(vncServer.getErrorStream()));
            BufferedReader vncStdout =
               new BufferedReader(new InputStreamReader(vncServer.getInputStream()));
            // Parse output for desktop number.  Also send to
            // web page.
            out.println("<pre>");
            while ((line = vncStdout.readLine()) != null) {
               out.println(line);
            }
            while ((line = vncStderr.readLine()) != null) {
               out.println(line);
               if (line.indexOf("desktop") != -1)
                  xDisplayNum = String.valueOf(line.charAt(line.lastIndexOf(':')+1));
            }
            out.println("xDisplayNum: " + xDisplayNum);
            out.println("xvncUser: " + xvncUser);
            out.println("</pre>");
            vncStderr.close();
            vncStdout.close();
/*          } catch (Exception ioe) { */
/*             ioe.printStackTrace(out); */
/*          } */
         // Add desktop number as a SSOToken attribute
         // Should verify we have a netlet rule for this desktop first XXX
         token.setProperty(GEP_DESKTOP_ATTR, xDisplayNum);
         // Register SSOTokenListener for shutting down vncserver on logout
         token.addSSOTokenListener(new SSOTokenChange());

         // Launch vncviewer applet to connect to xDisplayNum
         String jsLaunchX="window.open(\'/portal/NetletConfig?func=" + X_NETLET + ":" + xDisplayNum + "&machine=" + X_SERVER + "\', \'xAppWindow\', \'width=1024,height=800\')";

         // Output javascript to pop X desktop window
         out.println("<script language=javascript>");
         out.println(jsLaunchX);
         out.println("</script>");
      } else {
         // Kill vncserver started for this SSOToken
         String[] cmdargs = new String[5];
         cmdargs[0] = "su";
         cmdargs[1] = "-";
         cmdargs[2] = xvncUser;
         cmdargs[3] = "-c";
         cmdargs[4] = VNCSERVER + " -kill :" + xDisplayNum;
     
         Process vncServer =
            Runtime.getRuntime().exec(cmdargs);
         if (out != null) {
            BufferedReader vncStderr =
               new BufferedReader(new InputStreamReader(vncServer.getErrorStream()));
            out.println("Output of vncserver shutdown:");
            out.println("<pre>");
            while ((line = vncStderr.readLine()) != null)
               out.println(line);
            out.println("</pre>");
            vncStderr.close();
         }
         // Clear xDisplayNum SSOToken attribute.  "::" is the marker for
         // a closed vncserver SSOToken.
         if (token != null) {
            token.setProperty(GEP_DESKTOP_ATTR, "::");
         }
      }
   }

   private void launchVNCServer(String uid, String home, String domain_name, 
      HttpServletRequest request, HttpServletResponse response) 
      throws Exception {
      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
      SSOTokenManager manager = SSOTokenManager.getInstance();
      SSOToken token = manager.createSSOToken(request);

      // Make sure xDisplayNum is set before calling startOrStopVNCServer
      xDisplayNum = token.getProperty(GEP_DESKTOP_ATTR);

      String kill = request.getParameter("kill");
      if (kill == null) {
         htmlHeader(out, "Launching vncserver", "");
         startOrStopVNCServer(true, token, out, uid);
      } else {
         htmlHeader(out, "Shutting down vncserver", "");
         startOrStopVNCServer(false, token, out, uid);
      }

      out.println("<center><form><input type=button value=Close onClick=\"window.close()\"></form></center>");
      htmlFooter(out);
   }

   private void submitNewJob(String uid, String submit_uid, String home, String domain_name, 
      HttpServletRequest request, HttpServletResponse response) 
      throws Exception {

      openWorkspace(uid,home); // open paths to the information

      JobInfo ji = new JobInfo(request, home, uid);
      String line;

      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
      htmlHeader(out, "New Job Status", "");

    
      if (ji.xApp.equals("yes")) {
         try {
            SSOTokenManager manager = SSOTokenManager.getInstance();
            SSOToken token = manager.createSSOToken(request);

            // Does user already have a tunneled X desktop (provided by this
            // servlet) available?  If so, use that.  Otherwise, allocate an
            // unused X desktop session to the user and invoke it.
            xDisplayNum = token.getProperty(GEP_DESKTOP_ATTR);
            startOrStopVNCServer(true, token, out, uid);
         } catch (Exception e) {
            out.println("<pre>");
            e.printStackTrace(out);
            out.println("</pre>");
         }
      }

      // write to /tmp first and copy back 
      
      PrintWriter script =
          new PrintWriter(new BufferedWriter(new FileWriter(new File("/tmp/", uid+GEP_QSUB_SCRIPT))));

      script.println("#!/bin/ksh");
      script.println("# qsub script automatically generated by gep");
      script.println("#$ -N " + ji.jobName);
      script.println("#$ -S /bin/ksh");
      script.println("#$ -o " + ji.output);
      script.println("#$ -e " + ji.error);
      script.println("#$ -A " + ji.appName);
    
      if (ji.email.length() > 0) {
         script.println("#$ -M " + ji.email);
         script.println("#$ -m es");
      } else
         script.println("#$ -m n");
         
      script.println("#$ -cwd");
      script.println("#$ -v PATH -v SGE_ROOT -v COMMD_PORT -v SGE_CELL -v LD_LIBRARY_PATH");
      // Only go through CRE parallel == "mpi"
      if (ji.parallel.equals("mpi")) {
         script.println("#$ -l cre");
         script.println("#$ -pe hpc " + ji.cpu); 
      }

      // Retrieve environment variables, append to script
      Iterator i = ji.envVars.iterator();
      while (i.hasNext())
         script.println(i.next());

      if (ji.xApp.equals("yes")) {
         script.println("export DISPLAY=" + X_SERVER + ":" + xDisplayNum);
      }

      script.println("cat > tmp$$ << EOF");
      script.println(SGE_ROOT + "bin/" + SGE_ARCH + "/qacct -j $JOB_ID >> $SGE_STDOUT_PATH");
      script.println("EOF");
      script.println(SGE_ROOT + "bin/" + SGE_ARCH + "/qsub -hold_jid $JOB_ID -o /dev/null -e /dev/null tmp$$ > /dev/null");
      script.println("rm tmp$$");
      // Finish up qsub script
      // Only go through CRE parallel == "mpi"
      if (ji.parallel.equals("mpi"))
         script.println(SGE_ROOT + SGE_MPRUN + " -np $NSLOTS -Mf $TMPDIR/machines " + ji.binary + " " + ji.input);
      else
         script.println(ji.binary + " " + ji.input);

      script.close();

      // save the batch script to the project directory

      copyFileAsUser(ji.projectDirectory+"/"+GEP_QSUB_SCRIPT,
                        "/tmp/"+uid+GEP_QSUB_SCRIPT, uid, true);

      //
      // create the script to set the environment and do the the submission
      //
      PrintWriter sume =
         new PrintWriter(new BufferedWriter(new FileWriter(new File("/tmp", 
                                                uid+GEP_SU_SCRIPT))));
      sume.println("#!/bin/ksh");
      sume.println("cd " + ji.projectDirectory);
      for (int j=0; j<SGE_EXPORT.length; j++) 
         sume.println("export " + SGE_EXPORT[j]);
      sume.println(SGE_ROOT + "bin/" + SGE_ARCH + "/qsub " + GEP_QSUB_SCRIPT);
      sume.close();

      String sutasktmp = "/tmp/"+uid+GEP_SU_SCRIPT;
      String sutaskfile = ji.projectDirectory+"/"+GEP_SU_SCRIPT;

      copyFileAsUser(sutaskfile, sutasktmp, uid, true);
      //
      // make the files invisible to others
      //
      chmodFile(uid, "544", sutaskfile);
      chmodFile(uid, "444", ji.projectDirectory+"/"+GEP_QSUB_SCRIPT);


      // This is where the job gets launched
      Process qsub = Runtime.getRuntime().exec("su - " + submit_uid + 
                                               " -c " + sutaskfile);

      closeWorkspace(uid,home);

      // Get handle on job's stdout, stderr streams
      final BufferedReader job =
         new BufferedReader(new InputStreamReader(qsub.getInputStream()));
      final BufferedReader ejob =
         new BufferedReader(new InputStreamReader(qsub.getErrorStream()));

      // Send to html page
      out.println("<p>New Job Status:");
      out.println("<pre>");
      while ((line = ejob.readLine()) != null) 
         out.println(line);
      while ((line = job.readLine()) != null) 
         out.println(line);
      out.println("</pre>");
      out.println("<center><form><input type=button value=Close onClick=\"opener.location.reload(); window.close()\"></form></center>");
      htmlFooter(out);
   }

   private void getxDisplayNum(String uid, String home, HttpServletRequest request, HttpServletResponse response) 
      throws Exception {

      SSOTokenManager manager = SSOTokenManager.getInstance();
      SSOToken token = manager.createSSOToken(request);

      response.setContentType("text/html");
      PrintWriter out = response.getWriter();

      String xDisplayNum = token.getProperty(GEP_DESKTOP_ATTR);

      if (xDisplayNum == null)
         out.println("SSOToken property \"" + GEP_DESKTOP_ATTR + "\" is null");
      else
         out.println("SSOToken property \"" + GEP_DESKTOP_ATTR + "\" is \"" + xDisplayNum + "\"");
   }

   private void killJob(String uid, String home, HttpServletRequest request, HttpServletResponse response)
      throws Exception {
      String id = request.getParameter("id");
      String line;
      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
    
      htmlHeader(out, "Job Status", "");
      try {
         Process qdel =
            Runtime.getRuntime().exec(SGE_ROOT + "bin/" + SGE_ARCH + "/qdel " + id,
                                  SGE_EXPORT);
         BufferedReader jobs =
            new BufferedReader(new InputStreamReader(qdel.getInputStream()));
         BufferedReader error =
            new BufferedReader(new InputStreamReader(qdel.getErrorStream()));
      
         out.println("<pre>");
         while ((line = jobs.readLine()) != null) 
            out.println(line);
         while ((line = error.readLine()) != null) 
            out.println(line);
         out.println("</pre>");
      } catch (Exception e) {
         out.println("<pre>");
         e.printStackTrace(out);      
         out.println("</pre>");
      }
      out.println("<center><form><input type=button value=Continue onClick=\"opener.location.reload(); window.close()\"></form></center>");
      htmlFooter(out);
   }

   private void applicationList(String uid, HttpServletResponse response) 
      throws Exception {
      StringTokenizer st;
      String line, app, text;
      boolean dismiss = false;
      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
    
      try {
         BufferedReader list =
            new BufferedReader(new FileReader(APP_HOME_DIR + GEP_LIST));
    
         out.println("<html><body><table width=100%>");
         while ((line = list.readLine()) != null) {
            st = new StringTokenizer(line, "\t");
            app = st.nextToken();
            text = st.nextToken();
            if (st.hasMoreTokens()) {
               dismiss = true;
               while (st.hasMoreTokens()) {
                  if (st.nextToken(" \t").equals(uid)) {
                     dismiss = false;
                     break;
                  }
               }
               if (dismiss) 
                  continue;
            }
            out.println("<tr><td><li><a href=\"GEPServlet?action=applicationInfo&application="
                    + app + "\" target=GEPServletApplication>");
            out.println(text + "</a></li></td></tr>");
         }
         out.println("</table></body></html>");
      } catch (FileNotFoundException e) {
         out.println("<html><body>There are no applications available.</body></html>");      
      }
   }     

   private void applicationAdminList(HttpServletResponse response) throws Exception {
      StringTokenizer st;
      String line, app;

      response.setContentType("text/html");
      PrintWriter out = response.getWriter();

      out.println("<html><body>");
      try {
         BufferedReader list = new BufferedReader(new FileReader(APP_HOME_DIR + GEP_LIST));

         out.println("<table width=100%>");
         while ((line = list.readLine()) != null) {
            st = new StringTokenizer(line, "\t");
            app = st.nextToken();
            out.println("<tr><td><li><a href=\"GEPServlet?action=applicationAdminInfo&application="
                    + app + "\" target=GEPServletApplication>");
            out.println(st.nextToken() + "</a></li></td>");
            out.println("<td><a href=URL onCLick=\"window.open(\'GEPServlet?action=editAdminApplicationForm&application="
                    + app + "\', \'GEPServletApplication\'); return false\">edit</a></td>");
            out.println("<td><a href=URL onCLick=\"window.open(\'GEPServlet?action=deleteAdminApplication&application="
                    + app + "\', \'GEPServletApplication\'); return false\">delete</a></td></tr>");
         }
         out.println("</table>");
      } catch (FileNotFoundException e) {
         out.println("There are no applications available.");
      }
      out.println("<table width=100%><tr><td align=right>");
      out.println("<a href=URL onClick=\"window.open(\'GEPServlet?action=newAdminApplicationForm\',\'TCPApplication\'); return false\">");
      out.println("Add a new application...</a>");
      out.println("</td></tr></table>");
      out.println("</body></html>");
   }

   private void newAdminApplicationForm(HttpServletResponse response) throws Exception {
      StringTokenizer st;
      String line;

      response.setContentType("text/html");
      PrintWriter out = response.getWriter();

      htmlHeader(out, "New Application Form", "");

      out.println("<form method=post action=GEPServlet enctype=multipart/form-data>");
      out.println("<input type=hidden name=action value=createNewAdminApplication>");
      out.println("<table>");
      out.println("<tr><td>Application name:</td>");
      out.println("<td><input type=text name=name1 size=25></td></tr>");
      //out.println("<tr><td>Description:</td>");
      //out.println("<td><input type=text name=name2 size=25></td></tr>");
      out.println("<tr><td>Binary or script name:</td>");
      out.println("<td><input type=text name=binary size=25></td></tr>");
      out.println("<tr><td>Parallel mode:</td>");
      out.println("<td><select name=parallel>");
      out.println("<option value=no>sequential");
      out.println("<option value=mpi>message passing");
      out.println("<option value=smp>multithreaded");
      out.println("</select></td></tr>");
      out.println("<tr><td><input type=checkbox name=xApp value=yes>");
      out.println("check here if App uses X11 for graphical display</td></tr>");
      out.println("<tr><td>Customized form:</td>");
      out.println("<td><select name=form>");
      out.println("<option value=no>no");
      out.println("<option value=yes>yes");
      out.println("</select></td></tr>");
      out.println("<tr><td>Form name:</td>");
      out.println("<td><input type=text name=namef size=25></td></tr>");
      out.println("<tr><td>Input file:</td>");
      out.println("<td><input type=file name=input size=25></td></tr>");
      out.println("<tr><td colspan=2><input type=checkbox name=tar value=yes>");
      out.println("check here if this file is a compressed tar archive.</td></tr>");
      out.println("<tr><td colspan=2><input type=checkbox name=acl value=yes>");
      out.println("user access list? <input type=text name=users size=25></td></tr>");
      out.println("<tr><td valign=top>Environment variables:</td>");
      out.println("<td><textarea name=export rows=5 cols=25></textarea></td></tr>");
      out.println("</table>");
      out.println("<center><input type=submit value=Submit></center>");
      out.println("</form>");

      htmlFooter(out);
   }

   private void createNewAdminApplication(MultipartRequest multi, PrintWriter out)
      throws Exception {
      String name = "A" + System.currentTimeMillis();
      String path = APP_HOME_DIR + name;
      File directory = new File(path);
      directory.mkdirs();
      Runtime.getRuntime().exec("/usr/bin/chmod 755 " + path).waitFor();
      
      String name1 = multi.getParameter("name1").trim();
      //String name2 = multi.getParameter("name2").trim();
      String binary = multi.getParameter("binary").trim();
      String parallel = multi.getParameter("parallel");
      String xApp = multi.getParameter("xApp");
      String form = multi.getParameter("form");
      String namef = multi.getParameter("namef").trim();
      String export = multi.getParameter("export");
      String acl = multi.getParameter("acl");
      String users = multi.getParameter("users").trim();
    
      htmlHeader(out, "New Application Status", "");
    
      Enumeration files = multi.getFileNames();
      if (files.hasMoreElements())
         if (multi.getParameter("tar") != null) {
            String filename = multi.getFilesystemName((String)files.nextElement());
            PrintWriter sume = new PrintWriter(new BufferedWriter(new FileWriter(new File(directory, GEP_SU_SCRIPT))));
            sume.println("cd " + path);
            sume.println("gunzip -c " + filename + " | tar xf -");
            sume.close();
            Runtime.getRuntime().exec("/usr/bin/chmod 755 " + path + "/" + GEP_SU_SCRIPT).waitFor();
            Runtime.getRuntime().exec("/usr/bin/chmod 644 " + path + "/" + filename).waitFor();
      
            Runtime.getRuntime().exec("/usr/bin/ksh exec " + path + "/" + GEP_SU_SCRIPT).waitFor();
      
            if (form.equals("yes"))
               Runtime.getRuntime().exec("cp " + path + "/" + namef + " " + path + "/" + GEP_APP_FORM);
         } else {
            Runtime.getRuntime().exec("/usr/bin/chmod 755 " + path + "/" + binary).waitFor();
         } 
         if (name1.length() > 0) {
            String listfile = APP_HOME_DIR + GEP_LIST;
            PrintWriter list = new PrintWriter(new BufferedWriter(new FileWriter(listfile, true)));
            list.print(name + "\t" + name1);
            if (multi.getParameter("acl") != null) {
               list.print("\t" + users);
            }
            list.println();
            list.close();

            PrintWriter ressource = new PrintWriter(new BufferedWriter(new FileWriter(new File(path, GEP_APP))));
            ressource.println(name1);
            ressource.println(binary);
            ressource.println(parallel);
            ressource.println(new String("yes").equals(xApp) ? "yes" : "no");
            ressource.println(form);
            ressource.println(users);
      
            try {
               StringTokenizer st_equal, st_newline = new StringTokenizer(export, "\n\r\f");
               String value;
               while (st_newline.hasMoreTokens()) {
                  st_equal = new StringTokenizer(st_newline.nextToken(), "= \t");
                  ressource.print(st_equal.nextToken() + "=");
                  value = st_equal.nextToken();
                  if (value.equals("$HOME")) {
                     ressource.println(path);
                  } else {
                     ressource.println(value);
                  }
               }
               out.println("<p>Application <i>" + name1 + "</i> was created successfully.");
            } catch (NoSuchElementException e) {
               out.println("<p>Error parsing export variables. You can add/correct variables by editing the newly created application.");
            }
            out.println("<center><form><input type=button value=Continue onClick=\"opener.location.reload(); window.close()\"></form></center>");
            ressource.close();
         } else {
            Runtime.getRuntime().exec("/usr/bin/rm -rf " + path);           
            out.println("<p>Go back and enter a non-empty application name.");
            out.println("<center><form><input type=button value=Back onClick=window.back()></form></center>");
         }
         htmlFooter(out);
      }

   private void editAdminApplicationForm(HttpServletRequest request, HttpServletResponse response) throws Exception {
      StringTokenizer st;
      String line;
      String id = request.getParameter("application");
      String directory = APP_HOME_DIR + id;

      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
    
      htmlHeader(out, "Application Form", "");
      try {
         BufferedReader read = new BufferedReader(new FileReader(new File(directory, GEP_APP)));
    
         String app = read.readLine();
         String binary = read.readLine();
         String parallel = read.readLine();
         String xApp = read.readLine();
         String form = read.readLine();
         String users = read.readLine();
         Vector export = new Vector();
         while ((line = read.readLine()) != null) {
            export.addElement(line);
         }
         read.close();

         out.println("<form method=get action=GEPServlet>");
         out.println("<input type=hidden name=action value=updateAdminApplication>");
         out.println("<input type=hidden name=id value=" + id + ">");
         out.println("<table>");
         out.println("<tr><td>Application name:</td>");
         out.println("<td><input type=text name=name1 value=\""
                  + app + "\" size=25></td></tr>");
         out.println("<tr><td>Binary name:</td>");
         out.println("<td><input type=text name=binary value=\""
                  + binary + "\" size=25></td></tr>");

         out.println("<tr><td>Parallel mode:</td>");
         out.println("<td><select name=parallel>");
         out.println("<option" + ((parallel.equals("no")) ? " selected " : " ") + "value=no>sequential");
         out.println("<option" + ((parallel.equals("mpi")) ? " selected " : " ") + "value=mpi>message passing");
         out.println("<option" + ((parallel.equals("smp")) ? " selected " : " ") + "value=smp>multithreaded");
         out.println("</select></td></tr>");
         out.println("<tr><td><input type=checkbox name=xApp value=yes" + (xApp.equals("yes") ? " checked>" : ">"));
         out.println("check here if App uses X11 for graphical display</td></tr>");
         out.println("<tr><td>Customized form:</td>");
         out.println("<td><select name=form>");
         out.println("<option" + ((form.equals("no")) ? " selected " : " ") + "value=no>no");
         out.println("<option" + ((form.equals("yes")) ? " selected " : " ") + "value=yes>yes");
         out.println("</select></td></tr>");  
         out.println("<tr><td colspan=2><input type=checkbox name=acl value=yes"
                  + (users.equals("") ? ">" : " checked>"));
         out.println("user access list? <input type=text name=users value=\""
                  + users + "\" size=25></td></tr>");

         out.println("<tr><td valign=top>Environment variables:</td>");
         out.println("<td><textarea name=export rows=5 cols=25>");
         for (int i = 0; i < export.size(); i++) {
            out.println((String)export.get(i));
         }
         out.println("</textarea></td></tr>");
         out.println("</table>");
         out.println("<center><input type=submit value=Submit></center>");
         out.println("</form>");
      } catch (FileNotFoundException e) {
         out.println("Error accessing selected application.");      
      }
      htmlFooter(out);
   }     

   private void updateAdminApplication(HttpServletRequest request, HttpServletResponse response)
      throws Exception {
      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
    
      String id = request.getParameter("id");
      String name1 = request.getParameter("name1").trim();
      String binary = request.getParameter("binary");
      String parallel = request.getParameter("parallel");
      String xApp = request.getParameter("xApp");
      String form = request.getParameter("form");
      String export = request.getParameter("export");
      String users = request.getParameter("users").trim();
    
      htmlHeader(out, "Project Status", "");
      if (name1.length() > 0) {
         Vector list = new Vector();
         String line;
      
         BufferedReader read = new BufferedReader(new FileReader(APP_HOME_DIR + GEP_LIST));
         while ((line = read.readLine()) != null) {
            if (!((new StringTokenizer(line, "\t")).nextToken().equals(id))) {
               list.addElement(line);
            }
         }
         read.close();
      
         PrintWriter write = new PrintWriter(new BufferedWriter(new FileWriter(APP_HOME_DIR + GEP_LIST)));
         for (int i = 0; i < list.size(); i++) {
            write.println((String)list.get(i));
         }
         write.print(id + "\t" + name1);
         if (request.getParameter("acl") != null) {
            write.print("\t" + users);
         }
         write.println();
         write.close();

         PrintWriter ressource = new PrintWriter(new BufferedWriter(new FileWriter(new File(APP_HOME_DIR + id, GEP_APP))));
         ressource.println(name1);
         ressource.println(binary);
         ressource.println(parallel);
         ressource.println(new String("yes").equals(xApp) ? "yes" : "no");
         ressource.println(form);
         ressource.println(users);
         try {
            String value;
            StringTokenizer st_equal, st_newline = new StringTokenizer(export, "\n\r");
            while (st_newline.hasMoreTokens()) {
               st_equal = new StringTokenizer(st_newline.nextToken(), "= \t");
               ressource.print(st_equal.nextToken() + "=");
               value = st_equal.nextToken();
               if (value.equals("$HOME")) {
                  ressource.println(APP_HOME_DIR + id);
               } else {
                  ressource.println(value);
               }
            }
            out.println("<p>Application <i>" + name1 + "</i> was updated successfully.");
         } catch (NoSuchElementException e) {
            out.println("<p>Error parsing export variables. You can add/correct variables by editing again the application.");
         }
         out.println("<center><form><input type=button value=Continue onClick=\"opener.location.reload(); window.close()\"></form></center>");
         ressource.close();
      } else {
         out.println("<p>Go back and enter a non-empty application name.");
         out.println("<center><form><input type=button value=Back onClick=window.back()></form></center>");
      }
      htmlFooter(out);
   }

   private void deleteAdminApplication(HttpServletRequest request, HttpServletResponse response)
      throws Exception {
      String app = request.getParameter("application");
      String line;
      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
    
      htmlHeader(out, "Application Status", "");
      try {
         Vector list = new Vector();
      
         Runtime.getRuntime().exec("/usr/bin/rm -rf " + APP_HOME_DIR + app); 
         File listfile = new File(APP_HOME_DIR + GEP_LIST);
         BufferedReader read = new BufferedReader(new FileReader(listfile));
         while ((line = read.readLine()) != null) {
            if (!((new StringTokenizer(line, "\t")).nextToken().equals(app))) {
               list.addElement(line);
            }
         }
         read.close();
      
         if (list.size() > 0) {
            PrintWriter write = new PrintWriter(new BufferedWriter(new FileWriter(listfile)));
            for (int i = 0; i < list.size(); i++) {
               write.println((String)list.get(i));
            }
            write.close();
         } else {
            listfile.delete();
         }
         out.println("The application was successfully deleted.");
      } catch (Exception e) {
         out.println("<pre>");
         e.printStackTrace(out);      
         out.println("</pre>");
      }
      out.println("<center><form><input type=button value=Continue onClick=\"opener.location.reload(); window.close()\"></form></center>");
      htmlFooter(out);
   }

   private void jobAdminAccounting(HttpServletResponse response)
      throws Exception {
      String line, token;
      StringTokenizer st;
      int nb, uid, aid;
      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
    
      htmlHeader(out, "Job Accounting", "");
      try {
         try {
            BufferedReader acct = new BufferedReader(new FileReader(SGE_ROOT + SGE_ACCT));
            // SGE_ACCT has owner, account (= application), wall time in columns 4, 7, 14
            out.println("<table border=\"2\"><tr><th>&nbsp;</th>");
            Vector account = new Vector();
            try {
               BufferedReader app = new BufferedReader(new FileReader(APP_HOME_DIR + GEP_LIST));
               while ((line = app.readLine()) != null) {
                  st = new StringTokenizer(line, "\t");
                  account.addElement(st.nextToken());
                  out.println("<th>" + st.nextToken() + "</th>");
               }
            } catch (FileNotFoundException e) {}
            account.addElement("sge");
            out.println("<th>Others</th></tr>");
            nb = account.size();
      
            Vector user = new Vector();
            Vector time = new Vector();
            while ((line = acct.readLine()) != null) {
               if (line.startsWith("#")) {
                  continue;
               }   
               st = new StringTokenizer(line, ":");
               st.nextToken(); st.nextToken(); st.nextToken();
               token = st.nextToken();
               if (!user.contains(token)) {
                  user.addElement(token);
                  time.addElement(new int[nb]);
               }
               uid = user.indexOf(token);
               st.nextToken(); st.nextToken();
               aid = account.indexOf(st.nextToken());
               if (aid == -1) {
                  aid = nb - 1;
               }
               st.nextToken(); 
               st.nextToken(); 
               st.nextToken(); 
               st.nextToken(); 
               st.nextToken(); 
               st.nextToken();
               ((int[])time.get(uid))[aid] += Integer.parseInt(st.nextToken());
            }
      
            int[] tab;
            for (uid = 0; uid < user.size(); uid++) {
               out.println("<tr><td>" + (String)user.get(uid) + "</td>");
               tab = (int[])time.get(uid);
               for (aid = 0; aid < nb; aid++) {
                  out.println("<td align=center>" + tab[aid] + "</td>");
               }
               out.println("</tr>");
            }
            out.println("</table>");
         } catch (FileNotFoundException e) {
            out.println("There is no recorded activity.");
         }
      } catch (Exception e) {
         out.println("</table>");
         out.println("<pre>");
         e.printStackTrace(out);      
         out.println("</pre>");
      }
      out.println("<center><form><input type=button value=Continue onClick=window.close()></form></center>");
      htmlFooter(out);
   }

   private void installationInfo(HttpServletRequest request, HttpServletResponse response)
      throws Exception {

      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
    
      htmlHeader(out, "Installation Information", "");
      out.println("<pre>");
      try {
         SSOTokenManager manager = SSOTokenManager.getInstance();
         SSOToken token = manager.createSSOToken(request);
         if (manager.getInstance().isValidToken(token)) {
            String host = token.getHostName();
            String authType = token.getAuthType();
            int level = token.getAuthLevel();
      out.println("host:" + host);
      out.println("authType:" + authType);
      out.println("level:" + level);
            AMStoreConnection amConn = new AMStoreConnection(token);
            String userDN = token.getPrincipal().getName();
            AMUser amUser = amConn.getUser(userDN);
      out.println("userDN:" + userDN);
      out.println("amUser:" + amUser);
            Map userAttrs = amUser.getAttributes();
            for (Iterator i=userAttrs.entrySet().iterator(); i.hasNext(); ) {
               Map.Entry e = (Map.Entry) i.next();
               out.println(e.getKey() + ": " + e.getValue());
            }
         } else {
            // redirect to login page
            out.println("<pre>");
            out.println("no valid token");
         }
         for (int i=0; i < SGE_EXPORT.length; i++) {
            out.println("SGE_EXPORT[" + i + "] = " + SGE_EXPORT[i]);
         }   
         out.println("</pre>");
         out.println("</td></tr></table>");
      } catch (Exception e) {
         out.println("<pre>");
         e.printStackTrace(out);      
         out.println("</pre>");
      }
      out.println("<center><form><input type=button value=Continue onClick=window.close()></form></center>");
      htmlFooter(out);
   }
  
   private void applicationInfo(HttpServletRequest request, HttpServletResponse response)
      throws Exception {
      String application = request.getParameter("application");
      String line;

      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
    
      htmlHeader(out, "Application Information", "");
      try {
         BufferedReader info =
            new BufferedReader(new FileReader(new File(APP_HOME_DIR + application, GEP_APP)));
    
         out.println("<table>");
         out.println("<tr><td>Application name:</td><td>" + info.readLine() + "</td></tr>");
         out.println("<tr><td>Binary name:</td><td>" + info.readLine() + "</td></tr>");
         out.println("<tr><td>Parallel mode:</td><td>" + info.readLine() + "</td></tr>");
         out.println("<tr><td>Uses X11:</td><td>" + info.readLine() + "</td></tr>");
         out.println("<tr><td>Customized form:</td><td>" + info.readLine() + "</td></tr>");
         out.println("<tr><td>User access list:</td><td>" + info.readLine() + "</td></tr>");
         out.println("<tr><td valign=top>Exports:</td><td>");
         while ((line = info.readLine()) != null) {
            out.println(line + "<br>");
         }  
         out.println("</td></tr></table>");
      } catch (FileNotFoundException e) {
         out.println("<pre>");
         e.printStackTrace(out);      
         out.println("</pre>");
      }
      out.println("<center><form><input type=button value=Continue onClick=window.close()></form></center>");
      htmlFooter(out);
   }

   private void applicationAdminInfo(HttpServletRequest request, HttpServletResponse response)
      throws Exception {
      String app = request.getParameter("application");
      String line;
      String path = APP_HOME_DIR + app;

      response.setContentType("text/html");
      PrintWriter out = response.getWriter();

      htmlHeader(out, "Application Information", "");
      try {
         BufferedReader info = new BufferedReader(new FileReader(new File(path, GEP_APP)));

         out.println("<table>");
         out.println("<tr><td>Application name:</td><td>" + info.readLine() + "</td></tr>");
         out.println("<tr><td>Binary name:</td><td>" + info.readLine() + "</td></tr>");
         out.println("<tr><td>Parallel mode:</td><td>" + info.readLine() + "</td></tr>");
         out.println("<tr><td>Uses X11:</td><td>" + info.readLine() + "</td></tr>");
         out.println("<tr><td>Customized form:</td><td>" + info.readLine() + "</td></tr>");
         out.println("<tr><td>User access list:</td><td>" + info.readLine() + "</td></tr>");

         out.println("<tr><td valign=top>Exports:</td>");
         if ((line = info.readLine()) != null)
            out.println("<td>" + line + "</td>");
         out.println("</tr>");
         while ((line = info.readLine()) != null)
            out.println("<tr><td>&nbsp;</td><td>" + line + "</td></tr>");

         File directory = new File(path);
         String[] files = directory.list();

         out.println("<tr><td valign=top>Files:</td>");
         if (files.length > 0) {
            out.println("<td>" + files[0] + "</td>");
            out.println("<td>&nbsp;</td><td><a href=\"GEPServlet?action=viewAdminFile&view=head&application="
                    + app + "&file=" + files[0] + "\">head</a></td>");
            out.println("<td>&nbsp;</td><td><a href=\"GEPServlet?action=viewAdminFile&view=tail&application="
                    + app + "&file=" + files[0] + "\">tail</a></td>");
         }
         out.println("</tr>");
         for (int i = 1; i < files.length; i++) {
            out.println("<tr><td>&nbsp;</td><td>" + files[i] + "</td>");
            out.println("<td>&nbsp;</td><td><a href=\"GEPServlet?action=viewAdminFile&view=head&application="
                      + app + "&file=" + files[i] + "\">head</a></td>");
            out.println("<td>&nbsp;</td><td><a href=\"GEPServlet?action=viewAdminFile&view=tail&application="
                      + app + "&file=" + files[i] + "\">tail</a></td>");
         }

         out.println("</table>");
      } catch (FileNotFoundException e) {
         out.println("<pre>");
         e.printStackTrace(out);
         out.println("</pre>");
      }
      out.println("<center><form><input type=button value=Continue onClick=window.close()></form></center>");
      htmlFooter(out);
   }

   private void htmlHeader(PrintWriter page, String title, String headInfo) {
      page.println("<html><head>" + headInfo + "</head><body bgcolor=FFFFFF>");
      page.println("<table border=0 cellpadding=2 cellspacing=0 width=100% bgcolor=888888>");    
      page.println("<tr><td bgcolor=666699><font color=FFFFFF><b>GEPServlet "
                 + title + "</b></font></td></tr><tr><td bgcolor=DDDDDD>");    
   }
  
   private void htmlFooter(PrintWriter page) {
      page.println("</td></tr><tr><td bgcolor=000000 align=center>");
      page.println("<font color=FFFFFF>Copyright 2001 Sun Microsystems, Inc. All rights reserved.</font></td></tr>");    
      page.println("</table></body></html>");    
   }

   private void viewFile(String uid, String home, HttpServletRequest request, HttpServletResponse response)
      throws Exception {
      String project = request.getParameter("project");
      String view = request.getParameter("view");
      String file = request.getParameter("file");
      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
      final int NBLINES = 40;
      String line;
    
      openWorkspace(uid,home);

      htmlHeader(out, "File View", "");
      out.println("<pre>");
      try {
         String directory = home + project;
      
         if (view.equals("view")) {
            BufferedReader zaq =
               new BufferedReader(new FileReader(new File(directory, file)));
            while ((line = zaq.readLine()) != null) 
               out.println(line);
            zaq.close();
         } else if (view.equals("head")) {
            BufferedReader head =
               new BufferedReader(new FileReader(new File(directory, file)));
            for (int i = 0; ((line = head.readLine()) != null) && (i < NBLINES); i++)
               out.println(line);
            head.close();
         } else if (view.equals("tail")) {
            Process ptail =
               Runtime.getRuntime().exec("/usr/bin/tail -" + NBLINES + " " + directory + "/" + file);
            BufferedReader tail =
               new BufferedReader(new InputStreamReader(ptail.getInputStream()));
            while ((line = tail.readLine()) != null)
               out.println(line);
         } else
            out.println("Unrecognized view option.");
      } catch (Exception e) {
         e.printStackTrace(out);      
      }
      closeWorkspace(uid,home);

      out.println("</pre>");
      out.println("<center><form><input type=button value=Continue onClick=\"opener.location.reload(); window.close()\"></form></center>");  
      htmlFooter(out);
   }

   private void viewAdminFile(HttpServletRequest request, HttpServletResponse response)
      throws Exception {
      String app = request.getParameter("application");
      String view = request.getParameter("view");
      String file = request.getParameter("file");
      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
      final int NBLINES = 40;
      String line;
    
      htmlHeader(out, "File View", "");
      out.println("<pre>");
      try {
         String directory = APP_HOME_DIR+ app;
      
         if (view.equals("head")) {
            BufferedReader head = new BufferedReader(new FileReader(new File(directory, file)));
            for (int i = 0; ((line = head.readLine()) != null) && (i < NBLINES); i++) {
               out.println(line);
            }
            head.close();
         } else if (view.equals("tail")) {
            Process ptail = Runtime.getRuntime().exec("/usr/bin/tail -" + NBLINES + " " + directory + "/" + file);
            BufferedReader tail = new BufferedReader(new InputStreamReader(ptail.getInputStream()));
            while ((line = tail.readLine()) != null) {
               out.println(line);
            }
         } else {
            out.println("Unrecognized view option.");
         }
      } catch (Exception e) {
         e.printStackTrace(out);      
      }
      out.println("</pre>");
      htmlFooter(out);
   }
  
   private void downloadFile(String uid, String home, HttpServletRequest request, HttpServletResponse response)
      throws Exception {
      String project = request.getParameter("project");
      String name = request.getParameter("file");
      String directory = home + project;
    
      openWorkspace(uid,home);

      File file = new File(directory, name);
      BufferedInputStream in = new BufferedInputStream(new FileInputStream(file));
      response.setContentType("application/binary");
      ServletOutputStream out = response.getOutputStream();
      long offset = 0, length = file.length();
      int BLOCK = 8 * 1024, nb;
      byte[] data = new byte[BLOCK];

      try {
         while (offset < length) {
            nb = in.read(data, 0, BLOCK);
            out.write(data, 0, nb);
            offset += nb;
         }
      } catch (IOException e) {
      } finally {
         in.close();
         out.close();
         data = null;
      }
      closeWorkspace(uid,home);
   }
  
   private void notYetImplemented(HttpServletResponse response) throws Exception {
      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
      htmlHeader(out, "Status", "");
      out.println("Not yet Implemented.");
      out.println("<center><form><input type=button value=Continue onClick=window.close()></form></center>");
      htmlFooter(out);
   }
  
   private void unknownAction(String uid, HttpServletResponse response) throws Exception {
      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
      out.println("<html><body>");
      out.println("Unkown action for user " + uid);
      out.println("</body></html>");
   }

   private void deleteFile(String uid, String home, HttpServletRequest request, HttpServletResponse response)
      throws Exception {
      String project = request.getParameter("project");
      String file = request.getParameter("file");
      String line;
      response.setContentType("text/html");
      PrintWriter out = response.getWriter();

      openWorkspace(uid,home);
    
      htmlHeader(out, "File Status", "");
      monitor.log("MONITOR " + uid + " Deleting file "+project+"/"+file);
      try {
         synchronized (Class.forName("com.sun.gep.GEPServlet")) {
            String directory = home + project;
      
            chmodFile(uid,"o+w", directory);
            chmodFile(uid,"o+w", directory+"/"+file);

            removeFile(uid,directory+"/"+file);

            out.println("The file was successfully deleted.");
            monitor.log("MONITOR " + uid + " Deletion successful for file "+project+"/"+file);
         } // synchronized
      } catch (Exception e) {
            monitor.log("MONITOR " + uid + " Deletion error for file "+project+"/"+file);
            monitor.log("MONITOR " + uid + " Deletion error exception "+e.toString());
            out.println("Error accessing this file.");
      }

      closeWorkspace(uid,home);

      out.println("<center><form><input type=button value=Continue onClick=\"opener.location.reload(); window.close()\"></form></center>");  
      htmlFooter(out);
   }
}
