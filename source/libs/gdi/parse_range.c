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
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "sge_gdi_intern.h"
#include "parse_range.h"
#include "sge_rangeL.h"
#include "sge_answerL.h"
#include "sgermon.h"
#include "sge_log.h"
#include "sge_string.h"
#include "sge_exit.h"
#include "msg_gdilib.h"

#define SEPCHARS ","

/***** functions **********************************/

static void expand_range_list(lListElem *r, lList **rl);

static lListElem* extract_range(const char *rstr, int step_allowed, 
                                lList **alpp, int inf_allowed);

static lListElem *extract_range(
const char *rstr,
int step_allowed,
lList **alpp,
int inf_allowed 
) {
   const char *old_str;
   char *dptr;
   u_long32 rmin, rmax, ldummy, step = 1;
   lListElem *r;
   char msg[BUFSIZ];

   DENTER(TOP_LAYER, "extract_range");

   old_str = rstr;
  
   if (!strcasecmp(rstr, "UNDEFINED")) {
      DEXIT;
      return NULL;
   }
   r = lCreateElem(RN_Type);

   rmin = rmax = 0;
   if (rstr[0] == '-') {
      /* rstr e.g. is "-<n>" ==> min=1 */
      rmin = 1;
      rstr++;
      if (*rstr == '\0') {
         if (inf_allowed) {
            /* rstr is just "-" <==> "1-inf" */
            lSetUlong(r, RN_min, rmin);
            lSetUlong(r, RN_max, RANGE_INFINITY);
            DEXIT;
            return r;
         }
         else {
            DEXIT;
            return (NULL);
         }
      }
   }

   /* rstr should point to a decimal now */
   ldummy = strtol(rstr,&dptr,10);
   if ((ldummy == 0) && (rstr == dptr)) {
      sprintf(msg, MSG_GDI_INITIALPORTIONSTRINGNODECIMAL_S, rstr);
      sge_add_answer(alpp, msg, STATUS_ESYNTAX, 0);  
      lFreeElem(r);
      DEXIT;
      return NULL;
   }

   if (rmin != 0) {
      /* rstr is "-<n>" and ldummy contains <n>
       * dptr poits right after <n>.
       */
      if (*dptr != '\0' || (step_allowed && *dptr != ':')) {
         sprintf(msg, MSG_GDI_RANGESPECIFIERWITHUNKNOWNTRAILER_SS , 
                        old_str, rstr);
         sge_add_answer(alpp, msg, STATUS_ESYNTAX, 0);  
         lFreeElem(r);
         DEXIT;
         return NULL;
      }

      /* <n> is the max-value */
      rmax = ldummy;

   } else {	/* rmin==0) */
      /*
      ** rstr is "<n>" or "<n>-..." and ldummy contains <n>
      ** dptr poits right after <n>.
      */
      if (*dptr == '\0') {
         /* rstr is just "<n>" */
         rmin = ldummy;
         rmax = ldummy;
      } else {
         /* rstr should be "<n>-..." */
         if (!(*dptr == '-' || isdigit((int) *(dptr+1)) || *(dptr+1) == '\0' 
               || (step_allowed && *dptr == ':'))) {
	         /* ... but isn't */
            sprintf(msg, MSG_GDI_RANGESPECIFIERWITHUNKNOWNTRAILER_SS , 
                           old_str, dptr);
            sge_add_answer(alpp, msg, STATUS_ESYNTAX, 0);    
            lFreeElem(r);
            DEXIT;
            return NULL;
         } else {
	         /* it is. set min to ldummy. now, what's after the - */
            rmin = ldummy;
            rstr  = dptr+1;
            if (*rstr == '\0') {
               /* the range string was "<n>-" <==> "ldummy-inf" */
               if (inf_allowed) 
                  rmax = RANGE_INFINITY;
               else {
                  DEXIT;
                  return (NULL);
               }
            } else {
	            /* the trailer should contain a decimal - go for it */
	            ldummy = strtol(rstr,&dptr,10);
	            if ((ldummy == 0) && (rstr == dptr)) {
                  sprintf(msg, MSG_GDI_INITIALPORTIONSTRINGNODECIMAL_S , rstr); 
                  sge_add_answer(alpp, msg, STATUS_ESYNTAX, 0);    
                  lFreeElem(r);
                  DEXIT;
                  return NULL;
	            }

               if (!(*dptr == '\0' || (step_allowed && *dptr == ':'))) {
                  sprintf(msg, MSG_GDI_RANGESPECIFIERWITHUNKNOWNTRAILER_SS , 
                           rstr, dptr); 
                  sge_add_answer(alpp, msg, STATUS_ESYNTAX, 0);    
                  lFreeElem(r);
                  DEXIT;
                  return NULL;
               }
               /* finally, we got the max-value in ldummy */
               rmax = ldummy;

               if (step_allowed && *dptr && *dptr == ':') {

                  /* Issue 799 begin
                  rstr  = dptr+1;
                  ldummy = strtol(rstr,&dptr,10);
                  if ((ldummy == 0) && (rstr == dptr)) {
                     sprintf(msg, MSG_GDI_INITIALPORTIONSTRINGNODECIMAL_S , rstr); 
                     sge_add_answer(alpp, msg, STATUS_ESYNTAX, 0);    
                     lFreeElem(r);
                     DEXIT;
                     return NULL;
                  }
                  if (*dptr != '\0') {
                     sprintf(msg, MSG_GDI_RANGESPECIFIERWITHUNKNOWNTRAILER_SS , 
                              rstr, dptr); 
                     sge_add_answer(alpp, msg, STATUS_ESYNTAX, 0);    
                     lFreeElem(r);
                     DEXIT;
                     return NULL;
                  }
                  */

                 const double epsilon = 1.0E-12;
                 double       dbldummy;
 
                  rstr = dptr + 1;
                  dbldummy = strtod(rstr, &dptr);
                  ldummy = dbldummy;
                  
                  if (dbldummy > 0) {
                     if (( dbldummy - ldummy > epsilon) ||
                        ((ldummy == 0) && (rstr == dptr))) {
                        sprintf(msg, MSG_GDI_INITIALPORTIONSTRINGNODECIMAL_S,
                                rstr);
                        sge_add_answer(alpp, msg, STATUS_ESYNTAX, 0); 
                        lFreeElem(r);
                        DEXIT;
                        return NULL;
                     }
                  }
                  else if (dptr == rstr) {
                     sprintf(msg, MSG_GDI_INITIALPORTIONSTRINGNODECIMAL_S,
                             rstr);
                     sge_add_answer(alpp, msg, STATUS_ESYNTAX, 0); 
                     lFreeElem(r);
                     DEXIT;
                     return NULL;
                  }
                  else {
                      sprintf( msg, MSG_GDI_NEGATIVSTEP );
                      sge_add_answer(alpp, msg, STATUS_ESYNTAX, 0); 
                      lFreeElem(r);
                      DEXIT;
                      return NULL;
                  }
                   
                  if (*dptr != '\0') {
                     sprintf(msg, MSG_GDI_RANGESPECIFIERWITHUNKNOWNTRAILER_SS,
                             rstr, dptr);
                     sge_add_answer(alpp, msg, STATUS_ESYNTAX, 0); 
                     lFreeElem(r);
                     DEXIT;
                     return NULL;
                  }

                  /* finally, we got the max-value in ldummy */
                  step = ldummy;
               }
	         } /* if (*rstr == '\0') -- else clause */
         } /* if (*dptr != '-') -- else clause */
      } /* if (*dptr == '\0') -- else clause */
   } /* if (r->min != 0) -- else clause */

   /* We're ready? Not quite! We still have to check whether min<=max */
   if (rmin > rmax) {
      ldummy = rmax;
      rmax = rmin;
      rmin = ldummy;
   }

   lSetUlong(r, RN_min, rmin);
   lSetUlong(r, RN_max, rmax);
   lSetUlong(r, RN_step, step);

   /* Ughhhh! Done ... */

   DEXIT;
   return r;
}

/* we keep a descending sorted list of non overlapping ranges */
/* why descending? don't ask me! ask the initiator of this function */
static void expand_range_list(
lListElem *r,
lList **rl 
) {
   u_long32 rmin, rmax, rstep;
   lListElem *ep, *rr;

   DENTER(TOP_LAYER, "expand_range_list");

   rmin = lGetUlong(r, RN_min);
   rmax = lGetUlong(r, RN_max);
   rstep = lGetUlong(r, RN_step);

   /* create list */
   if ( !*rl ) {
      *rl = lCreateList("ranges", RN_Type);
   }

   if ( lGetNumberOfElem(*rl)!=0 ) {
      ep = lFirst(*rl);
      while (ep) {

         if (rstep != lGetUlong(ep, RN_step) || rstep > 1 || 
             lGetUlong(ep, RN_step) > 1) {
            lInsertElem(*rl, NULL, r);
            break;
         } else if (rmin > lGetUlong(ep, RN_max)) {

            /* 
            ** r and ep are non-overlapping and r is to be
            ** sorted ahead of ep.
            */
            lInsertElem(*rl, NULL, r);
            break;

         } else if ( rmax < lGetUlong(ep, RN_min)) {
            /* 
            ** r and ep are non-overlapping and r is to be
            ** sorted after ep ==> go for the next iteration
            */
            ep = lNext(ep);
            if (!ep)
               /* We are at the end of the list ==> append r */
               lAppendElem(*rl, r);

            continue;

         } else if ((rmax <= lGetUlong(ep, RN_max)) &&
                    (rmin >= lGetUlong(ep, RN_min))) {

            /* 
            ** r is fully contained within ep ==> delete it 
            */
            lFreeElem(r);
            break;

         } else if (rmin < lGetUlong(ep, RN_min)) {

            /* 
            ** ep is either fully contained within r
            ** or r overlaps ep at ep's bottom ==>
            ** overwrite ep and see if further list elements
            ** are covered also;
            */
            if (rmax > lGetUlong(ep, RN_max)) {
               /* r contains ep */
               lSetUlong(ep, RN_max, rmax);
            }
            lSetUlong(ep, RN_min, rmin);

            /* we're already sure we can free r */
            lFreeElem(r);
            rr = ep;
            ep = lNext(ep);
            while(ep) {
               if (rmin < lGetUlong(ep, RN_min)) {

                  /* it also contains this one */
                  r = ep;
                  ep = lNext(ep);
                  lRemoveElem(*rl, r);

               } else if (rmin <= lGetUlong(ep, RN_max)) {

                  /* these overlap ==> glue them */
                  lSetUlong(rr, RN_min, lGetUlong(ep, RN_min));

                  r = ep;
                  ep = lNext(ep);
                  lRemoveElem(*rl, r);
                  break;

               } else 
                  /* they are non overlapping */
                  break;

               ep = lNext(ep);
            }  

            /* we're now sure we inserted it */
            break;
         } else if (rmax > lGetUlong(ep, RN_max)) {
            /*
            ** r and ep overlap and r starts above ep
            ** ==> glue them
            */
            lSetUlong(ep, RN_max, rmax);
            lFreeElem(r);
            break;
         }
      }
   }
   else {
      lAppendElem(*rl, r);
   }

   DEXIT;
   return;
}



/* 

   converts a range string into a range cull list

   an undefined range return NULL

   if alpp is delivered no exit occurs instead the function fills the 
   answer list and returns NULL, *alpp must be NULL !

*/
lList *parse_ranges(const char *str, int just_parse, int step_allowed,
                    lList **alpp, lList **rl, int inf_allowed) {
   const char *s;
   lListElem *r = NULL;
   lList *range_list = NULL;
   int undefined = 0;
   int first = 1;
   int _error=0;

   DENTER(TOP_LAYER, "parse_ranges");
  
   if (!rl)
      rl = &range_list;

   for (s = sge_strtok(str,SEPCHARS); s; s = sge_strtok(NULL,SEPCHARS)) {
      if ( !first && undefined && !_error) {
         _error = 1;
         /* first was undefined - no more ranges allowed */
         ERROR((SGE_EVENT, MSG_GDI_UNEXPECTEDRANGEFOLLOWINGUNDEFINED ));
         if (alpp) {
            sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, 0);
            DEXIT;
            return NULL;
         }
         else
            SGE_EXIT(1);
      }

      /* translate string to range-struct */
      r = extract_range(s, step_allowed, alpp, inf_allowed);

      if (!r) {
         if (first)  {
            undefined = 1;
         }
         else {
            /* second range may not be undefined ! */
            ERROR((SGE_EVENT, MSG_GDI_UNEXPECTEDUNDEFINEDFOLLOWINGRANGE ));
            if (alpp) {
               sge_add_answer(alpp, SGE_EVENT, STATUS_ESYNTAX, 0);
               DEXIT;
               return NULL;
            }
            else
               SGE_EXIT(1);
         }
      }
      first = 0;

      if (!just_parse) {
         /* insert into range list; malloc new elements;
         ** free unecessary elements
         */
         if (r)
            expand_range_list(r, rl);
      }
      else {
         lFreeElem(r);
      }
   }

   DEXIT;
   return *rl;
}

/*
** NAME
**   unparse_ranges  -  convert ranges to string
** PARAMETER
**   buff    - buffer to store output
**   max_len - maximum number of bytes to write to the buffer
**   ep      - RN_Type list element to be converted to string
**  
** RETURN
**   
** EXTERNAL
**
** DESCRIPTION
**   
*/
int unparse_ranges(
FILE *fp,
char *buff,
u_long32 max_len,
lList *lp 
) {
   char str[256 + 1];
   u_long32 ul;
   lListElem *ep;
   int print_sep = 0;

   DENTER(TOP_LAYER, "unparse_ranges");
   
   if (!fp && !buff) {
      DPRINTF(("unparse_ranges: File pointer and buffer can't both be NULL\n"));
      DEXIT;
      return -2;
   }

   if (!lp) {
      strcpy(str, "UNDEFINED");
      if (fp) 
         fprintf(fp, str);
      else {
         if (max_len < strlen(str) + 1) {
            DPRINTF(("unparse_ranges: buffer too short\n"));
            DEXIT;
            return -3;
         }
         strcpy(buff, str);
         buff += strlen(str);
         max_len -= strlen(str);
      }

      DEXIT;
      return 0;
   }

   for_each(ep, lp) {
      if (print_sep) {
         if (fp) {
            fprintf(fp, ",");
         }
         else {
            if (max_len < 2) {
               DPRINTF(("unparse_ranges: buffer too short\n"));
               DEXIT;
               return -4;
            }
            strcpy(buff, ",");
            buff++;
            max_len--;
         }
      }

      /*
      ** if the 2 ranges are equal, we just write m for m-m
      */
      if ((ul = lGetUlong(ep, RN_min)) == lGetUlong(ep, RN_max)) {
         sprintf(str, uu32, ul);
         if (fp) {
            fprintf(fp, "%s", str);
         }
         else {
            if (max_len < strlen(str) + 1) {
               DPRINTF(("unparse_ranges: buffer too short\n"));
               DEXIT;
               return -4;
            }
            strcpy(buff, str);
            buff += strlen(str);
            max_len -= strlen(str);
         }
         print_sep = 1;
         continue;
      }
      
      if (!(ul = lGetUlong(ep, RN_min))) {
         ul = 1;
      }
      sprintf(str, uu32, ul);
      strcat(str, "-");
      if ((ul = lGetUlong(ep, RN_max))) {
         sprintf(str + strlen(str), uu32, ul);
      }
      
      if (fp) {
         fprintf(fp, "%s", str);
      }
      else {
         if (max_len < strlen(str) + 1) {
            DPRINTF(("unparse_ranges: buffer too short\n"));
            DEXIT;
            return -5;
         }
         strcpy(buff, str);
         buff += strlen(str);
         max_len -= strlen(str);
      }
      print_sep = 1;
   } /* end for_each */

   DEXIT;
   return 0;
}


/* --------------------------------------------------

   show_ranges()

DESCR

   writes range into a string buffer (if given)
   or writes it to the fp 

*/
void show_ranges(
char *buffer,
int step_allowed,
FILE *fp,
lList *rlp 
) {
   lListElem *ep;
   u_long32 min, max, step;

   DENTER(TOP_LAYER, "show_ranges");

   if (!fp && !buffer) {
      DEXIT;
      return;
   }

   if ( !rlp )
      if (fp) 
         fprintf(fp, "UNDEFINED\n");

   if (buffer)
      /* ensure empty buffer */
      buffer[0] = '\0';

   ep = lLast(rlp); 
   while (ep ) {
      min = lGetUlong(ep, RN_min);
      max = lGetUlong(ep, RN_max);
      step = lGetUlong(ep, RN_step);

      if (min==max) {
         if (fp)
            fprintf(fp, u32, min); 
         else 
            sprintf(&buffer[strlen(buffer)], u32, min); 
      } else {
         if (fp) {
            if (step_allowed)       
               fprintf(fp, u32"-"u32":"u32, min, max, step); 
            else
               fprintf(fp, u32"-"u32, min, max); 
         }
         else {
            if (step_allowed)       
               sprintf(&buffer[strlen(buffer)], u32"-"u32":"u32, 
                                    min, max, step); 
            else
               sprintf(&buffer[strlen(buffer)], u32"-"u32, min, max); 
         }
      } 
      ep = lPrev(ep);
      if ( ep ) {
         if (fp)
            fprintf(fp, ",");
         else 
            strcat(buffer, ",");
      }
   } 

   DEXIT;
   return;
}

int id_in_range(
u_long32 id,
lList *rlp 
) {
   lListElem *ep;
   u_long32 min, max;

   DENTER(TOP_LAYER, "id_in_range");

   if ( !rlp )
      return -1;

   ep = lLast(rlp);
   while (ep ) {
      min = lGetUlong(ep, RN_min);
      max = lGetUlong(ep, RN_max);

      if (min <= id && id <= max)
         return 1;

      ep = lPrev(ep);
   }

   DEXIT;
   return 0;
}


