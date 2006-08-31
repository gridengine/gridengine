/*
 * AuthUserWrapperTest.java
 *
 * Created on July 18, 2006, 2:28 PM
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */

package com.sun.grid.security.login;

import com.sun.grid.TestConfiguration;
import com.sun.grid.util.SGEUtil;
import java.io.File;
import java.util.Iterator;
import java.util.Set;
import java.util.logging.Level;
import java.util.logging.Logger;
import junit.framework.TestCase;

/**
 *
 * @author rh150277
 */
public class AuthUserWrapperTest extends TestCase{
    
    private static final Logger LOGGER = Logger.getLogger(AuthUserWrapperTest.class.getName());
    private File authuser;
    public AuthUserWrapperTest(String testName) {
        super(testName);
    }

    protected void setUp() throws Exception {
        TestConfiguration config = TestConfiguration.getInstance();
        File utilbin = new File(config.getSgeRoot(), "utilbin");
        File utilbinArch = new File(utilbin, SGEUtil.getArch(config.getSgeRoot()));
        authuser = new File(utilbinArch, "authuser");
    }

    protected void tearDown() throws Exception {
    }
    
    public void testShadow() throws Exception {
        
        TestConfiguration config = TestConfiguration.getInstance();
        AuthUserWrapper wrapper = AuthUserWrapper.newInstanceForShadow(authuser.getAbsolutePath());
        
        Set principals = wrapper.authenticate(config.getTestUser(), config.getTestUserPassword());

        assertNotNull("no principals found", principals);
        
        Iterator iter = principals.iterator();
        while(iter.hasNext()) {
            LOGGER.log(Level.FINE,"user {0} has principal {1}", new Object [] { config.getTestUser(), iter.next() });
        }
        
        principals =  wrapper.authenticate(config.getTestUser(), new char[0]); 
        assertNull("login which empty password successeded", principals);
    }
    
    public void testPam() throws Exception {
        TestConfiguration config = TestConfiguration.getInstance();
    
        AuthUserWrapper wrapper = AuthUserWrapper.newInstanceForPam(authuser.getAbsolutePath(), config.getPamService());
        
        Set principals = wrapper.authenticate(config.getTestUser(), config.getTestUserPassword());

        assertNotNull("no principals found", principals);
        
        Iterator iter = principals.iterator();
        while(iter.hasNext()) {
            LOGGER.log(Level.FINE,"user {0} has principal {1}", new Object [] { config.getTestUser(), iter.next() });
        }
        
        principals =  wrapper.authenticate(config.getTestUser(), new char[0]); 
        assertNull("login which empty password successeded", principals);
    }
    
}
