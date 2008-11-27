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

import java.io.BufferedReader;
import java.io.IOException;
import java.util.List;
import java.io.File;
import java.io.InputStreamReader;
import java.io.InputStream;
import java.util.Map;
import java.util.Vector;

import com.izforge.izpack.util.Debug;

/**
 * Executes the given commands and stores their exit value and output streams.
 */
public class CommandExecutor {
    private ProcessBuilder processBuilder = null;

    private StreamHandler outputStream = null;
    private StreamHandler ttyStream = null;
    private StreamHandler errorStream  = null;
    
    private int exitValue = -1;

    public static final int EXITVAL_OTHER       = -2;
    public static final int EXITVAL_INTERRUPTED = -3;
    public static final int EXITVAL_TERMINATED  = -4;

    private static int WAIT_TIME     = 50;
    private int MAX_WAIT_TIME = 8000;

    public CommandExecutor(String... command) {
        processBuilder = new ProcessBuilder(command);
    }

    public CommandExecutor(int timeout, String... command) {
        MAX_WAIT_TIME=timeout;
        processBuilder = new ProcessBuilder(command);        
    }

    public CommandExecutor(List<String> commands) {
        processBuilder = new ProcessBuilder(commands);
    }
    
    public CommandExecutor(int timeout, List<String> commands) {
        MAX_WAIT_TIME=timeout;
        processBuilder = new ProcessBuilder(commands);
    }

    public void execute() {
        Process process = null;

        try {
           process = processBuilder.start();
            

            outputStream = new StreamHandler("out", process.getInputStream());
            errorStream  = new StreamHandler("err",process.getErrorStream());
            //ttyStream = new StreamHandler("tty", new FileInputStream("/dev/tty"));

            outputStream.start();
            errorStream.start();
            //ttyStream.start();

            long waitTime = 0;
            //Must wait until both threads for out/err streams finished
            while (true) {
                try {
                    exitValue = process.exitValue();
                    break;
                } catch (IllegalThreadStateException ex) {
                    Thread.sleep(WAIT_TIME);
                    waitTime += WAIT_TIME;
                    if (waitTime > MAX_WAIT_TIME) {
                        Debug.error("Terminate command: '" + processBuilder.command() + "'!");
                        process.destroy();
                        exitValue = EXITVAL_TERMINATED;
                        break;
                    }
                }
            }
            outputStream.interrupt();
            errorStream.interrupt();

            Debug.trace("Command: " + processBuilder.command().toString() + " exitValue: "+exitValue);
        } catch (IOException ex) {
            exitValue = EXITVAL_OTHER;
            //Debug.error(ex);
        } catch (InterruptedException ex) {
            exitValue = EXITVAL_INTERRUPTED;
            Debug.error(ex);
        }
    }

    public void setWorkingDirectory(File directory) {
        processBuilder.directory(directory);
    }

    public List getComamnds() {
        return processBuilder.command();
    }

    public Map getEnvironment() {
        return processBuilder.environment();
    }

    public String getLocalizedMessage(int value) {
        return "";
    }
    
    public Vector<String> getOutput() {
        if (outputStream != null) {
            return outputStream.getInput();
        } else {
            return null;
        }
    }
    
    public Vector<String> getError() {
        return errorStream.getInput();
    }

    /**
     * @return the exitValue
     */
    public int getExitValue() {
        return exitValue;
    }
    
    private class StreamHandler extends Thread {
        private String name             = null;
        private InputStream inputStream = null;
        private Vector<String> input    = null;
        private BufferedReader bufferedReader = null;

        public StreamHandler(InputStream inputStream) {
            this.inputStream = inputStream;
       
            input = new Vector<String>();
        }

        public StreamHandler(String name, InputStream inputStream) {
            this(inputStream);
            this.name = name;
        }
        
        @Override
        public void run() {
            InputStreamReader inputStreamReader = new InputStreamReader(inputStream);
            bufferedReader = new BufferedReader(inputStreamReader);

            try {
                String line = null;
                while (!bufferedReader.ready()) {
                    if (isInterrupted()) {
                        return;
                    }
                    yield();
                }
                while (isInterrupted() != true && (line = bufferedReader.readLine()) != null) {
                    input.add(line);
                    Debug.trace(name + " line:" + line);
                }
            /*} catch (InterruptedException ex) {
                Debug.trace("Stream "+name+" interrupted");*/
            } catch (IOException ex) {
                //Debug.error(ex);
            } finally {
                try {
                    if (bufferedReader != null) {
                       bufferedReader.close();
                    }
                } catch (IOException ex) {
                    //Debug.error(ex);
                }
            }
        }        

        public Vector<String> getInput() {
            if (input != null) {
                return (Vector<String>)input.clone();
            } else {
                return null;
            }
        }

        @Override
        public void interrupt() {
            if (bufferedReader != null) {
                try {
                    bufferedReader.close();
                } catch (IOException ex) {
                    //Debug.trace(ex);
                }
            }
            super.interrupt();
        }

    }
}
