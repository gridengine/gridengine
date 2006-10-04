/*
 * Copyright 2003-2004 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 * -----------------------------------------------------------------------------
 * $Id: JobFinalUsageEvent.java,v 1.1 2006/10/04 12:02:17 rhierlmeier Exp $
 * -----------------------------------------------------------------------------
 */
package com.sun.grid.jgdi.event;

/**
 * Java Wrapper class for the JOB_FINAL_USAGE event
 *
 * @author  richard.hierlmeier@sun.com
 */
public class JobFinalUsageEvent extends JobUsageEvent {
    
    public JobFinalUsageEvent(long timestamp, int evtId) {
        super(timestamp, evtId);
    }
    
}
