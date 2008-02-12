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

import javax.net.ssl.KeyManager;


import com.sun.grid.security.login.GECATrustManager;
import com.sun.grid.security.login.GECAKeyManager;
import java.io.File;
import java.security.KeyStore;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocketFactory;
import javax.net.ssl.TrustManager;

/**
 * Helper class for SSL setup.
 */
public final class SSLHelper {

    private final static String SSL_PROTOCOL = "SSL";

    private static SSLContext ctx;
    private static final GECAKeyManager keyManager = new GECAKeyManager();
    private static final GECATrustManager trustManager = new GECATrustManager();
    private static final Lock lock = new ReentrantLock();
    
    private SSLHelper () {
    }
    
    private static void initSSLContext() {
        lock.lock();
        try {
            if(ctx == null) {
                try {
                    ctx = SSLContext.getInstance(SSL_PROTOCOL);
                    ctx.init(new KeyManager[]{keyManager},
                            new TrustManager[]{trustManager},
                            null);
                } catch (Exception ex) {
                    throw new SecurityException("Cannot create SSLContext", ex);
                }
            }
        } finally {
            lock.unlock();
        }
    }
    
    /**
     * Set ca top for the SSL context (is used to find a ca certificate.
     * @param catop  the ca top directory
     */
    static void setCaTop(File catop) {
        lock.lock();
        try {
            initSSLContext();
            trustManager.setCaTop(catop);
        } finally {
            lock.unlock();
        }
    }
    
    /**
     * Initialize the SSL setup
     * @param caTop  the ca top directory
     * @param ks     the keystore
     * @param pw     the password for the keystore
     */
    static void init(File caTop, KeyStore ks, char [] pw) {
        lock.lock();
        try {
            setCaTop(caTop);
            setKeystore(ks, pw);
        } finally {
            lock.unlock();
        }
    }
    
    /**
     * Initialize the SSL setup
     * @param caTop  the ca top directory
     * @param ks     the keystore file
     * @param pw     the password for the keystore
     */
    static void init(File caTop, File ks, char [] pw) {
        lock.lock();
        try {
            setCaTop(caTop);
            setKeystore(ks, pw);
        } finally {
            lock.unlock();
        }
    }
    
    /**
     * Set the keystore for the JGDI ssl context
     * @param ks   the keystore
     * @param pw   the password for the keystore
     */
    static void setKeystore(KeyStore ks, char [] pw) {
        lock.lock();
        try {
            initSSLContext();
            keyManager.setKeystore(ks, pw);
        } finally {
            lock.unlock();
        }
    }
    
    /**
     * Set the keystore for the JGDI ssl context
     * @param keystore   the keystore file
     * @param pw   the password for the keystore
     */
    static void setKeystore(File keystore, char [] pw) {
        lock.lock();
        try {
            initSSLContext();
            keyManager.setKeystore(keystore, pw);
        } finally {
            lock.unlock();
        }
    }
    
    /**
     * Reset the JGDI ssl context
     */
    static void reset() {
        lock.lock();
        try {
            ctx = null;
            keyManager.reset();
            trustManager.setCaTop(null);
        } finally {
            lock.unlock();
        }
    }
            
    
    /**
     *  Get the ssl socket factory for the application
     *  @return the socket factor for the application
     */
    static SSLSocketFactory getSocketFactory() {
        lock.lock();
        try {
            if(ctx == null) {
                return (SSLSocketFactory)SSLSocketFactory.getDefault();
            } else {
                return ctx.getSocketFactory();
            }
        } finally {
            lock.unlock();
        }
    }
}
