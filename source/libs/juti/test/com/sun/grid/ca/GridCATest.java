/*
 * GridCATest.java
 *
 * Created on July 4, 2006, 11:40 AM
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */

package com.sun.grid.ca;

import com.sun.grid.TestConfiguration;
import java.io.File;
import java.net.InetAddress;
import java.security.KeyStore;
import java.security.cert.Certificate;
import java.security.cert.CertificateException;
import java.security.cert.X509Certificate;
import junit.framework.*;

/**
 *
 * @author rh150277
 */
public class GridCATest extends TestCase {
    
    private File baseDir;
    private File catop;
    private File calocaltop;
    private GridCA ca;
    
    /** Creates a new instance of GridCATest */
    public GridCATest(String name) {
        super(name);
    }
    
    protected void setUp() throws Exception {
        super.setUp();
        String basedirStr = "build/test/" + System.currentTimeMillis();
        basedirStr = basedirStr.replace('/', File.separatorChar);
        
        baseDir = new File(basedirStr);
        baseDir.mkdirs();
        
        catop = new File(baseDir, "catop");
        calocaltop = new File(baseDir, "calocaltop");
        
        TestConfiguration testConfig = TestConfiguration.getInstance();

        
        GridCAConfiguration config = new GridCAConfiguration();

        config.setCaTop(catop);
        config.setCaLocalTop(calocaltop);
        config.setSgeCaScript(testConfig.getCaScript());
        config.setCaHost("localhost");
        config.setAdminUser(testConfig.getAdminUser());
        config.validate();

        ca = GridCAFactory.newInstance(config);
    }
    
    
    public void test1() throws Exception {
        
        String username = "test";
        
        String country = "de";
        String state = "Bayern";
        String location = "Regensburg";
        String org = "Sun";
        String orgUnit = "Gridengine";
        String adminEmailAddress = "admin@blubber";
        ca.init(country, state, location, org, orgUnit, adminEmailAddress);
        ca.createUser(username, "test", "test@blubber");
        
        X509Certificate cert = ca.getCertificate("test");
        
        char [] pw = "changeit".toCharArray();
        KeyStore ks = ca.createKeyStore("test", pw, pw);
        
        Certificate [] chain =  ks.getCertificateChain("test");
        
        assertNotNull("certificate chain for user " + username + " not found in keystore", chain);
        
        if(System.getProperty("user.name").equals("root")) {
            ca.renewCertificate(username, 10);
        }
    }
    
}
