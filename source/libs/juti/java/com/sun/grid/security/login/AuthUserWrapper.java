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

import com.sun.grid.util.expect.Expect;
import com.sun.grid.util.expect.ExpectBuffer;
import com.sun.grid.util.expect.ExpectHandler;
import com.sun.grid.util.expect.ExpectPasswordHandler;
import com.sun.grid.util.expect.ExpectStringHandler;
import com.sun.security.auth.UnixNumericGroupPrincipal;
import com.sun.security.auth.UnixNumericUserPrincipal;
import com.sun.security.auth.UnixPrincipal;
import java.io.IOException;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.StringTokenizer;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.security.auth.login.LoginException;

/**
 * This class used by the <code>UnixLoginModule</code> to execute the authuser binary
 *
 * @author richard.hierlmeier@sun.com
 */
class AuthUserWrapper {
    
    private static final Logger LOGGER = Logger.getLogger(AuthUserWrapper.class.getName());
        
    private String [] command;
    
    private AuthUserWrapper(String [] command) {
        this.command = command;
    }
    
    /**
     * Create a new instance of <code>AuthUserWrapper</code> which uses
     * the PAM authentication system.
     * 
     * @param authuser    path the the authuser binary
     * @param pamServiceName  name of the user pam service
     * @return the <code>AuthUserWrapper</code>.
     */
    public static AuthUserWrapper newInstanceForPam(String authuser, String pamServiceName) {
        return new AuthUserWrapper( new String [] {
             authuser, "pam", "-s", pamServiceName
        });
    }
    
    /**
     * Create a new instance of <code>AuthUserWrapper</code> which uses
     * the shadow authentication system.
     * 
     * @param authuser    path the the authuser binary
     * @return the <code>AuthUserWrapper</code>.
     */
    public static AuthUserWrapper newInstanceForShadow(String authuser) {
        return new AuthUserWrapper( new String [] {
             authuser, "shadow"
        });
    }

    /**
     * Authenticate a user
     *
     * @param username unix username
     * @param password the password
     * @throws javax.security.auth.login.LoginException <ul>
     *     <li>if the authuser binary reports and error (authuser exited with status 2)</li>
     *     <li>if the authuser can not be started</li>
     *     <li>if the authuser has been interrupted</li>
     * </ul>
     * @return <ul>
     *    <li><code>null</code> if <code>username</code> of <code>password</code> is invalid.
     *        (authuser exited with status 1)
     *    </li>
     *    <li> else a <code>Set</code> containing <ul>
     *          <li> a {@link com.sun.security.auth.UnixPrincipal} of the authenticated user</li>
     *          <li> a {@link com.sun.security.auth.UnixNumericUserPrincipal} with the user id
     *              of the authenticated user</li>
     *          <li> a {@link  com.sun.security.auth.UnixNumericGroupPrincipal} for each group the authenticated
     *               user belongs too</li>
     *         </ul></li>
     *  </ul>
     */
    public Set authenticate(final String username, final char[] password) throws LoginException {
        
        Set ret = null;
        
        Expect expect = new Expect(command);

        expect.add(new ExpectStringHandler("username: ", username.toCharArray()));
        expect.add(new ExpectPasswordHandler("password: ", password));
        
        PrincipalHandler principalHandler = new PrincipalHandler();        
        expect.add(principalHandler);
        
        ErrorHandler errorHandler = new ErrorHandler();
        expect.add(errorHandler);
        
        try {            
            int exitCode = expect.exec(60*1000);
            
            // exit codes are defined in juti.h (see auth_result_t)
            // 0 means success, 1 means invalid username of password,
            // 2 means error
            switch(exitCode) {
                case 0:  // success
                    return principalHandler.getPrinicipals();
                case 1: // authentication failed  
                    return null;
                default:
                    if(errorHandler.getError() == null) {
                        throw new LoginException("authuser command failed (" + exitCode + ")");
                    } else {
                        throw new LoginException("authuser command failed: " + errorHandler.getError());
                    }
            }
        } catch (InterruptedException ex) {
            throw new LoginException("login has been interrupted");
        } catch (IOException ex) {
            LoginException le = new LoginException("io error");
            le.initCause(ex);
            throw le;
        }
        
    }
    
    /**
     *  Handles error message of the authuser
     */
    class ErrorHandler implements ExpectHandler {
        
        String error;
        public void handle(Expect expect, ExpectBuffer buffer) throws IOException {
            String msg = buffer.consumeLine("error: ");
            if(msg != null) {
                error = msg.trim();
            }
        }
        
        public String getError() {
            return error;
        }
    }
    
    /**
     *  Handles the uid and gid output of the authuser
     */
    class PrincipalHandler implements ExpectHandler {

        private Set principals = new HashSet();
        
        public void handle(Expect expect, ExpectBuffer buffer) throws IOException {

            String line = buffer.consumeLine("uid ");
            if(line != null) {
                
                line = line.trim();
                
                UnixNumericUserPrincipal p = new UnixNumericUserPrincipal(line);
                principals.add(p);
            }
            
            line = buffer.consumeLine("gid ");
            if(line != null) {
                StringTokenizer st = new StringTokenizer(line.trim(), ",");
                boolean primaryGroup = true;
                while(st.hasMoreTokens()) {
                    UnixNumericGroupPrincipal p = new UnixNumericGroupPrincipal(st.nextToken(), primaryGroup);
                    principals.add(p);
                    primaryGroup = false;
                }
            }
        }
        
        public Set getPrinicipals() {
            return principals;
        }
    }    
    
}
