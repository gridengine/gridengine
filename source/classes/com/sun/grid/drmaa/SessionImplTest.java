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
/*
 * SessionImplTest.java
 * JUnit based test
 *
 * Created on November 15, 2004, 10:41 AM
 */

package com.sun.grid.drmaa;

import java.util.*;
import junit.framework.*;
import org.ggf.drmaa.*;

/**
 *
 * @author dan.templeton@sun.com
 */
public class SessionImplTest extends TestCase {
   private Session session = null;
   
   public SessionImplTest (java.lang.String testName) {
      super (testName);
   }
   
   public static Test suite () {
      TestSuite suite = new TestSuite (SessionImplTest.class);
      return suite;
   }
   
   public void setUp () {
      session = SessionFactory.getFactory ().getSession ();
   }
   
   public void tearDown () {
      session = null;
   }
   
   /** Test of init & exit methods, of class com.sun.grid.drmaa.SessionImpl. */
   public void testInitExit () {
      System.out.println ("testInitExit");
      
      this.initSession ();
      this.exitSession ();
   }
   
   private void initSession () {
      try {
         session.init (null);
      }
      catch (DrmaaException e) {
         fail ("Unable to initialize session: " + e.getMessage ());
      }
   }

   private void exitSession () {
      try {
         session.exit ();
      }
      catch (DrmaaException e) {
         fail ("Unable to exit session: " + e.getMessage ());
      }
   }
   
   /** Test of getContact method, of class com.sun.grid.drmaa.SessionImpl. */
   public void testGetContact () {
      System.out.println ("testGetContact");
      
      assertEquals ("", session.getContact ());

      this.initSession ();
      
      assertEquals ("", session.getContact ());
      
      this.exitSession ();
   }
   
   /** Test of getDRMSystem method, of class com.sun.grid.drmaa.SessionImpl. */
   public void testGetDrmSystem () {
      System.out.println ("testGetDrmSystem");
      
      
      assertTrue (session.getDrmSystem ().equals ("SGE 6.0u2") ||
                  session.getDrmSystem ().equals ("N1GE 6.0u2"));

      this.initSession ();
      
      assertTrue (session.getDrmSystem ().equals ("SGE 6.0u2") ||
                  session.getDrmSystem ().equals ("N1GE 6.0u2"));
      
      this.exitSession ();
   }
   
   /** Test of getDRMAAImplementation method, of class com.sun.grid.drmaa.SessionImpl. */
   public void testGetDrmaaImplementation () {
      System.out.println ("testGetDrmaaImplementation");
      
      assertEquals ("DRMAA 1.0 Java Binding 0.5 -- SGE 6.0",
                    session.getDrmaaImplementation ());

      this.initSession ();
      
      assertEquals ("DRMAA 1.0 Java Binding 0.5 -- SGE 6.0",
                    session.getDrmaaImplementation ());
      
      this.exitSession ();
   }
   
   /** Test of getVersion method, of class com.sun.grid.drmaa.SessionImpl. */
   public void testGetVersion () {
      System.out.println ("testGetVersion");
      
      Version v = new Version (1, 0);
      
      this.initSession ();
      
      assertEquals (v, session.getVersion ());
      
      this.exitSession ();
   }
   
   /** Test of create|deleteJobTemplate method, of class com.sun.grid.drmaa.SessionImpl. */
   public void testJobTemplate () {
      System.out.println ("testJobTemplate");
      
      JobTemplate jt = null;
      
      this.initSession ();
      
      try {
         jt = session.createJobTemplate ();
      }
      catch (DrmaaException e) {
         fail ("Unable to create job template: " + e.getMessage ());
      }
      
      assertNotNull (jt);
      assertTrue (jt instanceof JobTemplateImpl);
      
      try {
         session.deleteJobTemplate (jt);
      }
      catch (InvalidJobTemplateException e) {
         fail ("Unable to delete job template: " + e.getMessage ());
      }
      catch (DrmaaException e) {
         fail ("Unable to create job template: " + e.getMessage ());
      }
      
      try {
         session.deleteJobTemplate (jt);
         fail ("Able to delete job template twice");
      }
      catch (InvalidJobTemplateException e) {
         /* Don't care */
      }
      catch (DrmaaException e) {
         fail ("Unable to delete job template: " + e.getMessage ());
      }
      
      this.exitSession ();
   }
}
