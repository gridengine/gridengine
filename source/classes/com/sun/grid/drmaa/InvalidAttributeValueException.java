/*
 * InvalidAttributeValueException.java
 *
 * Created on June 17, 2003, 11:03 AM
 */

package com.sun.grid.drmaa;

/** The value for the job attribute is invalid.
 * @author dan.templeton@sun.com
 */
public class InvalidAttributeValueException extends InvalidArgumentException {
	
	/**
	 * Creates a new instance of <code>InvalidAttributeValueException</code> without detail message.
	 */
	public InvalidAttributeValueException () {
	}
	
	
	/**
	 * Constructs an instance of <code>InvalidAttributeValueException</code> with the specified detail message.
	 * @param msg the detail message.
	 */
	public InvalidAttributeValueException (String msg) {
		super (msg);
	}
}
