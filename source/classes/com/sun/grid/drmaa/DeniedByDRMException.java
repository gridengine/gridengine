/*
 * DeniedByDRMException.java
 *
 * Created on June 17, 2003, 12:55 PM
 */

package com.sun.grid.drmaa;

/** The DRM system rejected the job. The job will never be accepted
 * due to DRM configuration or job template settings.
 * @author dan.templeton@sun.com
 */
public class DeniedByDRMException extends DRMAAException {
	
	/**
	 * Creates a new instance of <code>DeniedByDRMException</code> without detail message.
	 */
	public DeniedByDRMException () {
	}
	
	
	/**
	 * Constructs an instance of <code>DeniedByDRMException</code> with the specified detail message.
	 * @param msg the detail message.
	 */
	public DeniedByDRMException (String msg) {
		super (msg);
	}
}
