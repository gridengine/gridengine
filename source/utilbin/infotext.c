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


/* Interactive formatted localized text*/
/* __          _          _        ____*/
/* -> infotext binary */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sgermon.h" 
#include "basis_types.h"
#include "sge_language.h"
#include "msg_utilbin.h"
#include "sge_dstring.h"


typedef struct sge_infotext_opt {
      int e;     /* print to stderr */
      int n;     /* no new line */
      int u;     /* underline output */
      char* D;   /* dash string */
      int S;     /* nr of spaces */
      char* yes; /* yes parameter for -ask */
      char* no;  /* no parameter for -ask */
      char* def; /* default parameter for -ask */
   } sge_infotext_options;




static void  sge_infotext_welcome(void);
static void  sge_infotext_usage(void);
static int   sge_infotext_get_nr_of_substrings(char* buffer, char* substring);
static char* sge_infotext_string_replace(dstring* buf, char* arg, char* what, char* with, int only_first );
static char* sge_infotext_string_input_parsing(dstring* buf,char* string);
static char* sge_infotext_string_output_parsing(dstring* buf,char* string);
static void  sge_infotext_print_line(dstring* dash_buf, sge_infotext_options* options, dstring* line);
static void  sge_infotext_format_output(dstring* dash_buf, sge_infotext_options* options, char* text);
static char* sge_infotext_get_next_word(dstring* buf,char* text);
static const char* sge_infotext_build_dash(dstring* dash_buf, sge_infotext_options* options);
static char* sge_infotext_build_test_msgstr(dstring* buffer, char* text);
static char* sge_infotext_make_line_break(dstring* buffer, char* text);
int main(int argc, char *argv[]);



static char* sge_infotext_make_line_break(dstring* buffer, char* text) {

   int h;
   int line;
   char hbuf[3];

   strcpy(hbuf,"a");

   sge_dstring_copy_string(buffer,"");
   sge_dstring_append(buffer,"\"");

   line = 0;
   for (h=0; h < strlen(text) ; h++) {
      if (line > 75 && text[h] == ' ' ) {
         line = 0;
         sge_dstring_append(buffer,"\"\n\"");
      }
      hbuf[0] = text[h];
      sge_dstring_append(buffer, hbuf);
      line++;
   }
   sge_dstring_append(buffer,"\"");
   return (char*) sge_dstring_get_string(buffer);

}


char* sge_infotext_build_test_msgstr(dstring* buffer, char* text) {
   int h;
   char app_text[2];

   app_text[1] = 0;   

   sge_dstring_copy_string(buffer,"");
   for (h=0; h < strlen(text) ; h++) {
      app_text[0] = text[h];
      sge_dstring_append(buffer,app_text);
      if (text[h] != '%'  &&
          text[h] != '\\' ) {
          if (h>0) {
             if ( text[h-1] == '\\' ) {
                continue;
             }
          }
          sge_dstring_append(buffer,"_");
      }
   }
   return (char*) sge_dstring_get_string(buffer);
}

const char* sge_infotext_build_dash(dstring* dash_buf, sge_infotext_options* options) {
   int i;
 
   sge_dstring_copy_string(dash_buf,"");
   for(i=0;i<options->S;i++) {
      sge_dstring_append(dash_buf," ");
   }
   if (strlen(options->D) > 0) {
      sge_dstring_append(dash_buf,_(options->D));
      sge_dstring_append(dash_buf," ");
   }
   return sge_dstring_get_string(dash_buf);
}



char* sge_infotext_get_next_word(dstring* buf, char* text) {
   char* p1;
   char* buffer;
   int i,b,not_last,nr_spaces;
   int start;

   sge_dstring_copy_string(buf,text);
   buffer = (char*)sge_dstring_get_string(buf);
   p1 = buffer;
   nr_spaces = 0;
   for (start=0;start<strlen(buffer);start++) {
      if ( buffer[start] == ' ' ) {
         nr_spaces++;
         continue; 
      } else {
         break;
      }
   }
   if (nr_spaces > 1) {
      int stop = strlen(p1);
      for (i=0;i<stop;i++) {
          if( p1[i] != ' ') {
             p1[i-1] = 0;
             break;
          }
      } 
      return p1;
   } 

   if (nr_spaces == 1) {
      p1++;
   }

   

   for (i=0;i<strlen(p1);i++) {
      if( p1[i] == ' ') {
         not_last = 0;
         for(b=i;b<strlen(p1);b++) {
            if (p1[b] != ' ') {
               not_last = 1;
               break;
            }
         }
         if (not_last == 1) {
            p1[i] = 0;
         }
         break;
      }
      if( p1[i] == '\n') {
         p1[i+1] = 0;
         break;
      }
   }
   return p1;
}

void  sge_infotext_format_output(dstring* dash_buf,sge_infotext_options* options, char* text) {
   char* column_var = NULL;
   int max_column = 79;
   char* tp = NULL;
   int i;
   int nr_word = 0;
   dstring dash = DSTRING_INIT;
   dstring tmp_buf = DSTRING_INIT;
   dstring line = DSTRING_INIT;
   dstring line2 = DSTRING_INIT;
   
   int new_line_opt = options->n;

   DENTER(TOP_LAYER,"sge_infotext_format_output" );


   DPRINTF(("format 1\n"));
   options->n = 0;

   column_var = getenv("SGE_INFOTEXT_MAX_COLUMN");
   if (column_var != NULL) {
      max_column = atoi(column_var);
   } 

   tp = text;
   DPRINTF(("do strcpy"));
   if (sge_dstring_get_string(dash_buf) == NULL) {
      DPRINTF(("dash_buf is null"));
   }
   sge_dstring_copy_dstring(&line,dash_buf);
   DPRINTF(("strcpy done"));
   sge_dstring_copy_string(&dash,"");
   DPRINTF(("copy done"));
   for(i=0;i< sge_dstring_strlen(&line);i++) {
      sge_dstring_append(&dash," ");
   }
   nr_word = 0;
   DPRINTF(("while\n"));

/*   printf("\nformat_output of:\"%s\"\n",text); */

   while(1) {
      char* next_word = NULL;
      int new_line_buffer;

      next_word = sge_infotext_get_next_word(&tmp_buf, tp);
      if (strlen(next_word) == 0 ) {
         if (nr_word != 0) {
            options->n = new_line_opt;
            sge_infotext_print_line(dash_buf,options,&line); 
         }
         nr_word = 0;
         break;
      }

      if(nr_word != 0) {
         sge_dstring_append(&line," ");
      }
      
      sge_dstring_append(&line,next_word);
      nr_word++;
      tp = (char*)strstr(tp,next_word);
      for(i=0;i<strlen(next_word);i++) {
         tp++;
      }
      

      if(sge_dstring_strlen(&line) > max_column || strstr(sge_dstring_get_string(&line),"\n") != NULL ) {
#if 0
/*      
         uncomment this code if a word break should be done
         ==================================================
*/
         int z;
         int l;
         sge_dstring_copy_dstring(&line2,&line);
         line_buf  = (char*) sge_dstring_get_string(&line);
         line2_buf = (char*) sge_dstring_get_string(&line2);
         l=0;
         for(z=0;z <= sge_dstring_strlen(&line);z++) {
            line2_buf[l++] = line_buf[z];
            if (l >= max_column ) {
               line2_buf[l]=0;
               sge_infotext_print_line(dash_buf,options,&line2);
               sge_dstring_copy_dstring(&line2, &dash);
               l=sge_dstring_strlen(&dash);
            } 
         }
         if (l>0) {
            line2_buf[l]=0;
            sge_infotext_print_line(dash_buf,options,&line2);
         }
#endif

#if 1
         /*  this will do no word break */ 
         /*  ========================== */
         new_line_buffer = options->n;
         options->n = 1;
         sge_infotext_print_line(dash_buf,options,&line); 
         options->n = new_line_buffer;
#endif

         nr_word = 0;
         sge_dstring_copy_dstring(&line,&dash);
/*         sge_dstring_copy_string(&line,""); */
      } else {
         next_word = sge_infotext_get_next_word(&tmp_buf,tp);
         if (strlen(next_word) == 0) {
            options->n = new_line_opt;
            sge_infotext_print_line(dash_buf,options,&line); 
            nr_word = 0;
         break;
         }
         if( sge_dstring_strlen(&line) + strlen(next_word) + 1 > max_column &&
            nr_word != 0 ) {
            sge_infotext_print_line(dash_buf,options,&line);
            nr_word = 0;
            sge_dstring_copy_dstring(&line,&dash);  
         }      
      }
   }
   DPRINTF(("free strings\n"));
   options->n = new_line_opt;
   sge_dstring_free(&dash);
   sge_dstring_free(&tmp_buf);
   sge_dstring_free(&line);
   sge_dstring_free(&line2);
   DEXIT;
}


void  sge_infotext_print_line(dstring* dash_buf, sge_infotext_options* options, dstring* line_arg) {
   int i;
   FILE* output;
   int line_length;
   int lc;
   char* line_buf;
   dstring line = DSTRING_INIT;
   dstring dash = DSTRING_INIT;
   dstring line_buf_buffer = DSTRING_INIT;

   sge_dstring_copy_dstring(&line,dash_buf);
   sge_dstring_copy_string(&dash,"\n");
   for(i=0;i< sge_dstring_strlen(&line);i++) {
      sge_dstring_append(&dash," ");
   }
/*
   line_buf = sge_infotext_string_replace(&line_buf_buffer,(char*)sge_dstring_get_string(line_arg),"\n",(char*)sge_dstring_get_string(&dash),0);
   sge_dstring_copy_string(&line, line_buf);
*/
   sge_dstring_copy_dstring(&line, line_arg);


   line_length = 0;
   lc = 0;
   line_buf = (char*)sge_dstring_get_string(&line);
   for(i=0;i<sge_dstring_strlen(&line);i++) {
      lc++;
      if (line_buf[i] == '\n') {
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

   fprintf(output,"%s",sge_dstring_get_string(&line));
   if (options->n != 1 && line_length > 0 ) {
      fprintf(output,"\n");
   }

   if (options->u == 1 && line_length > 0) {
      if (options->n == 1) {
         fprintf(output,"\n");
      }
      for(i=0;i<line_length;i++) {
         fprintf(output,SGE_INFOTEXT_UNDERLINE);
      }
      if (options->n != 1) {
         fprintf(output,"\n");
      }
   }

   sge_dstring_free(&line);  
   sge_dstring_free(&dash);
   sge_dstring_free(&line_buf_buffer);
}


char* sge_infotext_string_input_parsing(dstring* string_buffer,char* string) {
    
    dstring tmp_buf = DSTRING_INIT;

    sge_dstring_copy_string(string_buffer, sge_infotext_string_replace(&tmp_buf, string       , "\\n", "\n",0));
    sge_dstring_copy_string(string_buffer, sge_infotext_string_replace(&tmp_buf, (char*)sge_dstring_get_string(string_buffer), "\\r", " ",0));
    sge_dstring_copy_string(string_buffer, sge_infotext_string_replace(&tmp_buf, (char*)sge_dstring_get_string(string_buffer), "\\t", " ",0));

    sge_dstring_free(&tmp_buf);
    return (char*)sge_dstring_get_string(string_buffer);
}

char* sge_infotext_string_output_parsing(dstring* string_buffer,char* string) {

    dstring tmp_buf = DSTRING_INIT;


    sge_dstring_copy_string(string_buffer, sge_infotext_string_replace(&tmp_buf, string       , "\n", "\\n",0));

    sge_dstring_free(&tmp_buf);

    return (char*)sge_dstring_get_string(string_buffer);
}



char* sge_infotext_string_replace(dstring* tmp_buf, char* arg, char* what, char* with, int only_first) {
   int i;
   char* p1;
   char* p2;

   dstring arg_copy = DSTRING_INIT;

   sge_dstring_copy_string(&arg_copy,arg);
   p2 = (char*) sge_dstring_get_string(&arg_copy);

   p1 = strstr(p2, what);
   if (p1 == NULL) {
      sge_dstring_copy_string(tmp_buf,arg);
      sge_dstring_free(&arg_copy);
      return (char*) sge_dstring_get_string(tmp_buf);
   }
   sge_dstring_copy_string(tmp_buf,"");
   while (p1 != NULL) {
      *p1 = 0;
      sge_dstring_append(tmp_buf,p2);
      sge_dstring_append(tmp_buf,with);
      p2 = p1;
      for(i=0;i<strlen(what);i++) {
         p2++;
      }
      if (only_first == 1) {
         sge_dstring_append(tmp_buf,p2);
         sge_dstring_free(&arg_copy);
         return (char*) sge_dstring_get_string(tmp_buf);
      } 

      p1 = strstr(p2, what);
   }
   sge_dstring_append(tmp_buf,p2);
   sge_dstring_free(&arg_copy);
   return (char*) sge_dstring_get_string(tmp_buf);
}

int  sge_infotext_get_nr_of_substrings(char* buffer, char* substring) {
   char* p1 = NULL;
   char* buf = NULL;
   int nr = 0;

   buf=buffer;
   p1 = (char*)strstr(buf, substring);
   while (p1 != NULL) {
      buf = ++p1;
      p1 = (char*)strstr(buf, substring);
      nr++;
   }
   return nr;
}
 


void sge_infotext_welcome(void) {

   char* user = NULL;
   user = getenv("USER");
   if (user == NULL) {
      user = "(no USER environment variable set)";
   }
   
   printf("\nno l10n:\n");
   printf(SGE_INFOTEXT_TESTSTRING_S, user);
   printf("\nl10n:\n");
   printf(_(SGE_INFOTEXT_TESTSTRING_S), user);
}

void sge_infotext_usage(void) {
   printf("usage:\n");
   printf("infotext -help    : show help\n");
   printf("infotext -test    : test localization\n");
   printf("infotext -message : print empty po file string\n");
   printf("infotext -message-space : print po file string for test purposes\n");
   printf("infotext [-enu] [-D STRING] [-S COUNT] FORMAT_STRING ARGUMENTS\n\n");
   printf("FORMAT_STRING - printf format string\n");
   printf("ARGUMENTS     - printf arguments\n\n");
   printf("options:\n");   
   printf("  e - print to stderr\n");
   printf("  n - no new line\n");
   printf("  u - underline output\n");
   printf("  D - dash sign, e.g. -D \"->\"\n"); 
   printf("  S - nr of spaces, e.g. -S \"5\"\n\n");
   printf("infotext [-auto 0|1|true|false] [-wait] [-ask YES NO] [-def YES|NO]\n");
   printf("         FORMAT_STRING ARGUMENTS\n\n");
   printf("YES - user answer for exit status 0, e.g. -ask \"y\"\n");
   printf("NO  - user answer for exit status 1, e.g. -ask \"n\"\n\n");
   printf("options:\n");  
   printf("  auto - switch auto off/on [0|false/1|true], this will don't ask or wait,\n");
   printf("         just use default\n");
   printf("  wait - wait for return key\n");
   printf("  ask  - wait for user input\n");
   printf("  def  - default answer when user is just pressing RETURN\n\n");
   printf("used environment variables:\n");
   printf("SGE_INFOTEXT_MAX_COLUMN - column for word break (default 79)\n");
}


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
int main(
int argc,
char **argv 
) {
   int ret_val = 0;
   int show_help = 0;
   int do_test = 0;
   int do_message = 0;
   int do_message_space = 0;
   int do_ask = 0;
   int do_wait = 0;
   int do_auto = 0;
   int auto_option_used = 0;
   int do_def = 0;
   int args_ok = 1;
   int i = 0;
   int last_option = 0;
   int arg_start = 0;   
   int max_args = 0;
   int string_arguments = 0;
   int first_arg = 0;
   int real_args = 0;
   char* help_str = NULL;
   sge_infotext_options options;
   dstring buffer = DSTRING_INIT;
   dstring buffer2 = DSTRING_INIT;
   dstring sge_infotext_dash_buffer = DSTRING_INIT;
   dstring tmp_buf = DSTRING_INIT;

   DENTER_MAIN(TOP_LAYER, "sge_infotext");

#ifdef __SGE_COMPILE_WITH_GETTEXT__  
   install_language_func((gettext_func_type)        gettext,
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
   options.no = "";
   options.yes = "";
   options.def = "";

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
            break;
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
         if (strcmp(option,"wait") == 0) {
            do_wait = 1;
            continue;
         }
         if (strcmp(option,"auto") == 0) {
            do_auto = 1;
            auto_option_used=1;
            if ( (i+1) < argc) {
               i++;
               if (strcmp(argv[i], "false") == 0) {
                  do_auto = 0;
                  continue;
               } 
               if (strcmp(argv[i], "true") == 0) {
                  continue;
               } 

               if (atoi(argv[i]) == 0) {
                  do_auto = 0;
                  continue; 
               }
            } else {
               printf("no complete -auto option argument\n");
               args_ok = 0;
            }
            continue;
         }

         if ( strcmp(option,"ask") == 0) {
            do_ask = 1;
            if ( (i+2) < argc) {
               i++;
               options.yes = argv[i];
               i++;
               options.no  = argv[i];
            } else {
               printf("no complete -ask option arguments\n");
               args_ok = 0;
            }
            continue;
         }
         if ( strcmp(option,"def") == 0) {
            do_def = 1;
            if ( (i+1) < argc) {
               i++;
               options.def = argv[i];
            } else {
               printf("no complete -def option argument\n");
               args_ok = 0;
            }
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
      sge_infotext_usage();
      exit(10); 
   }

   if (do_test == 1) {
      sge_infotext_welcome();
      exit(0);
   }

   if (arg_start <= last_option && do_message == 0) {
      printf("no format string\n");
      args_ok = 0;
   }
   if (auto_option_used == 1 && do_def == 0 && do_ask == 1) {
      printf("used -auto -ask options without -def option\n");
      args_ok = 0;
   }   
   DPRINTF(("pass 1\n"));
   /* first pass - get number of %s arguments */
   sge_dstring_copy_string(&buffer,"");
   max_args = argc - arg_start;
   for(i=arg_start; i < (arg_start + max_args) ; i++) {
      char* arg = argv[i];
      sge_dstring_append(&buffer, arg);
      string_arguments = sge_infotext_get_nr_of_substrings((char*)sge_dstring_get_string(&buffer),"%s");
   }

   if (args_ok != 1) {
      printf("syntax error! Type sge_infotext -help for usage!\n");
      /* sge_infotext_usage(); */
      exit(10);
   }

   DPRINTF(("pass 2\n"));

   /* second pass - get format string */
   sge_dstring_copy_string(&buffer,"");

   for(i=arg_start; sge_infotext_get_nr_of_substrings((char*)sge_dstring_get_string(&buffer),"%s") < string_arguments ; i++) {
      char* arg = argv[i];
      if (i > arg_start) {
         sge_dstring_append(&buffer," ");
      }
      sge_dstring_append(&buffer, sge_infotext_string_input_parsing(&tmp_buf,arg));
   }
   first_arg = i;
   real_args = 0;
   for(i=first_arg; i < argc ; i++) {
      /* printf("args[%d] is: \"%s\"\n",i,argv[i]); */
      real_args++;
   }

   if (real_args < string_arguments) {
      printf("to less arguments\n");
      exit(10);
   }

   /* if we have to much args add the rest to the string buffer */
   while(real_args > string_arguments) {
      char* arg = argv[first_arg];
      char* hcp = NULL;

      hcp = (char*)sge_dstring_get_string(&buffer);
      if (strcmp(hcp,"") != 0 ) {
         if (arg[0] != ' ') {
            sge_dstring_append(&buffer," ");
         }
      }
      sge_dstring_append(&buffer,sge_infotext_string_input_parsing(&tmp_buf, arg));
      first_arg++;
      real_args--;
   }

   DPRINTF(("pass 3\n"));
   /* 3rd pass - localize format string */
   if (do_message == 1) {
      dstring help_buf = DSTRING_INIT;
      dstring help_buf2 = DSTRING_INIT;
      if (strlen(options.D) > 0) {
         printf("#\n# This is a (dash) sign, used for enumerations\n");
         printf("msgid  %s\n", sge_infotext_make_line_break(&help_buf2,options.D));
         if(do_message_space == 0) {
            DPRINTF(("do_message_space == 1\n"));
            printf("msgstr \"\"\n\n");
         } else {
            sge_infotext_build_test_msgstr(&help_buf, options.D);
            printf("msgstr %s\n\n",
                   sge_infotext_make_line_break(&help_buf2,
                                               (char*)sge_dstring_get_string(&help_buf)));  
         }
      }

      if (strlen(options.yes) > 0) {
         printf("#\n# This is used as shortcut for yes\n");
         printf("msgid  %s\n", sge_infotext_make_line_break(&help_buf2,options.yes));
         if(do_message_space == 0) {
            DPRINTF(("do_message_space == 1\n"));
            printf("msgstr \"\"\n\n");
         } else {
            sge_infotext_build_test_msgstr(&help_buf, options.yes);
            printf("msgstr %s\n\n",
                   sge_infotext_make_line_break(&help_buf2,
                                               (char*)sge_dstring_get_string(&help_buf)));
         }
      }
      if (strlen(options.no) > 0) {
         printf("#\n# This is used as shortcut for no\n");
         printf("msgid  %s\n", sge_infotext_make_line_break(&help_buf2,options.no));
         if(do_message_space == 0) {
            DPRINTF(("do_message_space == 1\n"));
            printf("msgstr \"\"\n\n");
         } else {
            sge_infotext_build_test_msgstr(&help_buf, options.no);
            printf("msgstr %s\n\n",
                   sge_infotext_make_line_break(&help_buf2,
                                               (char*)sge_dstring_get_string(&help_buf)));
         }
      }
      if (strlen(options.def) > 0) {
         printf("#\n# This is a default sign, must be shortcut for yes or no\n");
         printf("msgid  %s\n", sge_infotext_make_line_break(&help_buf2,options.def));
         if(do_message_space == 0) {
            DPRINTF(("do_message_space == 1\n"));
            printf("msgstr \"\"\n\n");
         } else {
            sge_infotext_build_test_msgstr(&help_buf, options.def);
            printf("msgstr %s\n\n",
                   sge_infotext_make_line_break(&help_buf2,
                                               (char*)sge_dstring_get_string(&help_buf)));
         }
      }


      printf("msgid  %s\n", 
             sge_infotext_make_line_break(&help_buf2, 
                                          sge_infotext_string_output_parsing(&tmp_buf,(char*)sge_dstring_get_string(&buffer))));
      if(do_message_space == 0) { 
         printf("msgstr \"\"\n");
      } else {
         sge_infotext_build_test_msgstr(&help_buf,
                                        sge_infotext_string_output_parsing(&tmp_buf,
                                                                           (char*)sge_dstring_get_string(&buffer)));
         printf("msgstr %s\n\n", sge_infotext_make_line_break(&help_buf2,
                                                             (char*)sge_dstring_get_string(&help_buf)));
      }
      printf("\n");
      sge_dstring_free(&sge_infotext_dash_buffer);
      sge_dstring_free(&tmp_buf);
      sge_dstring_free(&help_buf);
      sge_dstring_free(&help_buf2);

      exit(0);
   }

   help_str = (char*) sge_dstring_get_string(&buffer);
   sge_dstring_copy_string(&buffer2, (char*)_(help_str));
/*   printf("format string is: \"%s\"\n",buffer);
   printf("l10n string is: \"%s\"\n", buffer2);*/

   /* format output */
/*   printf("options: %d %d %d \"%s\" %d \"%s\" \"%s\" \"%s\"\n", options.e, options.n, options.u, options.D, options.S, options.yes , options.no ,options.def);
*/
  
   /* 4th pass - insert parameters */ 
   DPRINTF(("pass 4\n"));

   if (real_args > 0) {
      for(i=0;i<real_args;i++) {
/*      printf("argument[%d]: \"%s\"\n",i,argv[first_arg +i]); */
         sge_dstring_copy_string(&buffer, sge_infotext_string_replace(&tmp_buf, (char*)sge_dstring_get_string(&buffer2),"%s",argv[first_arg +i],1));
         sge_dstring_copy_dstring(&buffer2,&buffer); 
      }  
   } else {
      sge_dstring_copy_dstring(&buffer,&buffer2);
   }
      
   /* output */
   DPRINTF(("build_dash\n"));
   sge_dstring_append(&sge_infotext_dash_buffer,"");
   sge_infotext_build_dash(&sge_infotext_dash_buffer,&options);
   if (sge_dstring_get_string(&sge_infotext_dash_buffer) == NULL) {
      DPRINTF(("sge_infotext_dash_buffer is NULL"));
   }
   DPRINTF(("output\n"));
   if (do_ask != 1) {
      sge_infotext_format_output(&sge_infotext_dash_buffer,&options,(char*)sge_dstring_get_string(&buffer));
   }

   ret_val = 0;
   if (do_ask == 1 || do_wait == 1 ) {
      char input_buffer[2048];
      char* help = NULL;
      int done = 0;
      while (done == 0) {
         if (do_wait != 1) {
            sge_infotext_format_output(&sge_infotext_dash_buffer,&options,(char*)sge_dstring_get_string(&buffer));
         }
         if (do_auto == 0) {
            fgets(input_buffer, 2047, stdin);
            help = strstr(input_buffer, "\n");
            if (help != NULL) {
               *help = 0;
            }
         } else {
            strcpy(input_buffer,_(options.def));
         }
         if (strcmp(input_buffer,"") == 0) {
            if (do_wait == 1) {
               break;
            }
           
            strcpy(input_buffer,_(options.def));
         }

         if (strcmp(_(options.yes),input_buffer) == 0) {
            ret_val = 0;
            done = 1;
         }
         if (strcmp(_(options.no),input_buffer) == 0) {
            ret_val = 1;
            done = 1;
         }
         if (done != 1) {
            printf( SGE_INFOTEXT_ONLY_ALLOWED_SS , _(options.yes), _(options.no));
            if (do_auto != 0) {
               do_auto = 0;
            }
         }
      }
      printf("\n");
   }

   DPRINTF(("free strings\n"));
   sge_dstring_free(&sge_infotext_dash_buffer);
   sge_dstring_free(&tmp_buf);
   sge_dstring_free(&buffer);
   sge_dstring_free(&buffer2);
   DEXIT;
   return ret_val;
}
