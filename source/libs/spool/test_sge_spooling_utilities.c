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

#include "cull.h"

#include "sge_string.h"
#include "sge_dstring.h"

#include "sge_all_listsL.h"
#include "sge_object.h"

#include "sge_spooling_utilities.h"
#include "read_write_queue.h"

int main(int argc, char *argv[])
{
   lListElem *queue, *copy;
   const lDescr *descr;
   spooling_field *fields;

   dstring queue_str = DSTRING_INIT;
   dstring copy_str  = DSTRING_INIT;

   lList *answer_list;

   int i;

   lInit(nmv);

   queue = create_template_queue();
   copy  = lCreateElem(QU_Type);

   descr = lGetElemDescr(queue);
   

/*    lWriteElemTo(queue, stdout); */

   for(i = 0; descr[i].mt != lEndT; i++) {
      int nm;
      const char *name;
      const char *value, *reread_value;

      nm = descr[i].nm;
      name = lNm2Str(nm);
      value = object_get_field_contents(queue, &answer_list, &queue_str, nm);
      reread_value = NULL;

      if(value != NULL) {
         if(!object_set_field_contents(copy, &answer_list, nm, value)) {
            fprintf(stderr, "setting value for field %s failed\n", name);
         } else {
            reread_value = object_get_field_contents(copy, &answer_list, &copy_str, nm);
         }
      }

#if 1
      printf("%s\t%s\t%s\n", name, 
             value == NULL ? "<null>" : value,
             reread_value == NULL ? "<null>" : reread_value);
#endif

      if(sge_strnullcmp(value, reread_value) != 0) {
         fprintf(stderr, "regression test for object_[gs]et_field_contents failed for attribute "SFQ": "SFQ" != "SFQ"\n", 
                 name, 
                 value != NULL ? value : "<null>", 
                 reread_value != NULL ? reread_value : "<null>");
      }
   }

   fields = spool_get_fields_to_spool(&answer_list, queue, &spool_config_instruction);
   printf("\nthe following fields will be spooled:");
   for(i = 0; fields[i].nm >= 0; i++) {
      printf(" %s", lNm2Str(fields[i].nm));
   }
   printf("\n");
   fields = spool_free_spooling_fields(fields);

   /* cleanup */
   queue = lFreeElem(queue);
   copy  = lFreeElem(copy);

   sge_dstring_free(&queue_str);
   sge_dstring_free(&copy_str);

   return EXIT_SUCCESS;
}
