/*
 * InvalidJobException.java
 *
 * Created on June 18, 2003, 10:47 AM
 */

package com.sun.grid.drmaa;

/** The job specified by the 'jobId' does not exist.
 * @author dan.templeton@sun.com
 */
public class InvalidJobException extends DRMAAException {
	
	/**
	 * Creates a new instance of <code>InvalidJobException</code> without detail message.
	 */
	public InvalidJobException () {
	}
	
	
	/**
	 * Constructs an instance of <code>InvalidJobException</code> with the specified detail message.
	 * @param msg the detail message.
	 */
	public InvalidJobException (String msg) {
		super (msg);
	}
}
