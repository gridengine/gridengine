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
/*
 * Version.java
 *
 * Created on October 6, 2004, 6:05 PM
 */

package org.ggf.drmaa;

/** Class used to represent the DRM version info
 *
 * @author  dan.templeton@sun.com
 * @since 0.5
 */
public class Version implements java.io.Serializable {
   /** The major version number */
   private int major;
   /** The minor version number */
   private int minor;
   
   /** Create a new Version object
    * @param major major version number (non-negative integer)
    * @param minor minor version number (non-negative integer)
    */
   public Version (int major, int minor) {
      if (major < 0) {
         throw new IllegalArgumentException ("Major version number must be non-negative");
      }
      else if (minor < 0) {
         throw new IllegalArgumentException ("Minor version number must be non-negative");
      }
      
      this.major = major;
      this.minor = minor;
   }
   
   /** Get the major version number.
    * @return major version number (non-negative integer)
    */
   public int getMajor () {
      return major;
   }
   
   /** Get the minor version number.
    * @return minor version number (non-negative integer)
    */
   public int getMinor () {
      return minor;
   }
   
   /** Converts this Version object into a printable String.  The String's
    * format is &lt;major&gt;.&lt;minor&gt;.
    * @return a printable String of the format &lt;major&gt;.&lt;minor&gt;
    */
   public String toString () {
      return Integer.toString (major) + "." + Integer.toString (minor);
   }
   
   /** Test for equality between two Version objects.
    * @param obj the object against which to test
    * @return whether the given object has the same major and minor version numbers as this
    * object
    */   
   public boolean equals (Object obj) {
      if (!(obj instanceof Version)) {
         return false;
      }
      else {
         return ((((Version)obj).major == major) &&
                 (((Version)obj).minor == minor));
      }
   }
   
   /** Get a hash code based on the major and minor version numbers.
    * @return a hash code
    */   
   public int hashCode () {
      return (major * 100) + minor;
   }
   
   /* Returns a copy of this object.
    * @return a copy of this object.
    */
   public Object clone () {
      return new Version (major, minor);
   }
}