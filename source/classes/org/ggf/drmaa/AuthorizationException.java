/*************************************************************************
 * 
 *  The Contents of this file are made available subject to the terms of
 *  the Sun Industry Standards Source License Version 1.2
 * 
 *  Sun Microsystems Inc., March, 2001
 * 
 * 
 *  Sun Industry Standards Source License Version 1.2
 *  =================================================
 *  The contents of this file are subject to the Sun Industry Standards
 *  Source License Version 1.2 (the "License"); You may not use this file
 *  except in compliance with the License. You may obtain a copy of the
 *  License at http://gridengine.sunsource.net/Gridengine_SISSL_license.html
 * 
 *  Software provided under this License is provided on an "AS IS" basis,
 *  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
 *  WITHOUT LIMITATION, WARRANTIES THAT THE SOFTWARE IS FREE OF DEFECTS,
 *  MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE, OR NON-INFRINGING.
 *  See the License for the specific provisions governing your rights and
 *  obligations concerning the Software.
 * 
 *   The Initial Developer of the Original Code is: Sun Microsystems, Inc.
 * 
 *   Copyright: 2001 by Sun Microsystems, Inc.
 * 
 *   All Rights Reserved.
 * 
 ************************************************************************/
/*
 * AuthorizationException.java
 *
 * Created on June 17, 2003, 1:00 PM
 */

package org.ggf.drmaa;

/** The specified request is not processed successfully due to
 * authorization failure.
 * @author dan.templeton@sun.com
 */
public class AuthorizationException extends DRMAAException {
	
	/**
	 * Creates a new instance of <code>AuthorizationException</code> without detail message.
	 */
	public AuthorizationException () {
	}
	
	
	/**
	 * Constructs an instance of <code>AuthorizationException</code> with the specified detail message.
	 * @param msg the detail message.
	 */
	public AuthorizationException (String msg) {
		super (msg);
	}
}
