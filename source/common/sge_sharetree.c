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
/*
   This is the module for handling the SGE sharetree
   We save the sharetree to <spool>/qmaster/sharetree
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include "sgermon.h"
#include "sge_eventL.h"
#include "sge_answerL.h"
#include "sge_confL.h"
#include "sge_usageL.h"
#include "sge_share_tree_nodeL.h"
#include "sge_sharetree.h"
#include "cull_parse_util.h"
#include "sge_m_event.h"
#include "sge_log.h"
#include "sge_exit.h"
#include "sge_string_append.h"
#include "scheduler.h"                                                                
#include "sgeee.h"                                                                
#include "sge_support.h"                                                                
#include "msg_common.h"
#include "sge_spoolmsg.h"
#include "sge_feature.h"
#include "sge_stdio.h"

static lListElem *search_unspecified_node(lListElem *ep);
static int id_sharetree(lListElem *ep, int id);
static lListElem *search_nodeSN(lListElem *ep, u_long32 id);


/************************************************************************
  write_sharetree

  write a sharetree element human readable to file or fp. 
  If fname == NULL use fpout for writing.
  if spool!=0 store all information else store user relevant information
  sharetree file looks like this:

  id=1             when spooled 
  name=trulla
  type=1
  version=178      when spooled and root_node == true 
  shares=22
  childnodes=2,3   when spooled

  If the file contains a whole tree repetitions follow. "childnodes" is used
  as delimiter.

 ************************************************************************/
int write_sharetree(
lList **alpp,
lListElem *ep,
char *fname,
FILE *fpout,
int spool,
int recurse,        /* if =1 recursive write */
int root_node 
) {
   FILE *fp; 
   intprt_type print_elements[] = { STN_id, 0 };
   const char *delis[] = {":", ",", NULL};
   lListElem *cep;
   int i;

   DENTER(TOP_LAYER, "_write_sharetree");

   if (!ep) {
      if (!alpp) {
         ERROR((SGE_EVENT, MSG_OBJ_NOSTREEELEM));
         SGE_EXIT(1);
      } 
      else {
         sge_add_answer(alpp, MSG_OBJ_NOSTREEELEM, STATUS_EEXIST, 0);
         DEXIT;
         return -1;
      }
   }

   if (fname) {
      if (!(fp = fopen(fname, "w"))) {
         ERROR((SGE_EVENT, MSG_FILE_NOOPEN_SS, fname, strerror(errno)));
         if (!alpp) {
            SGE_EXIT(1);
         } 
         else {
            sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
            DEXIT;
            return -1;
         }
      }
   }
   else
      fp = fpout;

   if (spool && root_node && sge_spoolmsg_write(fp, COMMENT_CHAR,
             feature_get_product_name(FS_VERSION)) < 0) {
      goto FPRINTF_ERROR;
   }  

   if (recurse) 
      FPRINTF((fp, "id=" u32 "\n", lGetUlong(ep, STN_id)));
  
   if (root_node) {
      id_sharetree(ep, 0);
      if (spool)
         FPRINTF((fp, "version=" u32 "\n", lGetUlong(ep, STN_version)));
   }

   FPRINTF((fp, "name=%s\n", lGetString(ep, STN_name)));
   FPRINTF((fp, "type=" u32 "\n", lGetUlong(ep, STN_type)));
   FPRINTF((fp, "shares=" u32 "\n", lGetUlong(ep, STN_shares)));
   if (recurse) {
      int fret;

      FPRINTF((fp, "childnodes="));
      fret = uni_print_list(fp, NULL, 0, lGetList(ep, STN_children), print_elements, 
                     delis, 0);
      if (fret < 0) {
         sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
         DEXIT;
         return -1;
      }
      FPRINTF((fp, "\n"));
   }

   /* if this is a recursive call proceed with the children */
   if (recurse) {
      for_each(cep, lGetList(ep, STN_children)) {     
         if ((i = write_sharetree(alpp, cep, NULL, fp, spool, recurse, 0))) {
            DEXIT;
            return i;
         }
      }
   }

   if (fname)
      fclose(fp);

   DEXIT;
   return 0;
FPRINTF_ERROR:
   if (fname)
      fclose(fp); 
   sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
   DEXIT;
   return -1;
}

/***************************************************
 Read sharetree

 Return 
   NULL if EOF
   NULL and errstr set if error
 ***************************************************/
lListElem *read_sharetree(
char *fname,
FILE *fp,
int spool,      /* from spooled file (may contain additional fields */
char *errstr,
int recurse,
lListElem *rootelem     /* in case of a recursive call this is the root elem
                           of the tree we already read in */
) {
   intprt_type print_elements[] = { STN_id, 0 };
   int complete=0;      /* set to 1 when the node is completly read */
   lListElem *ep=NULL, *unspecified;
   lList *child_list;
   static char buf[10000];
   char *name, *val;
   u_long32 id;
   int i, line=0;

   DENTER(TOP_LAYER, "read_sharetree");

   errstr[0] = '\0';

   if (!fp && !(fp = fopen(fname, "r"))) {
      sprintf(errstr, MSG_FILE_NOOPEN_SS, fname, strerror(errno));
      DEXIT;
      return NULL;
   }

   while (!complete && fgets(buf, sizeof(buf), fp)) {
      line++;

      if (buf[0] == '\0' || buf[0] == '#' || buf[0] == '\n')
         continue;

      name = strtok(buf, "=\n");
      val = strtok(NULL, "\n");

      if (!strcmp(name, "name")) {
         if (!ep) {
            sprintf(errstr, MSG_STREE_UNEXPECTEDNAMEFIELD);
            DEXIT;
            return NULL;
         }
         if ((lGetString(ep, STN_name))) {
            sprintf(errstr, MSG_STREE_NAMETWICE_I, line);
            DEXIT;
            return NULL;
         }
         lSetString(ep, STN_name, val);
      } else if (!strcmp(name, "id")) {
         id = strtol(val, NULL, 10);
         /* if there is a rootelem given we search the tree for our id.
            Our element is already created. */
         if (rootelem) {
            ep = search_nodeSN(rootelem, id);
            if (!ep) {
               sprintf(errstr, MSG_STREE_NOFATHERNODE_U, u32c(id));
               DEXIT;
               return NULL;
            }
         }
         else
            ep = lCreateElem(STN_Type);
         lSetUlong(ep, STN_id, id);
         lSetString(ep, STN_name, NULL);
         lSetUlong(ep, STN_type, 0);
         lSetUlong(ep, STN_shares, 0);
         lSetList(ep, STN_children, NULL); 

      } else if (!strcmp(name, "type")) {
         if (!ep) {
            sprintf(errstr, MSG_STREE_UNEXPECTEDTYPEFIELD);
            DEXIT;
            return NULL;
         }
         if ((lGetUlong(ep, STN_type))) {
            sprintf(errstr, MSG_STREE_TYPETWICE_I, line);
            DEXIT;
            return NULL;
         }
         lSetUlong(ep, STN_type, strtol(val, NULL, 10));
      } else if (!strcmp(name, "version")) {
         if (!rootelem && !spool && !ep) {
            sprintf(errstr, MSG_STREE_UNEXPECTEDVERSIONFIELD);
            DEXIT;
            return NULL;
         }
         if ((lGetUlong(ep, STN_version))) {
            sprintf(errstr, MSG_STREE_TYPETWICE_I, line);
            DEXIT;
            return NULL;
         }
         lSetUlong(ep, STN_version, strtol(val, NULL, 10));
      } else if (!strcmp(name, "shares")) {
         if (!ep) {
            sprintf(errstr, MSG_STREE_UNEXPECTEDSHARESFIELD);
            DEXIT;
            return NULL;
         }
         if ((lGetUlong(ep, STN_shares))) {
            sprintf(errstr, MSG_STREE_SHARESTWICE_I, line);
            DEXIT;
            return NULL;
         }
         lSetUlong(ep, STN_shares, strtol(val, NULL, 10));
      } else if (!strcmp(name, "childnodes")) {
         if (!ep) {
            sprintf(errstr, MSG_STREE_UNEXPECTEDCHILDNODEFIELD);
            DEXIT;
            return NULL;
         }
         if ((lGetList(ep, STN_children))) {
            sprintf(errstr, MSG_STREE_CHILDNODETWICE_I, line);
            DEXIT;
            return NULL;
         }
         complete = 1;
         child_list = NULL;
         i = cull_parse_simple_list(val, &child_list, "childnode list",
                                    STN_Type, print_elements);
         if (i) {
            lFreeElem(ep);
            strcpy(errstr, MSG_STREE_NOPARSECHILDNODES);
            DEXIT;
            return NULL;
         }
         lSetList(ep, STN_children, child_list);
         
         if (recurse) {
            if (!read_sharetree(NULL, fp, spool, errstr, recurse,
                                rootelem?rootelem:ep) &&
                errstr[0]) {
               if (!rootelem) { /* we are at the root level */
                  lFreeElem(ep);
               }
               DEXIT;
               return NULL;
            }
         }
      }   
      else {
         /* unknown line */
         sprintf(errstr, MSG_STREE_NOPARSELINE_I, line);
         DEXIT;
         return NULL;
      }
   }

   DTRACE;

   /* check for a reference to a node which is not specified */
   if (!rootelem && (unspecified = search_unspecified_node(ep))) {
      sprintf(errstr, MSG_STREE_NOVALIDNODEREF_U, u32c(lGetUlong(unspecified, STN_id)));

      lFreeElem(ep);
      DEXIT;
      return NULL;
   }

   if (!rootelem && line<=1) {
      strcpy(errstr, MSG_FILE_FILEEMPTY);
   }

   DEXIT;
   return ep;
}


/***************************************************
 Generate a Template for a sharetreenode
 ***************************************************/
lListElem *getSNTemplate()
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

/*****************************************************
 search for a node which is not specified
 *****************************************************/ 
static lListElem *search_unspecified_node(
lListElem *ep   /* root of the tree */
) {
   lListElem *cep, *ret;

   DENTER(TOP_LAYER, "check_unspecified_nodes");

   if (!ep) {
      DEXIT;
      return NULL;
   }

   for_each(cep, lGetList(ep, STN_children)) {
      if ((ret = search_unspecified_node(cep))) {
         DEXIT;
         return ret;
      }   
   }

   if (!lGetString(ep, STN_name)) {
      DEXIT;
      return ep;         /* no name filled in -> unspecified */
   }
   
   DEXIT;
   return NULL;
}


/********************************************************
 Search a share tree node with a given id in a share tree
 ********************************************************/
static lListElem *search_nodeSN(
lListElem *ep,  /* root of the tree */
u_long32 id 
) {
   lListElem *cep, *fep;

   DENTER(TOP_LAYER, "search_nodeSN");

   if (!ep) {
      DEXIT;
      return NULL;
   }

   if (lGetUlong(ep, STN_id) == id) {
      DEXIT;
      return ep;
   }

   for_each(cep, lGetList(ep, STN_children)) {
      if ((fep = search_nodeSN(cep, id))) {
         DEXIT;
         return fep;
      }
   }
      
   DEXIT;
   return NULL;
}


/************************************************************************
   id_sharetree - set the sharetree node id
************************************************************************/
static int id_sharetree(
lListElem *ep,
int id  
) {
   lListElem *cep;

   DENTER(TOP_LAYER, "id_sharetree");

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
         StringBufferT sb = {NULL, 0};
         if (!strcmp(path, "/"))
            sge_string_printf(&sb, "/%s", lGetString(cep, STN_name));
         else
            sge_string_printf(&sb, "%s/%s", path,
                                 lGetString(cep, STN_name));
         show_sharetree_path(root, sb.s);
         free(sb.s);
      }
   }
   else {
      fprintf(stderr, MSG_TREE_UNABLETOLACATEXINSHARETREE_S, path);
   }
 
   DEXIT;                                                                          return 0;
}                                                                               
