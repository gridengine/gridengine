#ifndef __SEC_CERTTEXT_H
#define __SEC_CERTTEXT_H
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

int sec_certtext(char *textfile, char *defaultfile);
void sec_set_defaults(char defaults[][]);
void sec_set_text(char text[][]);
int sec_read_data(char data[][], char defaults[][], char text[][]);
int sec_printable(unsigned char *s);  
int sec_write_data(char data[][], int last, char *textfile);
void sec_write_name(char data[][], int last, FILE *fp);
int sec_read_defaults(char defaults[][], char *defaultfile);
int sec_write_defaults(char defaults[][], char *defaultfile);

#endif /* __SEC_CERTTEXT_H */

