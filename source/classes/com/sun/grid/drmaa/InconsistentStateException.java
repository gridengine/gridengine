/*
 * InvalidStateException.java
 *
 * Created on June 18, 2003, 10:44 AM
 */

package com.sun.grid.drmaa;

/** The job cannot be moved to the requested state.
 * @author dan.templeton@sun.com
 */
public class InconsistentStateException extends DRMAAException {
	public static final int HOLD = 0;
	public static final int RELEASE = 1;
	public static final int RESUME = 2;
	public static final int SUSPEND = 3;
	
	private int state;
	
	/**
	 * Creates a new instance of <code>InvalidStateException</code> without detail message.
	 */
	public InconsistentStateException (int state) {
		this.state = state;
	}
	
	
	/**
	 * Constructs an instance of <code>InvalidStateException</code> with the specified detail message.
	 * @param msg the detail message.
	 */
	public InconsistentStateException (int state, String msg) {
		super (msg);
		
		this.state = state;
	}
}
