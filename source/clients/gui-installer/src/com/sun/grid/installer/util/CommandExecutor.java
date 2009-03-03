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
package com.sun.grid.installer.util;

import java.io.IOException;
import java.util.List;
import java.io.File;
import java.io.InputStream;
import java.util.Map;
import java.util.Vector;

import com.izforge.izpack.util.Debug;
import com.sun.grid.installer.gui.Host;
import java.io.FileReader;
import java.io.LineNumberReader;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Properties;

/**
 * Executes the given commands and stores their exit value and output streams.
 */
public class CommandExecutor implements Config {
    private ProcessBuilder processBuilder = null;
    
    private int exitValue                         = -1;
    public static final int EXITVAL_OTHER         = -2;
    public static final int EXITVAL_INTERRUPTED   = -3;
    public static final int EXITVAL_TERMINATED    = -4;
    public static final int EXITVAL_MISSING_FILE  = 15;

    private static final String SHELL           = "sh";
    private static final String SHELL_ARG       = "-c";

    private static int WAIT_TIME     = 50;
    private static int DEFAULT_MAX_WAIT_TIME = 8000;
    private int MAX_WAIT_TIME;

    private Vector<String> outVector, errVector, additionalErrors;

    private File outFile, errFile;
    private List<String> cmds = null;

    public CommandExecutor(String... command) {
        this(null, DEFAULT_MAX_WAIT_TIME, Arrays.asList(command));
    }

    public CommandExecutor(Properties variables, String... command) {
        this(variables, DEFAULT_MAX_WAIT_TIME, Arrays.asList(command));
    }

    public CommandExecutor(Properties variables, int timeout, String... command) {
        this(variables, timeout, Arrays.asList(command));
    }
    
    public CommandExecutor(Properties variables, int timeout, List<String> commands) {
        MAX_WAIT_TIME = timeout;

        Debug.trace("Initializing command: " + getSingleCommand(commands));

        List<String> tmp = commands;
        commands = new ArrayList<String>();
        additionalErrors = new Vector<String>();
        String singleCmd = null;

        String shellName = tmp.get(0);

        if (variables == null) {
           //Quick commands like ls, chmod, ...
           commands.add(SHELL);
           commands.add(SHELL_ARG);
           //singleCmd = "'" + getSingleCommand(tmp) + "' ";
           List<String> tmpList = new ArrayList<String>();
           tmpList.addAll(tmp);
           setupOutputFiles(tmpList); //Redirect command outputs
           singleCmd = getSingleCommand(tmpList);
           commands.add(singleCmd);
           Debug.trace("New quick command: " + getSingleCommand(commands));
        } else if (variables != null) {
            String destHost = tmp.get(1);
            boolean onLocalHost = destHost.equalsIgnoreCase(Host.localHostName);
            //Skip the copy command when on a local host
            if (isSameCommand(variables.getProperty(VAR_COPY_COMMAND), shellName)) {
                destHost = tmp.get(tmp.size() - 1).split(":")[0];
                //Some commands have attachaed additional check command via && if ....
                if (destHost.startsWith(" && ")) {
                    destHost = tmp.get(tmp.size() - 2).split(":")[0];
                }
                if (destHost.equalsIgnoreCase(Host.localHostName)) {
                    Debug.trace("Copying skipped: " + getSingleCommand(tmp));
                    exitValue = 0;
                    return;
                }
            //Skip ssh/rsh if on local host
            }
            if (onLocalHost && isSameCommand(variables.getProperty(VAR_SHELL_NAME), shellName)) {
                commands.add(SHELL);
                commands.add(SHELL_ARG);
                List<String> tmpList = new ArrayList<String>();
                for (int i = 2; i < tmp.size(); i++) {
                    //Single if statement cannot be in single quotes!
                    //sh -c 'if [ bla bla ]; ....' would fail
                    if (tmp.size() == 3 && tmp.get(i).startsWith("'if") && tmp.get(i).endsWith("'")) {
                        tmpList.add(tmp.get(i).substring(1, tmp.get(i).length() - 1));
                    } else {
                        tmpList.add(tmp.get(i));
                    }
                }
                setupOutputFiles(tmpList); //Redirect command outputs
                singleCmd = getSingleCommand(tmpList);
                commands.add(singleCmd);
                Debug.trace("New command: " + getSingleCommand(commands));
            // Add ssh/scp options if not on local host
            } else {
                commands.add(SHELL);
                commands.add(SHELL_ARG);
                //Make a single line
                List<String> tmpList = new ArrayList<String>();
                for (int i = 0; i < tmp.size(); i++) {
                    if (i == 1) {                  
                        //If ssh or scp
                        if (isSameCommand(shellName, "ssh") || isSameCommand(shellName, "scp")) {
                            //We don't want to wait on the acception unknown RSA keys
                            //We require a kerberos5 or public key for connecting (without password)!
                            tmpList.add("-o StrictHostKeyChecking=yes -o PreferredAuthentications=gssapi-keyex,publickey");
                        }
                    }
                    tmpList.add(tmp.get(i));
                }
                setupOutputFiles(tmpList); //Redirect command outputs
                singleCmd = getSingleCommand(tmpList);
                commands.add(singleCmd);                
                Debug.trace("New command: " + getSingleCommand(commands));
            }
        }
        processBuilder = new ProcessBuilder(commands);
    }

    private static boolean isSameCommand(String first, String second) {
        return first.equals(second) || first.endsWith("/"+second);
    }

    private static String getSingleCommand(List<String> cmds) {
        return getSingleCommand(0, cmds);
    }

    private static String getSingleCommand(int startElement, List<String> cmds) {
        String singleCmd="";
        for (int i=startElement; i < cmds.size(); i++) {
           singleCmd += cmds.get(i) + " ";
        }
        if (singleCmd.length() > 1) {
            return singleCmd.substring(0, singleCmd.length() - 1);
        }
        return null;
    }

    private void setupOutputFiles(List<String> commands) {
        try {
            outFile = File.createTempFile("gui-cmdexec", ".out", new File("/tmp"));
            errFile = File.createTempFile("gui-cmdexec", ".err", new File("/tmp"));
            commands.add("> "+outFile.getAbsolutePath()+" 2> "+ errFile.getAbsolutePath());
        } catch (IOException ex) {
            additionalErrors.add(this.getClass().getName() + ".init: " + ex.getMessage());
        }
    }


    public void execute() {
        //Exit if we skipped the commands (on local host)
        if (exitValue != -1) {
            return;
        }
 
        Process process = null;
        long cmdId = 0;

        try {
            process = processBuilder.start();
            
            long waitTime = 0;
            while (true) {
                try {
                    exitValue = process.exitValue();
                    break;
                } catch (IllegalThreadStateException ex) {
                    Thread.sleep(WAIT_TIME);
                    waitTime += WAIT_TIME;
                    if (waitTime > MAX_WAIT_TIME) {
                        Debug.error("Terminated ("+waitTime/1000.0+" sec): '" + processBuilder.command() + "'!");
                        process.destroy();
                        exitValue = EXITVAL_TERMINATED;
                        break;
                    }
                }
            }
            cmdId = (long)(Math.random()*1000000);
            Debug.trace(cmdId + " Command: " + getSingleCommand(processBuilder.command()) + " exitValue: "+exitValue);
        } catch (IOException ex) {
            exitValue = EXITVAL_OTHER;
            additionalErrors.add(ex.getLocalizedMessage());
        } catch (InterruptedException ex) {
            exitValue = EXITVAL_INTERRUPTED;
            additionalErrors.add(ex.getLocalizedMessage());
        } finally {
            outVector = getInput(outFile);
            errVector = getInput(errFile);
            errVector.addAll(additionalErrors);
            Debug.trace(cmdId + " output: "+ outVector);
            Debug.trace(cmdId + " error: "+ errVector);
            //Delete on exit to have better debug, until we close the APP
            outFile.deleteOnExit();
            errFile.deleteOnExit();
        }
    }

    public void setWorkingDirectory(File directory) {
        processBuilder.directory(directory);
    }

    public List getCommands() {
        return processBuilder.command();
    }

    public Map getEnvironment() {
        return processBuilder.environment();
    }

    public String getLocalizedMessage(int value) {
        return "";
    }
    
    public Vector<String> getOutput() {
        return outVector;
    }
    
    public Vector<String> getError() {
        return  errVector;
    }

    /**
     * @return the exitValue
     */
    public int getExitValue() {
        return exitValue;
    }
    
          
    private Vector<String> getInput(File f) {
        LineNumberReader lnr = null;
        Vector<String> v = new Vector<String>();
        try {
            lnr = new LineNumberReader(new FileReader(f));
            String line;
            while ((line = lnr.readLine()) != null) {
                v.add(line);
            }
            return v;
        } catch (IOException ex) {
            additionalErrors.add(ex.getLocalizedMessage());
            return v;
        } finally {
            try {
                lnr.close();
            } catch (IOException ex) {
            }
        }
    }    
}
