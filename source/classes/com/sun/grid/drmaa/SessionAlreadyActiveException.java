/*
 * SessionAlreadyActiveException.java
 *
 * Created on June 17, 2003, 10:36 AM
 */

package com.sun.grid.drmaa;

/** Initialization failed due to existing DRMAA session.
 * @author dan.templeton@sun.com
 */
public class SessionAlreadyActiveException extends SessionException {
	
	/**
	 * Creates a new instance of <code>SessionAlreadyActiveException</code> without detail message.
	 */
	public SessionAlreadyActiveException () {
	}
	
	
	/**
	 * Constructs an instance of <code>SessionAlreadyActiveException</code> with the specified detail message.
	 * @param msg the detail message.
	 */
	public SessionAlreadyActiveException (String msg) {
		super (msg);
	}
}
