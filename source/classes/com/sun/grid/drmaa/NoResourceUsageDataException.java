/*
 * NoResourceUsageInformationException.java
 *
 * Created on June 18, 2003, 11:59 AM
 */

package com.sun.grid.drmaa;

/** This error code is returned by DRMAASession.wait() when a job has finished
 * but no rusage and stat data could be provided.
 * @author dan.templeton@sun.com
 * @see DRMAASession#wait()
 */
public class NoResourceUsageDataException extends DRMAAException {
	
	/**
	 * Creates a new instance of <code>NoResourceUsageInformationException</code> without detail message.
	 */
	public NoResourceUsageDataException () {
	}
	
	
	/**
	 * Constructs an instance of <code>NoResourceUsageInformationException</code> with the specified detail message.
	 * @param msg the detail message.
	 */
	public NoResourceUsageDataException (String msg) {
		super (msg);
	}
}
