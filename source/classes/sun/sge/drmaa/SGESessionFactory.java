/*
 * SGESessionFactory.java
 *
 * Created on March 3, 2004, 12:04 PM
 */

package sun.sge.drmaa;

import com.sun.grid.drmaa.*;

/**
 *
 * @author  dan.templeton@sun.com
 */
public class SGESessionFactory extends DRMAASessionFactory {
   private SGESession thisSession = null;
   
   /** Creates a new instance of SGESessionFactory */
   public SGESessionFactory () {
   }
   
	/** Gets a DRMAASession object appropriate for the DRM in use.
	 * @return a DRMAASession object appropriate for the DRM in use
	 */	
	public DRMAASession getSession () {
		if (thisSession == null) {
			thisSession = new SGESession ();
		}
		
		return thisSession;
	}
}
