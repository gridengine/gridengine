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
 * DRMAASessionFactory.java
 *
 * Created on June 18, 2003, 10:07 AM
 */

package org.ggf.drmaa;

import java.io.*;
import java.util.Properties;

/** This class is used to retrieve a DRMAASession object tailored to the
 * DRM in use.  The factory will use the com.sun.grid.drmaa.DRMAASessionFactory
 * property to discover the DRM-specific DRMAASession implementation class.<BR>
 * An example DRMAA interaction would be:<BR>
 * <P><CODE>
 * //Get the session<BR>
 * DRMAASession session = DRMAASessionFactory.getSession ();<BR>
 * <BR>
 * //Initialize the session<BR>
 * session.init ("Sun ONE Grid Engine");<BR>
 * <BR>
 * //Get the job template<BR>
 * JobTemplate jt = session.createJobTemplate ();<BR>
 * <BR>
 * //Set some attributes<BR>
 * jt.setAttribute (JobTemplate.REMOTE_COMMAND, "uptime");<BR>
 * ...<BR>
 * jt.setAttribute (JobTempalte.DEADLINE_TIME, "2:30:0");<BR>
 * <BR>
 * //Run the job<BR>
 * String jobId = session.runJob (jt);<BR>
 * <BR>
 * //Wait for the job to finish<BR>
 * session.wait (jobId, DRMAASession.TIMEOUT_WAIT_FOREVER);<BR>
 * //Release the tempalte<BR>
 * jt.delete ();<BR>
 * //End the session<BR>
 * session.exit ();<BR>
 * </CODE></P>
 * @author dan.templeton@sun.com
 */
public abstract class DRMAASessionFactory {
	/** Right now, only one DRMAASession can exist at a time.  This is that session. */	
	private static DRMAASessionFactory thisFactory = null;
	/** The name of the property used to find the DRMAASession implementation
	 * class name.
	 */	
	private static final String SESSION_PROPERTY = "com.sun.grid.drmaa.DRMAASessionFactory";
	
	/** Creates a new instance of DRMAASessionFactory */
	protected DRMAASessionFactory () {
	}
	
	/** Gets a DRMAASession object appropriate for the DRM in use.
	 * @return a DRMAASession object appropriate for the DRM in use
	 */	
	public abstract DRMAASession getSession ();
	
	/** Gets a DRMAASessionFactory object appropriate for the DRM in use.
	 * @return a DRMAASessionFactory object appropriate for the DRM in use
	 */	
	public static DRMAASessionFactory getFactory () {
		if (thisFactory == null) {
			thisFactory = newFactory ();
		}
		
		return thisFactory;
   }
	
	/** Creates a DRMAASessionFactory object appropriate for the DRM in use.  This
	 * method uses the com.sun.grid.drmaa.DRMAASessionFactory property to find
	 * the appropriate class.  It looks first in the system properties.  If the
	 * property is not present, the method looks in
	 * $java.home/lib/drmaa.properties.  If the property still isn't found, the
	 * method will search the classpath for a
	 * META-INF/services/com.sun.grid.drmaa.DRMAASessionFactory resource.  If the
	 * property still has not been found, the method throws a ConfigurationError.
	 * @return a DRMAASession object appropriate for the DRM in use
	 */	
	private static DRMAASessionFactory newFactory () {
		ClassLoader classLoader = findClassLoader ();
		
		// Use the system property first
		try {
			String systemProp = System.getProperty (SESSION_PROPERTY);
			if (systemProp != null) {
				return (DRMAASessionFactory)newInstance (systemProp, classLoader);
			}
		}
		catch (SecurityException se) {
			//If we get a security exception, treat it as failure and try the next method
		}
		
		// try to read from $java.home/lib/drmaa.properties
		try {
			String javah = System.getProperty ("java.home");
			String configFile = javah + File.separator + "lib" + File.separator + "drmaa.properties";
			File f = new File (configFile);
			if (f.exists ()) {
				Properties props = new Properties ();
				props.load (new FileInputStream (f));
				String className = props.getProperty (SESSION_PROPERTY);
				return (DRMAASessionFactory)newInstance (className, classLoader);
			}
		}
		catch(Exception ex ) {
			ex.printStackTrace ();
		}
		
		String serviceId = "META-INF/services/" + SESSION_PROPERTY;
		// try to find services in CLASSPATH
		try {
			InputStream is = null;
			if (classLoader == null) {
				is = ClassLoader.getSystemResourceAsStream (serviceId);
			} else {
				is = classLoader.getResourceAsStream (serviceId);
			}
			
			if (is != null) {
				BufferedReader rd =	new BufferedReader (new InputStreamReader (is, "UTF-8"));
				
				String className = rd.readLine ();
				
				rd.close ();
				
				if (className != null && ! className.equals ("")) {
					return (DRMAASessionFactory)newInstance (className, classLoader);
				}
			}
		}
		catch (Exception ex) {
			ex.printStackTrace ();
		}
		
		throw new ConfigurationError ("Provider for " + SESSION_PROPERTY + " cannot be found", null);
	}
   	
	/** Figure out which ClassLoader to use.  For JDK 1.2 and later use the
	 * context ClassLoader if possible.  Note: we defer linking the class
	 * that calls an API only in JDK 1.2 until runtime so that we can catch
	 * LinkageError so that this code will run in older non-Sun JVMs such
	 * as the Microsoft JVM in IE.
	 * @throws ConfigurationError thrown if the classloader cannot be found or loaded
	 * @return an appropriate ClassLoader
	 */
	private static ClassLoader findClassLoader () throws ConfigurationError {
		ClassLoader classLoader;
		
		try {
			// Construct the name of the concrete class to instantiate
			classLoader = Thread.currentThread ().getContextClassLoader ();
		}
		catch (LinkageError le) {
			// Assume that we are running JDK 1.1, use the current ClassLoader
			classLoader = DRMAASessionFactory.class.getClassLoader ();
		}
		catch (Exception ex) {
			// Something abnormal happened so throw an error
			throw new ConfigurationError (ex.toString (), ex);
		}
		
		return classLoader;
	}
	
	/** Create an instance of a class using the specified ClassLoader
	 * @param className The name of the class to be used to create the object
	 * @param classLoader the classloader to use to create the object
	 * @throws ConfigurationError thrown is the class cannot be instantiated
	 * @return an instance of the given class
	 */
	private static Object newInstance (String className, ClassLoader classLoader)	throws ConfigurationError {
		try {
			Class spiClass;
			
			if (classLoader == null) {
				spiClass = Class.forName (className);
			} else {
				spiClass = classLoader.loadClass (className);
			}
			
			return spiClass.newInstance ();
		}
		catch (ClassNotFoundException ex) {
			throw new ConfigurationError ("Provider " + className + " not found", ex);
		}
		catch (Exception ex) {
			throw new ConfigurationError ("Provider " + className + " could not be instantiated: " + ex, ex);
		}
	}
	
	/** Exception used to indicate trouble loading the needed classes. */	
	private static class ConfigurationError extends Error {
		/** The Exception which caused this Exception */		
		private Exception exception;
		
		/** Construct a new instance with the specified detail string and
		 * exception.
		 * @param msg the error message
		 * @param x the original Exception which caused this Exception
		 */
		ConfigurationError (String msg, Exception x) {
			super (msg);
			this.exception = x;
		}
		
		/** Get the Exception which caused this Exception
		 * @return the Exception which caused this Exception
		 */		
		Exception getException () {
			return exception;
		}
	}
}
