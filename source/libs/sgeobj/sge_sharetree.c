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

#include <string.h>

#include "sge_string.h"
#include "sgermon.h"
#include "sge_log.h"
#include "cull_list.h"
#include "msg_common.h"
#include "sched/sge_support.h"
#include "uti/sge_unistd.h"

#include "sge_sharetree.h"

lList *Master_Sharetree_List = NULL;


/************************************************************************
   id_sharetree - set the sharetree node id
************************************************************************/
int id_sharetree(
lListElem *ep,
int id  
) {
   lListElem *cep = NULL;

   DENTER(TOP_LAYER, "id_sharetree");

   if (ep == NULL) {
      ERROR((SGE_EVENT, MSG_OBJ_NOSTREEELEM));
      SGE_EXIT(1);
   }
   
   lSetUlong(ep, STN_id, id++);

   /* handle the children */
   for_each(cep, lGetList(ep, STN_children)) {     
      id = id_sharetree(cep, id);
   }

   DEXIT;
   return id;
}  

/************************************************************************
  show_sharetree

  display a tree representation of sharetree 

 ************************************************************************/
int show_sharetree(
lListElem *ep,
char *indent 
) {
   lListElem *cep;
   FILE *fp = stdout;
   static int level = 0;
   int i;

   DENTER(TOP_LAYER, "show_sharetree");

   if (!ep) {
      DEXIT;
      return -1;
   }

   for (i=0;i<level;i++)
      fprintf(fp, "%s", indent ? indent : "");
   fprintf(fp, "%s="u32"\n", lGetString(ep, STN_name), 
            lGetUlong(ep, STN_shares));
   for_each(cep, lGetList(ep, STN_children)) {
      level++;
      show_sharetree(cep, "   ");
      level--;
   }   

   DEXIT;
   return 0;
}

/************************************************************************
  show_sharetree_path

  display a path representation of sharetree 

 ************************************************************************/
int show_sharetree_path(
lListElem *root,
const char *path 
) {
   lListElem *cep;
   lListElem *node;
   FILE *fp = stdout;
   ancestors_t ancestors;
   int i;
   dstring sb = DSTRING_INIT;
 
   DENTER(TOP_LAYER, "show_sharetree_path");
 
   if (!root) {
      DEXIT;
      return 1;
   }
 
   memset(&ancestors, 0, sizeof(ancestors));
   if (!strcmp(path, "/")) {
      node = root;
   }
   else {
      node = search_named_node_path(root, path, &ancestors);
   }
 
   if (node) {
      for(i=0; i<ancestors.depth; i++)
         fprintf(fp, "/%s", lGetString(ancestors.nodes[i], STN_name));
      if (!strcmp(path, "/"))
         fprintf(fp, "/="u32"\n", lGetUlong(node, STN_shares));
      else
         fprintf(fp, "="u32"\n", lGetUlong(node, STN_shares));
      free_ancestors(&ancestors);
      for_each(cep, lGetList(node, STN_children)) {

         if (!strcmp(path, "/"))
            sge_dstring_sprintf(&sb, "/%s", lGetString(cep, STN_name));
         else
            sge_dstring_sprintf(&sb, "%s/%s", path,
                                 lGetString(cep, STN_name));
         show_sharetree_path(root, sge_dstring_get_string(&sb));
      }
   }
   else {
      fprintf(stderr, MSG_TREE_UNABLETOLACATEXINSHARETREE_S, path);
   }
 
   sge_dstring_free(&sb);
   DEXIT;
   return 0;
}                                                                               

/***************************************************
 Generate a Template for a sharetreenode
 ***************************************************/
lListElem *getSNTemplate(void)
{
   lListElem *ep;

   DENTER(TOP_LAYER, "getSNTemplate");

   ep = lCreateElem(STN_Type);
   lSetString(ep, STN_name, "template");
   lSetUlong(ep, STN_type, 0);
   lSetUlong(ep, STN_id, 0);
   lSetUlong(ep, STN_shares, 0);
   lSetList(ep, STN_children, NULL);

   DEXIT;
   return ep;
}
