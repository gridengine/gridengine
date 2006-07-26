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
package com.sun.grid.security.login;

import java.io.IOException;
import java.util.Map;
import java.util.Set;
import java.util.logging.Logger;
import javax.security.auth.Subject;
import javax.security.auth.callback.Callback;
import javax.security.auth.callback.CallbackHandler;
import javax.security.auth.callback.NameCallback;
import javax.security.auth.callback.PasswordCallback;
import javax.security.auth.callback.UnsupportedCallbackException;
import javax.security.auth.login.LoginException;
import javax.security.auth.spi.LoginModule;

/**
 * This <code>LoginModule</code> authenticates a unix user with username
 * and password against the PAM or shadow authentication system.
 * 
 * The username is queried with a <code>NameCallback</code>, the password with 
 * a <code>PasswordCallback</code>
 * 
 * <p>After a successfull login this <code>LoginModule</code> adds
 * 
 * <ul>
 *    <li> a {@link com.sun.security.auth.UnixPrincipal} of the authenticated user</li>
 *    <li> a {@link com.sun.security.auth.UnixNumericUserPrincipal} with the user id
 *         of the authenticated user</li>
 *    <li> a {@link  com.sun.security.auth.UnixNumericGroupPrincipal} for each group the authenticated
 *         user belongs too</li>
 * </ul>
 * 
 * to the current subject.</p>
 * 
 * <p>This class uses a {@link java.util.logging.Logger} for log messages. The name of the <code>Logger</code>
 *    is equal to the fullqualified classname of this class.</p>
 * 
 * <H3>Options for UnixLoginModule</H3>
 * 
 * <table>
 *   <tr><th>Option</th><th>description</th></tr>
 *   <tr>
 *      <td>command</td>
 *      <td>path to the authuser binary (SGE_ROOT/utilbin/<arch>/authuser)</td>
 *   </tr>
 *   <tr>
 *      <td>auth_method</td>
 *      <td>Autehtication method. Valid values are "pam" and "shadow"</td>
 *   </tr>
 *   <tr>
 *      <td>pam_service</td>
 *      <td>Name of the pam service (see man pam(5). Required for
 *          PAM authentifcation</td>
 *   </tr>
 * </table>
 * 
 * <H3>Simple jaas config file for PAM authentication</H3>
 * 
 * <pre>
 *  sample {
 *   com.sun.grid.security.login.UnixLoginModule requisite
 *         command="/opt/sge/utilbin/sol-sparc64/authuser",
 *         auth_method="pam";
 *         pam_service="su";
 *  };
 * </pre>
 * 
 * <H3>Simple jaas config file for shadow authentication</H3>
 * 
 * <pre>
 *  sample {
 *   com.sun.grid.security.login.UnixLoginModule requisite
 *         command="/opt/sge/utilbin/sol-sparc64/authuser",
 *         auth_method="shadow";
 *  };
 * </pre>
 * 
 * @author richard.hierlmeier@sun.com
 * @see javax.security.auth.callback.NameCallback;
 * @see javax.security.auth.callback.PasswordCallback;
 */
public class UnixLoginModule implements LoginModule {
    
    private final static Logger LOGGER = Logger.getLogger(UnixLoginModule.class.getName());
    
    private String authMethod;
    private String pamService;
    private String command;
    
    private Subject subject;
    private boolean loginSucceded;
    private boolean commitSucceded;
    private CallbackHandler callbackHandler;
    private String username;
    private Set principals;
    private AuthUserWrapper authuser;

    
    /**
     * Initialize the <code>UnixLoginModule</code>
     * @param subject   the current subject
     * @param callbackHandler the callbackhandler (must at least handle a 
     *                        <code>NameCallback</code> and a 
     *                        </code>PasswordCallback</code>).
     * @param sharedState not used
     * @param options contains the options for the <code>UnixLoginModule</code>.
     */
    public void initialize(Subject subject, CallbackHandler callbackHandler, Map sharedState, Map options) {
       
       command = (String)options.get("command");
       authMethod = (String)options.get("auth_method");
       pamService = (String)options.get("pam_service");
       this.subject = subject;
       this.callbackHandler = callbackHandler;
       
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
        
        PasswordCallback pwCallback = new PasswordCallback("password: ", false);
        NameCallback nameCallback = new NameCallback("username: ");
        
        try {
            callbackHandler.handle( new Callback[] { nameCallback, pwCallback });
        } catch (IOException ex) {
            LoginException le =  new LoginException("io error in callbackhandler");
            le.initCause(ex);
            throw le;
        } catch (UnsupportedCallbackException ex) {
            LoginException le =  new LoginException("invalid callbackhandler");
            le.initCause(ex);
            throw le;
        }
        
        AuthUserWrapper authuser = null;
        if(authMethod == null) {
            throw new LoginException("missing autMethod option");
        } else if("pam".equals(authMethod)) {
            authuser = AuthUserWrapper.newInstanceForPam(command, pamService);
        } else if ("shadow".equals(authMethod)) {
            authuser = AuthUserWrapper.newInstanceForShadow(command);
        } else {
            throw new LoginException("unknown authMethod '" + authMethod + "'");
        }

        principals = authuser.authenticate(nameCallback.getName(), pwCallback.getPassword());
        if(principals != null) {
            loginSucceded = true;
        }
        return loginSucceded;
    }

    /**
     * Commit the login (adds the principals to the subject)
     *
     * @return <code>true</code> of the principals has been added to the subject.
     */
    public boolean commit() {
        if(loginSucceded) {
           subject.getPrincipals().addAll(principals);
           commitSucceded = true; 
        }
        return commitSucceded;
    }

    /**
     * Abort the login.
     * @return Always <code>true</code>
     */
    public boolean abort() {
        logout();
        return true;
    }

    /**
     * Removes all previously added prinicipals from the subject.
     *
     * @return Always <code>true</code>
     */
    public boolean logout() {
        if(commitSucceded) {
            subject.getPrincipals().removeAll(principals);
        }
        subject = null;
        principals = null;
        username = null;
        callbackHandler = null;
        loginSucceded = false;
        commitSucceded = false;
        return true;
    }
    
}
