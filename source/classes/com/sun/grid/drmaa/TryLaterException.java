/*
 * TryLaterException.java
 *
 * Created on June 17, 2003, 12:54 PM
 */

package com.sun.grid.drmaa;

/** Could not pass job now to DRM system.  A retry may succeed,
 * however (saturation).
 * @author dan.templeton@sun.com
 */
public class TryLaterException extends DRMAAException {
	
	/**
	 * Creates a new instance of <code>TryLaterException</code> without detail message.
	 */
	public TryLaterException () {
	}
	
	
	/**
	 * Constructs an instance of <code>TryLaterException</code> with the specified detail message.
	 * @param msg the detail message.
	 */
	public TryLaterException (String msg) {
		super (msg);
	}
}
