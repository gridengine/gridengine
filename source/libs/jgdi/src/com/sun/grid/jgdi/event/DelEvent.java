/*___INFO__MARK_BEGIN__*/
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
/*___INFO__MARK_END__*/
package com.sun.grid.jgdi.event;

/**
 *
 * @author  richard.hierlmeier@sun.com
 */
public abstract class DelEvent extends ChangedObjectEvent {
   
   /** Creates a new instance of AbstractDelEvent */
   public DelEvent(long timestamp, int eventId, Class objectType) {
      super(EventType.SGE_EMA_DEL, timestamp, eventId, objectType);
   }
   
   /**
    *  Set the primary key information of the deleted object into the del event.
    * 
    *  <p>This method is called from the the native part of jgdi to fill the
    *     primary key information into a del event</p>
    *
    *  @param  numKey1  first numerical primary key of the deleted object
    *  @param  numKey2  second numerical primary key of the deleted object
    *  @param  strKey1  first string primary key of the deleted object
    *  @param  strKey2  second string primary key of the deleted object
    */
   public abstract void setPKInfo(int numKey1, int numKey2, String strKey1, String strKey2);
   
   /**
    *  Determine if this event has deleted <code>obj</code>
    *  @param   obj  the object
    *  @return  <code>true</code> if this event has deleted <code>obj</code>
    */
   public abstract boolean hasDeletedObject(Object obj);

}
