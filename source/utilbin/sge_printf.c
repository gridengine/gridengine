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
#include <stdio.h>
#include <stdlib.h>
#include "sgermon.h" 

#include "basis_types.h"
#include "sge_language.h"
#include "msg_utilbin.h"
#include <strings.h>

typedef struct sge_printf_opt {
      int e;   /* print to stderr */
      int n;   /* no new line */
      int u;   /* underline output */
      char* D; /* dash string */
      int S;    /* nr of spaces */
   } sge_printf_options;




static void  sge_printf_welcome(void);
static void  sge_printf_usage(void);
static int   sge_printf_get_nr_of_substrings(char* buffer, char* substring);
static char* sge_printf_string_replace(char* arg, char* what, char* with, int only_first );
static char* sge_printf_string_input_parsing(char* string);
static char* sge_printf_string_output_parsing(char* string);
static void  sge_printf_print_line(sge_printf_options* options, char* line);
static void  sge_printf_format_output(sge_printf_options* options, char* text);
static char* sge_printf_get_next_word(char* text);
static char* sge_printf_build_dash(sge_printf_options* options);
int main(int argc, char *argv[]);

char* sge_printf_build_dash(sge_printf_options* options) {
   char dash[500];
   int i;
 
   strcpy(dash,"");
   for(i=0;i<options->S;i++) {
      strcat(dash," ");
   }
   if (strlen(options->D) > 0) {
      strcat(dash,_(options->D));
      strcat(dash," ");
   }
   return dash;
}

char* sge_printf_get_next_word(char* text) {
   char buf[2000];
   char* p1;
   int i;
   int start;

   strcpy(buf,text);
   p1 = buf;
   for (start=0;start<strlen(buf);start++) {
      if ( buf[start] == ' ' ) {
         p1++;
         continue;
      } else {
         break;
      }
   }

   for (i=0;i<strlen(p1);i++) {
      if( p1[i] == ' ') {
         p1[i] = 0;
         break;
      }
      if( p1[i] == '\n') {
         p1[i+1] = 0;
         break;
      }
   }
   return p1;
}

void  sge_printf_format_output(sge_printf_options* options, char* text) {
   char* column_var = NULL;
   int max_column = 79;
   char line[2000];
   char line2[2000];
   char dash[500];
   char* tp = NULL;
   int i;
   int nr_word = 0;
   
   int new_line_opt = options->n;

   options->n = 0;

   column_var = getenv("SGE_PRINTF_MAX_COLUMN");
   if (column_var != NULL) {
      max_column = atoi(column_var);
   } 

   tp = text;
  
   strcpy(dash,sge_printf_build_dash(options));
   strcpy(line,dash);
   strcpy(dash,"");
   for(i=0;i< strlen(line);i++) {
      strcat(dash," ");
   }
   nr_word = 0;
   while(1) {
      char* next_word = NULL;

      next_word = sge_printf_get_next_word(tp);
      if (strlen(next_word) == 0 ) {
         if (nr_word != 0) {
            options->n = new_line_opt;
            sge_printf_print_line(options,line);
         }
         nr_word = 0;
         break;
      }

      if(nr_word != 0) {
         strcat(line," ");
      }
      
      strcat(line,next_word);
      nr_word++;
      tp = strstr(tp,next_word);
      for(i=0;i<strlen(next_word);i++) {
         tp++;
      }
      

      if(strlen(line) > max_column || strstr(line,"\n") != NULL ) {
         int z;
         int l;
         strcpy(line2,line);
         l=0;
         for(z=0;z <= strlen(line);z++) {
            line2[l++] = line[z];
            if (l >= max_column ) {
               line2[l]=0;
               sge_printf_print_line(options,line2);
               strcpy(line2,dash);
               l=strlen(dash);
            }
         }
         if (l>0) {
            line2[l]=0;
            sge_printf_print_line(options,line2);
         }

         nr_word = 0;
         strcpy(line,dash);
      } else {
         next_word = sge_printf_get_next_word(tp);
         if (strlen(next_word) == 0) {
            options->n = new_line_opt;
            sge_printf_print_line(options,line);
            nr_word = 0;
         break;
         }
         if( strlen(line) + strlen(next_word) + 1 > max_column &&
            nr_word != 0 ) {
            sge_printf_print_line(options,line);
            nr_word = 0;
            strcpy(line,dash);
         }      
      }
   }
   options->n = new_line_opt;
}


void  sge_printf_print_line(sge_printf_options* options, char* line_arg) {
   int i;
   FILE* output;
   char line[2000];
   char dash[500];
   int line_length;
   int lc;

   strcpy(dash,sge_printf_build_dash(options));
   strcpy(line,dash);
   strcpy(dash,"\n");
   for(i=0;i< strlen(line);i++) {
      strcat(dash," ");
   }

   strcpy(line, sge_printf_string_replace(line_arg,"\n",dash,0));

   line_length = 0;
   lc = 0;
   for(i=0;i<strlen(line);i++) {
      lc++;
      if (line[i] == '\n') {
         lc=0;
      }

      if(line_length < lc) {
          line_length=lc;
      }
   }


   /* output */
   output=stdout;  
   if (options->e == 1) {
      output=stderr;
   }

   fprintf(output,"%s",line);
   if (options->n != 1 && line_length > 0 ) {
      fprintf(output,"\n");
   }

   if (options->u == 1 && line_length > 0) {
      if (options->n == 1) {
         fprintf(output,"\n");
      }
      for(i=0;i<line_length;i++) {
         fprintf(output,SGE_PRINTF_UNDERLINE);
      }
      if (options->n != 1) {
         fprintf(output,"\n");
      }
   }

}


char* sge_printf_string_input_parsing(char* string) {
    char string_buffer[2000];

/*    printf("input : \"%s\"\n",string); */
    strcpy(string_buffer, sge_printf_string_replace(string       , "\\n", "\n",0));
    strcpy(string_buffer, sge_printf_string_replace(string_buffer, "\\r", " ",0));
    strcpy(string_buffer, sge_printf_string_replace(string_buffer, "\\t", " ",0));
/*    printf("output: \"%s\"\n",string_buffer); */

    return string_buffer;
}

char* sge_printf_string_output_parsing(char* string) {
    char string_buffer[2000];

    strcpy(string_buffer, sge_printf_string_replace(string       , "\n", "\\n",0));

    return string_buffer;
}



char* sge_printf_string_replace(char* arg, char* what, char* with, int only_first) {
   char tmp_buf[2000];
   char arg_copy[2000];
   int i;
   char* p1;
   char* p2;

   strcpy(arg_copy,arg);
   p2 = arg_copy;

   p1 = strstr(p2, what);
   if (p1 == NULL) {
      strcpy(tmp_buf,arg);
      return tmp_buf;
   }
   strcpy(tmp_buf,"");
   while (p1 != NULL) {
      *p1 = 0;
      strcat(tmp_buf,p2);
      strcat(tmp_buf,with);
      p2 = p1;
      for(i=0;i<strlen(what);i++) {
         p2++;
      }
      if (only_first == 1) {
         strcat(tmp_buf,p2);
         return tmp_buf;
      } 

      p1 = strstr(p2, what);
   }
   strcat(tmp_buf,p2);

   return tmp_buf;
}

int  sge_printf_get_nr_of_substrings(char* buffer, char* substring) {
   char* p1 = NULL;
   char* buf = NULL;
   int nr = 0;

   buf=buffer;
   p1 = strstr(buf, substring);
   while (p1 != NULL) {
      buf = ++p1;
      p1 = strstr(buf, substring);
      nr++;
   }
   return nr;
}
 


void sge_printf_welcome(void) {

   char* user = NULL;
   user = getenv("USER");
   if (user == NULL) {
      user = "(no USER environment variable set)";
   }
   
   printf("\nno l10n:\n");
   printf(SGE_PRINTF_TESTSTRING_S, user);
   printf("\nl10n:\n");
   printf(_(SGE_PRINTF_TESTSTRING_S), user);
}

void sge_printf_usage(void) {
   printf("usage:\n");
   printf("sge_printf -help    : show help\n");
   printf("sge_printf -test    : test localization\n");
   printf("sge_printf -message : print empty po file string\n");
   printf("sge_printf -message-space : print po file string for test purposes\n");
   printf("sge_printf [-enu] [-D STRING] [-S COUNT] FORMAT_STRING ARGUMENTS\n\n");
   printf("FORMAT_STRING - printf format string\n");
   printf("ARGUMENTS     - printf arguments\n\n");
   printf("options:\n");   
   printf("  e - print to stderr\n");
   printf("  n - no new line\n");
   printf("  u - underline output\n");
   printf("  D - dash sign, e.g. -D \"->\"\n"); 
   printf("  S - nr of spaces, e.g. -S \"5\"\n\n");
   printf("used environment variables:\n");
   printf("SGE_PRINTF_MAX_COLUMN - column for word break (default 79)\n");
}


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
int main(
int argc,
char **argv 
) {
   int show_help = 0;
   int do_test = 0;
   int do_message = 0;
   int do_message_space = 0;
   int args_ok = 1;
   int i = 0;
   int a = 0;
   int last_option = 0;
   int arg_start = 0;   
   int max_args = 0;
   int string_arguments = 0;
   int first_arg = 0;
   int real_args = 0;
   sge_printf_options options;
   char buffer[2000];
   char buffer2[2000];
   char D_opt_buf[500];
   DENTER_MAIN(TOP_LAYER, "sge_printf");

#ifdef __SGE_COMPILE_WITH_GETTEXT__  
   sge_init_language_func((gettext_func_type)        gettext,
                         (setlocale_func_type)      setlocale,
                         (bindtextdomain_func_type) bindtextdomain,
                         (textdomain_func_type)     textdomain);
   sge_init_language(NULL,NULL);
#else
   printf ("\nBinary not compiled with gettext!!!\n");
#endif /* __SGE_COMPILE_WITH_GETTEXT__  */
  
      
   options.e = 0;
   options.n = 0;
   options.u = 0;
   options.D = "";
   options.S = 0;

   for(i=1; i< argc; i++) {
      char* arg = argv[i];

      if (arg[0] == '-' && arg_start == 0) {
         int o_start = 0;
         char* option = NULL;
         int h;
         int opt_length; 
         
         /* option */
         while( arg[o_start++] == '-' );
         option = &arg[--o_start];
         last_option = o_start;
         if ( strcmp(option,"help") == 0) {
            show_help = 1;
         }
         if ( strcmp(option,"test") == 0) {
            do_test = 1;
            break;
         }
         if ( strcmp(option,"message") == 0) {
            do_message = 1;
            continue;
         }
         if ( strcmp(option,"message-space") == 0) {
            do_message_space = 1;
            do_message = 1;
            continue;
         }


         opt_length = strlen(option);
         for(h=0;h<opt_length;h++) {
            if ( option[h] == 'e' ) {
               options.e=1;
               continue;
            }
            if ( option[h] == 'n' ) {
               options.n=1;
               continue;
            } 
            if ( option[h] == 'u' ) {
               options.u=1;
               continue;
            } 
            if ( option[h] == 'D' ) {
/*               D_opt_buf */
               if ( (i+1) < argc) {
                  i++;
                  options.D = argv[i];
               } else {
                  printf("no -D option argument\n");
                  args_ok = 0;
               }
               continue;
            }
            if ( option[h] == 'S' ) {
               if ( (i+1) < argc) {
                  i++;
                  options.S = atoi(argv[i]);
               } else {
                  printf("no -S option argument\n");
                  args_ok = 0;
               }
               continue;
            }
            printf("unkown option \"-%c\"\n",option[h] );
            args_ok = 0;
         }

         
      } else {
         /* argument */
         arg_start = i;
         break;
      }
   }
   if (show_help == 1) {
      sge_printf_usage();
      exit(1); 
   }

   if (do_test == 1) {
      sge_printf_welcome();
      exit(0);
   }

   if (arg_start <= last_option && do_message == 0) {
      printf("no format string\n");
      args_ok = 0;
   }
   /* first pass - get number of %s arguments */
   strcpy(buffer,"");
   max_args = argc - arg_start;
   for(i=arg_start; i < (arg_start + max_args) ; i++) {
      char* arg = argv[i];
      strcat(buffer, arg);
      string_arguments = sge_printf_get_nr_of_substrings(buffer,"%s");
   }

   if (args_ok != 1) {
      printf("syntax error! Type sge_printf -help for usage!\n");
      /* sge_printf_usage(); */
      exit(1);
   }

   
   /* second pass - get format string */
   strcpy(buffer,"");
   for(i=arg_start; sge_printf_get_nr_of_substrings(buffer,"%s") < string_arguments ; i++) {
      char* arg = argv[i];
      if (i > arg_start) {
         strcat(buffer," ");
      }
      strcat(buffer, sge_printf_string_input_parsing(arg));
   }
   first_arg = i;
   real_args = 0;
   for(i=first_arg; i < argc ; i++) {
      /* printf("args[%d] is: \"%s\"\n",i,argv[i]); */
      real_args++;
   }

   if (real_args < string_arguments) {
      printf("to less arguments\n");
      exit(1);
   }

   /* if we have to much args add the rest to the string buffer */
   while(real_args > string_arguments) {
      char* arg = argv[first_arg];
      if (strcmp(buffer,"") != 0) {
         strcat(buffer," ");
      }
      strcat(buffer,sge_printf_string_input_parsing(arg));
      first_arg++;
      real_args--;
   }


   /* 3rd pass - localize format string */
   if (do_message == 1) {
      printf("\nmsgid  \"%s\"\n",sge_printf_string_output_parsing(buffer));
      if(do_message_space == 0) { 
         printf("msgstr \"\"\n");
      } else {
         int h;
         char help_buf[2000];
         printf("msgstr \"");
         strcpy(help_buf, sge_printf_string_output_parsing(buffer));
         for( h=0 ; h < strlen(help_buf) ;h++) {
            printf("%c", help_buf[h]);
            if( h+1 < strlen(help_buf) && 
                help_buf[h] != '\\'     && 
                help_buf[h] != '%') {
               printf("_");
            }
         }
         printf("\"\n");
      }
      exit(0);
   }
   strcpy(buffer2, _(buffer));
/*   printf("format string is: \"%s\"\n",buffer);
   printf("l10n string is: \"%s\"\n", buffer2);*/

   /* format output */
/*   printf("options: %d %d %d \"%s\" %d\n", options.e, options.n, options.u, options.D, options.S);*/

 
   /* 4th pass - insert parameters */ 
   if (real_args > 0) {
      for(i=0;i<real_args;i++) {
/*      printf("argument[%d]: \"%s\"\n",i,argv[first_arg +i]); */
         strcpy(buffer, sge_printf_string_replace(buffer2,"%s",argv[first_arg +i],1));
         strcpy(buffer2,buffer); 
      }  
   } else {
      strcpy(buffer,buffer2);
   }

   /* output */
   sge_printf_format_output(&options,buffer);

   DEXIT;
   return 0;
}
