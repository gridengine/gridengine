/*
 * ExitTimeoutException.java
 *
 * Created on June 18, 2003, 11:34 AM
 */

package com.sun.grid.drmaa;

/** We have encountered a time-out condition for DRMAASession.synchronize()
 * or DRMAASession.wait().
 * @author dan.templeton@sun.com
 */
public class ExitTimeoutException extends DRMAAException {
	
	/**
	 * Creates a new instance of <code>ExitTimeoutException</code> without detail message.
	 */
	public ExitTimeoutException () {
	}
	
	
	/**
	 * Constructs an instance of <code>ExitTimeoutException</code> with the specified detail message.
	 * @param msg the detail message.
	 */
	public ExitTimeoutException (String msg) {
		super (msg);
	}
}
