/*
 * DRMAAException.java
 *
 * Created on June 17, 2003, 10:29 AM
 */

package com.sun.grid.drmaa;

/** The base class for all DRMAA Exceptions.
 * @author dan.templeton@sun.com
 */
public class DRMAAException extends java.lang.Exception {
	
	/**
	 * Creates a new instance of <code>DRMAAException</code> without detail message.
	 */
	public DRMAAException () {
	}
	
	
	/**
	 * Constructs an instance of <code>DRMAAException</code> with the specified detail message.
	 * @param msg the detail message.
	 */
	public DRMAAException (String msg) {
		super (msg);
	}
}
