/*
 * InvalidContextStringException.java
 *
 * Created on June 17, 2003, 10:35 AM
 */

package com.sun.grid.drmaa;

/** Initialization failed due to invalid contact string.
 * @author dan.templeton@sun.com
 */
public class InvalidContactStringException extends DRMAAException {
	
	/**
	 * Creates a new instance of <code>InvalidContextStringException</code> without detail message.
	 */
	public InvalidContactStringException () {
	}
	
	
	/**
	 * Constructs an instance of <code>InvalidContextStringException</code> with the specified detail message.
	 * @param msg the detail message.
	 */
	public InvalidContactStringException (String msg) {
		super (msg);
	}
}
