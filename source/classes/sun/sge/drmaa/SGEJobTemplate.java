/*
 * SGEJobTemplate.java
 *
 * Created on March 4, 2004, 10:09 AM
 */

package sun.sge.drmaa;

import java.util.*;

import com.sun.grid.drmaa.*;

/**
 *
 * @author  dan.templeton@sun.com
 */
public class SGEJobTemplate extends JobTemplate {
   private SGESession session = null;
   private int id = -1;
   
   /** Creates a new instance of SGEJobTemplate */
   SGEJobTemplate (SGESession session, int id) {
      this.session = session;
      this.id = id;
   }
   
   public List getAttribute (String name) {
      String[] values = session.nativeGetAttribute (name);
      
      return Arrays.asList (values);
   }
   
   public Set getAttributeNames () {
      String[] names = session.nativeGetAttributeNames ();
      
      return new HashSet (Arrays.asList (names));
   }
   
   public void setAttribute (String name, List value) throws DRMAAException {
      session.nativeSetAttributeValues (name, (String[])value.toArray (new String[value.size ()]));
   }
   
   public void setAttribute (String name, String value) throws DRMAAException {
      session.nativeSetAttributeValue (name, value);
   }   
   
   public void delete () throws DRMAAException {
      session.nativeDeleteJobTemplate (this);
   }
   
   int getId () {
      return id;
   }
}
