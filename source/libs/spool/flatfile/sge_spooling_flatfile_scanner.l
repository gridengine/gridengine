/* scanner for Grid Engine ascii spool format
 *
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * !! when you change this file, remove sge_spooling_flatfile_scanner.c !!
 * !! before building Grid Engine                                       !!
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

%option noyywrap


%{
/* need this for the call to atof() below */
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "spool/flatfile/sge_spooling_flatfile_scanner.h"

#define YY_BREAK
#define YY_NO_UNPUT

int spool_line;
bool spool_return_whitespace;

int spool_scanner_initialize(FILE *input)
{
   spool_line = 1;
   spool_return_whitespace = false;
   spool_restart(input);

   return spool_lex();
}

void spool_scanner_shutdown(void)
{
}

%}

DIGIT    [0-9]
ID       [a-z/\$][a-z0-9_/\.\-\$]*
COLON    :
DELIMITER [,;:|=\{\}]
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

#[^\n]*{NL}   spool_line++; break;       /* eat up comments */

{NL}          { 
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

.           return SPFT_UNKNOWN;

%%

#if 0
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
