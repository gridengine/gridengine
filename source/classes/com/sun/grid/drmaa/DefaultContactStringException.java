/*
 * DefaultContactStringException.java
 *
 * Created on June 17, 2003, 10:37 AM
 */

package com.sun.grid.drmaa;

/** DRMAA could not use the default contact string to connect to DRM
 * system.
 * @author dan.templeton@sun.com
 */
public class DefaultContactStringException extends DRMAAException {
	
	/**
	 * Creates a new instance of <code>DefaultContactStringException</code> without detail message.
	 */
	public DefaultContactStringException () {
	}
	
	
	/**
	 * Constructs an instance of <code>DefaultContactStringException</code> with the specified detail message.
	 * @param msg the detail message.
	 */
	public DefaultContactStringException (String msg) {
		super (msg);
	}
}
