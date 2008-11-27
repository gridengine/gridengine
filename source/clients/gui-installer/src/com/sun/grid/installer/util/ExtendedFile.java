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
package com.sun.grid.installer.util;

import java.io.File;

public class ExtendedFile extends File {

	private static final long serialVersionUID = 4913214587201656292L;
	private String owner       = null;
    private String permissions = null;
    private String group       = null;

    public ExtendedFile(String pathName) {
        super(pathName);
        
        setExtendedProps();
    }

    public void setExtendedProps() {
        String[] props = FileHandler.getFileProps(getAbsolutePath());
        
        assert props.length >= 4;
        
        owner       = props[FileHandler.POS_OWNER_FIELD];
        permissions = props[FileHandler.POS_PERMISSION_FIELD];
        group       = props[FileHandler.POS_GROUP_FIELD];
        
        assert owner       != null;
        assert permissions != null;
        assert group       != null;
    }

    public String getGroup() {
        return group;
    }

    public String getOwner() {
        return owner;
    }

    public String getPermissions() {
        return permissions;
    }

    @Override
    public String toString() {
        return permissions + " " + owner + " " + group + " " + super.toString();
    }
}
