/*
 * AttributeException.java
 *
 * Created on June 17, 2003, 10:47 AM
 */

package com.sun.grid.drmaa;

/** The value or format of an attribute is invalid.
 * @author dan.templeton@sun.com
 */
public class InvalidAttributeException extends DRMAAException {
   
   /**
    * Creates a new instance of <code>AttributeException</code> without detail message.
    */
   public InvalidAttributeException () {
   }
   
   
   /**
    * Constructs an instance of <code>AttributeException</code> with the specified detail message.
    * @param msg the detail message.
    */
   public InvalidAttributeException (String msg) {
      super (msg);
   }
}
