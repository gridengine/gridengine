/*
 * TestConfiguration.java
 *
 * Created on July 18, 2006, 9:31 AM
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */

package com.sun.grid;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.Iterator;
import java.util.Properties;

/**
 *
 * @author rh150277
 */
public class TestConfiguration {
    
    private File catop;
    private File calocaltop;
    private File cascript;
    private String adminUser;
    private Properties props = new Properties();
    
    private static TestConfiguration theInstance;
    public synchronized static TestConfiguration getInstance() throws IOException {
        if(theInstance == null) {
            theInstance = new TestConfiguration();
        }
        return theInstance;
    }
    
    protected TestConfiguration() throws IOException {
        
        File file = new File("test/TestConfiguration.properties".replace('/', File.separatorChar));
        props.load(new FileInputStream(file));
        
        File privFile = new File("test/TestConfiguration_private.properties".replace('/', File.separatorChar));
        
        if(privFile.exists()) {
            
            Properties privProps = new Properties();
            privProps.load(new FileInputStream(privFile));
            
            Iterator iter = privProps.keySet().iterator();
            while(iter.hasNext()) {
                Object prop = iter.next();
                props.put(prop, privProps.get(prop));
            }
        }  
        
        String fileProp = System.getProperty(getClass().getName() + ".file");
        if(fileProp != null) {
            Properties fileProps = new Properties();
            fileProps.load(new FileInputStream(fileProp));
            
            Iterator iter = fileProps.keySet().iterator();
            while(iter.hasNext()) {
                Object prop = iter.next();
                props.put(prop, fileProps.get(prop));
            }
        }
        
    }
    
    private File getFileFromConfig(String key) {
        String str = props.getProperty(key);
        if(str == null) {
            throw new IllegalArgumentException(key + " is not defined in test configuration");
        }
        return new File(str);
    }
    
    public File getCatop() {
        if(catop == null) {
            String str = getCell() + "common/sgeCA";
            catop = new File(getSgeRoot(), str.replace('/', File.separatorChar) );
        }
        return catop;
    }
    
    public File getCaLocalTop() {
        if(calocaltop == null) {
            String str = "/var/sgeCA/port" + getQMasterPort() + "/" + getCell();
            calocaltop = new File(str.replace('/', File.pathSeparatorChar));
        }
        return calocaltop;
    }
    
    public File getCaScript() {
        if(cascript == null) {
            cascript = new File(getSgeRoot(), "util/sgeCA/sge_ca".replace('/', File.separatorChar));
        }
        return cascript;
    }
    
    
    private int qmasterPort = -1;
    public int getQMasterPort() {
        if(qmasterPort < 0) {
            qmasterPort = Integer.parseInt(props.getProperty("sge_qmaster_port"));
        }
        return qmasterPort;
    }
    
    public String getAdminUser() {
        if(adminUser == null) {
            adminUser = props.getProperty("adminuser");
        }
        return adminUser;
    }
    
    public String getTestUser() {
        return props.getProperty("testuser");
    }
    
    public char [] getTestUserPassword() {
        String pw = props.getProperty("testuser_pw");
        if(pw != null) {
            return pw.toCharArray();
        }
        return null;
    }
    
    public String getUserVerifier() {
        return props.getProperty("userverifier");
    }
    
    public String getPamService() {
        return props.getProperty("pam_service");
    }
    
    private File sgeRoot;
    
    public File getSgeRoot() {
        if(sgeRoot == null) {
            sgeRoot = new File(props.getProperty("sge_root"));
        }
        return sgeRoot;
    }
    
    private String cell;
    
    public String getCell() {
        if(cell == null) {
            cell = props.getProperty("sge_cell");
        }
        return cell;
    }
}
