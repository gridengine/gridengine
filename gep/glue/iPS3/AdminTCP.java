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
import java.security.*;
import javax.servlet.*;
import javax.servlet.http.*;

import com.iplanet.portalserver.session.*;
import com.iplanet.portalserver.profile.*;

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
 * Sun Technical Computing Portal admin servlet
 *
 * @author Frederic Pariente, Sun Microsystems
 * @author Dan Fraser, Sun Microsystems (modifications)
 * @author Eric Sharakan, Sun Microsystems (X11; Admin; SGE cells)
 * @version 0.96, 2001/07/27, added support for X11 Apps, non-root Admin,
 *			      SGE cells
 * @version 0.95, 2001/07/24, added dynamic user home dir capability
 * @version 0.94, 2001/05/15, added usage of $HOME for export variables
 * @version 0.93, 2001/05/15, added support for access list per application
 * @version 0.9, 2001/04/25
*/

public class AdminTCP extends HttpServlet {
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
    "LD_LIBRARY_PATH=/export/gridengine"
  };
  private String SGE_ARCH;
  private String SUNTCP_SU_SCRIPT = ".suntcp-su";
  private String SUNTCP_LIST = ".suntcp-list";
  private String SUNTCP_APP = ".suntcp-app";
  private String SUNTCP_APP_FORM = ".suntcp-form";

  public void init(ServletConfig config) throws ServletException {
    String s;
    super.init(config);
    
    s = getInitParameter("app_home");
    if (s != null) APP_HOME_DIR = s;
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
    
    SGE_ACCT = SGE_CELL + SGE_ACCT;
    SGE_EXPORT[3] = "LD_LIBRARY_PATH="+ SGE_ROOT + "lib/" + SGE_ARCH;
  }

  public void doGet(HttpServletRequest request, HttpServletResponse response)
      throws ServletException, IOException {
    try {
      SessionID sid = new SessionID(request);
      Session sess = Session.getSession(sid);
      String uid = sess.getClientID();
      Profile prof = sess.getUserProfile();
      if (!prof.isAllowed("iwtAdmin-execute")) throw new AccessControlException("User " + uid + " does not have Admin privileges");
      String action = request.getParameter("action");
      
      if (action.equals("applicationList"))
        applicationList(response);
      else if (action.equals("applicationInfo"))
        applicationInfo(request, response);
      else if (action.equals("newApplicationForm"))
        newApplicationForm(response);
      else if (action.equals("editApplicationForm"))
        editApplicationForm(request, response);
      else if (action.equals("updateApplication"))
        updateApplication(request, response);
      else if (action.equals("deleteApplication"))
        deleteApplication(request, response);
      else if (action.equals("jobList"))
        jobList(response);
      else if (action.equals("jobInfo"))
        jobInfo(request, response);
      else if (action.equals("killJob"))
        killJob(request, response);
      else if (action.equals("jobAccounting"))
        jobAccounting(response);
      else if (action.equals("viewFile"))
        viewFile(request, response);
      else
        unknownAction(response);
    } catch (Exception e) {
      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
      out.println("<html><body>");
      out.println(e.toString());
      out.println("</body></html>");
    }
  }
  
  public void doPost(HttpServletRequest request, HttpServletResponse response)
      throws ServletException, IOException {
      response.setContentType("text/html");
      PrintWriter out = response.getWriter();
    try {
      SessionID sid = new SessionID(request);
      Session sess = Session.getSession(sid);
      String uid = sess.getClientID();
      Profile prof = sess.getUserProfile();
      if (!prof.isAllowed("iwtAdmin-execute")) throw new AccessControlException("User " + uid + " does not have Admin privileges");
      createNewApplication(request, response, out);
    } catch (Exception e) {
      out.println("<html><body><pre>");
      out.println(e.toString());
      out.println("</pre></body></html>");
    }
  }

  private void applicationList(HttpServletResponse response) throws Exception {
    StringTokenizer st;
    String line, app;

    response.setContentType("text/html");
    PrintWriter out = response.getWriter();
    
    out.println("<html><body>");    
    try {
      BufferedReader list =
        new BufferedReader(new FileReader(APP_HOME_DIR + SUNTCP_LIST));
    
      out.println("<table width=100%>");
      while ((line = list.readLine()) != null) {
        st = new StringTokenizer(line, "\t");
        app = st.nextToken();
        out.println("<tr><td><li><a href=\"AdminTCP?action=applicationInfo&application="
                    + app + "\" target=SunTCPApplication>");
        out.println(st.nextToken() + "</a></li></td>");
        out.println("<td><a href=URL onCLick=\"window.open(\'AdminTCP?action=editApplicationForm&application="
                    + app + "\', \'SunTCPApplication\'); return false\">edit</a></td>");
        out.println("<td><a href=URL onCLick=\"window.open(\'AdminTCP?action=deleteApplication&application="
                    + app + "\', \'SunTCPApplication\'); return false\">delete</a></td></tr>");
      }
      out.println("</table>");
    } catch (FileNotFoundException e) {
      out.println("There are no applications available.");      
    }
    out.println("<table width=100%><tr><td align=right>");
    out.println("<a href=URL onClick=\"window.open(\'AdminTCP?action=newApplicationForm\',\'TCPApplication\'); return false\">");
    out.println("Add a new application...</a>");
    out.println("</td></tr></table>");
    out.println("</body></html>");
  }     

  private void applicationInfo(HttpServletRequest request, HttpServletResponse response)
      throws Exception {
    String app = request.getParameter("application");
    String line;
    String path = APP_HOME_DIR + app;

    response.setContentType("text/html");
    PrintWriter out = response.getWriter();
    
    htmlHeader(out, "Application Information");
    try {
      BufferedReader info =
        new BufferedReader(new FileReader(new File(path, SUNTCP_APP)));
    
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
        out.println("<td>&nbsp;</td><td><a href=\"AdminTCP?action=viewFile&view=head&application="
                    + app + "&file=" + files[0] + "\">head</a></td>");
        out.println("<td>&nbsp;</td><td><a href=\"AdminTCP?action=viewFile&view=tail&application="
                    + app + "&file=" + files[0] + "\">tail</a></td>");
      }
      out.println("</tr>");
      for (int i = 1; i < files.length; i++) {
          out.println("<tr><td>&nbsp;</td><td>" + files[i] + "</td>");
          out.println("<td>&nbsp;</td><td><a href=\"AdminTCP?action=viewFile&view=head&application="
                      + app + "&file=" + files[i] + "\">head</a></td>");
          out.println("<td>&nbsp;</td><td><a href=\"AdminTCP?action=viewFile&view=tail&application="
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

  private void newApplicationForm(HttpServletResponse response) throws Exception {
    StringTokenizer st;
    String line;

    response.setContentType("text/html");
    PrintWriter out = response.getWriter();
    
    htmlHeader(out, "New Application Form");
    
      out.println("<form method=post action=AdminTCP enctype=multipart/form-data>");
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

  private void createNewApplication(HttpServletRequest request, HttpServletResponse response, PrintWriter out)
      throws Exception {
    String name = "A" + System.currentTimeMillis();
    String path = APP_HOME_DIR + name;
    File directory = new File(path);
    directory.mkdirs();
    Runtime.getRuntime().exec("/usr/bin/chmod 755 " + path).waitFor();
      
    MultipartRequest multi = 
      new MultipartRequest(request, path, 100*1024*1024); // 100MB
    String name1 = multi.getParameter("name1").trim();
    //String name2 = multi.getParameter("name2").trim();
    String binary = multi.getParameter("binary").trim();
    String parallel = multi.getParameter("parallel");
    String xApp = multi.getParameter("xApp");
    String form = multi.getParameter("form");
    String namef = multi.getParameter("namef").trim();
    String export = multi.getParameter("export");
    String users = multi.getParameter("users").trim();
    
    htmlHeader(out, "New Application Status");
    
    Enumeration files = multi.getFileNames();
    if (files.hasMoreElements())
      if (multi.getParameter("tar") != null) {
        String filename = multi.getFilesystemName((String)files.nextElement());
        PrintWriter sume =
          new PrintWriter(new BufferedWriter(new FileWriter(new File(directory, SUNTCP_SU_SCRIPT))));
        sume.println("cd " + path);
        sume.println("gunzip -c " + filename + " | tar xf -");
        sume.close();
        Runtime.getRuntime().exec("/usr/bin/chmod 755 " + path + "/" + SUNTCP_SU_SCRIPT).waitFor();
        Runtime.getRuntime().exec("/usr/bin/chmod 644 " + path + "/" + filename).waitFor();
      
        Runtime.getRuntime().exec("/usr/bin/ksh exec " + path + "/" + SUNTCP_SU_SCRIPT).waitFor();
      
        if (form.equals("yes"))
          Runtime.getRuntime().exec("cp " + path + "/" + namef + " " + path + "/" + SUNTCP_APP_FORM);
      } else
        Runtime.getRuntime().exec("/usr/bin/chmod 755 " + path + "/" + binary).waitFor();
      
    if (name1.length() > 0) {
      String listfile = APP_HOME_DIR + SUNTCP_LIST;
      PrintWriter list =
        new PrintWriter(new BufferedWriter(new FileWriter(listfile, true)));
      list.print(name + "\t" + name1);
      if (multi.getParameter("acl") != null)
        list.print("\t" + users);
      list.println();
      list.close();

      PrintWriter ressource =
        new PrintWriter(new BufferedWriter(new FileWriter(new File(path, SUNTCP_APP))));
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
          st_equal =
            new StringTokenizer(st_newline.nextToken(), "= \t");
          ressource.print(st_equal.nextToken() + "=");
          value = st_equal.nextToken();
          if (value.equals("$HOME"))
            ressource.println(path);
          else
            ressource.println(value);
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

  private void editApplicationForm(HttpServletRequest request, HttpServletResponse response) throws Exception {
    StringTokenizer st;
    String line;
    String id = request.getParameter("application");
    String directory = APP_HOME_DIR + id;

    response.setContentType("text/html");
    PrintWriter out = response.getWriter();
    
    htmlHeader(out, "Application Form");
    try {
      BufferedReader read =
        new BufferedReader(new FileReader(new File(directory, SUNTCP_APP)));
    
      String app = read.readLine();
      String binary = read.readLine();
      String parallel = read.readLine();
      String xApp = read.readLine();
      String form = read.readLine();
      String users = read.readLine();
      Vector export = new Vector();
      while ((line = read.readLine()) != null) export.addElement(line);
      read.close();

      out.println("<form method=get action=AdminTCP>");
      out.println("<input type=hidden name=action value=updateApplication>");
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
      for (int i = 0; i < export.size(); i++)
        out.println((String)export.get(i));
      out.println("</textarea></td></tr>");
      out.println("</table>");
      out.println("<center><input type=submit value=Submit></center>");
      out.println("</form>");
    } catch (FileNotFoundException e) {
      out.println("Error accessing selected application.");      
    }
    htmlFooter(out);
  }     

  private void updateApplication(HttpServletRequest request, HttpServletResponse response)
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
    
    htmlHeader(out, "Project Status");
    if (name1.length() > 0) {
      Vector list = new Vector();
      String line;
      
      BufferedReader read =
        new BufferedReader(new FileReader(APP_HOME_DIR + SUNTCP_LIST));
      while ((line = read.readLine()) != null)
        if (!((new StringTokenizer(line, "\t")).nextToken().equals(id)))
          list.addElement(line);
      read.close();
      
      PrintWriter write =
        new PrintWriter(new BufferedWriter(new FileWriter(APP_HOME_DIR + SUNTCP_LIST)));
      for (int i = 0; i < list.size(); i++)
        write.println((String)list.get(i));
      write.print(id + "\t" + name1);
      if (request.getParameter("acl") != null)
        write.print("\t" + users);
      write.println();
      write.close();

      PrintWriter ressource =
        new PrintWriter(new BufferedWriter(new FileWriter(new File(APP_HOME_DIR + id, SUNTCP_APP))));
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
          st_equal =
            new StringTokenizer(st_newline.nextToken(), "= \t");
          ressource.print(st_equal.nextToken() + "=");
          value = st_equal.nextToken();
          if (value.equals("$HOME"))
            ressource.println(APP_HOME_DIR + id);
          else
            ressource.println(value);
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

  private void deleteApplication(HttpServletRequest request, HttpServletResponse response)
      throws Exception {
    String app = request.getParameter("application");
    String line;
    response.setContentType("text/html");
    PrintWriter out = response.getWriter();
    
    htmlHeader(out, "Application Status");
    try {
      Vector list = new Vector();
      
      Runtime.getRuntime().exec("/usr/bin/rm -rf " + APP_HOME_DIR + app); 
      File listfile = new File(APP_HOME_DIR + SUNTCP_LIST);
      BufferedReader read =
        new BufferedReader(new FileReader(listfile));
      while ((line = read.readLine()) != null)
        if (!((new StringTokenizer(line, "\t")).nextToken().equals(app)))
          list.addElement(line);
      read.close();
      
      if (list.size() > 0) {
        PrintWriter write = new PrintWriter(new BufferedWriter(new FileWriter(listfile)));
        for (int i = 0; i < list.size(); i++) write.println((String)list.get(i));
        write.close();
      } else
        listfile.delete();
      
      out.println("The application was successfully deleted.");
    } catch (Exception e) {
      out.println("<pre>");
      e.printStackTrace(out);      
      out.println("</pre>");
    }
    out.println("<center><form><input type=button value=Continue onClick=\"opener.location.reload(); window.close()\"></form></center>");
    htmlFooter(out);
  }

  private void jobList(HttpServletResponse response) throws Exception {
    String line, id, name;
    response.setContentType("text/html");
    PrintWriter out = response.getWriter();
    
    out.println("<html><body>");
    try {
      Process qstat =
        Runtime.getRuntime().exec(SGE_ROOT + "bin/" + SGE_ARCH + "/qstat", SGE_EXPORT);
      BufferedReader jobs =
        new BufferedReader(new InputStreamReader(qstat.getInputStream()));
      BufferedReader error =
        new BufferedReader(new InputStreamReader(qstat.getErrorStream()));
      
      if ((jobs.readLine()) != null) {
        jobs.readLine(); // 2 header lines
        line = jobs.readLine();
        out.println("<table width=100%>");
        do {
          id = line.substring(0, 6).trim();
          if (id.length() > 0) {
            out.println("<tr><td><li><a href=\"AdminTCP?action=jobInfo&id="
                        + id + "\" target=SunTCPJob>");
            name = line.substring(13, 24).trim();
            if (name.length() == 10) name = name.concat("...");
            name = name.replace('_', ' ');
            out.println(name + "</a></li></td>");
            out.println("<td align=right><a href=URL onCLick=\"window.open(\'AdminTCP?action=killJob&id="
                        + id + "\', \'SunTCPJob\'); return false\">kill</a></td></tr>");
          }
        } while ((line = jobs.readLine()) != null);
        out.println("</table>");
      } else {
        out.println("There are no running jobs.");
        out.println("<pre>");
        while ((line = error.readLine()) != null) out.println(line);
        out.println("</pre>");
      }
    } catch (Exception e) {
      e.printStackTrace(out);      
    }
    out.println("<table width=100%><tr><td align=right>");
    out.println("<a href=URL onCLick=\"window.open(\'AdminTCP?action=jobAccounting\', \'SunTCPJob\'); return false\">");
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
    
    htmlHeader(out, "Job Information");
    out.println("<pre>");
    try {
      Process qstat =
        Runtime.getRuntime().exec(SGE_ROOT + "bin/" + SGE_ARCH + "/qstat -j " + id, SGE_EXPORT);
      BufferedReader jobs =
        new BufferedReader(new InputStreamReader(qstat.getInputStream()));
      
      while ((line = jobs.readLine()) != null) out.println(line);
    } catch (Exception e) {
      e.printStackTrace(out);      
    }
    out.println("</pre>");
    out.println("<center><form><input type=button value=Continue onClick=window.close()></form></center>");
    htmlFooter(out);
  }     

  private void killJob(HttpServletRequest request, HttpServletResponse response)
      throws Exception {
    String id = request.getParameter("id");
    String line;
    response.setContentType("text/html");
    PrintWriter out = response.getWriter();
    
    htmlHeader(out, "Job Status");
    try {
      Process qstat =
        Runtime.getRuntime().exec(SGE_ROOT + "bin/" + SGE_ARCH + "/qdel " + id, SGE_EXPORT);
      BufferedReader jobs =
        new BufferedReader(new InputStreamReader(qstat.getInputStream()));
      
      while ((line = jobs.readLine()) != null) out.println(line);
    } catch (Exception e) {
      out.println("<pre>");
      e.printStackTrace(out);      
      out.println("</pre>");
    }
    out.println("<center><form><input type=button value=Continue onClick=\"opener.location.reload(); window.close()\"></form></center>");
    htmlFooter(out);
  }

  private void jobAccounting(HttpServletResponse response)
      throws Exception {
    String line, token;
    StringTokenizer st;
    int nb, uid, aid;
    response.setContentType("text/html");
    PrintWriter out = response.getWriter();
    
    htmlHeader(out, "Job Accounting");
    try {
      try {
        BufferedReader acct = new BufferedReader(new FileReader(SGE_ROOT + SGE_ACCT));
        // SGE_ACCT has owner, account (= application), wall time in columns 4, 7, 14
        out.println("<table><tr><th>&nbsp;</th>");
        Vector account = new Vector();
        try {
          BufferedReader app = new BufferedReader(new FileReader(APP_HOME_DIR + SUNTCP_LIST));
          while ((line = app.readLine()) != null) {
            st = new StringTokenizer(line, "\t");
            account.addElement(st.nextToken());
            out.println("<th>" + st.nextToken() + "</th>");
          }
        } catch (FileNotFoundException e) {}
        account.addElement("codine");
        out.println("<th>Others</th></tr>");
        nb = account.size();
      
        Vector user = new Vector();
        Vector time = new Vector();
        while ((line = acct.readLine()) != null) {
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
          if (aid == -1) aid = nb - 1;
          st.nextToken(); st.nextToken(); st.nextToken(); st.nextToken(); st.nextToken(); st.nextToken();
          ((int[])time.get(uid))[aid] += Integer.parseInt(st.nextToken());
        }
      
        int[] tab;
        for (uid = 0; uid < user.size(); uid++) {
          out.println("<tr><td>" + (String)user.get(uid) + "</td>");
          tab = (int[])time.get(uid);
          for (aid = 0; aid < nb; aid++)
            out.println("<td align=center>" + tab[aid] + "</td>");
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
  
  private void htmlHeader(PrintWriter page, String title) {
    page.println("<html><body bgcolor=FFFFFF>");
    page.println("<table border=0 cellpadding=2 cellspacing=0 width=100% bgcolor=888888>");    
    page.println("<tr><td bgcolor=FF9966><font color=FFFFFF><b>SunTCP "
                 + title + "</b></font></td></tr><tr><td bgcolor=DDDDDD>");    
  }
  
  private void htmlFooter(PrintWriter page) {
    page.println("</td></tr><tr><td bgcolor=000000 align=right>");
    page.println("<font color=FFFFFF>Copyright 2001 Sun Microsystems, Inc. All rights reserved.</font></td></tr>");    
    page.println("</table></body></html>");    
  }

  private void viewFile(HttpServletRequest request, HttpServletResponse response)
      throws Exception {
    String app = request.getParameter("application");
    String view = request.getParameter("view");
    String file = request.getParameter("file");
    response.setContentType("text/html");
    PrintWriter out = response.getWriter();
    final int NBLINES = 40;
    String line;
    
    htmlHeader(out, "File View");
    out.println("<pre>");
    try {
      String directory = APP_HOME_DIR+ app;
      
      if (view.equals("head")) {
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
    htmlFooter(out);
  }
  
  private void notYetImplemented(HttpServletResponse response) throws Exception {
    response.setContentType("text/html");
    PrintWriter out = response.getWriter();
    htmlHeader(out, "Status");
    out.println("Not yet Implemented.");
    out.println("<center><form><input type=button value=Continue onClick=window.close()></form></center>");
    htmlFooter(out);
  }
  
  private void unknownAction(HttpServletResponse response) throws Exception {
    response.setContentType("text/html");
    PrintWriter out = response.getWriter();
    out.println("<html><body>");
    out.println("Unkown action.");
    out.println("</body></html>");
  }
}
