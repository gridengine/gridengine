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

import com.izforge.izpack.installer.InstallerFrame;
import com.izforge.izpack.util.Debug;
import com.izforge.izpack.util.VariableSubstitutor;
import com.sun.grid.installer.gui.*;
import java.awt.Component;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.StringReader;
import java.io.StringWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import java.util.Scanner;
import javax.swing.JFileChooser;
import javax.swing.SwingUtilities;

/*RFC 952 DoD Internet host table specification defines a hostname as
 <hostname> ::= <name>*["."<name>]
 <name>  ::= <letter>[*[<letter-or-digit-or-hyphen>]<letter-or-digit>]
*/
public class Util implements Config{
    enum Operation {
        ADD, REMOVE, DELETE
    }

    public enum SgeComponents { bdb, qmaster, shadowd, execd}

    final public static int RESOLVE_THREAD_POOL_SIZE = 8;
    final public static int INSTALL_THREAD_POOL_SIZE = 2;

    // Currently we accept pattern in list of hosts in a file
    public static List<String> parseFileList(File f) throws FileNotFoundException {
        List<String> hostList = new LinkedList<String>(), tempList;
        Host.Type type;
        Scanner s = new Scanner(f);
        s.useDelimiter("\\s+|;|,");
        String pattern = "";
        while (s.hasNext()) {
            pattern = s.next().toLowerCase().trim();
            type = getType(pattern);
            tempList = parsePattern(pattern, type);
            hostList = joinList(hostList, tempList);
        }
        s.close();
        return hostList;
    }

    private static Host.Type getType(String pattern) {
        char c;
        for (int i=0; i<pattern.length(); i++) {
            c = pattern.charAt(i);
            if (c >= '0' && c <= '9') {
                return Host.Type.IP;
            }
            if (c >= 'a' && c <= 'z') {
                return Host.Type.HOSTNAME;
            }
        }
        throw new IllegalArgumentException("Can't get hots type for pattern "+pattern);
    }

    public static List<String> parseHostPattern(String input) {
        //TODO: Might need some additional checks for all the hostnames
        return parsePattern(input.toLowerCase().trim(), Host.Type.HOSTNAME);
    }


    public static List<String> parseIpPattern(String input) {
        return parsePattern(input.toLowerCase().trim(), Host.Type.IP);
    }


    private static List<String> parsePattern(String input, Host.Type type) {
        long start=System.currentTimeMillis(),endL;
        LinkedList<List<String>> list = new LinkedList<List<String>>();
        int i=0;
        String elem;

        Scanner s = new Scanner(input);
        s.useDelimiter("\\.");
        while (s.hasNext()) {
            elem = s.next();
            i++;
            switch (type) {
                case HOSTNAME:
                    list.add(parseSinglePattern(elem, type));
                    break;
                case IP:
                    if (elem.charAt(0) >= '0' && elem.charAt(0) <= '9' || elem.charAt(0) == '[') {
                        List<String> singlePattern = parseSinglePattern(elem, type);
                        //Valid IP octed must be in 0 to 255
                        LinkedList<String> finalList = new LinkedList<String>();
                        int j;
                        String temp;
                        for (String val : singlePattern) {
                            j = Integer.valueOf(val).intValue();
                            if (j < 0 || j > 255) {
                                j = Math.max(0, j);
                                j = Math.min(255, j);
                                temp = String.valueOf(j);
                                if (!finalList.contains(temp)) {
                                    finalList.add(temp);
                                }
                            } else if (!finalList.contains(val)) {
                                finalList.add(val);
                            }
                        }
                        list.add(finalList);
                        break;
                    }                  
                default:
                    s.close();
                    throw new IllegalArgumentException("Invalid value " + elem + " for type " + type.toString());
            }
        }
        s.close();

        //Ip must have 4 octects
        if (type == Host.Type.IP && i != 4) {
            throw new IllegalArgumentException("Ip must have 4 octects. Got "+i);
        }

        //Final reduction to single host list
        List<String> hostList = (LinkedList<String>) list.poll();
        while (!list.isEmpty()) {
            if (list.size() >= 1) {
                LinkedList end = (LinkedList<String>) list.poll();
                hostList = (LinkedList<String>) concatenateList(hostList, end, ".");
            } else {
                hostList = (LinkedList<String>) list.poll();
            }
        }
        endL = System.currentTimeMillis();
        System.out.println("Generating "+hostList.size()+" took "+ (endL-start)+"ms ("+(endL-start)*100/hostList.size()+" per 100 items)");
        return hostList.size() != 0 ? hostList : null;
    }
    

    private  static List<String> parseSinglePattern(String input, Host.Type type) {
        LinkedList list = new LinkedList(), start;
        LinkedList<String> item = new LinkedList();
        char c;
        int pos=0;
        int level=0;
        StringBuilder sb = new StringBuilder();
        while (pos != input.length()) {
            c = input.charAt(pos++);
            switch (type) {
                case HOSTNAME:
                    //We have a string
                    if (c >= 'a' && c <= 'z' || (c >= '0' && c <='9')) {
                        sb.append(c);
                        continue;
                    }
                    break;
                case IP:
                    //We have a number
                    if (c >= '0' && c <= '9') {
                        sb.append(c);
                        continue;
                    }
                    break;
                default:
                    throw new IllegalArgumentException("Unknown type " + type.toString());
            }
            //Something else
            switch (c) {
                case '[':
                    //Store previous element
                    if (sb.toString().trim().length() > 0) {
                       item.add(sb.toString().trim());
                       list.addFirst(item);
                    }
                    sb = new StringBuilder();
                    level++; //increase bracket level
                    
                    //TODO We should reduce here what we can
                    /*if (item.size() == 0) {
                        if (list.peek().getClass() == String.class && ((String) list.peek()).equals("[")) {
                            item.add("");
                            list.addFirst(item);
                            list.addFirst("[");
                            item = new LinkedList<String>();
                            continue;
                        } else {
                            item = (LinkedList<String>) list.poll();
                        }
                    }
                    while (list.size() > 0 && !(list.peek().getClass() == String.class && ((String) list.peek()).equals("["))) {
                        if (list.peek().getClass() == String.class && ((String) list.peek()).equals("-")) {
                            list.poll();
                            //Special case test[,-A] => test,test-A ; test[-A, ] => test-A,test
                            if (list.peek().getClass() == String.class && (((String) list.peek()).equals(",") || ((String) list.peek()).equals("["))) {
                                item = (LinkedList<String>) concatenateList(Arrays.asList("-"), item);
                                list.poll();
                                start = (LinkedList<String>) list.poll();
                                item = (LinkedList<String>) joinList(start, item);
                                continue;
                            }
                            //Need to reduce the range to a list
                            start = (LinkedList<String>) list.poll();
                            item = (LinkedList<String>) generateRange(start, item, type);
                        } else if (list.peek().getClass() == String.class && ((String) list.peek()).equals(",")) {
                            //Need to reduce the range to a list
                            list.poll();
                            start = (LinkedList<String>) list.poll();
                            item = (LinkedList<String>) joinList(start, item);
                        } else {
                            start =(LinkedList<String>) list.poll();
                            item = (LinkedList<String>) concatenateList(start, item);
                        }
                    }*/
                    //
                    list.addFirst("[");                    
                    item = new LinkedList<String>();
                    continue;
                case ' ':
                case ',':
                    if (sb.toString().trim().length() > 0) {
                       item.add(sb.toString().trim());
                    }
                    sb = new StringBuilder();
                    if (item.size() == 0) {
                        if (list.size() == 0 || (list.peek().getClass() == String.class && ((String) list.peek()).equals(","))) {
                            continue; //Skip folowing whitespaces
                        } else if (list.peek().getClass() == String.class && ((String) list.peek()).equals("[")) {
                            item.add("");
                            list.addFirst(item);
                            list.addFirst(",");
                            item = new LinkedList<String>();
                            continue;
                        } else {
                            item = (LinkedList<String>) list.poll();
                        }
                    }
                    //Reduction to single list [3-5 6-9]
                    while (list.size() > 0 && !(list.peek().getClass() == String.class && ((String) list.peek()).equals("["))) {
                        if (list.peek().getClass() == String.class && ((String) list.peek()).equals("-")) {
                            //Need to reduce the range to a list
                            list.poll();
                            //Special case test[,-A] => test,test-A ; test[-A, ] => test-A,test
                            if (list.peek().getClass() == String.class && (((String) list.peek()).equals(",") || ((String) list.peek()).equals("["))) {
                                item = (LinkedList<String>) concatenateList(Arrays.asList("-"), item);
                                list.poll();
                                start = (LinkedList<String>) list.poll();
                                item = (LinkedList<String>) joinList(start, item);
                                continue;
                            }
                            start = (LinkedList<String>) list.poll();
                            item = (LinkedList<String>) generateRange(start, item, type);
                        } else if (list.peek().getClass() == String.class && ((String) list.peek()).equals(",")) {
                            //Need to reduce the range to a list
                            list.poll();
                            start = (LinkedList<String>) list.poll();
                            item = (LinkedList<String>) joinList(start, item);
                        } else {
                            start =(LinkedList<String>) list.poll();
                            item = (LinkedList<String>) concatenateList(start, item);
                        }
                    }
                    list.addFirst(item);
                    list.addFirst(",");
                    item = new LinkedList<String>();
                    continue;
                case '-':
                    //Add possible last element
                    if (sb.toString().trim().length() > 0) {
                       item.add(sb.toString().trim());
                       list.addFirst(item);
                    }
                    sb = new StringBuilder();
                    item = new LinkedList<String>();
                    list.addFirst("-");
                    continue;
                case ']':
                    level--;
                    if (sb.toString().trim().length() > 0) {
                       item.add(sb.toString().trim());
                    }
                    sb = new StringBuilder();
                    if (item.size() == 0) {
                        if (list.peek().getClass() == String.class && ((String) list.peek()).equals(",")) {
                            item.add("");
                        } else {
                            item = (LinkedList<String>) list.poll();
                        }
                    }
                    while (!(list.peek().getClass() == String.class && ((String) list.peek()).equals("["))) {
                        if (list.peek().getClass() == String.class && ((String) list.peek()).equals("-")) {                            
                            list.poll();
                            //Special case test[,-A] => test,test-A ; test[-A, ] => test-A,test
                            if (list.peek().getClass() == String.class && (((String) list.peek()).equals(",") || ((String) list.peek()).equals("["))) {
                                item = (LinkedList<String>) concatenateList(Arrays.asList("-"), item);
                                list.poll();
                                start = (LinkedList<String>) list.poll();
                                item = (LinkedList<String>) joinList(start, item);
                                continue;
                            }
                            //Need to reduce the range to a list
                            start = (LinkedList<String>) list.poll();
                            item = (LinkedList<String>) generateRange(start, item, type);
                        } else if (list.peek().getClass() == String.class && ((String) list.peek()).equals(",")) {
                            //Need to reduce the range to a list
                            list.poll();
                            start = (LinkedList<String>) list.poll();
                            item = (LinkedList<String>) joinList(start, item);
                        } else {
                            start =(LinkedList<String>) list.poll();
                            item = (LinkedList<String>) concatenateList(start, item);
                        }
                    }
                    list.poll();
                    list.addFirst(item);
                    item = new LinkedList<String>();
                    continue;
                default:
                    throw new IllegalArgumentException("parseSinglePattern - invalid character '"+c+"' in "+input);
            }
        }
        if (level > 0) {
            throw new IllegalArgumentException("parseSinglePattern - uneven brackets in "+input);
        }
        if (sb.toString().trim().length()>0) {
            if (item.size()>0) {
                list.addFirst(item);
            }
            item = new LinkedList<String>();
            item.addFirst(sb.toString().trim());
        }
        //Get first element if item is empty
        if (item.size() == 0) {
            if (list.peek().getClass() == String.class && ((String) list.peek()).equals(",")) {
                list.poll();
            }
            item = (LinkedList<String>) list.poll();
        }
        //Final reduction to single list
        while (!list.isEmpty()) {
            if (list.peek().getClass() == String.class && ((String) list.peek()).equals("-")) {
                //Need to reduce the range to a list
                list.poll();
                //Special case test[,-A] => test,test-A ; test[-A, ] => test-A,test
                if (list.peek().getClass() == String.class && (((String) list.peek()).equals(",") || ((String) list.peek()).equals("["))) {
                    item = (LinkedList<String>) concatenateList(Arrays.asList("-"), item);
                    list.poll();
                    start = (LinkedList<String>) list.poll();
                    item = (LinkedList<String>) joinList(start, item);
                    continue;
                }
                start = (LinkedList<String>) list.poll();
                item = (LinkedList<String>) generateRange(start, item, type);
            } else if (list.peek().getClass() == String.class && ((String) list.peek()).equals(",")) {
                //Need to reduce the range to a list
                list.poll();
                start = (LinkedList<String>) list.poll();
                item = (LinkedList<String>) joinList(start, item);
            } else if (list.size() >= 1) {
                start = (LinkedList<String>) list.poll();
                item = (LinkedList<String>) concatenateList(start, item);
            } else {
                item = (LinkedList<String>) list.poll();
            }
        }        
        return item;
    }

    private static List<String> generateRange(List<String> start, List<String> end, final Host.Type type) {
        List<String> list = new LinkedList();
        String val;
        for (String s : start) {
            for (String e : end) {
                int start_int=0;
                int end_int=0;
                try {
                   start_int = Integer.parseInt(s);
                   end_int = Integer.parseInt(e);
                } catch (NumberFormatException ex) {
                    //Range is not valid
                    switch (type) {
                        case HOSTNAME:
                            //We should just concatenete
                            return concatenateList(start, end, "-");
                        default:
                            throw new IllegalArgumentException("Invalid num-numeric IP range");
                    }
                }
                //Small fix for invalid in IP ranges
                if (type == Host.Type.IP) {
                    start_int = Math.min(255, start_int);
                    start_int = Math.max(0, start_int);
                    end_int = Math.min(255, end_int);
                    end_int = Math.max(0, end_int);
                }
                boolean isIncreasing = start_int <= end_int;
                do {
                    val = String.valueOf(start_int);
                    //Add trailing zeros
                    while (val.length() < s.length()) {
                        val = "0"+val;
                    }
                    if (!list.contains(val)) {
                        list.add(val);
                    }
                    if (isIncreasing)
                        start_int++;
                    else
                        start_int--;
                } while ((isIncreasing) ? !(start_int > end_int) : !(start_int < end_int));
            }
        }
        return list.size()==0 ? null : list;
    }

    private static List<String> concatenateList(List<String> start, List<String> end) {
        return concatenateList(start, end, "");
    }

    private static List<String> concatenateList(List<String> start, List<String> end, String glue) {
        List<String> list = new LinkedList();
        for (String s : start) {
            for (String e : end) {
                list.add(new String(s+glue+e));
            }
        }
        return list.size()==0 ? null : list;
    }

    private static List<String> joinList(List<String> start, List<String> end) {
        List<String> list = new LinkedList();
        list.addAll(start);
        for (String val : end) {
            if (!list.contains(val)) {
                list.add(val);
            }
        }
        return list.size() == 0 ? null : list;
    }

    public static void saveListToFile(Component component, List<String> list) {
        final JFileChooser fc = new JFileChooser();
        int ret = fc.showSaveDialog(SwingUtilities.getRootPane(component).getContentPane());
        if (ret == JFileChooser.APPROVE_OPTION) {
            FileWriter fw = null;
            File f = null;
            try {
                f = fc.getSelectedFile();
                fw = new FileWriter(f);
                String separator = System.getProperty("line.separator");
                for (String s : list) {
                    fw.write(s+separator);
                }
            } catch (IOException ex) {
                Debug.error("Error: Save failed - " + ex.getMessage());
            } finally {
                try {
                    fw.close();
                    Debug.trace("List saved to " + f.getName());
                } catch (IOException ex) {
                    Debug.error("Error: Save failed - " + ex.getMessage());
                }
            }
        }
    }

    public static String substituteMultiple(Properties vars, String str, String type) throws IllegalArgumentException
    {
        if (str == null)
        {
            return null;
        }

        // Create reader and writer for the strings
        StringReader reader = null;
        StringWriter writer = null;

        // Substitute any variables
        try
        {
        	int numOfSub = 1;
        	while (numOfSub != 0) {
        		reader = new StringReader(str);
        		writer = new StringWriter();
        		numOfSub = new VariableSubstitutor(vars).substitute(reader, writer, type);
        		str = writer.getBuffer().toString();
        	}
        }
        catch (IOException e)
        {
            throw new Error("Unexpected I/O exception when reading/writing memory "
                    + "buffer; nested exception is: " + e);
        }

        // Return the resulting string
        return str;
    }

    /**
     * Filles up a template file by substituting the variables names with their values
     * @param templateFilePath The template input file to fill up
     * @param resultFilePath The result output file
     * @param variables The variables and their values
     *
     * @throws java.lang.Exception
     *
     * @see VariableSubstitutor
     */
    public static String fillUpTemplate(String templateFilePath, String resultFilePath, Properties variables) throws Exception {
    	BufferedReader bufferedReader = null;
    	BufferedWriter bufferedWriter     = null;
        VariableSubstitutor vs = new VariableSubstitutor(variables);

        File f = File.createTempFile(resultFilePath, null);
    	
    	try {
    		bufferedReader = new BufferedReader(new FileReader(templateFilePath));
    		bufferedWriter = new BufferedWriter(new FileWriter(f, false));

    		String line = null;
    		while ((line = bufferedReader.readLine()) != null) {
    			line = vs.substituteMultiple(line, null);

    			bufferedWriter.write(line);
    			bufferedWriter.newLine();
    		}
    	} finally {
    		if (bufferedReader != null) {
    			bufferedReader.close();
    		}
    		if (bufferedWriter != null) {
    			bufferedWriter.close();
    		}
            return f.getAbsolutePath();
    	}
    }

    /**
     * Recursively searches for the parent InstallerFrame coponent
     * @param comp The Component to search in
     * @return The found InstallerFrame component if exists, null otherwise.
     *
     * @see InstallerFrame
     */
    public static Component findParentInstallerFrameComponent(Component comp) {
        Component parent = (Component)comp.getParent();

        if (parent instanceof InstallerFrame) {
            return parent;
        } else if (parent != null) {
            return findParentInstallerFrameComponent(parent);
        } else {
            return null;
        }
    }

    /**
     * Removes the domain from a FQDN host
     * @param hostName The fully qualified domain name
     * @return The domain name if exists, otherwise an empty String
     */
    public static String getDomainName(String fQDomainName) {
        String domain = "";
        int firstDotIndex = fQDomainName.indexOf(".", 0);

        if (firstDotIndex > -1) {
            domain = fQDomainName.substring(firstDotIndex + 1);
        }
        return domain;
    }

    /**
     * Pings the host
     * @param sgeRoot The SGE_ROOT
     * @param host The host to be reached
     * @param port The port to check
     * @param component The type of the component to find on the given port: "qmaster" or "execd"
     * @return True only and only if the host is reachable on the given port
     */
    public static boolean pingHost(Properties variables, Host host, String component, int maxTries) {
        String qping = variables.getProperty(VAR_SGE_ROOT) + "/bin/" + Host.localHostArch + "/qping";
        String port = (component.equalsIgnoreCase("qmaster")) ? variables.getProperty(VAR_SGE_QMASTER_PORT) : (component.equalsIgnoreCase("execd")) ? variables.getProperty(VAR_SGE_EXECD_PORT) : "-1";
        try {
            CommandExecutor cmdExec = null;
            int tries = 0;
            while (tries < maxTries) {
                // TODO does -tcp option is what we need?
                cmdExec = new CommandExecutor(qping, "-tcp", "-info", host.getHostAsString(), port, component, "1");
                Map env = cmdExec.getEnvironment();
                env.put("SGE_ROOT", variables.getProperty(VAR_SGE_ROOT));
                env.put("SGE_CELL", variables.getProperty(VAR_SGE_CELL_NAME));
                env.put("SGE_QMASTER_PORT", variables.getProperty(VAR_SGE_QMASTER_PORT));
                env.put("SGE_EXECD_PORT", variables.getProperty(VAR_SGE_EXECD_PORT));
                cmdExec.execute();

                if (cmdExec.getExitValue() == 0) {
                    return true;
                } else {
                    Thread.currentThread().sleep(2000);
                }

                tries++;
            }

            Debug.trace("Tried to ping host " + tries + " time(s).");
        } catch (Exception e) {
            Debug.error(e);
        }

        return false;
    }

    /**
     * Returns with the file system type of the given directory. If the directory does not exist first creates it
     * then after the check deletes it immediately.
     * @param variables The install data variables
     * @param dir The directory path to be checked
     * @return The FS type of the given directory if the check was successful, otherwise empty string.
     */
    public static String getDirFSType(Properties variables, String dir) {
       return getDirFSType(Host.localHostName, variables, dir);
    }

    public static String getDirFSType(String host, Properties variables, String dir) {
        VariableSubstitutor vs = new VariableSubstitutor(variables);
        CommandExecutor cmdExec = null;
        String result = "";

        dir = vs.substituteMultiple(dir, null);

        ExtendedFile file = new ExtendedFile(dir).getFirstExistingParent();
        Debug.trace("First existing parent of '" + dir + "' is '" + file.getAbsolutePath() +"'.");

        try {
            // Call the 'fstype' script of the proper architecture
            String fstypeScript = "${cfg.sge.root}/utilbin/${localhost.arch}/fstype";
            cmdExec = new CommandExecutor(variables, variables.getProperty(VAR_SHELL_NAME), host, vs.substituteMultiple(fstypeScript, null), dir);
            cmdExec.execute();

            if (cmdExec.getExitValue() == 0) {
                result = cmdExec.getOutput().firstElement().trim();
                Debug.trace("FSType of '" + dir + "' is '" + result +"'.");
            }
        } catch (Exception e) {
            Debug.error(e);
        }

        return result;
    }

    /**
     * Returns the group id of the user.
     * @param user The user name
     * @return The group id of the user if the process was successful, otherwise empty string.
     */
    public static String getUserGroup(String user) {
        String group = "";
        ExtendedFile tmpFile = null;

        try {
            tmpFile = new ExtendedFile(File.createTempFile("grouptest", null).getAbsolutePath());

            group = tmpFile.getGroup();
        } catch (IOException ex) {
            Debug.error("Can not creat file into default temporary directory! " + ex);
        } finally {
            if (tmpFile != null & tmpFile.exists()) {
                tmpFile.delete();
            }
        }

        Debug.trace("Group of user '" + user + "' is '" + group + "'.");

        return group;
    }

    /**
     * Sources the sge <SGE_ROOT>/<CELL_NAME>/common/settings.sh file.
     * @param sgeRoot The SGE_ROOT directory
     * @param cellName The CELL_NAME value
     * @return List of key values pairs sourced from the file.
     * 
     * @throws java.io.FileNotFoundException
     * @throws java.io.IOException
     */
    public static Properties sourceSGESettings(String sgeRoot, String cellName) throws FileNotFoundException, IOException {
        Properties settings = new Properties();
        String settingsshFile = sgeRoot + "/" + cellName + "/common/settings.sh";
        ArrayList<String> settingsshLines = FileHandler.readFileContent(settingsshFile, false);

        for (String line : settingsshLines) {
            // Process lines like 'SGE_CELL=default; export SGE_CELL'
            if (line.startsWith("SGE_")) {
                String[] keyValuePair = line.split(";")[0].split("=");
                settings.setProperty(keyValuePair[0].trim(), keyValuePair[1].trim());
            }
        }

        Debug.trace("Sourced settings from '" + settingsshFile + "' are " + settings + ".");

        return settings;
    }

    /**
     * Sources the sge <SGE_ROOT>/<CELL_NAME>/common/sboostrap file.
     * @param sgeRoot The SGE_ROOT directory
     * @param cellName The CELL_NAME value
     * @return List of key values pairs sourced from the file.
     *
     * @throws java.io.FileNotFoundException
     * @throws java.io.IOException
     */
    public static Properties sourceSGEBootstrap(String sgeRoot, String cellName) throws FileNotFoundException, IOException {
        Properties settings = new Properties();
        String boostrapFile = sgeRoot + "/" + cellName + "/common/bootstrap";
        ArrayList<String> bootstrapLines = FileHandler.readFileContent(boostrapFile, false);
        
        for (String line : bootstrapLines) {
            // Process lines like 'spooling_method         berkeleydb'
            if (!line.startsWith("#")) {
                int spaceIndex = line.indexOf(' ');
                settings.setProperty(line.substring(0, spaceIndex).trim(), line.substring(spaceIndex).trim());
            }
        }

        Debug.trace("Sourced settings from '" + boostrapFile + "' are " + settings + ".");

        return settings;
    }

    /**
     * Sources the <SGE_ROOT>/<CELL_NAME>/common/act_qmaster file and return with the name of qmaster host.
     * @param sgeRoot The SGE_ROOT directory.
     * @param cellName The CELL_NAME value
     * @return The name of the qmaster host.
     * @throws java.io.FileNotFoundException
     * @throws java.io.IOException
     */
    public static String getQmasterHost(String sgeRoot, String cellName) throws FileNotFoundException, IOException {
        String qmasterHost = "";
        String actQmasterFile = sgeRoot + "/" + cellName + "/common/act_qmaster";
        ArrayList<String> settingsshLines = FileHandler.readFileContent(actQmasterFile, false);

        if (settingsshLines.size() > 0) {
            qmasterHost = settingsshLines.get(0).trim();
        }

        Debug.trace("Found qmaster host name in '" + actQmasterFile + "' is '" + qmasterHost + "'.");

        return qmasterHost;
    }
}
