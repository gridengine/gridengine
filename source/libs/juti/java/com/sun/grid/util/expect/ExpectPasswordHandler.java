/*
 * Copyright 2003-2004 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 * -----------------------------------------------------------------------------
 * $Id: ExpectPasswordHandler.java,v 1.2 2006/07/26 09:38:07 rhierlmeier Exp $
 * -----------------------------------------------------------------------------
 */
package com.sun.grid.util.expect;

import java.io.IOException;

/**
 *
 * @author  richard.hierlmeier@sun.com
 */
public class ExpectPasswordHandler implements ExpectHandler {
    
    private final String question;
    private final char [] password;
    
    /** Creates a new instance of ExpectStringHandler */
    public ExpectPasswordHandler(String question, char [] password) {
        this.question = question;
        this.password = password;
    }

    public void handle(Expect expect, ExpectBuffer buffer) throws IOException {
        
        if(buffer.consume(question) != null) {
            expect.printlnPassword(password);
            expect.flush();
        }
    }
    
}
