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

package com.sun.tcp;

import java.io.*;
import java.util.*;
import java.net.*;
import javax.servlet.*;
import javax.servlet.http.*;

import com.iplanet.portalserver.session.*;

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

public class SunTCP extends HttpServlet {
   private String APP_HOME_DIR = "/export/apps/";
   private String SGE_ROOT = "/export/gridengine/";
   private String SGE_CELL = "default";
   private String COMMD_PORT = "667";
   private String[] SGE_EXPORT = {
      "SGE_ROOT=/export/gridengine",
      "SGE_CELL=default",
      "COMMD_PORT=667",
      "LD_LIBRARY_PATH=/export/gridengine",
      "SGP_ROOT=/gridware/Tools/SGP",
      "PATH=/bin:/usr/bin:/usr/openwin/bin"
   };
   private String SGE_ARCH;
   private String SGE_MPRUN = "mpi/MPRUN";
   private String SUNTCP_QSUB_SCRIPT = ".suntcp-qsub";
   private String SUNTCP_SU_SCRIPT = ".suntcp-su";
   private String SUNTCP_PROJECT_DIR = "/suntcp/";
   private String SUNTCP_LIST = ".suntcp-list";
   private String SUNTCP_PROJECT = ".suntcp-project";
   private String SUNTCP_APP = ".suntcp-app";
   private String SUNTCP_APP_FORM = ".suntcp-form";
   private String SUNTCP_DESKTOP_ATTR = "xdesktop";
   private String X_NETLET = "Xvnc";
   private String X_SERVER;
   private String VNCSERVER = "vncserver";
   private String SGP_ROOT = "/gridware/Tools/SGP";
   private String GETWORKSPACE = "/gridware/Tools/SGP/bin/gethomedir";
   private String ADMINRUN = "adminrun";
   private String xDisplayNum = null;
   private RequestDispatcher reqDisp;

   // Inner class for receiving iPlanet session chenge events.  Used to
   // shutdown vncserver when session no longer valid.
   class SessionChange implements SessionListener {
      public void sessionChanged(SessionEvent se) {
         int type = se.getType();

         // If session is history, shutdown vncserver
         if (type == SessionEvent.IDLE_TIMEOUT ||
             type == SessionEvent.MAX_TIMEOUT ||
             type == SessionEvent.LOGOUT ||
             type == SessionEvent.DESTROY) {
            String uid = se.getSession().getClientID();
            try {
               startOrStopVNCServer(false, null, null, uid);
            } catch (Exception sex) {
               // Should never happen when called with a null session parameter
            }
         }
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
         email = request.getParameter("email");
         input = request.getParameter("input");
         output = request.getParameter("output");
         error = request.getParameter("error");
         cpu = request.getParameter("cpu");

         // Compute derived values
         projectDirectory = home + project;
         projectInfo = new BufferedReader(new FileReader(new File(projectDirectory, SUNTCP_PROJECT)));
         jobName = projectInfo.readLine().replace(' ', '_').replace('\'', '_');
         appName = projectInfo.readLine();
         appInfo = new BufferedReader(new FileReader(new File(APP_HOME_DIR + appName, SUNTCP_APP)));
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
      }
   }


   public void init(ServletConfig config) throws ServletException {
      String s;
      super.init(config);
    
      s = getInitParameter("app_home");
      if (s != null) {
         APP_HOME_DIR = s;
      }  
      s = getInitParameter("sgp_root");
      if (s != null) {
         SGP_ROOT = s;
         SGE_EXPORT[4] = "SGP_ROOT=" + s;
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


      SGE_EXPORT[3] = "LD_LIBRARY_PATH="+ SGE_ROOT + "lib/" + SGE_ARCH;

      GETWORKSPACE = SGP_ROOT + "bin/gethomedir " + SGP_ROOT + " ";
      ADMINRUN = SGE_ROOT + "utilbin/" + SGE_ARCH + "/adminrun ";
   }

   private String getWorkspace(String uid, String mapped_uid)
      throws IOException, InterruptedException {
      String workdir;

      Process test_user = Runtime.getRuntime().exec(GETWORKSPACE + uid);
      test_user.waitFor();
      if ( test_user.exitValue() == 0 ) {
         BufferedReader test_home = new BufferedReader(new InputStreamReader(test_user.getInputStream())); 
         workdir = test_home.readLine() + SUNTCP_PROJECT_DIR;
      } else {
         Process test_user2 = Runtime.getRuntime().exec(GETWORKSPACE + 
                                                         mapped_uid);
         test_user2.waitFor();
         BufferedReader test_home2 = new BufferedReader(new InputStreamReader(test_user2.getInputStream())); 
         workdir = test_home2.readLine() + uid + SUNTCP_PROJECT_DIR;

      }
      return workdir;
   }

   public void doGet(HttpServletRequest request, HttpServletResponse response)
      throws ServletException, IOException {
      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
      try {
            
         SessionID sid = new SessionID(request);
         String uid = Session.getSession(sid).getClientID();
         String domain_name = Session.getSession(sid).getClientDomain().substring(1);

         String submit_uid = uid;

         String home = getWorkspace(uid, domain_name);
         String action = request.getParameter("action");
      
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
            jobInfo(uid, home, request, response);
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
         else
            unknownAction(uid, response);
      } catch (Exception e) {
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
         SessionID sid = new SessionID(request);
         String uid = Session.getSession(sid).getClientID();
         String domain_name = Session.getSession(sid).getClientDomain().substring(1);
         String submit_uid = uid;
         String home = getWorkspace(uid, domain_name);
      
         String name = "T" + System.currentTimeMillis();
         String tmpdir = "/tmp/" + name;
         Runtime.getRuntime().exec("mkdir -m 777 -p " + tmpdir).waitFor();
         PrintWriter sume;
         MultipartRequest multi = 
            new MultipartRequest(request, tmpdir, 100*1024*1024); // 100MB
         String action = multi.getParameter("action");
         String project = multi.getParameter("project").trim();
         String app = multi.getParameter("app");
         String export = multi.getParameter("export");
         String filename = null;
    
         Enumeration files = multi.getFileNames();
         if (files.hasMoreElements()) {
            filename = multi.getFilesystemName((String)files.nextElement());
            Runtime.getRuntime().exec("/usr/bin/chmod 644 " + tmpdir + "/" + filename).waitFor();
            if (multi.getParameter("tar") != null) {
               sume = new PrintWriter(new BufferedWriter(new FileWriter(new File(tmpdir, SUNTCP_SU_SCRIPT))));
               sume.println("cd " + tmpdir);
               sume.println("gunzip -c " + filename + " | tar xf -");
               sume.println("chmod -R 644 .");
               sume.close();
               Runtime.getRuntime().exec("/usr/bin/chmod 755 " + tmpdir + "/" + SUNTCP_SU_SCRIPT).waitFor();
               Process zip = Runtime.getRuntime().exec("su - " + submit_uid + " -c " + tmpdir + "/" + SUNTCP_SU_SCRIPT);
            }
         }
         if (action.equals("createNewProject")) {
            createNewProject(uid, submit_uid, home, tmpdir, filename, project, app, export, request, out);
         } else if (action.equals("updateProject"))  {
            String id = multi.getParameter("id");
            updateProject(uid, submit_uid, home, id, tmpdir, filename, project, app, export, request, response);  
         } else {
            unknownAction(uid, response);
         }   
      } catch (Exception e) {
         out.println("<html><body><pre>");
         e.printStackTrace(out);
         out.println("</pre></body></html>");
      }
   }

   private void projectList(String uid, String home, HttpServletResponse response) throws Exception {
      StringTokenizer st;
      String line, project;
      String directory = home;

      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
    
      out.println("<html><body>");    
      try {
         synchronized (Class.forName("com.sun.tcp.SunTCP")) {
            BufferedReader list =
            new BufferedReader(new FileReader(directory + SUNTCP_LIST));
    
            out.println("<table width=100%>");
            while ((line = list.readLine()) != null) {
               st = new StringTokenizer(line, "\t");
               project = st.nextToken();
               out.println("<tr><td><li><a href=\"SunTCP?action=projectInfo&project="
                    + project + "\" target=SunTCPProject>");
               out.println(st.nextToken() + "</a></li></td>");
               out.println("<td><a href=\"/Edit Project\" onClick=\"window.open(\'SunTCP?action=editProjectForm&project=" + project + "\', \'SunTCPProject\'); return false\">");
               out.println("edit</a></td>");
               out.println("<td><a href=URL onClick=\"window.open(\'SunTCP?action=deleteProject&project=" + project + "\', \'SunTCPProject\'); return false\">");
               out.println("delete</a></td></tr>");
            }
            out.println("</table>");
            list.close();
         }
      } catch (FileNotFoundException e) {
         out.println("You have no project.");      
      }
      out.println("<table width=100%><tr><td align=right>");
      out.println("<a href=URL onClick=\"window.open(\'SunTCP?action=newProjectForm\', \'SunTCPProject\'); return false\">");
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
    
      htmlHeader(out, "Project Information", "");
      try {
         BufferedReader info =
            new BufferedReader(new FileReader(new File(path, SUNTCP_PROJECT)));
    
         out.println("<table>");
         out.println("<tr><td><a target=SunTCPFiles> Project name:</td><td>" + info.readLine() + "</a></td></tr>");
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
            out.println("<td><a href=\"SunTCP?action=downloadFile&project="
                    + project + "&file=" + files[0] + "\" target=SunTCPFile1>" + files[0] + "</a></td>");
            File temp = new File (path, files[0]);
            out.println("<td>&nbsp;</td><td>(" + temp.length() + " bytes) </td>");
            out.println("<td>&nbsp;</td><td><a href=URL onClick=\"window.open(\'SunTCP?action=viewFile&view=view&project=" + project + "&file=" + files[0] + "\', \'SunTCPFile1\'); return false\">");
            out.println("view</a></td>");   
            out.println("<td>&nbsp;</td><td><a href=URL onClick=\"window.open(\'SunTCP?action=viewFile&view=head&project=" + project + "&file=" + files[0] + "\', \'SunTCPFile1\'); return false\">");
            out.println("head</a></td>");   
            out.println("<td>&nbsp;</td><td><a href=URL onClick=\"window.open(\'SunTCP?action=viewFile&view=tail&project=" + project + "&file=" + files[0] + "\', \'SunTCPFile1\'); return false\">");
            out.println("tail</a></td>");   
            if (!files[0].equals(".suntcp-project"))  {
               out.println("<td>&nbsp;</td><td><a href=URL onClick=\"window.open(\'SunTCP?action=deleteFile&project=" + project + "&file=" + files[0] + "\', \'SunTCPFile1\'); return false\">");
               out.println("delete</a></td>");   
            }
         }
         out.println("</tr>");
         for (int i = 1; i < files.length; i++) {
            out.println("<td>&nbsp;</td><td><a href=\"SunTCP?action=downloadFile&project="
                    + project + "&file=" + files[i] + "\" target=SunTCPFile2>" + files[i] + "</a></td>");
            File temp = new File (path, files[i]);
            out.println("<td>&nbsp;</td><td>(" + temp.length() + " bytes) </td>");
            out.println("<td>&nbsp;</td><td><a href=URL onClick=\"window.open(\'SunTCP?action=viewFile&view=view&project=" + project + "&file=" + files[i] + "\', \'SunTCPFile2\'); return false\">");
            out.println("view</a></td>");   
            out.println("<td>&nbsp;</td><td><a href=URL onClick=\"window.open(\'SunTCP?action=viewFile&view=head&project=" + project + "&file=" + files[i] + "\', \'SunTCPFile2\'); return false\">");
            out.println("head</a></td>");   
            out.println("<td>&nbsp;</td><td><a href=URL onClick=\"window.open(\'SunTCP?action=viewFile&view=tail&project=" + project + "&file=" + files[i] + "\', \'SunTCPFile2\'); return false\">");
            out.println("tail</a></td>");   
            if (!files[i].equals(".suntcp-project"))  {
               out.println("<td>&nbsp;</td><td><a href=URL onClick=\"window.open(\'SunTCP?action=deleteFile&project=" + project + "&file=" + files[i] + "\', \'SunTCPFile2\'); return false\">");
               out.println("delete</a></td>"); 
            }
            out.println("</tr>");
         }
      
         out.println("</table>");
      } catch (FileNotFoundException e) {
         out.println("Error accessing this project.");
         //out.println("</pre>");
         //e.printStackTrace(out);      
         //out.println("</pre>");
      }
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
      try {
         BufferedReader list =
            new BufferedReader(new FileReader(APP_HOME_DIR + SUNTCP_LIST));
    
         out.println("<form method=post action=SunTCP enctype=multipart/form-data>");
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
      File directory = new File(path);
      sume =
         new PrintWriter(new BufferedWriter(new FileWriter("/tmp/" + SUNTCP_SU_SCRIPT + uid)));
      sume.println("mkdir -m 755 -p " + path);
      sume.println("/usr/bin/chmod 755 " + home);
      sume.close();
      Runtime.getRuntime().exec("/usr/bin/chmod 755 " + "/tmp/" + SUNTCP_SU_SCRIPT + uid).waitFor();
      Runtime.getRuntime().exec("su - " + submit_uid + " -c " + "/tmp/" + SUNTCP_SU_SCRIPT + uid).waitFor();
      
      htmlHeader(out, "New Project Status", "");
      out.println("Pathname: " + path);
    
      if (filename != null) {
         sume = new PrintWriter(new BufferedWriter(new FileWriter(new File(directory, SUNTCP_SU_SCRIPT))));
         sume.println("cd " + tmpdir);
         sume.println("/usr/bin/cp * " + path);
         sume.close();
         Runtime.getRuntime().exec("/usr/bin/chmod 755 " + path + "/" + SUNTCP_SU_SCRIPT).waitFor();
         Runtime.getRuntime().exec("su - " + submit_uid + " -c " + path + "/" + SUNTCP_SU_SCRIPT).waitFor();
      }
    
      if (project.length() > 0) {
         synchronized (Class.forName("com.sun.tcp.SunTCP")) {
            String listfile = home + SUNTCP_LIST;
            PrintWriter list =
               new PrintWriter(new BufferedWriter(new FileWriter(listfile, true)));
            list.println(name + "\t" + project);
            list.close();

            PrintWriter ressource =
               new PrintWriter(new BufferedWriter(new FileWriter(new File(path, SUNTCP_PROJECT))));
            ressource.println(project);
            ressource.println(app);
      
            try {
               String value;
               StringTokenizer st_equal, st_newline = new StringTokenizer(export, "\n\r\f");
               while (st_newline.hasMoreTokens()) {
                  st_equal =
                     new StringTokenizer(st_newline.nextToken(), "= \t");
                  ressource.print(st_equal.nextToken() + "=");
                  value = st_equal.nextToken();
                  if (value.equals("$HOME"))
                     ressource.println(path);
                  else
                     ressource.println(value);
               }
               Runtime.getRuntime().exec("/usr/bin/rm -rf " + tmpdir).waitFor();  
               out.println("<p>Project <i>" + project + "</i> was created successfully.");
            } catch (NoSuchElementException e) {
               out.println("<p>Error parsing export variables. You can add/correct variables by editing the newly created project.");
            }
            out.println("<center><form><input type=button value=Continue onClick=\"opener.location.reload(); window.close()\"></form></center>");
            ressource.close();
         } // synchronized
      } else {
         Runtime.getRuntime().exec("/usr/bin/rm -rf " + path);     
         out.println("<p>Go back and enter a non-empty project name.");
         out.println("<center><form><input type=button value=Back onClick=window.back()></form></center>");
      }
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
    
      htmlHeader(out, "Project Form", "");
      try {
         BufferedReader read =
            new BufferedReader(new FileReader(new File(directory, SUNTCP_PROJECT)));
    
         String project = read.readLine();
         String application = read.readLine();
         Vector export = new Vector();
         while ((line = read.readLine()) != null)
            export.addElement(line);
         read.close();

         BufferedReader list =
            new BufferedReader(new FileReader(APP_HOME_DIR + SUNTCP_LIST));
    
         out.println("<form method=post action=SunTCP enctype=multipart/form-data>");
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
      htmlFooter(out);
   }     

   private void updateProject(String uid, String submit_uid, String home, String id, String tmpdir, 
      String filename, String project, String app, String export, HttpServletRequest request, HttpServletResponse response)
      throws Exception {
      PrintWriter sume;
      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
    
      String directory = home + id;
      if (filename != null) {
         sume = new PrintWriter(new BufferedWriter(new FileWriter(new File(directory, SUNTCP_SU_SCRIPT))));
         sume.println("cd " + tmpdir);
         sume.println("/usr/bin/cp * " + directory);
         sume.close();
         Runtime.getRuntime().exec("/usr/bin/chmod 755 " + directory + "/" + SUNTCP_SU_SCRIPT).waitFor();
         Runtime.getRuntime().exec("su - " + submit_uid + " -c " + directory + "/" + SUNTCP_SU_SCRIPT).waitFor();
      }
    
      htmlHeader(out, "Project Status", "");
      if (project.length() > 0) {
         synchronized (Class.forName("com.sun.tcp.SunTCP")) {
            Vector list = new Vector();
            String line;
            String path = home;
      
            BufferedReader read =
               new BufferedReader(new FileReader(path + SUNTCP_LIST));
            while ((line = read.readLine()) != null) {
               if (!((new StringTokenizer(line, "\t")).nextToken().equals(id))) {
                  list.addElement(line);
               }   
            }      
            read.close();
      
            PrintWriter write =
               new PrintWriter(new BufferedWriter(new FileWriter(path + SUNTCP_LIST)));
            for (int i = 0; i < list.size(); i++)
               write.println((String)list.get(i));
            write.println(id + "\t" + project);
            write.close();

            PrintWriter ressource =
               new PrintWriter(new BufferedWriter(new FileWriter(new File(directory, SUNTCP_PROJECT))));
            ressource.println(project);
            ressource.println(app);
            try {
               String value;
               StringTokenizer st_equal, st_newline = new StringTokenizer(export, "\n\r\f");
               while (st_newline.hasMoreTokens()) {
                  st_equal =
                     new StringTokenizer(st_newline.nextToken(), "= \t");
                  ressource.print(st_equal.nextToken() + "=");
                  value = st_equal.nextToken();
                  if (value.equals("$HOME"))
                     ressource.println(directory);
                  else
                     ressource.println(value);
               }
               Runtime.getRuntime().exec("/usr/bin/rm -rf " + tmpdir).waitFor();  
               out.println("<p>Project <i>" + project + "</i> was updated successfully.");
            } catch (NoSuchElementException e) {
               out.println("<p>Error parsing export variables. You can add/correct variables by editing again the project.");
            }
            out.println("<center><form><input type=button value=Continue onClick=\"opener.location.reload(); window.close()\"></form></center>");
            ressource.close();
         } // synchronized
      } else {
         out.println("<p>Go back and enter a non-empty project name.");
         out.println("<center><form><input type=button value=Back onClick=window.back()></form></center>");
      }
      htmlFooter(out);
   }

   private void deleteProject(String uid, String home, HttpServletRequest request, HttpServletResponse response)
      throws Exception {
      String project = request.getParameter("project");
      String line;
      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
    
      htmlHeader(out, "Project Status", "");
      try {
         synchronized (Class.forName("com.sun.tcp.SunTCP")) {
            Vector list = new Vector();
            String directory = home;
            Runtime.getRuntime().exec("/usr/bin/rm -rf " + directory + project);
      
            FilePermission perm = new FilePermission(directory + SUNTCP_LIST, "read,write,execute");
            File listfile = new File(directory + SUNTCP_LIST);
            BufferedReader read =
               new BufferedReader(new FileReader(listfile));
            while ((line = read.readLine()) != null) {
               if (!((new StringTokenizer(line, "\t")).nextToken().equals(project))) {
                  list.addElement(line);
               }   
            }      
            read.close();
      
            if (list.size() > 0) {
               PrintWriter write = new PrintWriter(new BufferedWriter(new FileWriter(listfile)));
               for (int i = 0; i < list.size(); i++) 
                  write.println((String)list.get(i));
               write.close();
            } else {
               listfile.delete();
            }
            out.println("The project was successfully deleted.");
         } // synchronized
      } catch (Exception e) {
         out.println("Error accessing this project.");
         //out.println("</pre>");
         //e.printStackTrace(out);      
         //out.println("</pre>");
      }
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
         Process qstat =
         Runtime.getRuntime().exec(SGE_ROOT + "bin/" + SGE_ARCH + "/qstat -u " + submit_uid,
                                  SGE_EXPORT);
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
                  out.println("<tr><td><li><a href=\"SunTCP?action=jobInfo&id="
                        + id + "\" target=SunTCPJob>");
                  name = line.substring(13, 24).trim();
                  if (name.length() == 10) 
                     name = name.concat("...");
                  name = name.replace('_', ' ');
                  out.println(name + "</a></li></td>");
                  out.println("<td align=right><a href=URL onCLick=\"window.open(\'SunTCP?action=killJob&id="
                        + id + "\', \'SunTCPJob\'); return false\">kill</a></td></tr>");
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
      out.println("<a href=URL onCLick=\"window.open(\'SunTCP?action=newJobForm\', \'SunTCPJob\'); return false\">");
      out.println("Submit new job...</a>");
      out.println("</td></tr></table>");
      out.println("</body></html>");
   }     

   private void jobInfo(String uid, String home, HttpServletRequest request, HttpServletResponse response)
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
      BufferedReader projectinfo, appinfo;
      Vector all, yes;
      int i;

      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
    
      htmlHeader(out, "New Job Form", "");
      try {
         String listfile = home + SUNTCP_LIST;
         BufferedReader list = new BufferedReader(new FileReader(listfile));

         all = new Vector();
         yes = new Vector();
         while ((line = list.readLine()) != null) {
            st = new StringTokenizer(line, "\t");
            project = st.nextToken();
            all.addElement(project);
            all.addElement(st.nextToken());
            path = home + project;
            projectinfo = new BufferedReader(new FileReader(new File(path, SUNTCP_PROJECT)));
            projectinfo.readLine();
            application = projectinfo.readLine();
            appinfo = new BufferedReader(new FileReader(new File(APP_HOME_DIR + application, SUNTCP_APP)));
            appinfo.readLine(); appinfo.readLine(); appinfo.readLine(); appinfo.readLine();
            if (appinfo.readLine().equals("yes")) {
               yes.addElement(project);
               yes.addElement(application);
            }
            projectinfo.close();
            appinfo.close();
         }

         int length = yes.size();
         if (length > 0) {
            out.println("<script language=javascript>"); // externalize javascript code as iplanet rewrites location tags
            out.println("function loadCustom() {");
            out.println("  var myselect = document.forms[0].elements[1];");
            out.println("  var project = myselect.options[myselect.selectedIndex].value;");
            out.println("  var p = new Array(" + length + ");");
            for (i = 0; i < length; i++)
               out.println("  p[" + i + "] = \"" + (String)yes.get(i) + "\";");
            out.println("  for (var i = 0; i < p.length; i += 2) {");
            out.println("    if (project == p[i]) {");
            out.println("      window.open(\"SunTCP?action=newJobCustomForm&project=\" + p[i] + \"&app=\" + p[i + 1], p[i], \"menubar=no,toolbar=no,location=no,status=no\");");
            out.println("      return false;");
            out.println("    }");
            out.println("  }");
            out.println("}");
            out.println("</script>");
         }
         
         out.println("<form method=get action=SunTCP>");
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
         out.println("There are no projects available.");      
      }
      htmlFooter(out);
   }     

   private void newJobCustomForm(String uid, String home, HttpServletRequest request, HttpServletResponse response) 
      throws Exception {
      String line;
      String project = request.getParameter("project");
      String application = request.getParameter("app");
      BufferedReader form = new BufferedReader(new FileReader(new File(APP_HOME_DIR + application, SUNTCP_APP_FORM)));
  
      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
    
      out.println("<html><body><form>");
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

   private void startOrStopVNCServer(boolean start, Session sess, PrintWriter out, String xvncUser) 
      throws Exception {
      String line;
      // Handle error cases first
      if (start && isxDisplayNumValid()) {
         String jsLaunchXURL = "/NetletConfig?func=" + X_NETLET + ":" + xDisplayNum + "&machine=" + X_SERVER;

         out.println("vncserver already launched for this session.");
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
            Process vncServer =
               Runtime.getRuntime().exec("su - " + xvncUser + " -c " + VNCSERVER, SGE_EXPORT);
            BufferedReader vncStderr =
               new BufferedReader(new InputStreamReader(vncServer.getErrorStream()));
            BufferedReader vncStdout =
               new BufferedReader(new InputStreamReader(vncServer.getInputStream()));
            // Parse output for desktop number.  Also send to
            // web page.
            out.println("Output of vncserver launch:");
            out.println("<pre>");
            while ((line = vncStdout.readLine()) != null) {
               out.println(line);
            }
            while ((line = vncStderr.readLine()) != null) {
               out.println(line);
               if (line.indexOf("desktop") != -1)
                  xDisplayNum = String.valueOf(line.charAt(line.lastIndexOf(':')+1));
            }
            out.println("</pre>");
            vncStderr.close();
            vncStdout.close();
/*          } catch (Exception ioe) { */
/*             ioe.printStackTrace(out); */
/*          } */
         // Add desktop number as a session attribute
         // Should verify we have a netlet rule for this desktop first XXX
         sess.setProperty(SUNTCP_DESKTOP_ATTR, xDisplayNum);
         // Register session listener for shutting down vncserver on logout
         sess.addSessionListener(new SessionChange());

         // Launch vncviewer applet to connect to xDisplayNum
         String jsLaunchX="window.open(\'/NetletConfig?func=" + X_NETLET + ":" + xDisplayNum + "&machine=" + X_SERVER + "\', \'xAppWindow\', \'width=1024,height=800\')";

         // Output javascript to pop X desktop window
         out.println("<script language=javascript>");
         out.println(jsLaunchX);
         out.println("</script>");
      } else {
         // Kill vncserver started for this session
/*          try { */
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
            // Clear xDisplayNum session attribute.  "::" is the marker for
            // a closed vncserver session.
            if (sess != null)
               sess.setProperty(SUNTCP_DESKTOP_ATTR, "::");
/*          } catch (IOException ioe) { */
/*             ioe.printStackTrace(out); */
/*          } */
      }
   }

   private void launchVNCServer(String uid, String home, String domain_name, 
      HttpServletRequest request, HttpServletResponse response) 
      throws Exception {
      response.setContentType("text/html");
      PrintWriter out = response.getWriter();

      SessionID sid = new SessionID(request);
      Session session = Session.getSession(sid);

      // Make sure xDisplayNum is set before calling startOrStopVNCServer
      xDisplayNum = session.getProperty(SUNTCP_DESKTOP_ATTR);

      String kill = request.getParameter("kill");
      if (kill == null) {
         htmlHeader(out, "Launching vncserver", "");
         startOrStopVNCServer(true, session, out, uid);
      } else {
         htmlHeader(out, "Shutting down vncserver", "");
         startOrStopVNCServer(false, session, out, uid);
      }

      out.println("<center><form><input type=button value=Close onClick=\"window.close()\"></form></center>");
      htmlFooter(out);
   }

   private void submitNewJob(String uid, String submit_uid, String home, String domain_name, 
      HttpServletRequest request, HttpServletResponse response) 
      throws Exception {
      JobInfo ji = new JobInfo(request, home, uid);
      String line;

      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
      htmlHeader(out, "New Job Status", "");
    
      if (ji.xApp.equals("yes")) {
         try {
            SessionID sid = new SessionID(request);
            Session session = Session.getSession(sid);

            // Does user already have a tunneled X desktop (provided by this
            // servlet) available?  If so, use that.  Otherwise, allocate an
            // unused X desktop session to the user and invoke it.
            xDisplayNum = session.getProperty(SUNTCP_DESKTOP_ATTR);
            startOrStopVNCServer(true, session, out, uid);
         } catch (Exception e) {
            out.println("<pre>");
            e.printStackTrace(out);
            out.println("</pre>");
         }
      }
   
      PrintWriter script =
         new PrintWriter(new BufferedWriter(new FileWriter(new File(ji.projectDirectory, SUNTCP_QSUB_SCRIPT))));

      script.println("#!/bin/ksh");
      script.println("# qsub script automatically generated by suntcp");
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

      PrintWriter sume =
         new PrintWriter(new BufferedWriter(new FileWriter(new File(ji.projectDirectory, SUNTCP_SU_SCRIPT))));
      sume.println("#!/bin/ksh");
      sume.println("cd " + ji.projectDirectory);
      for (int j=0; j<SGE_EXPORT.length; j++) 
         sume.println("export " + SGE_EXPORT[j]);
      sume.println(SGE_ROOT + "bin/" + SGE_ARCH + "/qsub " + SUNTCP_QSUB_SCRIPT);
      sume.close();

      Runtime.getRuntime().exec("/usr/bin/chmod 755 " + ji.projectDirectory + "/" + SUNTCP_SU_SCRIPT).waitFor();
      Runtime.getRuntime().exec("/usr/bin/chmod 644 " + ji.projectDirectory + "/" + SUNTCP_QSUB_SCRIPT).waitFor();

      // This is where the job gets launched
      Process qsub = Runtime.getRuntime().exec("su - " + submit_uid + " -c " + ji.projectDirectory + "/" + SUNTCP_SU_SCRIPT);

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

      SessionID sid = new SessionID(request);
      Session session = Session.getSession(sid);

      response.setContentType("text/html");
      PrintWriter out = response.getWriter();

      String xDisplayNum = session.getProperty(SUNTCP_DESKTOP_ATTR);

      if (xDisplayNum == null)
         out.println("Session property \"" + SUNTCP_DESKTOP_ATTR + "\" is null");
      else
         out.println("Session property \"" + SUNTCP_DESKTOP_ATTR + "\" is \"" + xDisplayNum + "\"");
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
            new BufferedReader(new FileReader(APP_HOME_DIR + SUNTCP_LIST));
    
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
            out.println("<tr><td><li><a href=\"SunTCP?action=applicationInfo&application="
                    + app + "\" target=SunTCPApplication>");
            out.println(text + "</a></li></td></tr>");
         }
         out.println("</table></body></html>");
      } catch (FileNotFoundException e) {
         out.println("<html><body>There are no applications available.</body></html>");      
      }
   }     

   private void installationInfo(HttpServletRequest request, HttpServletResponse response)
      throws Exception {

      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
    
      htmlHeader(out, "Installation Information", "");
      out.println("<pre>");
      try {
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
            new BufferedReader(new FileReader(new File(APP_HOME_DIR + application, SUNTCP_APP)));
    
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
  
   private void htmlHeader(PrintWriter page, String title, String headInfo) {
      page.println("<html><head>" + headInfo + "</head><body bgcolor=FFFFFF>");
      page.println("<table border=0 cellpadding=2 cellspacing=0 width=100% bgcolor=888888>");    
      page.println("<tr><td bgcolor=FF9966><font color=FFFFFF><b>SunTCP "
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
      out.println("</pre>");
      out.println("<center><form><input type=button value=Continue onClick=\"opener.location.reload(); window.close()\"></form></center>");  
      htmlFooter(out);
   }
  
   private void downloadFile(String uid, String home, HttpServletRequest request, HttpServletResponse response)
      throws Exception {
      String project = request.getParameter("project");
      String name = request.getParameter("file");
      String directory = home + project;
    
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
    
      htmlHeader(out, "File Status", "");
      try {
         synchronized (Class.forName("com.sun.tcp.SunTCP")) {
            String directory = home + project;
      
            Runtime.getRuntime().exec("/usr/bin/rm " + directory + "/" + file).waitFor();
            out.println("The file was successfully deleted.");
         } // synchronized
      } catch (Exception e) {
         out.println("Error accessing this file.");
         //out.println("</pre>");
         //e.printStackTrace(out);      
         //out.println("</pre>");
      }
      out.println("<center><form><input type=button value=Continue onClick=\"opener.location.reload(); window.close()\"></form></center>");  
      htmlFooter(out);
   }
}
