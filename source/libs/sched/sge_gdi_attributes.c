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

/* for MAXHOSTLEN */
#include "commlib.h"

/* basis gdi */
#include "sge.h"
#include "sge_gdi.h"
#include "sge_ja_task.h"
#include "sge_answer.h"
#include "sgermon.h"
#include "sge_host.h"
#include "msg_schedd.h"
#include "sge_string.h"
#include "sge_prog.h"
#include "sge_job.h"
#include "sge_host.h"
#include "sge_centry.h"

/* scheduling library */
#include "sge_complex_schedd.h"

/* myself */
#include "sge_gdi_attributes.h"


double sge_gdi_myhost_scaling(
const char *attr_name 
) {
   double scaling;

   DENTER(TOP_LAYER, "sge_gdi_myhost_scaling");
  
   sge_gdi_setup("scaling", NULL);
   scaling = sge_gdi_host_scaling(uti_state_get_qualified_hostname(), attr_name);
   sge_gdi_shutdown();

   DEXIT;
   return scaling;
}

const char *sge_gdi_myhost_attribute(
const char *attr_name 
) {
   const char *attr;

   DENTER(TOP_LAYER, "sge_gdi_myhost_attribute");

   sge_gdi_setup("attributes", NULL);
   attr = sge_gdi_host_attribute(uti_state_get_qualified_hostname(), attr_name);
   sge_gdi_shutdown();

   DEXIT;
   return attr;
}

int sge_gdi_myhost_nslots(
u_long32 jobid 
) {
   char *s;
   int nslots;

   DENTER(TOP_LAYER, "sge_gdi_myhost_nslots");

   if (!jobid && 
      (!(s=getenv("JOB_ID")) || !(jobid = atoi(s)))) {
      DEXIT;
      return 0; 
   } 
   sge_gdi_setup("nslots", NULL);
   nslots = sge_gdi_host_nslots(uti_state_get_qualified_hostname(), jobid);

   sge_gdi_shutdown();

   DEXIT;
   return nslots;
}

double sge_gdi_host_scaling(
const char *hostname,
const char *attr_name 
) {
   double scaling;
   lList *exechost_list = NULL, *alp;
   lListElem *sep, *aep, *hep;
   lEnumeration *what;
   lCondition* where;
   char unique[MAXHOSTLEN];
   int pos;

   DENTER(TOP_LAYER, "sge_gdi_host_scaling");

   if (!hostname || !attr_name) {
      fprintf(stderr, "bad argument\n");
      DEXIT;
      return -1;
   }

   /* resolve hostname */
   if (sge_resolve_hostname(hostname, unique, EH_name)) {
      fprintf(stderr, MSG_NET_UNKNOWNHOSTNAME_S , hostname);
      DEXIT;
      return -1;
   }

   /* request host entry */
   what = lWhat("%T(%I%I)", EH_Type, EH_name, EH_scaling_list);
   where = lWhere("%T( %Ih=%s )", EH_Type, EH_name, unique);
   alp = sge_gdi(SGE_EXECHOST_LIST, SGE_GDI_GET, &exechost_list, where, what);
   lFreeWhere(where);
   lFreeWhat(what);

   /* evaluate answer list */
   if (!(aep=lFirst(alp)) || (pos=lGetPosViaElem(aep, AN_status))<0 ||
         lGetPosUlong(aep, pos)!=STATUS_OK) {
      lFreeList(alp);
      fprintf(stderr, MSG_LIST_FAILEDRETRIEVINGEXECHOSTOBJ_S , 
            aep?lGetString(aep, AN_text):MSG_ERROR_UNKNOWNREASON);
      DEXIT;
      return -1;
   }

   lFreeList(alp);

   if (!(hep=host_list_locate(exechost_list, unique))) {
      lFreeList(exechost_list);
      fprintf(stderr, MSG_LIST_NOEXECHOSTOBJECT_S , unique);
      DEXIT;
      return -1;
   }

   if (!(sep=lGetSubStr(hep, HS_name, attr_name, EH_scaling_list))) {
      lFreeList(exechost_list);
      DEXIT;
      return 1.0;
   }

   scaling = lGetDouble(sep, HS_value);
   lFreeList(exechost_list);

   DEXIT;
   return scaling;
}

const char *sge_gdi_host_attribute(
const char *hostname,
const char *attr_name 
) {
   int global, ret;
   lList *alp, *centry_list = NULL, *exechost_list = NULL, *attributes = NULL;
   static char attribute_value[1024];
   lListElem *cep, *aep, *hep;
   lEnumeration *what;
   lCondition* where;
   const char *s;
   char unique[MAXHOSTLEN];
   int pos;

   DENTER(TOP_LAYER, "sge_gdi_host_scaling");

   if (!hostname || !attr_name) {
      fprintf(stderr, MSG_ERROR_BADARGUMENT );
      DEXIT;
      return NULL;
   }

   /* resolve hostname */
   if (sge_resolve_hostname(hostname, unique, EH_name)) {
      fprintf(stderr, MSG_NET_UNKNOWNHOSTNAME_S , hostname);
      DEXIT;
      return NULL;
   }

   global = !strcmp("global", unique);

   /* get host object(s) */
   if (global) 
      where = lWhere("%T(%I==%s)", EH_Type, EH_name, SGE_GLOBAL_NAME);
   else 
      where = lWhere("%T(%I==%s || %Ih=%s)", EH_Type, EH_name, SGE_GLOBAL_NAME, EH_name, unique);
   what = lWhat("%T(ALL)", EH_Type);
   alp = sge_gdi(SGE_EXECHOST_LIST, SGE_GDI_GET, &exechost_list, where, what);
   lFreeWhat(what);
   lFreeWhere(where);
   
   /* evaluate answer list */
   if (!(aep=lFirst(alp)) || (pos=lGetPosViaElem(aep, AN_status))<0 ||
         lGetPosUlong(aep, pos)!=STATUS_OK) {
      lFreeList(alp);
      fprintf(stderr, MSG_LIST_FAILEDRETRIEVINGEXECHOSTOBJS_S , 
            aep?lGetString(aep, AN_text):MSG_ERROR_UNKNOWNREASON );
      DEXIT;
      return NULL;
   }
   lFreeList(alp);

   /* get complexes */
   what = lWhat("%T(ALL)", CE_Type);
   alp = sge_gdi(SGE_CENTRY_LIST, SGE_GDI_GET, &centry_list, NULL, what);
   lFreeWhat(what);
   
   /* evaluate answer list */
   if (!(aep=lFirst(alp)) || (pos=lGetPosViaElem(aep, AN_status))<0 ||
         lGetPosUlong(aep, pos)!=STATUS_OK) {
      lFreeList(alp);
      lFreeList(exechost_list);
      fprintf(stderr, MSG_LIST_FAILEDRETRIEVINGCOMPLEXLISTS_S , 
            aep?lGetString(aep, AN_text):MSG_ERROR_UNKNOWNREASON );
      DEXIT;
      return NULL;
   }
   lFreeList(alp);

   
   /* search for host */
   if (!(hep=host_list_locate(exechost_list, unique))) {
      fprintf(stderr, MSG_LIST_NOEXECHOSTOBJECT_S , unique);
      lFreeList(centry_list);
      lFreeList(exechost_list);
      DEXIT;
      return NULL;
   }

   /* build attributes */
   if ((ret = global?
         global_complexes2scheduler(&attributes, hep, centry_list,  NULL, 0):
         host_complexes2scheduler(&attributes, hep, exechost_list, centry_list, NULL, 0))) {
      fprintf(stderr, MSG_LIST_FAILEDBUILDINGATTRIBUTESFORHOST_S , unique);
      lFreeList(centry_list);
      lFreeList(exechost_list);
      DEXIT;
      return NULL;
   }

   if (!(cep=lGetElemStr(attributes, CE_name, attr_name)) && 
       !(cep=lGetElemStr(attributes, CE_shortcut, attr_name))) {
      fprintf(stderr, MSG_LIST_NOATTRIBUTEXFORHOSTY_SS , 
            attr_name, unique);
      lFreeList(attributes);
      lFreeList(centry_list);
      lFreeList(exechost_list);
      DEXIT;
      return NULL;
   }
   
   if (!(lGetUlong(cep, CE_pj_dominant)&DOMINANT_TYPE_VALUE))
      s = lGetString(cep, CE_pj_stringval);
   else 
      s = lGetString(cep, CE_stringval);
   if (s) {
      strncpy(attribute_value, s, sizeof(attribute_value)-1);
   } else
      attribute_value[0] = '\0';

   lFreeList(attributes);
   lFreeList(centry_list);
   lFreeList(exechost_list);
   DEXIT;
   return attribute_value;
}


int sge_gdi_host_nslots(
const char *hostname,
u_long32 jobid 
) {
   lListElem *gdil_ep, *aep;
   lList *job_list = NULL, *alp;
   lList *help_list = NULL;
   lCondition *where;
   lEnumeration *what;
   char unique[MAXHOSTLEN];
   int pos, nslots = 0;
   const void *iterator = NULL;


   DENTER(TOP_LAYER, "sge_gdi_host_nslots");

   if (!hostname || !jobid) {
      DEXIT;
      return 0;
   }

   /* resolve hostname */
   if (sge_resolve_hostname(hostname, unique, EH_name)) {
      fprintf(stderr, MSG_NET_UNKNOWNHOSTNAME_S , hostname);
      DEXIT;
      return 0;
   }

   /* get this job from qmaster */
   what = lWhat("%T(%I %I))", JB_Type, JB_job_number, JB_ja_tasks);
   where = lWhere("%T( %I == %u )", JB_Type, JB_job_number, jobid);
   alp = sge_gdi(SGE_JOB_LIST, SGE_GDI_GET, &job_list, where, what);
   lFreeWhere(where);
   lFreeWhat(what);

   /* evaluate answer list */
   if (!(aep=lFirst(alp)) || (pos=lGetPosViaElem(aep, AN_status))<0 ||
         lGetPosUlong(aep, pos)!=STATUS_OK ||
         lGetNumberOfElem(job_list)!=1) {
      fprintf(stderr, MSG_LIST_FAILEDRETRIEVINGJOB_US ,
            u32c(jobid), aep?lGetString(aep, AN_text):MSG_ERROR_UNKNOWNREASON );
      lFreeList(alp);
      lFreeList(job_list);
      DEXIT;
      return 0;
   }
   lFreeList(alp);

   /* accumulate slots if host matches */    
   help_list = lGetList(lFirst(lGetList(lFirst(job_list), JB_ja_tasks)), JAT_granted_destin_identifier_list);
   gdil_ep = lGetElemHostFirst(help_list, JG_qhostname, unique, &iterator); 
   while (gdil_ep != NULL) {
      nslots += lGetUlong(gdil_ep, JG_slots);
      gdil_ep = lGetElemHostNext(help_list, JG_qhostname , unique, &iterator); 
   }

   DEXIT;
   return nslots;
}
