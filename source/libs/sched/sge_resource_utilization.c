#include <string.h>
#include "sge.h"
#include "sgermon.h"
#include "cull.h"
#include "sge_ulong.h"
#include "msg_common.h"

#include "sge_pe.h"
#include "sge_log.h"
#include "sge_job.h"
#include "sge_host.h"
#include "sge_qinstance.h"
#include "sge_centry.h"
#include "sge_hostname.h"
#include "sge_select_queue.h"
#include "sge_resource_utilization.h"
#include "sge_schedd_conf.h"
#include "sge_serf.h"

#ifdef MODULE_TEST_SGE_RESOURCE_UTILIZATION
/*
cc -o sge_resource_utilization -Isecurity/sec -Icommon -Ilibs -Ilibs/uti -Ilibs/gdi -Ilibs/japi -Ilibs/sgeobj -Ilibs/cull
-Ilibs/rmon -Ilibs/comm -Ilibs/comm/lists -Ilibs/sched -DMODULE_TEST_SGE_RESOURCE_UTILIZATION
libs/sched/sge_resource_utilization.c -xarch=v9 -xildoff -L/vol2/SW/db-4.2.52/sol-sparc64/lib -LSOLARIS64 -g -lpthread -lsched
-lmir -levc -lgdi -lsgeobj -lsgeobjd -lsec -lcull -lrmon -lcomm -lcommlists -luti -llck -ldl -lsocket -lnsl -lm
*/
int main(int argc, char *argv[]) 
{
   lListElem *cr;

   DENTER_MAIN(TOP_LAYER, "sge_resource_utilization");

   cr = lCreateElem(RUE_Type);
   lSetString(cr, RUE_name, "slots");

   printf("adding a 200s now assignment of 4 starting at 800\n");
   utilization_add(cr, 800, 200, 8, 100, 1, PE_TAG, "pe_slots", "STARTING");   
   utilization_print(cr, "pe_slots");

   printf("adding a 100s now assignment of 4 starting at 1000\n");
   utilization_add(cr, 1000, 100, 4, 100, 1, PE_TAG, "pe_slots", "STARTING");   
   utilization_print(cr, "pe_slots");

   printf("adding a 100s reservation of 8 starting at 1100\n");
   utilization_add(cr, 1100, 100, 8, 101, 1, PE_TAG, "pe_slots", "RESERVING");   
   utilization_print(cr, "pe_slots");

   printf("max utilization starting at 1000 for a 100s job: %f\n",
      utilization_max(cr, 1000, 100));

   printf("max utilization starting at 1200 for a 1000s job: %f\n",
      utilization_max(cr, 1200, 1000));
   return 0;
}



#endif

static void utilization_normalize(lList *diagram);
static u_long32 utilization_endtime(u_long32 start, u_long32 duration);

static int rc_add_job_utilization(lListElem *jep, u_long32 task_id, const char *type, 
      lListElem *ep, lList *centry_list, int slots, int config_nm, int actual_nm, 
      const char *obj_name, u_long32 start_time, u_long32 end_time, u_long32 tag);

static void utilization_find_time_or_prevstart_or_prev(lList *diagram, 
      u_long32 time, lListElem **hit, lListElem **before);

/****** sge_resource_utilization/utilization_print_to_dstring() ****************
*  NAME
*     utilization_print_to_dstring() -- Print resource utilization to dstring
*
*  SYNOPSIS
*     bool utilization_print_to_dstring(const lListElem *this_elem, dstring 
*     *string) 
*
*  FUNCTION
*     Print resource utlilzation as plain number to dstring.
*
*  INPUTS
*     const lListElem *this_elem - A RUE_Type element
*     dstring *string            - The string 
*
*  RESULT
*     bool - error state
*        true  - success
*        false - error
*
*  NOTES
*     MT-NOTE: utilization_print_to_dstring() is MT safe
*******************************************************************************/
bool utilization_print_to_dstring(const lListElem *this_elem, dstring *string)
{
   if (!this_elem || !string) 
      return true;
   return double_print_to_dstring(lGetDouble(this_elem, RUE_utilized_now), string);
}


void utilization_print_all(const lList* pe_list, lList *host_list, const lList *queue_list)
{
   lListElem *ep, *cr;
   const char *name;

   DENTER(TOP_LAYER, "utilization_print_all");

   /* pe_list */
   /* ...     */

   /* global */
   if ((ep=host_list_locate(host_list, SGE_GLOBAL_NAME))) {
      DPRINTF(("-------------------------------------------\n"));
      DPRINTF(("GLOBL HOST RESOURCES\n"));
      for_each (cr, lGetList(ep, EH_resource_utilization)) {
         utilization_print(cr, SGE_GLOBAL_NAME);
      }
   }

   /* exec hosts */
   for_each (ep, host_list) {
      name = lGetHost(ep, EH_name);
      if (sge_hostcmp(name, SGE_GLOBAL_NAME)) {
         DPRINTF(("-------------------------------------------\n"));
         DPRINTF(("EXEC HOST \"%s\"\n", name));
         for_each (cr, lGetList(ep, EH_resource_utilization)) {
            utilization_print(cr, name);
         }
      }
   }

   /* queue instances */
   for_each (ep, queue_list) {
      name = lGetString(ep, QU_full_name);
      if (strcmp(name, SGE_TEMPLATE_NAME)) {
         DPRINTF(("-------------------------------------------\n"));
         DPRINTF(("QUEUE \"%s\"\n", name));
         for_each (cr, lGetList(ep, QU_resource_utilization)) {
            utilization_print(cr, name);
         }
      }
   }
   DPRINTF(("-------------------------------------------\n"));
   
   DEXIT;
   return;
}

void utilization_print(const lListElem *cr, const char *object_name) 
{ 
   lListElem *rde;
   DENTER(TOP_LAYER, "utilization_print");

   DPRINTF(("resource utilization: %s \"%s\"\n", object_name?object_name:"<unknown_object>", lGetString(cr, RUE_name)));
   for_each (rde, lGetList(cr, RUE_utilized)) {
      DPRINTF(("\t"U32CFormat"  %f\n", lGetUlong(rde, RDE_time), lGetDouble(rde, RDE_amount))); 
#ifdef MODULE_TEST_SGE_RESOURCE_UTILIZATION
      printf("\t"U32CFormat"  %f\n", lGetUlong(rde, RDE_time), lGetDouble(rde, RDE_amount)); 
#endif
   }

   DEXIT;
   return;
}

static u_long32 utilization_endtime(u_long32 start, u_long32 duration)
{
   u_long32 end_time;
   if (((double)start + (double)duration) < ((double)U_LONG32_MAX))
      end_time = start + duration;
   else
      end_time = U_LONG32_MAX;
   return end_time;
}

int utilization_add(lListElem *cr, u_long32 start_time, u_long32 duration, double utilization, 
   u_long32 job_id, u_long32 ja_taskid, u_long32 level, const char *object_name, const char *type) 
{
   lList *resource_diagram;
   lListElem *this, *prev, *start, *end;
   const char *name = lGetString(cr, RUE_name);
   char level_char = CENTRY_LEVEL_TO_CHAR(level);
   u_long32 end_time;
   double util_prev;
   
   DENTER(TOP_LAYER, "utilization_add");

   end_time = utilization_endtime(start_time, duration);

   serf_record_entry(job_id, ja_taskid, (type!=NULL)?type:"<unknown>", start_time, end_time, 
         level_char, object_name, name, utilization);

#ifndef MODULE_TEST_SGE_RESOURCE_UTILIZATION
   if (sconf_get_max_reservations()==0 || duration==0) {
      DEXIT;
      return 0;
   }
#endif

   /* ensure resource diagram is initialized */
   if (!(resource_diagram=lGetList(cr, RUE_utilized))) {
      resource_diagram = lCreateList(name, RDE_Type);
      lSetList(cr, RUE_utilized, resource_diagram);
   }

   utilization_find_time_or_prevstart_or_prev(resource_diagram, 
         start_time, &start, &prev);

   if (start) {
      /* if we found one add the utilization amount to it */
      lAddDouble(start, RDE_amount, utilization);
   } else {
      /* otherwise insert a new one after the element before resp. at the list begin */
      if (prev)
         util_prev = lGetDouble(prev, RDE_amount);
      else 
         util_prev = 0;
      start = lCreateElem(RDE_Type);
      lSetUlong(start, RDE_time, start_time);
      lSetDouble(start, RDE_amount, utilization + util_prev);
      lInsertElem(resource_diagram, prev, start);
   }

   end = NULL;
   prev = start;
   this = lNext(start);

   /* find existing end element or the element before */
   while (this) {
      if (end_time == lGetUlong(this, RDE_time)) {
         DPRINTF(("found existing end\n"));
         end = this;
         break;
      }
      if (end_time < lGetUlong(this, RDE_time)) {
         break;
      }
      /* increment amount of elements in-between */
      lAddDouble(this, RDE_amount, utilization);
      prev = this;
      this = lNext(this);
   }

   if (!end) {
      util_prev = lGetDouble(prev, RDE_amount);
      end = lCreateElem(RDE_Type);
      lSetUlong(end, RDE_time, end_time);
      lSetDouble(end, RDE_amount, util_prev - utilization);
      lInsertElem(resource_diagram, prev, end);
   }

#if 0
   utilization_print(cr, "pe_slots");
   printf("this was before normalize\n");
#endif

   utilization_normalize(resource_diagram);
   DEXIT;
   return 0;
}

/* 
   Find element with specified time or the element before it 

   If the element exists it is returned in 'hit'. Otherwise
   the element before it is returned or NULL if no such exists.

*/


static void utilization_find_time_or_prevstart_or_prev(lList *diagram, u_long32 time, lListElem **hit, lListElem **before)
{ 
   lListElem *start, *this, *prev;

   start = NULL;
   this = lFirst(diagram); 
   prev = NULL;

   while (this) {
      if (time == lGetUlong(this, RDE_time)) {
         start = this;
         break;
      }
      if (time < lGetUlong(this, RDE_time)) {
         break;
      }
      prev = this;
      this = lNext(this);
   }

   *hit    = start;
   *before = prev;
}

/* normalize utilization diagram 

   Note: Diagram doesn't need to vanish (NULL list) 
         as long as utilization is added only
*/
static void utilization_normalize(lList *diagram)
{
   lListElem *this, *next;
   double util_prev;

   this = lFirst(diagram);
   next = lNext(this);
   util_prev = lGetDouble(this, RDE_amount);

   while ((this=next)) {
      next = lNext(this);
      if (util_prev == lGetDouble(this, RDE_amount))
         lRemoveElem(diagram, this);
      else
         util_prev = lGetDouble(this, RDE_amount);
   }

   return;
}

/* return the utilization at queue end time */
double utilization_queue_end(const lListElem *cr)
{
   const lListElem *ep = lLast(lGetList(cr, RUE_utilized));

   if (ep)
      return lGetDouble(ep, RDE_amount);
   else
      return 0.0;
}


/* return the maximum unitilization within the specified time frame */
double utilization_max(const lListElem *cr, u_long32 start_time, u_long32 duration)
{
   const lListElem *rde;
   lListElem *start, *prev;
   double max = 0.0;
   u_long32 end_time = utilization_endtime(start_time, duration);

   DENTER(TOP_LAYER, "utilization_max");


#if 0
   utilization_print(cr, "the object");
#endif

   utilization_find_time_or_prevstart_or_prev(lGetList(cr, RUE_utilized), start_time, 
         &start, &prev);

   if (start) {
      max = lGetDouble(start, RDE_amount);
      rde = lNext(start);
   } else {
      if (!prev) {
         DEXIT;
         return 0; 
      }
      max = lGetDouble(prev, RDE_amount);
      rde = lNext(prev);
   }

   /* now watch out for the maximum before end time */ 
   while (rde && end_time > lGetUlong(rde, RDE_time)) {
      max = MAX(max, lGetDouble(rde, RDE_amount));
      rde = lNext(rde);
   }

   DEXIT;
   return max; 
}

/* return the earliest time when utilization is below 'max_util' starting
   at the end of the utilization diagram */
u_long32 utilization_below(const lListElem *cr, double max_util, const char *object_name)
{
   const lListElem *rde;
   double util = 0;
   u_long32 when = DISPATCH_TIME_NOW;

   DENTER(TOP_LAYER, "utilization_below");

#if 0
   utilization_print(cr, object_name);
#endif

   /* search backward starting at the diagrams end */
   for_each_rev (rde, lGetList(cr, RUE_utilized)) {
      util = lGetDouble(rde, RDE_amount);
      if (util <= max_util) {
         lListElem *p = lPrev(rde);
         if (p && lGetDouble(p, RDE_amount) > max_util) {
            when = lGetUlong(rde, RDE_time);
            break;
         }
      }
   }

   if (when == DISPATCH_TIME_NOW) {
      DPRINTF(("no utilization\n"));
   } else {
      DPRINTF(("utilization below %f (%f) starting at "U32CFormat"\n", 
         max_util, util, when));
   }

   DEXIT;
   return when; 
}


int add_job_utilization(const sge_assignment_t *a, const char *type)
{
   lListElem *gel, *qep, *hep; 
   int slots = 0;

   DENTER(TOP_LAYER, "add_job_utilization");

   /* parallel environment  */
   if (a->pe)
      utilization_add(lFirst(lGetList(a->pe, PE_resource_utilization)), a->start, a->duration, a->slots,
            a->job_id, a->ja_task_id, PE_TAG, lGetString(a->pe, PE_name), type);

   /* global */
   hep = host_list_locate(a->host_list, SGE_GLOBAL_NAME);
   rc_add_job_utilization(a->job, a->ja_task_id, type, hep, a->centry_list, a->slots, 
         EH_consumable_config_list, EH_resource_utilization, 
         lGetHost(hep, EH_name), a->start, a->duration, GLOBAL_TAG);  

   /* hosts */
   for_each(hep, a->host_list) {
      lListElem *next_gel;
      const void *queue_iterator = NULL;
      const char *eh_name = lGetHost(hep, EH_name);

      if (!strcmp(eh_name, SGE_GLOBAL_NAME)) 
         continue;

      /* determine number of slots per host */
      slots = 0;
      for (next_gel = lGetElemHostFirst(a->gdil, JG_qhostname, eh_name, &queue_iterator);
          (gel = next_gel);
           next_gel = lGetElemHostNext(a->gdil, JG_qhostname, eh_name, &queue_iterator)) {
         
         slots += lGetUlong(gel, JG_slots);
      }
      if (slots != 0) {
         rc_add_job_utilization(a->job, a->ja_task_id, type, hep, a->centry_list, slots,
                  EH_consumable_config_list, EH_resource_utilization, eh_name, a->start, 
                  a->duration, HOST_TAG);
      }
   }

   /* queues */
   for_each(gel, a->gdil) {  
      int slots = lGetUlong(gel, JG_slots); 
      const char *qname = lGetString(gel, JG_qname);
      qep = qinstance_list_locate2(a->queue_list, qname); 
      rc_add_job_utilization(a->job, a->ja_task_id, type, qep, a->centry_list, slots,
               QU_consumable_config_list, QU_resource_utilization, qname, a->start, 
               a->duration, QUEUE_TAG);
   }

   DEXIT;
   return 0;
}

static int 
rc_add_job_utilization(lListElem *jep, u_long32 task_id, const char *type, 
      lListElem *ep, lList *centry_list, int slots, int config_nm, int actual_nm, 
      const char *obj_name, u_long32 start_time, u_long32 end_time, u_long32 tag) 
{
   lListElem *cr, *cr_config, *dcep;
   double dval;
   const char *name;
   int mods = 0;

   DENTER(TOP_LAYER, "rc_add_job_utilization");

   if (!ep || !slots) {
      ERROR((SGE_EVENT, "rc_add_job_utilization NULL object or 0 slot amount "
            "(job "u32" obj %s type %s) slots %d ep %p\n", 
            lGetUlong(jep, JB_job_number), obj_name, type, slots, ep));
      DEXIT;
      return 0;
   }

   for_each (cr_config, lGetList(ep, config_nm)) {
      name = lGetString(cr_config, CE_name);
      dval = 0;

      /* search default request */  
      if (!(dcep = centry_list_locate(centry_list, name))) {
         ERROR((SGE_EVENT, MSG_ATTRIB_MISSINGATTRIBUTEXINCOMPLEXES_S , name));
         DEXIT; 
         return -1;
      } 

      if (!lGetBool(dcep, CE_consumable)) {
         /* no error */
         continue;
      }

      /* ensure attribute is in actual list */
      if (!(cr = lGetSubStr(ep, RUE_name, name, actual_nm))) {
         cr = lAddSubStr(ep, RUE_name, name, actual_nm, RUE_Type);
         /* CE_double is implicitly set to zero */
      }
   
      if (jep) {
         bool tmp_ret = job_get_contribution(jep, NULL, name, &dval, dcep);

         if (tmp_ret && dval != 0.0) {
            /* update RUE_utilized resource diagram to reflect jobs utilization */
            utilization_add(cr, start_time, end_time, slots * dval,
               lGetUlong(jep, JB_job_number), task_id, tag, obj_name, type);
            mods++;
         }  
      }
   }

   DEXIT;
   return mods;
}
