#ifndef __SGE_STRING_H
#define __SGE_STRING_H
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
 *  License at http://www.gridengine.sunsource.net/license.html
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



#ifdef  __cplusplus
extern "C" {
#endif

char *sge_basename(char *name, int delim);
char *sge_delim_str(char *str, char **delim_pos, char *delim);
char *sge_dirname(char *name, int delim);
char *sge_strdup(char *old, const char *src);
char *sge_strtok(const char *str, const char *delimitor);
int sge_is_pattern(char *p);

struct saved_vars_s {
   char *static_cp;
   char *static_str;
};
char *sge_strtok_r(const char *str, const char *delimitor, struct saved_vars_s **last);
void free_saved_vars(struct saved_vars_s *last);

char *stradd(char *cp1, char *cp2, char *res);
int check_fname(char *fname);
int sge_strnullcasecmp(char *a, char *b);
int sge_strnullcmp(char *a, char *b);
void strip_blanks(char *str);

/* compare hosts with FQDN or not */
#define MAXHOSTLEN 256
extern int fqdn_cmp;
/* compare hosts by using a default domain FQDN or not */
extern char *default_domain;
int hostcmp(const char *h1, const char *h2);
void hostdup(const char *raw, char *dst);

#ifndef WIN32NATIVE
#define SGE_STRCASECMP(a, b) strcasecmp(a, b)
#else
#define SGE_STRCASECMP(a, b) stricmp(a, b)
#endif

#ifdef  __cplusplus
}
#endif
 
#endif /* __SGE_STRING_H */

