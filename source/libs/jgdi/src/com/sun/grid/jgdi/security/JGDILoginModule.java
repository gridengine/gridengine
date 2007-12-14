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
package com.sun.grid.jgdi.security;

import java.security.Principal;
import java.util.Map;
import java.util.concurrent.atomic.AtomicLong;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.security.auth.Subject;
import javax.security.auth.callback.CallbackHandler;
import javax.security.auth.login.LoginException;
import javax.security.auth.spi.LoginModule;

/**
 */
public class JGDILoginModule implements LoginModule {

    private final static Logger log = Logger.getLogger(JGDILoginModule.class.getName());
    private Subject subject;
    private boolean commitSucceded;
    private String trustedPrincipalClass;
    private JGDIPrincipal principal;
    
    private final static AtomicLong sessionId = new AtomicLong();
    /**
     * Initialize the <code>JGDILoginModule</code>
     * @param subject   the current subject
     * @param callbackHandler the callbackhandler (must at least handle a 
     *                        <code>NameCallback</code> and a 
     *                        </code>PasswordCallback</code>).
     * @param sharedState not used
     * @param options contains the options for the <code>JGDILoginModule</code>.
     */
    public void initialize(Subject subject, CallbackHandler callbackHandler, Map sharedState, Map options) {
        log.entering("JGDILoginModule", "initialize");

        this.subject = subject;
        trustedPrincipalClass = (String) options.get("trustedPrincipal");
        log.log(Level.FINE, "trustedPrincipalClass: {0}", trustedPrincipalClass);
        
        log.exiting("JGDILoginModule", "initialize");
    }

    /**
     * Perform the login.
     *
     * @throws javax.security.auth.login.LoginException <ul>
     *      <li>if the callbackhandler reports an error</li>
     *      <li>if some options are missing (please check the jass.config file)</li>
     *      <li>if the underlying authentication system report an error</li>
     *    </ul>
     * @return <code>true</code> on successfull authentication. <code>false</code>
     *         if username of password is invalid.
     */
    public boolean login() throws LoginException {
        log.entering("JGDILoginModule", "login");
        log.exiting("JGDILoginModule", "login", true);
        return true;
    }

    /**
     * Commit the login (adds the principals to the subject)
     *
     * @return <code>true</code> of the principals has been added to the subject.
     */
    public boolean commit() {
        log.entering("JGDILoginModule", "commit");

        if (trustedPrincipalClass != null) {
            String trustedName = null;
            for (Principal p : subject.getPrincipals()) {
                if (p.getClass().getName().equals(trustedPrincipalClass)) {
                    trustedName = p.getName();
                    break;
                }
            }

            if (trustedName != null) {
                long sid =  sessionId.incrementAndGet();
                principal = new JGDIPrincipal(trustedName, sid);
                log.log(Level.FINE, "Adding JGDIPrincipal {0} to subject", principal);
                subject.getPrincipals().add(principal);
                commitSucceded = true;
            } else {
                commitSucceded = false;
            }
        } else {
            commitSucceded = false;
        }

        log.exiting("JGDILoginModule", "commit", commitSucceded);
        return commitSucceded;
    }

    /**
     * Abort the login.
     * @return Always <code>true</code>
     */
    public boolean abort() {
        log.entering("JGDILoginModule", "abort");
        logout();
        log.exiting("JGDILoginModule", "abort", true);
        return true;
    }

    /**
     * Removes all previously added prinicipals from the subject.
     *
     * @return Always <code>true</code>
     */
    public boolean logout() {
        log.entering("JGDILoginModule", "logout");
        if (commitSucceded) {
            subject.getPrincipals().remove(principal);
        }
        subject = null;
        principal = null;
        commitSucceded = false;
        log.exiting("JGDILoginModule", "logout", true);
        return true;
    }
}
