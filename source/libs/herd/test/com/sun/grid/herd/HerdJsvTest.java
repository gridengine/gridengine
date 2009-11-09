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
 *   Copyright: 2009 by Sun Microsystems, Inc.
 *
 *   All Rights Reserved.
 *
 ************************************************************************/
/*___INFO__MARK_END__*/

package com.sun.grid.herd;

import com.sun.grid.jsv.JsvManager;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import junit.framework.TestCase;

/**
 *
 */
public class HerdJsvTest extends TestCase {
    public HerdJsvTest(String testName) {
        super(testName);
    }

    /**
     * Test of onStart method, of class HerdJsv.
     */
    public void testOnStart() {
        System.out.println("onStart()");
        JsvManager jsv = new JsvManager();
        HerdJsv instance = new HerdJsv();

        // This is really just a smoke test.  The onStart() method doesn't
        // actually do anything.
        instance.onStart(jsv);
    }

    /**
     * Test of buildBlockRequests() method, of class HerdJsv.
     */
    public void testBuildBlockRequests() throws Exception {
        System.out.println("buildBlockRequests()");
    }

    /**
     * Test of toIdString() method, of class HerdJsv.
     */
    public void testToIdString() throws Exception {
        System.out.println("toIdString()");
    }

    /**
     * Test of buildRackRequests() method, of class HerdJsv.
     */
    public void testBuildRackRequests() throws Exception {
        System.out.println("buildRackRequests()");
    }

    /**
     * Test of collateRacks() method, of class HerdJsv.
     */
    public void testCollateRacks() throws Exception {
        System.out.println("collateRacks()");
    }

    /**
     * Test of getSortedRacks() method, of class HerdJsv.
     */
    public void testGetSortedRacks() throws Exception {
        System.out.println("getSortedRacks()");
    }

    private static Object callPrivateMethod(Object obj, String method, Class[] params, Object[] args) throws Exception {
        Object ret = null;
        Method m = obj.getClass().getDeclaredMethod(method, params);

        m.setAccessible(true);
        ret = m.invoke(obj, args);

        return ret;
    }

    private static <T> T getPrivateField(Object obj, String field) throws Exception {
        T ret = null;
        Field f = obj.getClass().getDeclaredField(field);

        f.setAccessible(true);
        ret = (T) f.get(obj);

        return ret;
    }

    private static void setPrivateField(Object obj, String field, Object value) throws Exception {
        Field f = obj.getClass().getDeclaredField(field);

        f.setAccessible(true);
        f.set(obj, value);
    }
}
