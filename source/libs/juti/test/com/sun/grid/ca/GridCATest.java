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
import java.security.KeyStore;
import java.security.cert.Certificate;
import java.security.cert.X509Certificate;
import java.util.Calendar;
import java.util.Date;
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
        File tmpDir = new File(System.getProperty("java.io.tmpdir"));
        
        baseDir = new File(tmpDir, System.getProperty("user.name") + "_" + System.currentTimeMillis());
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
    
    private void initCA() throws Exception {
        
        InitCAParameters params = new InitCAParameters();
        params.setCountry("de");
        params.setState("Bayern");
        params.setLocation("Regensburg");
        params.setOrganization("Sun");
        params.setOrganizationUnit("Software Engineering");
        params.setAdminEmailAddress("admin@blubber");
        ca.init(params);
    } 
    
    public void testUser() throws Exception {
        
        String username = "test";
        initCA();
        
        ca.createUser(username, "test", "test@blubber");
        
        X509Certificate cert = ca.getCertificate(username);
        
        char [] pw = "changeit".toCharArray();
        KeyStore ks = ca.createKeyStore("test", pw, pw);
        
        Certificate [] chain =  ks.getCertificateChain(username);
        
        assertNotNull("certificate chain for user " + username + " not found", chain);
        
        Calendar cal = Calendar.getInstance();
        int days = 10;
        ca.renewCertificate(username, days);
        
        X509Certificate renewedCert =  ca.getCertificate(username);
        assertNotNull("renewed certificate chain for user " + username + " not found", renewedCert);
        
        cal.add(Calendar.DAY_OF_YEAR, days + 1);
        assertTrue( cal.getTimeInMillis() > renewedCert.getNotAfter().getTime());
    }
    
    public void testDaemon() throws Exception {
        
        String daemon = "test";
        String user = System.getProperty("user.name");
        initCA();
        
        ca.createDaemon(daemon, user, user + "@blubber");
        
        X509Certificate cert = ca.getDaemonCertificate("test");
        
        char [] pw = "changeit".toCharArray();
        KeyStore ks = ca.createDaemonKeyStore(daemon);
        
        
        Certificate [] chain =  ks.getCertificateChain(daemon);
        
        assertNotNull("certificate chain for daemon " + daemon + " not found is keystore", chain);
        
        Calendar cal = Calendar.getInstance();
        int days = 10;
        ca.renewDaemonCertificate(daemon, days);
        
        X509Certificate renewedCert =  ca.getDaemonCertificate(daemon);
        assertNotNull("renewed certificate chain for daemon " + daemon + " not found", renewedCert);
        
        cal.add(Calendar.DAY_OF_YEAR, days + 1);
        assertTrue( cal.getTimeInMillis() > renewedCert.getNotAfter().getTime());
    }
    
}
