/*
 * DRMSExitException.java
 *
 * Created on June 17, 2003, 11:00 AM
 */

package com.sun.grid.drmaa;

/** DRM system disengagement failed.
 * @author dan.templeton@sun.com
 */
public class DRMSExitException extends SessionException {
	
	/**
	 * Creates a new instance of <code>DRMSExitException</code> without detail message.
	 */
	public DRMSExitException () {
	}
	
	
	/**
	 * Constructs an instance of <code>DRMSExitException</code> with the specified detail message.
	 * @param msg the detail message.
	 */
	public DRMSExitException (String msg) {
		super (msg);
	}
}
