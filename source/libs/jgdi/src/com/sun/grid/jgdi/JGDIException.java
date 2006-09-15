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
package com.sun.grid.jgdi;

import java.util.MissingResourceException;
import java.util.ResourceBundle;

/**
 * Base exception class for all errors on the GDI layer.
 * @author richard.hierlmeier@sun.com
 */
public class JGDIException extends java.rmi.RemoteException {
   
   private Object[] params;
   //private static ResourceBundle RB = ResourceBundle.getBundle("com.sun.grid.jgdi.Resources");
   
   
   /** Creates a new instance of JGDIException */
   public JGDIException() {
   }
   
   public JGDIException(String msg) {
      super(msg);
   }
   
   public JGDIException(String msg, Object[] params) {
      super(msg);
      this.params = params;
   }
   
   public JGDIException(String msg, Object param) {
      this(msg, new Object[] { param } );
   }
   
   public JGDIException(String msg, Object param1, Object param2 ) {
      this(msg, new Object[] { param1, param2 } );
      
   }
   
   public JGDIException(String msg, Object param1, Object param2, Object param3 ) {
      this(msg, new Object[] { param1, param2, param3 } );
   }

   
//   public String getLocalizedMessage() {
//
//      try {
//         String ret = RB.getString(getMessage());
//
//         if( ret != null && params != null ) {
//            ret = java.text.MessageFormat.format(ret,params);
//         }
//         return ret;
//      } catch( MissingResourceException mre ) {
//         return getMessage();
//      }
//   }

   
}
