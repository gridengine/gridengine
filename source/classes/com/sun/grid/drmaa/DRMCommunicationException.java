/*
 * CommunicationException.java
 *
 * Created on June 17, 2003, 10:41 AM
 */

package com.sun.grid.drmaa;

/** Could not contact DRM system for this request.
 * @author dan.templeton@sun.com
 */
public class DRMCommunicationException extends DRMAAException {
	
	/**
	 * Creates a new instance of <code>CommunicationException</code> without detail message.
	 */
	public DRMCommunicationException () {
	}
	
	
	/**
	 * Constructs an instance of <code>CommunicationException</code> with the specified detail message.
	 * @param msg the detail message.
	 */
	public DRMCommunicationException (String msg) {
		super (msg);
	}
}
