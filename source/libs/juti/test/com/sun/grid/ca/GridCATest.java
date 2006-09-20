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
        
        catop.mkdir();
        calocaltop.mkdir();
        
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
    
    protected void tearDown() throws Exception {
        Runtime.getRuntime().exec("chmod -R u+w " + baseDir.getAbsolutePath());
        Runtime.getRuntime().exec("rm -rf " + baseDir.getAbsolutePath());
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
