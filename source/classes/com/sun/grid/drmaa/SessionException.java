/*
 * InitException.java
 *
 * Created on June 17, 2003, 10:46 AM
 */

package com.sun.grid.drmaa;

/** A problem occured with the DRM session preventing the routine from completing.
 * @author dan.templeton@sun.com
 */
public class SessionException extends DRMAAException {
	
	/**
	 * Creates a new instance of <code>InitException</code> without detail message.
	 */
	public SessionException () {
	}
	
	
	/**
	 * Constructs an instance of <code>InitException</code> with the specified detail message.
	 * @param msg the detail message.
	 */
	public SessionException (String msg) {
		super (msg);
	}
}
