/*
 * AuthorizationException.java
 *
 * Created on June 17, 2003, 1:00 PM
 */

package com.sun.grid.drmaa;

/** The specified request is not processed successfully due to
 * authorization failure.
 * @author dan.templeton@sun.com
 */
public class AuthorizationException extends DRMAAException {
	
	/**
	 * Creates a new instance of <code>AuthorizationException</code> without detail message.
	 */
	public AuthorizationException () {
	}
	
	
	/**
	 * Constructs an instance of <code>AuthorizationException</code> with the specified detail message.
	 * @param msg the detail message.
	 */
	public AuthorizationException (String msg) {
		super (msg);
	}
}
