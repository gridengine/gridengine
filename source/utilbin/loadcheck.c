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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "basis_types.h"
#include "sge_getloadavg.h"
#include "sge_nprocs.h"
#include "sge_arch.h"
#include "sge_loadmem.h"
#include "msg_utilbin.h"
#include "sge_language.h"
#include "sge_hostL.h"

void usage(void);
void print_mem_load(char *, char *, int, double, char*);

void usage()
{
   fprintf(stderr, "%s loadcheck [-int] [-loadval name]\n",MSG_UTILBIN_USAGE);
   exit(1);
}
   
int main(int argc, char *argv[])
{
   double avg[3];
   int loads;
	char *name;

#ifdef SGE_LOADMEM
   sge_mem_info_t mem_info;
#endif

#ifdef SGE_LOADCPU
	double total;	
#endif

   int i, pos = 0, print_as_int = 0, precision;
   char *m;

  

#ifdef __SGE_COMPILE_WITH_GETTEXT__   
   /* init language output for gettext() , it will use the right language */
   install_language_func((gettext_func_type)        gettext,
                         (setlocale_func_type)      setlocale,
                         (bindtextdomain_func_type) bindtextdomain,
                         (textdomain_func_type)     textdomain);
   sge_init_language(NULL,NULL);   
#endif /* __SGE_COMPILE_WITH_GETTEXT__  */


   for (i = 1; i < argc;) {
      if (!strcmp(argv[i], "-int"))
         print_as_int = 1;
      else if (!strcmp(argv[i], "-loadval")) {
         if (i + 1 < argc)
            pos=i+1;
         else
            usage();
         i++;
      }
      else
         usage();
      i++;
   }
   
   if (print_as_int) {
      m = "";
      precision = 0;
   }   
   else {
      m = "M";
      precision = 6;
   }   

   if ((pos && !strcmp("arch", argv[pos])) || !pos)
      printf("arch            %s\n", sge_arch());
      
   if ((pos && !strcmp("num_proc", argv[pos])) || !pos)    
      printf("num_proc        %d\n", sge_nprocs());

	 loads = sge_getloadavg(avg, 3);

   if (loads>0 && ((pos && !strcmp("load_short", argv[pos])) || !pos)) 
      printf("load_short      %.2f\n", avg[0]);
   if (loads>1 && ((pos && !strcmp("load_medium", argv[pos])) || !pos)) 
      printf("load_medium     %.2f\n", avg[1]);
   if (loads>2 && ((pos && !strcmp("load_long", argv[pos])) || !pos))
      printf("load_long       %.2f\n", avg[2]);

#ifdef SGE_LOADMEM
   /* memory load report */
   memset(&mem_info, 0, sizeof(sge_mem_info_t));
   if (loadmem(&mem_info)) {
      fprintf(stderr, MSG_SYSTEM_RETMEMORYINDICESFAILED );
      return 1;
   }

   if (pos)
      name = argv[pos];
   else
      name = NULL;
         
   print_mem_load(LOAD_ATTR_MEM_FREE, name, precision, mem_info.mem_free, m); 
   print_mem_load(LOAD_ATTR_SWAP_FREE, name, precision, mem_info.swap_free, m); 
   print_mem_load(LOAD_ATTR_VIRTUAL_FREE, name, precision, mem_info.mem_free  + mem_info.swap_free, m); 

   print_mem_load(LOAD_ATTR_MEM_TOTAL, name, precision, mem_info.mem_total, m); 
   print_mem_load(LOAD_ATTR_SWAP_TOTAL, name, precision, mem_info.swap_total, m); 
   print_mem_load(LOAD_ATTR_VIRTUAL_TOTAL, name, precision, mem_info.mem_total + mem_info.swap_total, m);

   print_mem_load(LOAD_ATTR_MEM_USED, name, precision, mem_info.mem_total - mem_info.mem_free, m); 
   print_mem_load(LOAD_ATTR_SWAP_USED, name, precision, mem_info.swap_total - mem_info.swap_free, m); 
   print_mem_load(LOAD_ATTR_VIRTUAL_USED, name, precision,(mem_info.mem_total + mem_info.swap_total) - 
                                          (mem_info.mem_free  + mem_info.swap_free), m); 
#  ifdef IRIX6
   print_mem_load(LOAD_ATTR_SWAP_USED, name, precision, mem_info.swap_rsvd, m); 
#  endif
#endif /* SGE_LOADMEM */

#ifdef SGE_LOADCPU
   loads = sge_getcpuload(&total);
   sleep(1);
   loads = sge_getcpuload(&total);
   if (loads != -1) {
      print_mem_load("cpu", name,  1, total, "%");
   }
#endif /* SGE_LOADCPU */

	return 0;
}

void print_mem_load(
char *name,
char *thisone,
int precision,
double value,
char *m 
) {

   if ((thisone && !strcmp(name, thisone)) || !thisone)
      printf("%-15s %.*f%s\n", name, precision, value, m);
}

