/*
 * SGEUtilTest.java
 * JUnit based test
 *
 * Created on July 24, 2006, 10:49 AM
 */

package com.sun.grid.util;

import com.sun.grid.TestConfiguration;
import junit.framework.*;

/**
 *
 * @author rh150277
 */
public class SGEUtilTest extends TestCase {
    
    private TestConfiguration config;
    
    public SGEUtilTest(String testName) {
        super(testName);
    }

    protected void setUp() throws Exception {
        config = TestConfiguration.getInstance();
    }

    protected void tearDown() throws Exception {
    }
    
    public void testGetArch() throws Exception {        
        String arch = SGEUtil.getArch(config.getSgeRoot());
        assertNotNull("arch must not be null", arch);
    }
}
