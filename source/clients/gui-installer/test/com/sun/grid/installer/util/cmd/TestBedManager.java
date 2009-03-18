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
package com.sun.grid.installer.util.cmd;

import com.sun.grid.installer.gui.Host.State;
import com.sun.grid.installer.util.Config;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Random;

public class TestBedManager implements Config {
    private static ArrayList<String> ipAddresses = null;

    // Store ip-architecture-log mappings
    private Hashtable<String, String> ipAddressMap = null;
    private Hashtable<String, String> architectureMap = null;
    private Hashtable<String, String> logMap = null;

    // Store exit values for certain actions in <hostName, exitValue> form.
    private Hashtable<String, Integer> copyMap = null;
    private Hashtable<String, Integer> resolveMap = null;
    private Hashtable<String, Integer> getArchitectureMap = null;
    private Hashtable<String, Integer> validationMap = null;
    private Hashtable<String, Integer> installationMap = null;

    private static final Object SYNC = new Object();
    private static Random random = new Random();

    private static TestBedManager testBedManager = null;

    private static int HOST_NAME_INDEX = 0;
    private static final String HOST_NAME_PREFIX = "grid";

    public static int EXIT_VAL_SOMETHING = 666;

    public static final String[] ARCHITECTURES = new String[] {
        "sol-amd64","sol-sparc64", "hp11", "hp11-64", "aix51", "lx24-ia64", "lx24-amd64", "darwin-ppc"
    };
    public static final int[] RESOLVE_EXIT_VALUES = new int[]{
        EXIT_VAL_SUCCESS, // Increase the hit ratio of Success
        EXIT_VAL_SUCCESS,
        EXIT_VAL_SUCCESS,
        EXIT_VAL_SUCCESS,
        EXIT_VAL_UNKNOWN_HOST,
        EXIT_VAL_SOMETHING, // TODO define an overall failed exit value,
        EXIT_VAL_CMDEXEC_TERMINATED,
        EXIT_VAL_CMDEXEC_INTERRUPTED
    };
    public static final int[] COPY_EXIT_VALUES = new int[]{
        EXIT_VAL_SUCCESS, // Increase the hit ratio of Success
        EXIT_VAL_SUCCESS,
        EXIT_VAL_SUCCESS,
        EXIT_VAL_SUCCESS,
        EXIT_VAL_SUCCESS,
        EXIT_VAL_SUCCESS,
        EXIT_VAL_SOMETHING, // TODO define an overall failed exit value
        EXIT_VAL_CMDEXEC_TERMINATED,
        EXIT_VAL_CMDEXEC_MISSING_FILE,
        EXIT_VAL_CMDEXEC_INTERRUPTED
    };
    public static final int[] GETARCHITECTURE_EXIT_VALUES = new int[]{
        EXIT_VAL_SUCCESS, // Increase the hit ratio of Success
        EXIT_VAL_SUCCESS,
        EXIT_VAL_SUCCESS,
        EXIT_VAL_SUCCESS,
        EXIT_VAL_SUCCESS,
        EXIT_VAL_SUCCESS,
        EXIT_VAL_SOMETHING, // TODO define an overall failed exit value
        EXIT_VAL_CMDEXEC_TERMINATED,
        EXIT_VAL_CMDEXEC_MISSING_FILE,
        EXIT_VAL_CMDEXEC_INTERRUPTED
    };
    public static final int[] VALIDATION_EXIT_VALUES = new int[]{
        EXIT_VAL_SUCCESS, // Increase the hit ratio of Success
        EXIT_VAL_SUCCESS,
        EXIT_VAL_SUCCESS,
        EXIT_VAL_SUCCESS,
        EXIT_VAL_SUCCESS,
        EXIT_VAL_SUCCESS,
        EXIT_VAL_SUCCESS,
        EXIT_VAL_SUCCESS,
        EXIT_VAL_SUCCESS,
        EXIT_VAL_SUCCESS,
        EXIT_VAL_SUCCESS,
        EXIT_VAL_QMASTER_SPOOL_DIR_PERM_DENIED,
        EXIT_VAL_EXECD_SPOOL_DIR_PERM_DENIED,
        EXIT_VAL_JMX_KEYSTORE_PERM_DENIED,
        EXIT_VAL_JVM_LIB_DOES_NOT_EXIST_QMASTER,
        EXIT_VAL_JVM_LIB_INVALID_QMASTER,
        EXIT_VAL_BDB_SPOOL_DIR_EXISTS,
        EXIT_VAL_BDB_SPOOL_WRONG_FSTYPE,
        EXIT_VAL_BDB_SPOOL_DIR_PERM_DENIED,
        EXIT_VAL_JVM_LIB_DOES_NOT_EXIST_SHADOWD,
        EXIT_VAL_JVM_LIB_INVALID_SHADOWD,
        EXIT_VAL_EXECD_SPOOL_DIR_LOCAL_PERM_DENIED,
        EXIT_VAL_BDB_SERVER_SPOOL_DIR_PERM_DENIED,
        EXIT_VAL_BDB_SERVER_SPOOL_DIR_EXISTS,
        EXIT_VAL_ADMIN_USER_NOT_KNOWN,
        EXIT_VAL_SOMETHING, // TODO define an overall failed exit value
        EXIT_VAL_CMDEXEC_TERMINATED,
        EXIT_VAL_CMDEXEC_MISSING_FILE,
        EXIT_VAL_CMDEXEC_INTERRUPTED
    };
    public static final int[] INSTALLATION_EXIT_VALUES = new int[]{
        EXIT_VAL_SUCCESS, // Increase the hit ratio of Success
        EXIT_VAL_SUCCESS,
        EXIT_VAL_SUCCESS,
        EXIT_VAL_SUCCESS,
        EXIT_VAL_SUCCESS,
        EXIT_VAL_SUCCESS,
        EXIT_VAL_SUCCESS,
        EXIT_VAL_SUCCESS,
        EXIT_VAL_SUCCESS,
        EXIT_VAL_SOMETHING, // TODO define an overall failed exit value
        EXIT_VAL_FAILED_ALREADY_INSTALLED_COMPONENT,
        EXIT_VAL_CMDEXEC_TERMINATED,
        EXIT_VAL_CMDEXEC_MISSING_FILE,
        EXIT_VAL_CMDEXEC_INTERRUPTED
    };

    public static final int IP_RANGE_MIN = 0;
    public static final int IP_RANGE_MAX = 255;

    private static enum GenerationMode {
        ALWAYS_NEW,    // always generate new value
        ALWAYS_FIRST,  // generate only at first time then give the same value
        SECOND_SUCCEDS // generate only at first time then give EXIT_VAL_SUCCESS
    }

    private static final GenerationMode GENERATION_MODE = GenerationMode.SECOND_SUCCEDS;

    private TestBedManager() {
        ipAddresses = new ArrayList<String>();
        ipAddressMap = new Hashtable<String, String>();
        architectureMap = new Hashtable<String, String>();
        logMap = new Hashtable<String, String>();

        copyMap = new Hashtable<String, Integer>();
        resolveMap = new Hashtable<String, Integer>();
        getArchitectureMap = new Hashtable<String, Integer>();
        validationMap = new Hashtable<String, Integer>();
        installationMap = new Hashtable<String, Integer>();
    }

    public static TestBedManager getInstance() {
        if (testBedManager == null) {
            testBedManager = new TestBedManager();
        }

        return testBedManager;
    }

    public Hashtable<String, String> getArchitectureMap() {
        return architectureMap;
    }

    public Hashtable<String, Integer> getCopyMap() {
        return copyMap;
    }

    public Hashtable<String, Integer> getGetArchitectureMap() {
        return getArchitectureMap;
    }

    public Hashtable<String, Integer> getInstallationMap() {
        return installationMap;
    }

    public Hashtable<String, String> getIpAddressMap() {
        return ipAddressMap;
    }

    public Hashtable<String, String> getLogMap() {
        return logMap;
    }

    public Hashtable<String, Integer> getResolveMap() {
        return resolveMap;
    }

    public Hashtable<String, Integer> getValidationMap() {
        return validationMap;
    }

    public String getName(String ip) {
        String name = "";

        Enumeration<String> keys = ipAddressMap.keys();
        while (keys.hasMoreElements()) {
            name = keys.nextElement();
            if (ipAddressMap.get(name).equals(ip)) {
                return name;
            }
        }

        ipAddresses.add(ip);
        name = generateUniqueName();
        ipAddressMap.put(name, ip);

        return name;
    }

    public String getIPAddress(String host) {
        String ipAddress = "";

        if (ipAddressMap.containsKey(host)) {
            ipAddress = ipAddressMap.get(host);
        } else {
            ipAddress = generateUniqeIPAddress();
            ipAddressMap.put(host, ipAddress);
        }

        return ipAddress;
    }

    public String getArchitecture(String host) {
        String architecture = "";

        if (architectureMap.containsKey(host)) {
            architecture = architectureMap.get(host);
        } else {
            architecture = generateArchitecture();
            architectureMap.put(host, architecture);
        }

        return architecture;
    }

    public int getCopyExitValue(String host) {
        int exitValue = EXIT_VAL_SUCCESS;

        if (GENERATION_MODE != GenerationMode.ALWAYS_NEW &&
                copyMap.containsKey(host)) {

            if (GENERATION_MODE == GenerationMode.ALWAYS_FIRST) {
                exitValue = copyMap.get(host);
            }
        } else {
            exitValue = generateCopyExitValue();
            copyMap.put(host, exitValue);
        }

        return exitValue;
    }

    public int getResolveExitValue(String host) {
        int exitValue = EXIT_VAL_SUCCESS;

        if (GENERATION_MODE != GenerationMode.ALWAYS_NEW &&
                resolveMap.containsKey(host)) {

            if (GENERATION_MODE == GenerationMode.ALWAYS_FIRST) {
                exitValue = resolveMap.get(host);
            }
        } else {
            exitValue = generateGetArchitectureExitValue();
            resolveMap.put(host, exitValue);
        }

        return exitValue;
    }

    public int getGetArchitectureExitValue(String host) {
        int exitValue = EXIT_VAL_SUCCESS;

        if (GENERATION_MODE != GenerationMode.ALWAYS_NEW &&
                getArchitectureMap.containsKey(host)) {

            if (GENERATION_MODE == GenerationMode.ALWAYS_FIRST) {
                exitValue = getArchitectureMap.get(host);
            }
        } else {
            exitValue = generateGetArchitectureExitValue();
            getArchitectureMap.put(host, exitValue);
        }

        return exitValue;
    }

    public int getValidationExitValue(String host) {
        int exitValue = EXIT_VAL_SUCCESS;

        if (GENERATION_MODE != GenerationMode.ALWAYS_NEW &&
                validationMap.containsKey(host)) {

            if (GENERATION_MODE == GenerationMode.ALWAYS_FIRST) {
                exitValue = validationMap.get(host);
            }
        } else {
            exitValue = generateValidationExitValue();
            validationMap.put(host, exitValue);
        }

        return exitValue;
    }

    // TODO store component as key
    public int getInstallationExitValue(String host) {
        int exitValue = EXIT_VAL_SUCCESS;

        if (GENERATION_MODE != GenerationMode.ALWAYS_NEW &&
                installationMap.containsKey(host)) {

            if (GENERATION_MODE == GenerationMode.ALWAYS_FIRST) {
                exitValue = installationMap.get(host);
            }
        } else {
            exitValue = generateInstallationExitValue();
            installationMap.put(host, exitValue);
        }

        return exitValue;
    }

    public String getLog(String host) {
        String log = "";

        if (logMap.containsKey(host)) {
            log = logMap.get(host);
        } else {
            log = generateLog(host);
            logMap.put(host, log);
        }

        return log;
    }

    public static int generateResolveExitValue() {
        int exitValue;

        exitValue = RESOLVE_EXIT_VALUES[random(0, RESOLVE_EXIT_VALUES.length - 1)];

        return exitValue;
    }

    public static int generateCopyExitValue() {
        int exitValue;

        exitValue = COPY_EXIT_VALUES[random(0, COPY_EXIT_VALUES.length - 1)];

        return exitValue;
    }

    public static int generateGetArchitectureExitValue() {
        int exitValue;

        exitValue = GETARCHITECTURE_EXIT_VALUES[random(0, GETARCHITECTURE_EXIT_VALUES.length - 1)];

        return exitValue;
    }

    public static int generateValidationExitValue() {
        int exitValue;

        exitValue = VALIDATION_EXIT_VALUES[random(0, VALIDATION_EXIT_VALUES.length - 1)];

        return exitValue;
    }

    public static int generateInstallationExitValue() {
        int exitValue;

        exitValue = INSTALLATION_EXIT_VALUES[random(0, INSTALLATION_EXIT_VALUES.length - 1)];

        return exitValue;
    }

    public static State generateState() {
        State state;

        state = State.values()[random(0, State.values().length - 1)];

        return state;
    }

    public static String generateArchitecture() {
        String architecture = "";

        architecture = ARCHITECTURES[random(0, ARCHITECTURES.length - 1)];

        return architecture;
    }

    public String generateUniqeIPAddress() {
        String ipAddress = generateIPAddress();

        synchronized (SYNC) {
            if (ipAddresses.contains(ipAddress)) {
                ipAddress = generateUniqeIPAddress();
            } else {
                ipAddresses.add(ipAddress);
            }
        }

        return ipAddress;
    }

    public static String generateIPAddress() {
        String ipAddress = "";
        
        ipAddress += random(IP_RANGE_MIN, IP_RANGE_MAX);
        ipAddress += ".";
        ipAddress += random(IP_RANGE_MIN, IP_RANGE_MAX);
        ipAddress += ".";
        ipAddress += random(IP_RANGE_MIN, IP_RANGE_MAX);
        ipAddress += ".";
        ipAddress += random(IP_RANGE_MIN, IP_RANGE_MAX);
        
        return ipAddress;
    }

    public static String generateLog(String suffix) {
        String log = "";

        log = "This is a generated log for:" + suffix;

        return log;
    }

    public static String generateUniqueName() {
        String name = "";

        name = HOST_NAME_PREFIX + " " + HOST_NAME_INDEX;
        HOST_NAME_INDEX++;

        return name;
    }

    public static int random(int lo, int hi) {
        int n = hi - lo + 1;
        int i = random.nextInt() % n;
        if (i < 0) {
            i = -i;
        }
        return lo + i;
    }

    public static long getResolveSleepLength() {
        return random(1000, 2000);
    }

    public static long getGetArchitectureSleepLength() {
        return random(1000, 2000);
    }

    public static long getCopySleepLength() {
        return random(1000, 2000);
    }

    public static long getValidationSleepLength() {
        return random(2000, 4000);
    }

    public static long getInstallationSleepLength() {
        return random(3000, 5000);
    }

    public static long getLSSleepLength() {
        return 100;
    }

    public static long getFSTypeSleepLength() {
        return 100;
    }
}
