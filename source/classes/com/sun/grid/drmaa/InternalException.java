/*
 * InternalException.java
 *
 * Created on June 17, 2003, 10:42 AM
 */

package com.sun.grid.drmaa;

/** Unexpected or internal DRMAA error like memory allocation,
 * system call failure, etc.
 * @author dan.templeton@sun.com
 */
public class InternalException extends DRMAAException {
	
	/**
	 * Creates a new instance of <code>InternalException</code> without detail message.
	 */
	public InternalException () {
	}
	
	
	/**
	 * Constructs an instance of <code>InternalException</code> with the specified detail message.
	 * @param msg the detail message.
	 */
	public InternalException (String msg) {
		super (msg);
	}
}
