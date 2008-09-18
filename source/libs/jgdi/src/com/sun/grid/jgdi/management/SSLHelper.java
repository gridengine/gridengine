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
 *   Copyright: 2006 by Sun Microsystems, Inc
 *
 *   All Rights Reserved.
 *
 ************************************************************************/
/*___INFO__MARK_END__*/
package com.sun.grid.jgdi.management;

import java.util.logging.Logger;
import javax.net.ssl.KeyManager;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocketFactory;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509KeyManager;
import javax.net.ssl.X509TrustManager;


import com.sun.grid.security.login.GECATrustManager;
import com.sun.grid.security.login.GECAKeyManager;
import java.io.File;

/**
 * Helper class for SSL setup.
 *
 */
public final class SSLHelper {

    private static final Logger log = Logger.getLogger(SSLHelper.class.getName());
    private final static String SSL_PROTOCOL = "SSL";
    private static SSLContext ctx = null;

    private SSLHelper() {
    }
        
    static void init(File caTop, File serverKeystore, char[] serverKeystorePassword) {    

        X509TrustManager trustManager = new GECATrustManager(caTop);
        X509KeyManager keyManager = new GECAKeyManager(serverKeystore, serverKeystorePassword);

        try {
            ctx = SSLContext.getInstance(SSL_PROTOCOL);
            ctx.init(new KeyManager[]{keyManager},
                    new TrustManager[]{trustManager},
                    null);
        } catch (Exception ex) {
            throw new SecurityException("Cannot create SSLContext", ex);
        }
        
        // SSLContext.setDefault(getSSLContext());
    }

    /**
     *  Get the ssl context for the application.
     *
     *  @return the ssl context of <code>null</code>
     */
    static SSLContext getSSLContext() {
        return ctx;
    }

    /**
     *  Get the ssl socket factory for the application
     *  @return the socket factor for the application
     */
    static SSLSocketFactory getSocketFactory() {
        return (SSLSocketFactory) ctx.getSocketFactory();
    }
}
