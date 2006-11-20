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

/**
 * Factory class for {@link JGDI} objects.
 *
 * @author richard.hierlmeier@sun.com
 * @see com.sun.grid.jgdi.JGDI
 */
public class JGDIFactory {

    private static String versionString;
    
    private static boolean libNotLoaded = true;
    
    private static synchronized void initLib() throws JGDIException {
        if(libNotLoaded) {
            try {
                System.loadLibrary("jgdi");
                libNotLoaded = false;
            } catch (Throwable e) {
                JGDIException ex = new JGDIException("Can not load native jgdi lib: " + e.getMessage());
                // ex.initCause(e);  does not work for whatever reason
                throw ex;
            }
        }
    }
   /**
    * Get a new instance of a <code>JGDI</code> object. 
    *
    * @param url  JGDI connection url in the form
    *             <code>bootstrap://&lt;SGE_ROOT&gt;@&lt;SGE_CELL&gt;:&lt;SGE_QMASTER_PORT&gt;</code>
    * @throws com.sun.grid.jgdi.JGDIException on any error on the GDI layer
    * @return the <code>JGDI<code> instance
    */
   public static JGDI newInstance(String url) throws JGDIException { 
      initLib();
      com.sun.grid.jgdi.jni.JGDIImpl ret = new com.sun.grid.jgdi.jni.JGDIImpl();
      ret.init(url);
      
      return ret;
   }
   
   /**
    * Get a synchronized instance of a <code>JGDI</code> object. 
    *
    * @param url  JGDI connection url in the form
    *             <code>bootstrap://&lt;SGE_ROOT&gt;@&lt;SGE_CELL&gt;:&lt;SGE_QMASTER_PORT&gt;</code>
    * @throws com.sun.grid.jgdi.JGDIException on any error on the GDI layer
    * @return the <code>JGDI<code> instance
    * @since  0.91
    */
   public static JGDI newSynchronizedInstance(String url)  throws JGDIException {
       return new com.sun.grid.jgdi.jni.SynchronizedJGDI(newInstance(url));
   }
   
   
   /**
    * Create a new event client which receives events from a jgdi
    * connection.
    *
    * @param jgdi   the jgdi connection
    * @param evcId  id of the event client (0 mean dynamically assigned)
    * @throws com.sun.grid.jgdi.JGDIException 
    * @return the new event client
    */
   public static EventClient createEventClient(JGDI jgdi, int evcId) throws JGDIException {
      
      return new com.sun.grid.jgdi.jni.EventClientImpl(jgdi, evcId);
      
   }
   
   public static String getJGDIVersion() throws JGDIException {
       initLib();
       if (versionString == null) {
          versionString = setJGDIVersion(); 
       }
       return versionString;
   }
   
   private native static String setJGDIVersion();
   
}
