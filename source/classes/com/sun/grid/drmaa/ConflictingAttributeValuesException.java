/*
 * ConflictingAttributeValuesException.java
 *
 * Created on June 17, 2003, 11:03 AM
 */

package com.sun.grid.drmaa;

/** The value of this attribute is conflicting with a previously set
 * attributes.
 * @author dan.templeton@sun.com
 */
public class ConflictingAttributeValuesException extends InvalidAttributeException {
	
	/**
	 * Creates a new instance of <code>ConflictingAttributeValuesException</code> without detail message.
	 */
	public ConflictingAttributeValuesException () {
	}
	
	
	/**
	 * Constructs an instance of <code>ConflictingAttributeValuesException</code> with the specified detail message.
	 * @param msg the detail message.
	 */
	public ConflictingAttributeValuesException (String msg) {
		super (msg);
	}
}
