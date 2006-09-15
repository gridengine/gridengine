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

import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;

/**
 * Base class of all events which signalizes a changes of  
 * objects in the N1[tm] Grid Engine.
 *
 * @author  richard.hierlmeier@sun.com
 */
public abstract class ChangedObjectEvent extends Event {
   
   /** List of changed objects */
   private List changedObjects;
   
   /** Type of the changed objects */
   private Class objectType;
   
   /**
    * Creates a new instance of ChangedObjectEvent
    * @param eventType   type of event
    * @param timestamp   timestamp when the event occured
    * @param eventId     id of the event
    * @param objectType  type of the changed objects
    */
   public ChangedObjectEvent(EventType eventType, long timestamp, int eventId, Class objectType) {
      super(eventType, timestamp, eventId);
      this.objectType = objectType;
   }
   
   /**
    *  Get the type of the changed objects
    *  @return type of the changed objects
    */
   public Class getObjectType() {
      return objectType;
   }
   
   /**
    *  Add a changed object to the list of changed objects
    *  @param changedObj  the changed object
    *  @throws IllegalArgumentException if <code>changedObj</code> is <code>null</code>
    *  @throws IllegalArgumentException if the class object type for the event is not 
    *                                   assignable from the class of <code>changedObj</code>
    *  @see #getObjectType
    */
   public void add(Object changedObj) {
      if( changedObj == null ) {
         throw new IllegalArgumentException("changedObj must not be null");
      }
      if( !objectType.isAssignableFrom(changedObj.getClass())) {
         throw new IllegalArgumentException("changedObj must instanceof " + objectType.getName() );
      }
      if(changedObjects == null) {
         changedObjects = new ArrayList();
      }
      changedObjects.add(changedObj);
   }
   
   /**
    *  Get a list of all object which has been changed by this event
    *  @return list of changed objects
    */
   public List getChangedObjects() {
      if(changedObjects == null) {
         return Collections.EMPTY_LIST;
      } else {
         return Collections.unmodifiableList(changedObjects);
      }
   }

   public String toString() {
      StringBuffer ret = new StringBuffer();
      ret.append("[");
      ret.append(super.toString());
      Iterator iter = getChangedObjects().iterator();
      while(iter.hasNext()) {
         ret.append(",");
         ret.append(iter.next().toString());
      }
      ret.append("]");
      return ret.toString();
   }
   
   
}
