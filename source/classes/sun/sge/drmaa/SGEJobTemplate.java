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
/*
 * SGEJobTemplate.java
 *
 * Created on March 4, 2004, 10:09 AM
 */

package sun.sge.drmaa;

import java.util.*;

import com.sun.grid.drmaa.*;

/**
 *
 * @author  dan.templeton@sun.com
 */
public class SGEJobTemplate extends JobTemplate {
   private SGESession session = null;
   private int id = -1;
   
   /** Creates a new instance of SGEJobTemplate */
   SGEJobTemplate (SGESession session, int id) {
      this.session = session;
      this.id = id;
   }
   
   public List getAttribute (String name) {
      String[] values = session.nativeGetAttribute (name);
      
      return Arrays.asList (values);
   }
   
   public Set getAttributeNames () {
      String[] names = session.nativeGetAttributeNames ();
      
      return new HashSet (Arrays.asList (names));
   }
   
   public void setAttribute (String name, List value) throws DRMAAException {
      session.nativeSetAttributeValues (name, (String[])value.toArray (new String[value.size ()]));
   }
   
   public void setAttribute (String name, String value) throws DRMAAException {
      session.nativeSetAttributeValue (name, value);
   }   
   
   public void delete () throws DRMAAException {
      session.nativeDeleteJobTemplate (this);
   }
   
   int getId () {
      return id;
   }
}
