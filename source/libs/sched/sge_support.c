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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <math.h>

#ifndef NO_SGE_COMPILE_DEBUG   
#   define NO_SGE_COMPILE_DEBUG
#endif

#include "sge_all_listsL.h"
#include "sge_gdi_intern.h"
#include "sge_copy_append.h"
#include "commlib.h"
#include "sgermon.h"
#include "complex.h"
#include "sge_prognames.h"
#include "sge_c_event.h"
#include "sge_schedd.h"
#include "sge_time.h"
#include "sge_orders.h"
#include "sge_job_schedd.h"
#include "schedd_conf.h"
#include "scheduler.h"
#include "sgeee.h"
#include "sge_support.h"
#include "sgeee.h"
#include "sge_schedconfL.h"
#include "sge_jobL.h"
#include "sge_userprjL.h"
#include "sge_share_tree_nodeL.h"
#include "sge_usageL.h"
#include "sge_hostL.h"
#include "sge_queueL.h"
#include "sge_usersetL.h"


const long sge_usage_interval = SGE_USAGE_INTERVAL;
static double sge_decay_rate;
static double sge_decay_constant;

/*--------------------------------------------------------------------
 * decay_usage - decay usage for the passed usage list
 *--------------------------------------------------------------------*/

static void decay_usage(lList *usage_list, u_long curr_time, u_long usage_time_stamp);

static void
decay_usage(
lList *usage_list,
u_long curr_time,
u_long usage_time_stamp 
) {
   lListElem *usage;
   static int ua_value_pos = -1;

   if (ua_value_pos == -1) {
      lListElem *ua_elem = lCreateElem(UA_Type);
      ua_value_pos = lGetPosViaElem(ua_elem, UA_value);
      lFreeElem(ua_elem);
   }

   if (usage_list) {

      double decay;
      if (curr_time > usage_time_stamp) {
         decay = pow(sge_decay_constant,
                     (double)(curr_time - usage_time_stamp) /
                     (double)sge_usage_interval);
         for_each(usage, usage_list)
            lSetPosDouble(usage, ua_value_pos,
                          lGetPosDouble(usage, ua_value_pos) * decay);
      }

   }
   return;
}

/*--------------------------------------------------------------------
 * decay_userprj_usage - decay usage for the passed user/project object
 *--------------------------------------------------------------------*/

void
decay_userprj_usage(
lListElem *userprj,
u_long seqno,
u_long curr_time 
) {
   u_long usage_time_stamp;
   static int up_usage_seqno_pos = -1;
   static int up_usage_time_stamp_pos = -1;
   static int up_usage_pos = -1;
   static int up_project_pos = -1;
   static int upp_usage_pos = -1;

   if (up_usage_seqno_pos == -1) {
      lListElem *up_elem = lCreateElem(UP_Type);
      lListElem *upp_elem = lCreateElem(UPP_Type);
      up_usage_seqno_pos = lGetPosViaElem(up_elem, UP_usage_seqno);
      up_usage_time_stamp_pos = lGetPosViaElem(up_elem, UP_usage_time_stamp);
      up_usage_pos = lGetPosViaElem(up_elem, UP_usage);
      up_project_pos = lGetPosViaElem(up_elem, UP_project);
      upp_usage_pos = lGetPosViaElem(upp_elem, UPP_usage);
      lFreeElem(up_elem);
      lFreeElem(upp_elem);
   }

   if (userprj && seqno != lGetPosUlong(userprj, up_usage_seqno_pos)) {

   /*-------------------------------------------------------------
    * Note: In order to decay usage once per decay interval, we
    * keep a time stamp in the user/project of when it was last
    * decayed and then apply the approriate decay based on the time
    * stamp. This allows the usage to be decayed on the scheduling
    * interval, even though the decay interval is different than
    * the scheduling interval.
    *-------------------------------------------------------------*/

      usage_time_stamp = lGetPosUlong(userprj, up_usage_time_stamp_pos);

      if (usage_time_stamp > 0) {
         lListElem *upp;

         decay_usage(lGetPosList(userprj, up_usage_pos), curr_time,
                     usage_time_stamp);

         for_each(upp, lGetPosList(userprj, up_project_pos))
            decay_usage(lGetPosList(upp, upp_usage_pos), curr_time,
                        usage_time_stamp);

      }

      lSetPosUlong(userprj, up_usage_time_stamp_pos, curr_time);
      if (seqno != (u_long) -1)
	 lSetPosUlong(userprj, up_usage_seqno_pos, seqno);

   }

   return;
}


/*--------------------------------------------------------------------
 * calculate_decay_constant - calculates decay rate and constant based
 * on the decay half life and usage interval
 *--------------------------------------------------------------------*/


void
calculate_decay_constant(
int halftime 
) {
   if (halftime == 0) {
      sge_decay_rate = 0;
      sge_decay_constant = 1.0;
   } else {
      halftime = halftime * 60 * 60; /* convert to seconds */
      sge_decay_rate = - log(0.5) / halftime;
      sge_decay_constant = 1 - (sge_decay_rate * sge_usage_interval);
   }
   return;
}


/*--------------------------------------------------------------------
 * sge_for_each_node - visit each node and call the supplied function
 * until a non-zero return code is returned.
 *--------------------------------------------------------------------*/

int
sge_for_each_share_tree_node(
lListElem *node,
sge_node_func_t func,
void *ptr 
) {
   int retcode=0;
   lList *children;
   lListElem *child_node;
   static int sn_children_pos = -1;

   if (!node)
      return 0;

   if (sn_children_pos == -1)
      sn_children_pos = lGetPosViaElem(node, STN_children);

   if ((retcode = (*func)(node, ptr)))
      return retcode;

   if ((children = lGetPosList(node, sn_children_pos))) {
      for_each(child_node, children) {
         if ((retcode = sge_for_each_share_tree_node(child_node, func, ptr)))
            break;
      }
   }

   return retcode;
}


/*--------------------------------------------------------------------
 * zero_node_fields - zero out the share tree node fields that are 
 * passed to the qmaster from schedd and are displayed at qmon
 *--------------------------------------------------------------------*/

int
sge_zero_node_fields(
lListElem *node,
void *ptr 
) {
   static int sn_m_share_pos = -1;
   static int sn_adjusted_current_proportion_pos = -1;
   static int sn_job_ref_count_pos = -1;

   if (sn_m_share_pos == -1) {
      sn_m_share_pos = lGetPosViaElem(node, STN_m_share);
      sn_adjusted_current_proportion_pos =
            lGetPosViaElem(node, STN_adjusted_current_proportion);
      sn_job_ref_count_pos = lGetPosViaElem(node, STN_job_ref_count);
   }

   lSetPosDouble(node, sn_m_share_pos, 0);
   lSetPosDouble(node, sn_adjusted_current_proportion_pos, 0);
   lSetPosUlong(node, sn_job_ref_count_pos, 0);
   return 0;
}


/*--------------------------------------------------------------------
 * sge_zero_node_fields - zero out the share tree node fields that are 
 * passed to the qmaster from schedd and are displayed at qmon
 *--------------------------------------------------------------------*/

int
sge_init_node_fields(
lListElem *root 
) {
   return sge_for_each_share_tree_node(root, sge_zero_node_fields, NULL);
}


/*--------------------------------------------------------------------
 * sge_calc_node_usage - calculate usage for this share tree node
 * and all descendant nodes.
 *--------------------------------------------------------------------*/

double
sge_calc_node_usage(
lListElem *node,
lList *user_list,
lList *project_list,
lList *config_list,
u_long curr_time,
char *projname 
) {
   double usage_value = 0;
   int include_child_usage = 1;
   lListElem *child_node;
   lList *children;
   lListElem *userprj = NULL;
   lList *usage_weight_list=NULL, *usage_list=NULL;
   lListElem *usage_weight, *config, *usage_elem;
   double sum_of_usage_weights = 0;
   char *usage_name;
   static int sn_children_pos = -1;
   static int sn_combined_usage_pos = -1;
   static int sn_name_pos = -1;
   static int ua_name_pos = -1;
   static int ua_value_pos = -1;
   static int sc_usage_weight_list_pos = -1;
   static int up_usage_pos = -1;

   DENTER(TOP_LAYER, "sge_calc_node_usage");

   if (sn_children_pos == -1) {
      lListElem *ua_elem = lCreateElem(UA_Type);
      lListElem *sc_elem = lCreateElem(SC_Type);
      lListElem *up_elem = lCreateElem(UP_Type);
      sn_children_pos = lGetPosViaElem(node, STN_children);
      sn_combined_usage_pos = lGetPosViaElem(node, STN_combined_usage);
      sn_name_pos = lGetPosViaElem(node, STN_name);
      ua_name_pos = lGetPosViaElem(ua_elem, UA_name);
      ua_value_pos = lGetPosViaElem(ua_elem, UA_value);
      sc_usage_weight_list_pos = lGetPosViaElem(sc_elem, SC_usage_weight_list);
      up_usage_pos = lGetPosViaElem(up_elem, UP_usage);
      lFreeElem(ua_elem);
      lFreeElem(sc_elem);
      lFreeElem(up_elem);
   }

   children = lGetPosList(node, sn_children_pos);
   if (!children) {

      if (projname) {

         /*-------------------------------------------------------------
          * Get usage from project usage sub-list in user object
          *-------------------------------------------------------------*/

         if ((userprj = lGetElemStr(user_list, UP_name,
                                      lGetPosString(node, sn_name_pos)))) {

            lList *projects = lGetList(userprj, UP_project);
            lListElem *upp;

            if (projects)
               if ((upp=lGetElemStr(projects, UPP_name, projname)))
                  usage_list = lGetList(upp, UPP_usage);

         }

      } else {

         /*-------------------------------------------------------------
          * Get usage directly from corresponding user or project object
          *-------------------------------------------------------------*/

         if ((userprj = lGetElemStr(user_list, UP_name,
                                      lGetPosString(node, sn_name_pos)))) {

            usage_list = lGetList(userprj, UP_usage);

         } else if ((userprj = lGetElemStr(project_list, UP_name,
                              lGetPosString(node, sn_name_pos)))) {

            usage_list = lGetList(userprj, UP_usage);

         }

      }

   } else {

      /*-------------------------------------------------------------
       * If this is a project node, then return the project usage
       * rather than the children's usage
       *-------------------------------------------------------------*/

      if (!projname) {
         if ((userprj = lGetElemStr(project_list, UP_name,
                                lGetPosString(node, sn_name_pos)))) {
            include_child_usage = 0;
            usage_list = lGetList(userprj, UP_usage);
            projname = lGetString(userprj, UP_name);
         }
      }

   }

   if (usage_list) {

      /*-------------------------------------------------------------
       * Decay usage
       *-------------------------------------------------------------*/

      if (curr_time)
         decay_userprj_usage(userprj, -1, curr_time);

      /*-------------------------------------------------------------
       * Sum usage weighting factors
       *-------------------------------------------------------------*/

      if ((config = lFirst(config_list))) {
         usage_weight_list = lGetPosList(config, sc_usage_weight_list_pos);
         if (usage_weight_list) {
            for_each(usage_weight, usage_weight_list)
               sum_of_usage_weights +=
                     lGetPosDouble(usage_weight, ua_value_pos);
         }
      }

      /*-------------------------------------------------------------
       * Combine user/project usage based on usage weighting factors
       *-------------------------------------------------------------*/

      if (usage_weight_list) {
         for_each(usage_elem, usage_list) {
            usage_name = lGetPosString(usage_elem, ua_name_pos);
            usage_weight = lGetElemStr(usage_weight_list, UA_name,
                                       usage_name);
            if (usage_weight && sum_of_usage_weights>0) {
               usage_value += lGetPosDouble(usage_elem, ua_value_pos) *
                  (lGetPosDouble(usage_weight, ua_value_pos) /
                   sum_of_usage_weights);
            }
         }
      }
   }

   if (children) {
      double child_usage = 0;

      /*-------------------------------------------------------------
       * Sum child usage
       *-------------------------------------------------------------*/

      for_each(child_node, children) {
         child_usage += sge_calc_node_usage(child_node, user_list,
                                            project_list, config_list,
                                            curr_time, projname);
      }

      if (include_child_usage)
         usage_value += child_usage;
#ifdef notdef
      else {
         lListElem *default_node;
         if ((default_node=search_named_node(node, "default")))
            lSetPosDouble(default_node, sn_combined_usage_pos,
               MAX(usage_value - child_usage, 0));
      }
#endif
   }

   /*-------------------------------------------------------------
    * Set combined usage in the node
    *-------------------------------------------------------------*/

   lSetPosDouble(node, sn_combined_usage_pos, usage_value);

   DEXIT;
   return usage_value;
}


/*--------------------------------------------------------------------
 * sge_calc_node_proportions - calculate share tree node proportions
 * for this node and all descendant nodes.
 *--------------------------------------------------------------------*/


void
sge_calc_node_proportion(
lListElem *node,
double total_usage 
) {
   lList *children;
   lListElem *child_node;
   static int sn_children_pos = -1;
   static int sn_actual_proportion_pos = -1;
   static int sn_combined_usage_pos = -1;

   DENTER(TOP_LAYER, "sge_calc_node_proportions");

   if (sn_children_pos == -1) {
      sn_children_pos = lGetPosViaElem(node, STN_children);
      sn_actual_proportion_pos = lGetPosViaElem(node, STN_actual_proportion);
      sn_combined_usage_pos = lGetPosViaElem(node, STN_combined_usage);
   }

   /*-------------------------------------------------------------
    * Calculate node proportions for all children
    *-------------------------------------------------------------*/

   if ((children = lGetPosList(node, sn_children_pos))) {
      for_each(child_node, children) {
         sge_calc_node_proportion(child_node, total_usage);
      }
   }  

   /*-------------------------------------------------------------
    * Set proportion in the node
    *-------------------------------------------------------------*/

   if (total_usage == 0)
      lSetPosDouble(node, sn_actual_proportion_pos, 0);
   else 
      lSetPosDouble(node, sn_actual_proportion_pos,
         lGetPosDouble(node, sn_combined_usage_pos) / total_usage);

   DEXIT;
   return;
}


/*--------------------------------------------------------------------
 * sge_calc_share_tree_proportions - calculate share tree node
 * usage and proportions.
 *
 * Sets STN_combined_usage and STN_actual_proportion in each share
 * tree node contained in the passed-in share_tree argument.
 *--------------------------------------------------------------------*/

void
_sge_calc_share_tree_proportions(lList *share_tree, lList *user_list,
            lList *project_list, lList *config_list, u_long curr_time)
{
   lListElem *root;
   double total_usage;

   DENTER(TOP_LAYER, "sge_calc_share_tree_proportions");

   if (!share_tree || !config_list || !((root=lFirst(share_tree)))) {
      DEXIT;
      return;
   }

   calculate_decay_constant(lGetUlong(lFirst(config_list), SC_halftime));

   total_usage = sge_calc_node_usage(root,
                                     user_list,
                                     project_list,
                                     config_list,
                                     curr_time,
				     NULL);

   sge_calc_node_proportion(root, total_usage);

   DEXIT;
   return;
}


void
sge_calc_share_tree_proportions(lList *share_tree, lList *user_list,
            lList *project_list, lList *config_list)
{
   _sge_calc_share_tree_proportions(share_tree, user_list, project_list,
                                    config_list, sge_get_gmt());
   return;
}


/********************************************************
 Search the share tree for the node corresponding to the
 user / project combination
 ********************************************************/
lListElem *search_userprj_node(
lListElem *ep,  /* root of the tree */
char *username,
char *projname,
lListElem **pep  /* parent of found node */
) {
   lListElem *cep, *fep;
   static int sn_children_pos = -1;
   static int sn_name_pos = -1;
   static int sn_project_pos = -1;
   char *nodename;
   lList *children;

   DENTER(TOP_LAYER, "search_userprj_node");

   if (!ep || (!username && !projname)) {
      DEXIT;
      return NULL;
   }

   if (sn_name_pos == -1) {
      sn_children_pos = lGetPosViaElem(ep, STN_children);
      sn_name_pos = lGetPosViaElem(ep, STN_name);
      sn_project_pos = lGetPosViaElem(ep, STN_project);
   }

   nodename = lGetPosString(ep, sn_name_pos);

   /*
    * skip project nodes which don't match
    */

   if (lGetPosUlong(ep, sn_project_pos) &&
       (!projname || strcmp(nodename, projname))) {
      DEXIT;
      return NULL;
   }

   children = lGetPosList(ep, sn_children_pos);

   /*
    * if project name is supplied, look for the project
    */

   if (projname) {

      if (strcmp(nodename, projname) == 0) {

         /*
          * We have found the project node, now find the user node
          * within the project sub-tree. If there are no children,
          * return the project node.
          */

         if (!children) {
            DEXIT;
            return ep;
         }

         for_each(cep, children) {
            if ((fep = search_userprj_node(cep, username, NULL, pep))) {
               if (pep && (cep == fep))
                  *pep = ep;
               DEXIT;
               return fep;
            }
         }

         /*
          * The user is not in the project sub-tree, so look for
          * a user node called "default", which can be used to
          * specify shares for users not in the share tree.
          */

         for_each(cep, children) {
            if ((fep = search_userprj_node(cep, "default", NULL, pep))) {
               if (pep && (cep == fep))
                  *pep = ep;
               DEXIT;
               return fep;
            }
         }

         /*
          * user was not found, fall thru and return NULL
          */

      } else {

         /* 
          * search the child nodes for the project
          */

         for_each(cep, children) {
            if ((fep = search_userprj_node(cep, username, projname, pep))) {
               if (pep && (cep == fep))
                  *pep = ep;
               DEXIT;
               return fep;
            }
         }

         /*
          * project was not found, fall thru and return NULL
          */

      }

   } else {

      if (strcmp(nodename, username) == 0) {
         DEXIT;
         return ep;
      }

      /*
       * no project name supplied, so search for child node
       */

      for_each(cep, children) {
         if ((fep = search_userprj_node(cep, username, projname, pep))) {
            if (pep && (cep == fep))
               *pep = ep;
            DEXIT;
            return fep;
         }
      }

      /*
       * user was not found, fall thru and return NULL
       */

   }

   DEXIT;
   return NULL;
}



/********************************************************
 Search for a share tree node with a given name in a
 share tree
 ********************************************************/
lListElem *search_named_node(
lListElem *ep,  /* root of the tree */
char *name 
) {
   lListElem *cep, *fep;
   static int sn_children_pos = -1;
   static int sn_name_pos = -1;

   DENTER(TOP_LAYER, "search_named_node");

   if (!ep || !name) {
      DEXIT;
      return NULL;
   }

   if (sn_name_pos == -1) {
      sn_children_pos = lGetPosViaElem(ep, STN_children);
      sn_name_pos = lGetPosViaElem(ep, STN_name);
   }

   if (strcmp(lGetPosString(ep, sn_name_pos), name) == 0) {
      DEXIT;
      return ep;
   }

   for_each(cep, lGetPosList(ep, sn_children_pos)) {
      if ((fep = search_named_node(cep, name))) {
         DEXIT;
         return fep;
      }
   }
      
   DEXIT;
   return NULL;
}


/********************************************************
 Free internals of ancestors structure
 ********************************************************/
void free_ancestors(
ancestors_t *ancestors 
) {
   if (ancestors && ancestors->nodes) {
      free(ancestors->nodes);
      ancestors->nodes = NULL;
   }
}


/********************************************************
 Search for a share tree node with a given path in a
 share tree
 ********************************************************/
static lListElem *search_by_path(lListElem *ep, char *name, char *path, int delim, ancestors_t *ancestors, int depth);

static lListElem *search_by_path(
lListElem *ep,  /* root of the [sub]tree */
char *name,
char *path,
int delim,
ancestors_t *ancestors,
int depth 
) {
   lList *children;
   lListElem *ret = NULL, *child;
   char *buf=NULL, *bufp;

   if (name == NULL)
      delim = '.';

   if (name == NULL || !strcmp(name, "*") ||
       !strcmp(name, lGetString(ep, STN_name))) {
      if (*path == 0) {
         if (name) {
            ret = ep;
            if (ancestors && depth > 0) {
               ancestors->depth = depth;
               ancestors->nodes =
                     (lListElem **)malloc(depth * sizeof(lListElem *));
               ancestors->nodes[depth-1] = ep;
            }
         }
         return ret;
      }

      /* get next component from path */

      bufp = buf = (char *)malloc(strlen(path)+1);
      if (*path == '.' || *path == '/')
         delim = *path++;
      while (*path && *path != '.' && *path != '/')
         *bufp++ = *path++;
      *bufp = 0;
      name = buf;
   } else if (delim == '/')
      return NULL;

   if ((children = lGetList(ep, STN_children)))
      for(child=lFirst(children); child && !ret; child = child->next)
         ret = search_by_path(child, name, path, delim, ancestors, depth+1);

   if (ret && ancestors && ancestors->nodes && depth > 0)
      ancestors->nodes[depth-1] = ep;
   if (buf) free(buf);
   return ret;
}


/********************************************************
 Search for a share tree node with a given path in a
 share tree
 ********************************************************/
lListElem *search_named_node_path(
lListElem *ep,  /* root of the tree */
char *path,
ancestors_t *ancestors 
) {
   return search_by_path(ep, NULL, path, 0, ancestors, 0);
}


#ifdef notdef

/********************************************************
 Search for a share tree node with a given name in a
 share tree returning an array of ancestor nodes. The
 array is contained in the ancestors_t structure which
 consist of the depth and a dynamically allocated array
 of lListElem pointers for each node.  The nodes are
 ordered from the root node to the found node. The 
 caller is reponsible for freeing the nodes array.
 ********************************************************/

static
lListElem *search_ancestors(lListElem *ep, char *name, ancestors_t *ancestors, int depth);

lListElem *search_ancestor_list(
lListElem *ep,  /* root of the tree */
char *name,
ancestors_t *ancestors 
) {
   if (ancestors)
      return search_ancestors(ep, name, ancestors, 1);
   else
      return search_named_node(ep, name);
}

static
lListElem *search_ancestors(
lListElem *ep,
char *name,
ancestors_t *ancestors,
int depth 
) {
   lListElem *cep, *fep;
   static int sn_children_pos = -1;
   static int sn_name_pos = -1;

   DENTER(TOP_LAYER, "search_named_node");

   if (!ep || !name) {
      DEXIT;
      return NULL;
   }

   if (sn_name_pos == -1) {
      sn_children_pos = lGetPosViaElem(ep, STN_children);
      sn_name_pos = lGetPosViaElem(ep, STN_name);
   }
   if (strcmp(lGetPosString(ep, sn_name_pos), name) == 0) {
      ancestors->depth = depth;
      ancestors->nodes = (lListElem **)malloc(depth * sizeof(lListElem *));
      ancestors->nodes[depth-1] = ep;
      DEXIT;
      return ep;
   }

   for_each(cep, lGetPosList(ep, sn_children_pos)) {
      if ((fep = search_ancestors(cep, name, ancestors, depth+1))) {
         ancestors->nodes[depth-1] = ep;
         DEXIT;
         return fep;
      }
   }
      
   DEXIT;
   return NULL;
}

#endif /* notdef */


