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
#include <string.h>
#include <ctype.h>

/* do not compile in monitoring code */
#ifndef NO_SGE_COMPILE_DEBUG
#define NO_SGE_COMPILE_DEBUG
#endif

#include "sgermon.h"
#include "cull_dump_scan.h"
#include "cull_listP.h"
#include "cull_hashP.h"
#include "cull_multitypeP.h"
#include "cull_lerrnoP.h"
#include "basis_types.h"

#define READ_LINE_LENGHT 255

#define INDENT_STRING      "   "

/*------------ internal functions ------------------------------*/

static int space_comment(char *s);
static int fGetLine(FILE *fp, char *line, int max_line);

/* =========== implementation ================================= */

/* ===== functions used by the higher level functions ============ */
/* ------------------------------------------------------------

   writes a descriptor (for debugging purposes)

 */
int lDumpDescr(
FILE *fp,
const lDescr *dp,
int indent 
) {
   int i, ret = ~EOF;
   char space[256];

   DENTER(CULL_LAYER, "lDumpDescr");

   space[0] = '\0';
   for (i = 0; i < indent; i++)
      strcat(space, INDENT_STRING);

   if (!fp) {
      LERROR(LEFILENULL);
      DEXIT;
      return -1;
   }
   ret = fprintf(fp, "%s{ /* DESCR BEGIN */\n", space);

   if (!dp) {
      LERROR(LEDESCRNULL);
      DEXIT;
      return -1;
   }
   ret = fprintf(fp, "%s/* NUMBER OF DESCR FIELDS */ %d\n", space, lCountDescr(dp));

   for (i = 0; dp[i].mt != lEndT && ret != EOF; i++) {
      ret = fprintf(fp, "%s/* %-20.20s */ { %d, %d, %d }\n", space, lNm2Str(dp[i].nm),
                    dp[i].nm, dp[i].mt, dp[i].hash == NULL ? HASH_OFF : dp[i].hash->type);
   }

   ret = fprintf(fp, "%s} /* DESCR END */\n", space);

   DEXIT;
   return (ret == EOF) ? -1 : 0;
}

/* ------------------------------------------------------------

   read a descriptor from file (for debugging purposes)

 */

lDescr *lUndumpDescr(
FILE *fp 
) {
   int n, i;
   lDescr *dp = NULL;

   DENTER(CULL_LAYER, "lUndumpDescr");

   if (!fp) {
      LERROR(LEFILENULL);
      DEXIT;
      return NULL;
   }

   /* read bra */
   if (fGetBra(fp)) {
      printf("bra is missing\n");
      LERROR(LESYNTAX);
      DEXIT;
      return NULL;
   }

   /* read Descriptor Count */
   if (fGetInt(fp, &n)) {
      printf("reading integer from dump file failed\n");
      LERROR(LEFIELDREAD);
      DEXIT;
      return NULL;
   }

   if (!(dp = (lDescr *) malloc(sizeof(lDescr) * (n + 1)))) {
      LERROR(LEMALLOC);
      DEXIT;
      return NULL;
   }

   for (i = 0; i < n; i++) {
      /* read descriptor */
      if (fGetDescr(fp, &(dp[i]))) {
         LERROR(LEFGETDESCR);
         DEXIT;
         return NULL;
      }
   }
   dp[i].nm = NoName;
   dp[i].mt = lEndT;
   dp[i].hash = NULL;

   /* read ket */
   if (fGetKet(fp)) {
      printf("ket is missing");
      free(dp);
      LERROR(LESYNTAX);
      DEXIT;
      return NULL;
   }

   DEXIT;
   return dp;
}

/* ------------------------------------------------------------
   dumps a given element to the file fname

 */
int lDumpElem(
const char *fname,
const lListElem *ep,
int indent 
) {
   FILE *fp;
   int ret;

   if (!(fp = fopen(fname, "w"))) {
      LERROR(LEOPEN);
      return -1;
   }

   ret = lDumpElemFp(fp, ep, indent);
   fclose(fp);

   return ret;
}

/* ------------------------------------------------------------ 

   dumps a given element to the file fp 

 */
int lDumpElemFp(
FILE *fp,
const lListElem *ep,
int indent 
) {
   int i, ret = ~EOF;
   lList *tlp;
   char space[256];

   DENTER(CULL_LAYER, "lDumpElemFp");

   space[0] = '\0';
   for (i = 0; i < indent; i++)
      strcat(space, INDENT_STRING);

   if (!fp) {
      LERROR(LEFILENULL);
      DEXIT;
      return -1;
   }
   if (!ep) {
      LERROR(LEELEMNULL);
      DEXIT;
      return -1;
   }

   ret = fprintf(fp, "%s{ \n", space);
   for (i = 0, ret = 0; ep->descr[i].nm != NoName && ret != EOF; i++) {

      switch (ep->descr[i].mt) {
      case lIntT:
         ret = fprintf(fp, "%s/* %-20.20s */ %d\n",
                     space, lNm2Str(ep->descr[i].nm), lGetPosInt(ep, i));
         break;
      case lUlongT:
         ret = fprintf(fp, "%s/* %-20.20s */ " u32 "\n",
                   space, lNm2Str(ep->descr[i].nm), lGetPosUlong(ep, i));
         break;
      case lStringT:
         ret = fprintf(fp, "%s/* %-20.20s */ \"%s\"\n",
                  space, lNm2Str(ep->descr[i].nm), lGetPosString(ep, i));
         break;
      case lFloatT:
         ret = fprintf(fp, "%s/* %-20.20s */ %f\n",
                   space, lNm2Str(ep->descr[i].nm), lGetPosFloat(ep, i));
         break;
      case lDoubleT:
         ret = fprintf(fp, "%s/* %-20.20s */ %f\n",
                  space, lNm2Str(ep->descr[i].nm), lGetPosDouble(ep, i));
         break;
      case lLongT:
         ret = fprintf(fp, "%s/* %-20.20s */%ld \n",
                    space, lNm2Str(ep->descr[i].nm), lGetPosLong(ep, i));
         break;
      case lCharT:
         ret = fprintf(fp, "%s/* %-20.20s */ %c\n",
                    space, lNm2Str(ep->descr[i].nm), lGetPosChar(ep, i));
         break;
      case lRefT:
         ret = fprintf(fp, "%s/* %-20.20s */ %p\n",
                    space, lNm2Str(ep->descr[i].nm), lGetPosRef(ep, i));
         break;
      case lListT:
         if (!(tlp = lGetPosList(ep, i)))
            ret = fprintf(fp, "%s/* %-20.20s */ empty\n",
                          space, lNm2Str(ep->descr[i].nm));
         else {
            ret = fprintf(fp, "%s/* %-20.20s */ full\n",
                          space, lNm2Str(ep->descr[i].nm));
            if (ret != EOF)
               ret = lDumpList(fp, tlp, indent + 1);
         }
         break;
      }
   }

   ret = fprintf(fp, "%s}\n", space);

   DEXIT;
   return (ret == EOF) ? -1 : 0;
}

/* ------------------------------------------------------------ 

   writes a given list to 

 */
int lDumpList(
FILE *fp,
const lList *lp,
int indent 
) {
   lListElem *ep;
   int i, n, ret = ~EOF;

   char space[256];

   DENTER(CULL_LAYER, "lDumpList");

   space[0] = '\0';
   for (i = 0; i < indent; i++)
      strcat(space, INDENT_STRING);

   if (!fp) {
      LERROR(LEFILENULL);
      DEXIT;
      return -1;
   }
   if (!lp) {
      LERROR(LELISTNULL);
      DEXIT;
      return -1;
   }

   ret = fprintf(fp, "%s{ /* LIST BEGIN */\n", space);

   ret = fprintf(fp, "%s/* LISTNAME               */ \"%s\"\n", space, lGetListName(lp));
   ret = fprintf(fp, "%s/* NUMBER OF ELEMENTS     */ %d\n", space, n = lGetNumberOfElem(lp));

   ret = lDumpDescr(fp, lGetListDescr(lp), indent);

   for (ep = lFirst(lp); ep && ret != EOF; ep = lNext(ep))
      ret = lDumpElemFp(fp, ep, indent);

   ret = fprintf(fp, "%s} /* LIST END */\n", space);

   DEXIT;
   return (ret == EOF) ? -1 : 0;

}

lListElem *lUndumpElem(
FILE *fp,
const lDescr *dp 
) {
   lListElem *ep;
   int n, i;
   int ret = 0;
   char *str;

   DENTER(CULL_LAYER, "lUndumpElem");

   if (!fp) {
      LERROR(LEFILENULL);
      DEXIT;
      return NULL;
   }

   if (!dp) {
      LERROR(LEDESCRNULL);
      DEXIT;
      return NULL;
   }

   if (!(ep = lCreateElem(dp))) {
      LERROR(LECREATEELEM);
      DEXIT;
      return NULL;
   }

   if ((n = lCountDescr(dp)) <= 0) {
      LERROR(LECOUNTDESCR);
      lFreeElem(ep);
      DEXIT;
      return NULL;
   }

   /* read bra */
   if (fGetBra(fp)) {
      printf("bra is missing\n");
      LERROR(LESYNTAX);
      lFreeElem(ep);
      DEXIT;
      return NULL;
   }

   for (i = 0; i < n && ret == 0; i++) {
      switch (dp[i].mt) {
      case lIntT:
         ret = fGetInt(fp, &(ep->cont[i].i));
         break;
      case lUlongT:
         ret = fGetUlong(fp, &(ep->cont[i].ul));
         break;
      case lStringT:
         ret = fGetString(fp, &str);
         lSetPosString(ep, i, str);
         free(str);             /* fGetString strdup's */
         break;
      case lFloatT:
         ret = fGetFloat(fp, &(ep->cont[i].fl));
         break;
      case lDoubleT:
         ret = fGetDouble(fp, &(ep->cont[i].db));
         break;
      case lLongT:
         ret = fGetLong(fp, &(ep->cont[i].l));
         break;
      case lCharT:
         ret = fGetChar(fp, &(ep->cont[i].c));
         break;
      case lRefT:
         ret = 0;
         ep->cont[i].ref = NULL;
         break;
      case lListT:
         ret = fGetList(fp, &(ep->cont[i].glp));
         break;
      default:
         lFreeElem(ep);
         unknownType("lUndumpElem");
      }
   }

   /* error handling for loop */
   if (ret != 0) {
      lFreeElem(ep);
      LERROR(LEFIELDREAD);
      DEXIT;
      return NULL;
   }

   /* read ket */
   if (fGetKet(fp)) {
      lFreeElem(ep);
      printf("ket is missing\n");
      LERROR(LESYNTAX);
      DEXIT;
      return NULL;
   }

   DEXIT;
   return ep;
}

/*----------------------------------------------------------------------
        lUndumpList reads a by lDumpList dumped dump into the memory

        the args are:
        FILE *fp     file pointer
        char *name   new name of list or NULL if the old name in the dumpfile
                     should be used as listname
        lDescr *dp   new list descriptor or NULL if the old list descriptor
                     should be used as list descriptor

        Actually a type/name matching is only performed for the list itself 
        and not for its sublists. 
        If an implementation of changed sublist descriptors is
        desired we can probably use the following syntax for lUndumpList
        lList* lUndumpList(fp, name, formatstring, ...)
        with formatstring like "%T(%I -> %T(%I->%T))" and the varargs list:
        ".....", lDescr1, fieldname1, lDescr2, fieldname2, lDescr3
        or write a wrapper around lUndumpList which parses this format and 
        hands over the varargs list to lUndumpList
-------------------------------------------------------------------------*/

lList *lUndumpList(
FILE *fp,
const char *name,
const lDescr *dp 
) {
   lList *lp = NULL;
   lListElem *fep, *ep;
   lDescr *fdp = NULL;
   int i, j, nelem, n, k;
   int *found;
   char *oldname;

   DENTER(CULL_LAYER, "lUndumpList");

   if (!fp) {
      LERROR(LEFILENULL);
      DEXIT;
      return NULL;
   }

   /* read bra */
   if (fGetBra(fp)) {
      printf("bra is missing\n");
      LERROR(LESYNTAX);
      DEXIT;
      return NULL;
   }
   /* read listname */
   if (fGetString(fp, &oldname)) {
      printf("fGetString failed\n");
      LERROR(LEFIELDREAD);
      DEXIT;
      return NULL;
   }

   /* read number of elems */
   if (fGetInt(fp, &nelem)) {
      printf("fGetInt failed\n");
      LERROR(LEFIELDREAD);
      DEXIT;
      return NULL;
   }

   /* read Descriptor from file */
   if (!(fdp = lUndumpDescr(fp))) {
      LERROR(LEFGETDESCR);
      DEXIT;
      return NULL;
   }

   if (!dp)                     /* dp is NULL, use lDescr from dumpfile */
      dp = fdp;

   /* use old name (from file) if name is NULL */
   if (!(lp = lCreateList((name) ? name : oldname, dp))) {
      LERROR(LECREATELIST);
      DEXIT;
      return NULL;
   }
   free(oldname);               /* fGetString strdup's */

   if ((n = lCountDescr(dp)) <= 0) {
      LERROR(LECOUNTDESCR);
      lFreeList(lp);
      DEXIT;
      return NULL;
   }

   if (!(found = (int *) malloc(sizeof(int) * n))) {
      LERROR(LEMALLOC);
      lFreeList(lp);
      DEXIT;
      return NULL;
   }

   /* Initialize found array */
   for (i = 0; i < n; i++)
      found[i] = -1;

   /* Here warnings are displayed if there are additional or missing fields */
   for (j = 0; fdp[j].nm != NoName; j++) {
      for (i = 0; i < n; i++) {
         if (dp[i].nm == fdp[j].nm &&
             dp[i].mt == fdp[j].mt) {
            if (found[i] != -1)
               DPRINTF(("lUndumpList: field %s found twice\n",
                        lNm2Str(dp[i].nm)));
            found[i] = j;
            break;
         }
      }
      if (i == n)
         DPRINTF(("lUndumpList: field %s not needed\n", lNm2Str(fdp[j].nm)));
   }

   for (i = 0; i < n; i++)
      if (found[i] == -1)
         DPRINTF(("lUndumpList: field %s not found\n", lNm2Str(dp[i].nm)));

   /* LOOP OVER THE LIST ELEMENTS */
   for (k = 0; k < nelem; k++) {
      if (!(fep = lUndumpElem(fp, fdp))) {
         LERROR(LEUNDUMPELEM);
         lFreeList(lp);
         free(found);
         DEXIT;
         return NULL;
      }

      if (!(ep = lCreateElem(dp))) {
         lFreeList(lp);
         free(found);
         LERROR(LECREATEELEM);
         DEXIT;
         return NULL;
      }

      for (i = 0; i < n; i++) {
         if (found[i] == -1)
            continue;
         else if (lCopySwitch(fep, ep, found[i], i) == -1) {
            lFreeList(lp);
            lFreeElem(ep);
            free(found);
            LERROR(LECOPYSWITCH);
            DEXIT;
            return NULL;
         }
      }
      lFreeElem(fep);
      if (lAppendElem(lp, ep) == -1) {
         lFreeList(lp);
         lFreeElem(ep);
         free(found);
         LERROR(LEAPPENDELEM);
         DEXIT;
         return NULL;
      }

   }

   /* read ket */
   if (fGetKet(fp)) {
      lFreeList(lp);
      printf("ket is missing\n");
      LERROR(LESYNTAX);
      DEXIT;
      return NULL;
   }

   DEXIT;
   return lp;
}

/* ===== functions used by the higher level functions ============ */

static int space_comment(
char *s 
) {
   char *p, *t;

   DENTER(CULL_LAYER, "space_comment");

   while ((t = strstr(s, "/*"))) {
      if (!(p = strstr(t + 2, "*/"))) {
         DEXIT;
         return -1;
      }
      while (t < p + 2)
         *t++ = ' ';
   }
   DEXIT;
   return 0;

}

static int fGetLine(
FILE *fp,
char *line,
int max_line 
) {

   DENTER(CULL_LAYER, "fGetLine");

   if (!fp) {
      LERROR(LEFILENULL);
      DEXIT;
      return -1;
   }

   if (!(fgets(line, max_line, fp))) {
      LERROR(LEFGETS);
      DEXIT;
      return -1;
   }

   if (space_comment(line)) {
      LERROR(LESPACECOMMENT);
      DEXIT;
      return -1;
   }

   DEXIT;
   return 0;
}

int fGetBra(
FILE *fp 
) {
   char s[READ_LINE_LENGHT + 1];

   DENTER(CULL_LAYER, "fGetBra");

   if (fGetLine(fp, s, READ_LINE_LENGHT)) {
      LERROR(LEFGETLINE);
      DEXIT;
      return -1;
   }

   DEXIT;
   return strstr(s, "{") ? 0 : -1;
}

int fGetKet(
FILE *fp 
) {
   char s[READ_LINE_LENGHT + 1];

   DENTER(CULL_LAYER, "fGetKet");

   if (fGetLine(fp, s, READ_LINE_LENGHT)) {
      LERROR(LEFGETLINE);
      DEXIT;
      return -1;
   }

   DEXIT;
   return strstr(s, "}") ? 0 : -1;
}

int fGetDescr(
FILE *fp,
lDescr *dp 
) {
   char s[READ_LINE_LENGHT + 1];
   int mt, nm, hash;
   char bra[2], comma[2], comma1[2], ket[2];

   DENTER(CULL_LAYER, "fGetDescr");

   if (!fp) {
      LERROR(LEFILENULL);
      DEXIT;
      return -1;
   }

   if (!dp) {
      LERROR(LEDESCRNULL);
      DEXIT;
      return -1;
   }

   if (fGetLine(fp, s, READ_LINE_LENGHT)) {
      LERROR(LEFGETLINE);
      DEXIT;
      return -1;
   }

   /* 
      We use this strange form of scanf to skip the 
      white space at the beginning. scanf is magic isn't it?
    */
   if (sscanf(s, "%1s %d %1s %d %1s %d %1s", bra, &nm, comma, &mt, comma1, &hash, ket) != 7) {
      LERROR(LESSCANF);
      DEXIT;
      return -1;
   }

   if (bra[0] != '{' || comma[0] != ',' || comma1[0] != ',' || ket[0] != '}') {
      LERROR(LESYNTAX);
      DEXIT;
      return -1;
   }

   dp->nm = nm;
   dp->mt = mt;
   if(hash == HASH_OFF) {  /* no hashing */
      dp->hash = NULL;
   } else {        /* create hashing info */
      if((dp->hash = (lHash *) malloc(sizeof(lHash))) == NULL) {
         LERROR(LEMALLOC);
         DEXIT;
         return -1;
      }
      dp->hash->type = hash;
      dp->hash->table = NULL;
   }

   DEXIT;
   return 0;
}

int fGetInt(
FILE *fp,
int *ip 
) {
   char s[READ_LINE_LENGHT + 1];

   DENTER(CULL_LAYER, "fGetInt");

   if (!fp) {
      LERROR(LEFILENULL);
      DEXIT;
      return -1;
   }

   if (fGetLine(fp, s, READ_LINE_LENGHT)) {
      LERROR(LEFGETLINE);
      DEXIT;
      return -1;
   }

   if (sscanf(s, "%d", ip) != 1) {
      LERROR(LESSCANF);
      DEXIT;
      return -1;
   }

   DEXIT;
   return 0;
}

int fGetUlong(
FILE *fp,
lUlong *up 
) {
   char s[READ_LINE_LENGHT + 1];

   DENTER(CULL_LAYER, "fGetUlong");

   if (!fp) {
      LERROR(LEFILENULL);
      DEXIT;
      return -1;
   }

   if (fGetLine(fp, s, READ_LINE_LENGHT)) {
      LERROR(LEFGETLINE);
      DEXIT;
      return -1;
   }

   if (sscanf(s, u32, up) != 1) {
      LERROR(LESSCANF);
      DEXIT;
      return -1;
   }

   DEXIT;
   return 0;
}

int fGetString(
FILE *fp,
lString *tp 
) {
   int i;
   char line[READ_LINE_LENGHT + 1];
   char sp[READ_LINE_LENGHT + 1];
   char *s;

   DENTER(CULL_LAYER, "fGetString");

   if (!fp) {
      LERROR(LEFILENULL);
      DEXIT;
      return -1;
   }

   if (fGetLine(fp, line, READ_LINE_LENGHT)) {
      LERROR(LEFGETLINE);
      DEXIT;
      return -1;
   }
   s = line;

   while (isspace((int) *s))
      s++;
   if (*s++ != '"') {
      LERROR(LESYNTAX);
      DEXIT;
      return -1;
   }
   for (i = 0; s[i] != '\0' && s[i] != '"'; i++)
      sp[i] = s[i];
   if (s[i] != '"') {
      LERROR(LESYNTAX);
      DEXIT;
      return -1;
   }
   sp[i] = '\0';

   if (!(*tp = strdup(sp))) {
      LERROR(LESTRDUP);
      DEXIT;
      return -1;
   }

   DEXIT;
   return 0;
}

int fGetFloat(
FILE *fp,
lFloat *flp 
) {
   char s[READ_LINE_LENGHT + 1];

   DENTER(CULL_LAYER, "fGetFloat");

   if (!fp) {
      LERROR(LEFILENULL);
      DEXIT;
      return -1;
   }

   if (fGetLine(fp, s, READ_LINE_LENGHT)) {
      LERROR(LEFGETLINE);
      DEXIT;
      return -1;
   }

   if (sscanf(s, "%f", flp) != 1) {
      LERROR(LESSCANF);
      DEXIT;
      return -1;
   }

   DEXIT;
   return 0;
}

int fGetDouble(
FILE *fp,
lDouble *dp 
) {
   char s[READ_LINE_LENGHT + 1];

   DENTER(CULL_LAYER, "fGetDouble");

   if (!fp) {
      LERROR(LEFILENULL);
      DEXIT;
      return -1;
   }

   if (fGetLine(fp, s, READ_LINE_LENGHT)) {
      LERROR(LEFGETLINE);
      DEXIT;
      return -1;
   }

   if (sscanf(s, "%lf", dp) != 1) {
      LERROR(LESSCANF);
      DEXIT;
      return -1;
   }

   DEXIT;
   return 0;
}

int fGetLong(
FILE *fp,
lLong *lp 
) {
   char s[READ_LINE_LENGHT + 1];

   DENTER(CULL_LAYER, "fGetLong");

   if (!fp) {
      LERROR(LEFILENULL);
      DEXIT;
      return -1;
   }

   if (fGetLine(fp, s, READ_LINE_LENGHT)) {
      LERROR(LEFGETLINE);
      DEXIT;
      return -1;
   }

   if (sscanf(s, "%ld", lp) != 1) {
      LERROR(LESSCANF);
      DEXIT;
      return -1;
   }

   DEXIT;
   return 0;
}

int fGetChar(
FILE *fp,
lChar *cp 
) {
   char s[READ_LINE_LENGHT + 1];

   DENTER(CULL_LAYER, "fGetChar");

   if (!fp) {
      LERROR(LEFILENULL);
      DEXIT;
      return -1;
   }

   if (fGetLine(fp, s, READ_LINE_LENGHT)) {
      LERROR(LEFGETLINE);
      DEXIT;
      return -1;
   }

   if (sscanf(s, "%c", cp) != 1) {
      LERROR(LESSCANF);
      DEXIT;
      return -1;
   }

   DEXIT;
   return 0;
}

int fGetList(
FILE *fp,
lList **lpp 
) {
   char s[READ_LINE_LENGHT + 1];

   DENTER(CULL_LAYER, "fGetList");

   if (!fp) {
      LERROR(LEFILENULL);
      DEXIT;
      return -1;
   }

   if (fGetLine(fp, s, READ_LINE_LENGHT)) {
      LERROR(LEFGETLINE);
      DEXIT;
      return -1;
   }

   if (strstr(s, "empty"))
      *lpp = NULL;              /* empty sublist */
   else {
      if (!strstr(s, "full")) {
         LERROR(LESYNTAX);
         DEXIT;
         return -1;
      }

      if (!(*lpp = lUndumpList(fp, NULL, NULL))) {
         LERROR(LEUNDUMPLIST);
         DEXIT;
         return -1;
      }
   }

   DEXIT;
   return 0;
}
