/*
 * UnsupportedAttributeException.java
 *
 * Created on April 17, 2004, 10:52 AM
 */

package org.ggf.drmaa;

/**
 * This exception is thrown when a setter or getter is called for an unsupported
 * optional attribute.
 * @author  dan.templeton@sun.com
 */
public class UnsupportedAttributeException extends java.lang.RuntimeException {
   
   /**
    * Creates a new instance of <code>UnsupportedAttributeException</code> without detail message.
    */
   public UnsupportedAttributeException () {
   }
   
   
   /**
    * Constructs an instance of <code>UnsupportedAttributeException</code> with the specified detail message.
    * @param msg the detail message.
    */
   public UnsupportedAttributeException (String msg) {
      super (msg);
   }
}
