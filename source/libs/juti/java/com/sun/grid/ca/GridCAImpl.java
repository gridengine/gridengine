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
package com.sun.grid.ca;

import com.sun.grid.util.expect.Expect;
import com.sun.grid.util.expect.ExpectBuffer;
import com.sun.grid.util.expect.ExpectHandler;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.io.Writer;
import java.security.Key;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.UnrecoverableKeyException;
import java.security.cert.Certificate;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;
import java.util.Enumeration;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Default implementation of the <code>GridCA</ca>
 *
 * Uses the sge_ca script which is delivered with gridengine to perform
 * actions on the gridengine ca.
 *
 * @author  richard.hierlmeier@sun.com
 */
public class GridCAImpl implements GridCA {
    
    private static final Logger LOGGER = Logger.getLogger(GridCAImpl.class.getName());
    
    private GridCAConfiguration config;
    
    /**
     * Create a new instance of <code>GridCAImp</code>
     * @param config the configuration
     * @throws com.sun.grid.ca.GridCAException if the configuration is not valid
     */
    public GridCAImpl(GridCAConfiguration config) throws GridCAException {
        config.validate();
        this.config = config;
    }
  
    protected Expect createProcess() {
        Expect ret = new Expect();
        ret.command().add(config.getSgeCaScript().getAbsolutePath());
        ret.command().add("-catop");
        ret.command().add(config.getCaTop().getAbsolutePath());
        ret.command().add("-calocaltop");
        ret.command().add(config.getCaLocalTop().getAbsolutePath());
        ret.command().add("-cahost");
        ret.command().add(config.getCaHost());
        ret.command().add("-adminuser");
        ret.command().add(config.getAdminUser());
        return ret;
    }
    
    /**
     * Initialize the gridengine ca.
     *
     * @param countryCode       the two letter country code
     * @param state             the state
     * @param location          the location, e.g city or your buildingcode
     * @param organization      the organization (e.g. your company name)
     * @param organizationUnit  the organizational unit, e.g. your department
     * @param adminMailAddress  the email address of the CA administrator (you!)
     * @throws com.sun.grid.ca.GridCAException 
     */
    public void init(String countryCode,
                     String state,
                     String location,
                     String organization,
                     String organizationUnit,
                     String adminMailAddress) throws GridCAException {
        LOGGER.entering("GridCAImpl", "init");
        
        File autoFile = null;
        try {
            try {
                autoFile = File.createTempFile("auto", ".conf", config.getTmpDir());
                autoFile = new File(config.getTmpDir(), "auto.conf");
            } catch(IOException ioe) {
                GridCAException cae = new GridCAException("Can not create file in " + config.getTmpDir(), ioe);
                LOGGER.throwing( "GridCAImpl" ,"init", cae);
                throw cae;
            }
            
            try {
                PrintWriter pw = new PrintWriter(new FileWriter(autoFile));

                pw.print("CSP_COUNTRY_CODE=");
                pw.println(countryCode);
                pw.print("CSP_STATE=");
                pw.println(state);
                pw.print("CSP_LOCATION=");
                pw.println(location);
                pw.print("CSP_ORGA=");
                pw.println(organization);
                pw.print("CSP_ORGA_UNIT=");
                pw.println(organizationUnit);
                pw.print("CSP_MAIL_ADDRESS=");
                pw.println(adminMailAddress);
                pw.close();
                
                LOGGER.log(Level.FINE, "auto file successfully written");
            } catch(IOException ioe) {
                GridCAException cae = new GridCAException("Can not write auto.conf file", ioe);
                LOGGER.throwing( "GridCAImpl" ,"init", cae);
                throw cae;
            }
            
            Expect pb = createProcess();
            pb.command().add("-auto");
            pb.command().add(autoFile.getAbsolutePath());
            pb.command().add("-init");
            
            execute(pb);
            
            LOGGER.log(Level.FINE, "init command successfully executed");

        } finally {
//            if(autoFile != null) {
//                autoFile.delete();
//            }
        }
    }
    
    protected File getLocalUserDir(String username) {
        return new File(config.getCaLocalTop(), "userkeys" + File.separatorChar + username);
    }
    
    protected File getCertFileForUser(String username) {
        return new File(getLocalUserDir(username), "cert.pem");
    }

    protected File getLocalDaemonDir(String daemon) {
        return new File(config.getCaLocalTop(), "daemons" + File.separatorChar + daemon);
    }
    
    protected File getCertFileForDaemon(String daemon) {
        return new File(getLocalDaemonDir(daemon), "cert.pem");
    }
    
    
    protected void execute(Expect pb) throws GridCAException {
        LOGGER.entering("GridCAImpl", "execute");

        ErrorHandler eh = new ErrorHandler();
        pb.add(eh);
        int res;
        try {
            res = pb.exec(60 * 1000);
        } catch (IOException ex) {
            throw new GridCAException("IO error while execution sge_ca", ex);
        } catch (InterruptedException ex) {
            throw new GridCAException("Execution sge_ca has been interrupted", ex);
        }
        if(res != 0) {
            String error = eh.getError();
            if(error == null || error.length() == 0) {
                throw new GridCAException("sge_ca script with status " + res + ", no error message was reported");
            } else {
                throw new GridCAException(error);
            }
        }
        LOGGER.exiting("GridCAImpl", "execute");
    }
    
    class ErrorHandler implements ExpectHandler {
        
        private String error;
        
        public void handle(Expect expect, ExpectBuffer buffer) throws IOException {
            if(getError() == null) {
                error = buffer.consumeLine("Error:");
            }
        }

        public String getError() {
            return error;
        }
    }
    
    /**
     *  Create private key and certificate for a user.
     *
     *  @param username  name of the user
     *  @param group     group of the user
     *  @param email     email address of the user
     *  @throws GridCAException if the creation of the private key or the certificate fails
     */
    public void createUser(String username, String group, String email) throws GridCAException {
        LOGGER.entering("GridCAImpl", "createUser");
        Expect pb = createProcess();
        pb.command().add("-user");
        pb.command().add(username + ":" + group + ":" + email);

        execute(pb);
        LOGGER.exiting("GridCAImpl", "createUser");
    }
    
    /**
     * Create private key and certificate for a grm daemon.
     *
     * @param daemon name of the daemon
     * @param user   username of the daemon (owner of the process)
     * @param email  email address of the process owner
     * @throws com.sun.grid.ca.GridCAException if the create of the daemon failed
     */
    public void createDaemon(String daemon, String user, String email) throws GridCAException {
        LOGGER.entering("GridCAImpl", "createDaemon");
        Expect pb = createProcess();
        pb.command().add("-grm_daemon");
        pb.command().add(daemon + ":" + user + ":" + email);

        execute(pb);
        
        LOGGER.exiting("GridCAImpl", "createDaemon");
    }
    
    
    
    
    /**
     *  Get the X.509 certificate of a user.
     *
     *  @param username  name of the user
     *  @return X.509 certificate
     *  @throws GridCAException if the certificate does not exist
     */
    public X509Certificate getCertificate(String username) throws GridCAException {
        LOGGER.entering("GridCAImpl", "getCertificate");
        try {
            File certFile = getCertFileForUser(username);
            
            if(!certFile.exists()) {
                throw new GridCAException("cert file for user not found in ca");
            }
            if(!certFile.canRead()) {
                throw new GridCAException("cert file for user is not accessible");
            }
            
            FileInputStream in = new FileInputStream(certFile);
            try {
                LOGGER.exiting("GridCAImpl", "getCertificate");
                return (X509Certificate)CertificateFactory.getInstance("X.509").generateCertificate(in);
            } finally {
                in.close();
            }
        } catch(IOException ioe) {
            throw new GridCAException("I/O Error while reading cert file", ioe);
        } catch(CertificateException ex) {
            throw new GridCAException("cert file of user " + username + " does not contain a valid certificate", ex);
        }
    }
    
    /**
     *  Get the X.509 certificate of a daemon.
     *
     *  @param daemon  name of the daemon
     *  @return X.509 certificate
     *  @throws GridCAException if the certificate does not exist
     */
    public X509Certificate getDaemonCertificate(String daemon) throws GridCAException {
        LOGGER.entering("GridCAImpl", "getDaemonCertificate");
        try {
            File certFile = getCertFileForDaemon(daemon);
            
            if(!certFile.exists()) {
                throw new GridCAException("cert file for daemon not found in ca");
            }
            if(!certFile.canRead()) {
                throw new GridCAException("cert file for daemon is not accessible");
            }
            
            FileInputStream in = new FileInputStream(certFile);
            try {
                LOGGER.exiting("GridCAImpl", "getDaemonCertificate");
                return (X509Certificate)CertificateFactory.getInstance("X.509").generateCertificate(in);
            } finally {
                in.close();
            }
        } catch(IOException ioe) {
            throw new GridCAException("I/O Error while reading cert file", ioe);
        } catch(CertificateException ex) {
            throw new GridCAException("cert file of daemon " + daemon + " does not contain a valid certificate", ex);
        }
        
    }
    
    
    /**
     *  Renew the certificate of a user.
     *
     *  @param username  name of the user
     *  @param days      validity of the new certificate in days
     *  @return the renewed certificate
     *  @throws CAException if the certificate can not be renewed
     */
    public X509Certificate renewCertificate(String username, int days) throws GridCAException {
        LOGGER.entering("GridCAImpl", "renewCertificate");
        
        Expect pb = createProcess();
        pb.command().add("-renew");
        pb.command().add(username);
        pb.command().add("-days");
        pb.command().add(Integer.toString(days));

        execute(pb);
        
        X509Certificate ret = getCertificate(username);
        LOGGER.exiting("GridCAImpl", "renewCertificate");
        return ret;
    }
    
    /**
     *  Renew the certificate of a daemon.
     *
     *  @param daemon  name of the daemon
     *  @param days      validity of the new certificate in days
     *  @return the renewed certificate
     *  @throws CAException if the certificate can not be renewed
     */
    public X509Certificate renewDaemonCertificate(String daemon, int days) throws GridCAException {
        LOGGER.entering("GridCAImpl", "renewDaemonCertificate");
        Expect pb = createProcess();
        pb.command().add("-renew_grm");
        pb.command().add(daemon);
        pb.command().add("-days");
        pb.command().add(Integer.toString(days));

        execute(pb);

        X509Certificate ret = getDaemonCertificate(daemon);
        LOGGER.exiting("GridCAImpl", "renewDaemonCertificate");
        return ret;
    }
    
    
    private File createTempFile(String prefix, String suffix) throws GridCAException {
        try {
            return File.createTempFile(prefix, suffix, config.getTmpDir());
        } catch(IOException ioe) {
            throw new GridCAException("Can not create tmp file in directory " + config.getTmpDir(), ioe);
        }
    }
    
    /**
     *  Create a keystore which contains the private key and
     *  certificate of an user.
     *
     *  @param  username         name of the user
     *  @param  keystorePassword password used for encrypt the keystore
     *  @throws GridCAException if the keystore could not be created
     */
    public KeyStore createKeyStore(String username, char[] keystorePassword, char[] privateKeyPassword) throws GridCAException {
         return createKeyStore("user", username, keystorePassword, privateKeyPassword);
    }
    
    /**
     * Get the keystore for a daemon.
     *
     * This method can be used be the installation to create keystore for
     * the daemon of a grm system.
     *
     * @param daemon name of the daemon
     * @throws com.sun.grid.ca.GridCAException 
     * @return the keystore of the daemon
     */
    public KeyStore createDaemonKeyStore(String daemon) throws GridCAException {
         return createKeyStore("grm_daemon", daemon, new char[0], new char[0]);
    }
    
        
    private KeyStore createKeyStore(String type, String username, char[] keystorePassword, char[] privateKeyPassword) throws GridCAException {
        LOGGER.entering("GridCAImpl", "createKeyStore");
        
        if(keystorePassword == null) {
            throw new GridCAException("need a keystore password");
        }
        if(privateKeyPassword == null) {
            throw new GridCAException("need a private key password");
        }
        
        KeyStore ks;
        try {
            ks = KeyStore.getInstance("JKS");
        } catch (KeyStoreException ex) {
            throw new GridCAException("Can not create JKS keystore", ex);
        }
        
        char [] pkcs12PW = "changeit".toCharArray();
        try {
            ks.load(null, keystorePassword);
        } catch (NoSuchAlgorithmException ex) {
            throw new GridCAException("Can not load pkcs21 keystore" ,ex);
        } catch (CertificateException ex) {
            throw new GridCAException("Can not load pkcs21 keystore" ,ex);
        } catch (IOException ex) {
            throw new GridCAException("Can not load pkcs21 keystore" ,ex);
        }

        KeyStore pkcs12Ks = createPKCS12KeyStore(type, username, pkcs12PW);
        Enumeration aliases;
        try {
            aliases = pkcs12Ks.aliases();
        } catch (KeyStoreException ex) {
            throw new GridCAException("Can not get alias from pkcs12 keystore", ex);
        }

        while(aliases.hasMoreElements()) {
            String alias = (String)aliases.nextElement();

            boolean isKeyEntry = false;
            try {
                isKeyEntry = pkcs12Ks.isKeyEntry(alias);
            } catch (KeyStoreException ex) {
                throw new GridCAException("can not determin wether alias is a key", ex);
            }
            if (isKeyEntry) {
                LOGGER.log(Level.FINE, "Adding key for alias {0}", alias);

                Key key = null;
                try {
                    key = pkcs12Ks.getKey(alias, pkcs12PW);
                } catch(NoSuchAlgorithmException ex) {
                    throw new GridCAException("invalid algorithm in pkcs12 keystore", ex);
                } catch(UnrecoverableKeyException ex) {
                    throw new GridCAException("Can not access private key in pkcs12 file", ex);
                } catch (KeyStoreException ex) {
                    throw new GridCAException("Can not access private key in pkcs12 file", ex);
                }
                Certificate[] chain;
                try {
                    chain = pkcs12Ks.getCertificateChain(alias);
                } catch (KeyStoreException ex) {
                    throw new GridCAException("Can not access certificate in pkcs12 file", ex);
                }

                if(!(chain[0] instanceof X509Certificate)) {
                    throw new IllegalArgumentException("can only handle X509 Certificates");
                }
                X509Certificate cert = (X509Certificate)chain[0];

                String name = cert.getSubjectX500Principal().getName();

                int uidIndex = name.indexOf("UID=");
                if(uidIndex < 0) {
                    throw new GridCAException("UID not found in dnname");
                }
                uidIndex += 4;
                int endIndex = name.indexOf(",",uidIndex);
                alias = name.substring(uidIndex, endIndex);
                try {
                    ks.setKeyEntry(alias,key,privateKeyPassword,chain);
                } catch (KeyStoreException ex) {
                    throw new GridCAException("Can not add alias to JKS keystore", ex);
                }
                LOGGER.exiting("GridCAImpl", "createKeyStore");
                return ks;
            }
        }
        throw new GridCAException("alias for user " + username + " not found in pkcs12 keystore");
    }
    
    
    
    private final KeyStore createPKCS12KeyStore(String type, String username, char [] password) throws GridCAException {
        LOGGER.entering("GridCAImpl", "createPKCS12KeyStore");
        File userFile = null;
        File passwordFile = null;
        if(!config.getTmpDir().exists()) {
            throw new GridCAException("tmpdir " + config.getTmpDir() + " does not exist");
        }
        if(!config.getTmpDir().canWrite()) {
            throw new GridCAException("can not write int tmpdir " + config.getTmpDir());
        }
        File pkcs12File = new File(config.getTmpDir(), username + ".p12");        

        try {
            try {
                // write an empty password file, since the private keys are not crypted
                passwordFile = createTempFile("passin",".txt");

                try {
                    PrintWriter pw = new PrintWriter(new FileWriter(passwordFile));
                    pw.println(password);
                    pw.flush();
                    pw.close();
                } catch(IOException ex) {
                    throw new GridCAException("IO error while writing password file: " + ex.getLocalizedMessage());
                }

                Expect pb = createProcess();

                if(type.equals("user")) {
                    pb.command().add("-pkcs12");
                } else if (type.equals("grm_daemon")) {
                    pb.command().add("-grm_pkcs12");
                }
                pb.command().add(username);
                pb.command().add("-pkcs12pwf");
                pb.command().add(passwordFile.getAbsolutePath());
                pb.command().add("-pkcs12dir");
                pb.command().add(config.getTmpDir().getAbsolutePath());
                
                execute(pb);

            } finally {
//                if(userFile != null) {
//                    userFile.delete();
//                }
//                if(passwordFile != null) {
//                    passwordFile.delete();
//                }
            }
            try {
                KeyStore kspkcs12 = KeyStore.getInstance("PKCS12");
                kspkcs12.load(new FileInputStream(pkcs12File), password);
                
                LOGGER.exiting("GridCAImpl", "createPKCS12KeyStore");
                return kspkcs12;
            } catch (CertificateException ex) {
                throw new GridCAException("Certificate error", ex);
            } catch (IOException ex) {
                LOGGER.throwing("GridCAImpl", "createPKCS12KeyStore", ex);
                throw new GridCAException("Can not load pkcs12 keystore", ex);
            } catch (NoSuchAlgorithmException ex) {
                throw new GridCAException("PKCS12 Algorithm unknown", ex);
            } catch (KeyStoreException ex) {
                throw new GridCAException("KeyStore Error", ex);
            }        
        } finally {
            if(pkcs12File.exists()) {
                if(!pkcs12File.delete()) {
                    LOGGER.log(Level.WARNING,"Can not delete pkcs12 file {0}", pkcs12File);
                }
            }
        }
    }

    private static class OutputHandler implements Runnable {
        private InputStream in;
        private final StringWriter errors = new StringWriter();
        private PrintWriter pw;
        
        public OutputHandler(InputStream in, Writer out) {
            this.in = in;
            pw = new PrintWriter(out);
            
        }

        private static String [] ERROR_PATTERNS = {
            "Error:",
            "Error"
        };
        
        public void run() {
            
            PrintWriter epw = new PrintWriter(errors);
            try {
                String line = null;
                BufferedReader br = new BufferedReader(new InputStreamReader(in));
                boolean firstError = false;
                while((line = br.readLine()) != null) {
                    pw.println(line);
                    for(int i = 0; i < ERROR_PATTERNS.length; i++) {
                        if(line.startsWith(ERROR_PATTERNS[i])) {
                            if(firstError) {
                                firstError = false;
                            } else {
                                epw.println();
                            }
                            epw.print(line.substring(ERROR_PATTERNS[i].length()).trim());
                        }
                    }
                }
            } catch(IOException ioe) {
                LOGGER.log(Level.SEVERE, "IO error in sge_ca output handler", ioe);
            } finally {
                epw.close();
                pw.close();
            }
        }

        public String getError() {
            return errors.getBuffer().toString();
        }
    }
}


