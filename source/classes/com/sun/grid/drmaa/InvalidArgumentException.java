/*
 * InvalidArgumentException.java
 *
 * Created on June 17, 2003, 11:02 AM
 */

package com.sun.grid.drmaa;

/** The input value for an argument is invalid.
 * @author dan.templeton@sun.com
 */
public class InvalidArgumentException extends DRMAAException {
	
	/**
	 * Creates a new instance of <code>InvalidArgumentException</code> without detail message.
	 */
	public InvalidArgumentException () {
	}
	
	
	/**
	 * Constructs an instance of <code>InvalidArgumentException</code> with the specified detail message.
	 * @param msg the detail message.
	 */
	public InvalidArgumentException (String msg) {
		super (msg);
	}
}
