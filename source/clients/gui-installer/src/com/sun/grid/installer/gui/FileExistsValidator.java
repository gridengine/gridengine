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
package com.sun.grid.installer.gui;

import com.izforge.izpack.panels.ProcessingClient;
import com.izforge.izpack.panels.Validator;
import com.sun.grid.installer.util.FileHandler;
import java.io.File;
import java.util.Map;

/**
 * Validator. Check the entered file or directory if it exists. If the file parameter
 * has been given it will be appended to the entered direcory and then will be checked.
 */
public class FileExistsValidator implements Validator {
    private static String PARAM_FILE = "file";

    public boolean validate(ProcessingClient client) {
        String file = client.getText();

        if (file.equals("")) {
            return true;
        }

        // read file paramether if it's given and append it to the entered directory
        File f = new File(file);
        if (f.isDirectory() && client.hasParams()) {
        	Map<String, String> params = client.getValidatorParams();

        	if (params.containsKey(PARAM_FILE)) {
        		String fileSuffix = params.get(PARAM_FILE);

                if (!file.endsWith(FileHandler.SEPARATOR) && !fileSuffix.startsWith(FileHandler.SEPARATOR)) {
                    file += FileHandler.SEPARATOR;
                }
                
                file += fileSuffix;

                f = new File(file);
                return f.exists();
        	} else {
                return true;
            }
        } else {
            return f.exists();
        }
    }
}
