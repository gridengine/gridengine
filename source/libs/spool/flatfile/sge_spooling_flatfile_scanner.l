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
/* scanner for Grid Engine ascii spool format
 *
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * !! when you change this file, remove sge_spooling_flatfile_scanner.c !!
 * !! before building Grid Engine                                       !!
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

%option noyywrap
%array

%{
/* need this for the system calls below */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "spool/flatfile/sge_spooling_flatfile_scanner.h"
#include "basis_types.h"

#define YY_BREAK
#define YY_NO_UNPUT
#ifndef YYLMAX
#  define YYLMAX MAX_STRING_SIZE
#endif
int spool_line;
int spool_return_whitespace;
int spool_finish_line;
static int spool_cat_line;
static char spool_tmp_str[MAX_STRING_SIZE];

int spool_scanner_initialize(FILE *input)
{
   spool_line = 1;
   spool_return_whitespace = 0;
   spool_finish_line = 0;
   spool_cat_line = 0;
   spool_restart(input);

   return spool_lex();
}

void spool_scanner_shutdown(void)
{
}

%}

DIGIT    "-"?[0-9]
ID       [a-zA-Z0-9_/.#$\-]*
COLON    :
DELIMITER [,;:|={}[\]]
COMPOP   [=<>!]
NL       \r?\n

%%

{DIGIT}+{COLON}{DIGIT}+{COLON}{DIGIT}+  return SPFT_TIME;

{DIGIT}+    return SPFT_INT; /* JG: TODO: handle negative */

{DIGIT}+"."{DIGIT}*        return SPFT_FLOAT; /* JG: TODO: handle negative */

{DIGIT}+"-"{DIGIT}+  return SPFT_RANGE;

{COMPOP}{COMPOP}  return SPFT_COMPOP;

{DELIMITER}      return SPFT_DELIMITER;

{ID}        return SPFT_WORD;

#[^\n]*{NL}   spool_line++; break;        /* eat up comments */

[\\][ \t\r]*{NL}  spool_line++; break;    /* Ignore newlines preceeded by \ */

^[ \t\r]*{NL}   spool_line++; break;      /* Ignore blank lines */

[ \t\r]*{NL}  { 
                  strcpy(spool_text, "\n");
                  /* JG: TODO: has to be initialized for each scan run */
                  spool_line++; 
                  return SPFT_NEWLINE; 
              } 

[ \t\r]+       { /* whitespace */
                  if(spool_return_whitespace) {
                     strcpy(spool_text, " ");
                     return SPFT_WHITESPACE;
                  }

                  break;
               } 

                  /* Match whole lines not ending in \ */
^[^\n]*[^ \t\r\n\\][ \t\r]*$  %{
                                 if (!spool_cat_line) { REJECT; }
                                 else {
                                    int count;
                                    char *tmp_str = spool_text;
                                    
                                    /* Remove leading spaces */
                                    while (isspace (*tmp_str)) {
                                       tmp_str++;
                                    }

                                    strcat (spool_tmp_str, tmp_str);
                                    strcpy (spool_text, spool_tmp_str);
                                    spool_cat_line = 0;

                                    /* Remove trailing spaces */
                                    for (count = strlen (spool_text) - 1; count >= 0; count--) {
                                       if (!isspace (spool_text[count])) {
                                          spool_text[count + 1] = '\0';
                                          break;
                                       }
                                    }

                                    if (count == -1) {
                                       spool_text[0] = '\0';
                                    }

                                    spool_tmp_str[0] = '\0';
                                    spool_line++;

                                    return SPFT_WORD;
                                 }
                              %}
                /* Match whole lines ending in \ */
^[^\n]*[\\][ \t\r]*\n %{
                        if (!spool_cat_line) { REJECT; }
                        else {
                           char *tmp_str = spool_text;

                           /* Remove leading spaces */
                           while (isspace (*tmp_str)) {
                              tmp_str++;
                           }

                           strcat (spool_tmp_str, tmp_str);
                           tmp_str = (char *)strrchr (spool_tmp_str, '\\') - 1;

                           /* Remove trailing spaces and backslash */
                           while (tmp_str != spool_tmp_str) {
                              if (!isspace (*tmp_str)) {
                                 /* If the last character is a comma, don't add
                                  * any spaces */
                                 if (*tmp_str == ',') {
                                    tmp_str[1] = '\0';
                                 }
                                 /* Otherwise, add a single space to the end*/
                                 else {
                                    tmp_str[1] = ' ';
                                    tmp_str[2] = '\0';
                                 }

                                 break;
                              }

                              tmp_str--;
                           }

                           if (spool_tmp_str == tmp_str) {
                              spool_tmp_str[0] = '\0';
                           }

                           spool_line++;

                           break;
                        }
                      %}
                    /* Match remainders of lines not ending in \ */
[^ \t\r\n][^\n]*[^ \t\r\n\\][ \t\r]*$  %{
                                          if (!spool_finish_line) { REJECT; }
                                          else {
                                             spool_line++;
                                             return SPFT_WORD;
                                          }
                                       %}
                   /* Match remainders of lines ending in \ */
[^ \t\r\n][^\n]*[\\][ \t\r]*\n  %{
                                    if (!spool_finish_line) { REJECT; }
                                    else {
                                       char *tmp_str = NULL;

                                       spool_cat_line = 1;
                                       strcpy (spool_tmp_str, spool_text);
                                       tmp_str = (char *)strrchr (spool_tmp_str, '\\') - 1;

                                       /* Remove trailing spaces and backslash */
                                       while (tmp_str != spool_tmp_str) {
                                          if (!isspace (*tmp_str)) {
                                             /* If the last character is a comma, don't add
                                              * any spaces */
                                             if (*tmp_str == ',') {
                                                tmp_str[1] = '\0';
                                             }
                                             /* Otherwise, add a single space to the end*/
                                             else {
                                                tmp_str[1] = ' ';
                                                tmp_str[2] = '\0';
                                             }

                                             break;
                                          }

                                          tmp_str--;
                                       }

                                       if (spool_tmp_str == tmp_str) {
                                          spool_tmp_str[0] = '\0';
                                       }

                                       spool_line++;

                                       break;
                                    }
                                %}
.           return SPFT_UNKNOWN;

%%

#ifdef BUILD_FLATFILE_SCANNER_MAIN 
int main(int argc, char *argv[])
{
   int ret;

    ++argv, --argc;  /* skip over program name */
    if ( argc > 0 )
            spool_in = fopen( argv[0], "r" );
    else
            spool_in = stdin;

    while((ret = spool_lex())) {
      printf("line %3d: token %3d: %s\n", spool_line, ret, spool_text);
    }
}

#endif
