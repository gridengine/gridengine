/*
 * InvalidAttributeFormatException.java
 *
 * Created on June 17, 2003, 11:02 AM
 */

package com.sun.grid.drmaa;

/** The format for the job attribute value is invalid.
 * @author dan.templeton@sun.com
 */
public class InvalidAttributeFormatException extends InvalidArgumentException {
	
	/**
	 * Creates a new instance of <code>InvalidAttributeFormatException</code> without detail message.
	 */
	public InvalidAttributeFormatException () {
	}
	
	
	/**
	 * Constructs an instance of <code>InvalidAttributeFormatException</code> with the specified detail message.
	 * @param msg the detail message.
	 */
	public InvalidAttributeFormatException (String msg) {
		super (msg);
	}
}
