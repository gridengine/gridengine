/*
 * NoActiveSessionException.java
 *
 * Created on June 17, 2003, 11:00 AM
 */

package com.sun.grid.drmaa;

/** Exit routine failed because there is no active session.
 * @author dan.templeton@sun.com
 */
public class NoActiveSessionException extends SessionException {
	
	/**
	 * Creates a new instance of <code>NoActiveSessionException</code> without detail message.
	 */
	public NoActiveSessionException () {
	}
	
	
	/**
	 * Constructs an instance of <code>NoActiveSessionException</code> with the specified detail message.
	 * @param msg the detail message.
	 */
	public NoActiveSessionException (String msg) {
		super (msg);
	}
}
