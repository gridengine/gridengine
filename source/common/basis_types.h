#ifndef __BASIS_TYPES_H
#define __BASIS_TYPES_H
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

#include <sys/types.h>

#ifdef __SGE_COMPILE_WITH_GETTEXT__
#  include <libintl.h>
#  include <locale.h>
#  include "sge_language.h"
#  define SGE_ADD_MSG_ID(x) (sge_set_message_id_output(1),(x),sge_set_message_id_output(0),1) ? 1 : 0 
#  define _(x)               sge_gettext(x)
#  define _MESSAGE(x,y)      sge_gettext_((x),(y))
#  define _SGE_GETTEXT__(x)  sge_gettext__(x)
#else
#  define SGE_ADD_MSG_ID(x) (x)
#  define _(x)              (x)
#  define _MESSAGE(x,y)     (y)
#  define _SGE_GETTEXT__(x) (x)
#endif

#if 0
#ifndef FALSE
#   define FALSE                (0)
#endif

#ifndef TRUE
#   define TRUE                 (1)
#endif
#endif

#ifndef  __cplusplus
typedef enum {
  false = 0,
  true
} bool;
#endif

#define FALSE_STR "FALSE"
#define TRUE_STR  "TRUE"

#define FALSE_LEN 5
#define TRUE_LEN  4

#define NONE_STR  "NONE"
#define NONE_LEN  4

#define U32CFormat "%ld"
#define u32c(x)  (unsigned long)(x)

#define X32CFormat "%lx"
#define x32c(x)  (unsigned long)(x)


#if defined(IRIX6) || defined(IRIX64)
#define u64 "%lld"
#define u64c(x)  (unsigned long long)(x)
#endif

#if defined(SUN4)
#  include <sys/param.h>
#endif

#if defined(ALPHA) || defined(HP11)
#  include <limits.h>
#else
#  ifndef WIN32NATIVE
#     include <sys/param.h>
#  endif
#endif

#ifdef  __cplusplus
extern "C" {
#endif

#if defined(ALPHA) || defined(IRIX6) || defined(CRAY) || defined(SOLARIS64) || defined(NECSX4) || defined(NECSX5) || defined(ALINUX) || defined(DARWIN)
#  define u_long32 u_int
#elif defined(WIN32NATIVE)
#  define u_long32 unsigned long
#else
#  define u_long32 u_long
#endif

/* set u32 and x32 for 64 or 32 bit machines */
/* uu32 for strictly unsigned, not nice, but did I use %d for an unsigned? */
#if defined(ALPHA) || defined(IRIX6) || defined(CRAY) || defined(SOLARIS64) || defined(NECSX4) || defined(NECSX5) || defined(ALINUX) || defined(DARWIN)
#  define u32    "%d"
#  define uu32   "%u"
#  define x32    "%x"
#  define fu32   "d"
#else
#  define u32    "%ld"
#  define uu32   "%lu"
#  define x32    "%lx"
#  define fu32   "ld"
#endif

/* -------------------------------
   solaris (who else - it's IRIX?) uses long 
   variables for uid_t, gid_t and pid_t 
*/
#define uid_t_fmt pid_t_fmt

#if (defined(SOLARIS) && !defined(SOLARIS64)) || defined(IRIX6)
#  define pid_t_fmt    "%ld"
#else
#  define pid_t_fmt    "%d"
#endif

#if (defined(SOLARIS) && !defined(SOLARIS64)) || defined(IRIX6) 
#  define gid_t_fmt    "%ld"
#elif defined(LINUX5)
#  define gid_t_fmt    "%hu"
#elif defined(LINUX6)
#  define gid_t_fmt    "%u"
#else
#  define gid_t_fmt    "%d"
#endif

/* _POSIX_PATH_MAX is only 255 and this is less than in most real systmes */
#define SGE_PATH_MAX    1024  

#define MAX_STRING_SIZE 2048
typedef char stringT[MAX_STRING_SIZE];

#define INTSIZE     4           /* (4) 8 bit bytes */
#if defined(_UNICOS)
#define INTOFF      4           /* big endian 64-bit machines where sizeof(int) = 8 */
#else
#define INTOFF      0           /* the rest of the world; see comments in request.c */
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) ((a)<(b)?(b):(a))
#endif

/* types */
/* these are used for complexes */
#define TYPE_INT          1
#define TYPE_FIRST        TYPE_INT
#define TYPE_STR          2
#define TYPE_TIM          3
#define TYPE_MEM          4
#define TYPE_BOO          5
#define TYPE_CSTR         6
#define TYPE_HOST         7
#define TYPE_DOUBLE       8

/* used in config */
#define TYPE_ACC          9
#define TYPE_LOG          10
#define TYPE_LOF          11
#define TYPE_LAST         TYPE_LOF

/* save string format quoted */
#define SFQ  "\"%-.100s\""
/* save string format non-quoted */
#define SFN  "%-.100s"
/* non-quoted string not limited intentionally */
#define SN_UNLIMITED  "%s"

/* used for shepherd and procfs */
#if defined(LINUX) || defined(SUN4) || defined(AIX4) || defined(HP10) || defined(HP11)
#  define MAX_GROUPS NGROUPS
#elif defined(IRIX6) || defined(SOLARIS)
#  define MAX_GROUPS NGROUPS_UMAX
#elif defined(ALPHA) || defined(HP11) || defined(SUN4) || defined(NECSX4) || defined(NECSX5)
#  define MAX_GROUPS NGROUPS_MAX
#endif

#if defined(SUN4)
    int setgroups(int ngroups, gid_t gidset[]);
    int seteuid(uid_t euid);
    int setegid(gid_t egid);
#endif

#if defined(HP10) || defined(HP11)
#  define seteuid(euid) setresuid(-1, euid, -1)
#  define setegid(egid) setresgid(-1, egid, -1)
#endif
    

#ifdef  __cplusplus
}
#endif

#ifndef TRUE
#  define TRUE 1
#  define FALSE !TRUE
#endif

#if defined(SGE_MT)
#define GET_SPECIFIC(type, variable, init_func, key, func_name) \
   type * variable; \
   if(!pthread_getspecific(key)) { \
      variable = (type *)malloc(sizeof(type)); \
      init_func(variable); \
      if (pthread_setspecific(key, (void*)variable)) { \
         sprintf(SGE_EVENT, "pthread_set_specific(%s) failed: %s\n", func_name, strerror(errno)); \
         sge_log(LOG_CRIT, SGE_EVENT,__FILE__,func_name,__LINE__); \
         abort(); \
      } \
   } \
   else \
      variable = pthread_getspecific(key)
#else
#define GET_SPECIFIC(type, variable, init_func, key, func_name)
#endif

#if defined(SGE_MT)
#define COMMLIB_GET_SPECIFIC(type, variable, init_func, key, func_name) \
   type * variable; \
   if(!pthread_getspecific(key)) { \
      variable = (type *)malloc(sizeof(type)); \
      init_func(variable); \
      if (pthread_setspecific(key, (void*)variable)) { \
         fprintf(stderr, "pthread_set_specific(%s) failed: %s\n", func_name, strerror(errno)); \
         abort(); \
      } \
   } \
   else \
      variable = pthread_getspecific(key)
#else
#define COMMLIB_GET_SPECIFIC(type, variable, init_func, key, func_name)
#endif

#if !defined(AIX42) && !defined(FREEBSD)
#define HAS_GETPWNAM_R
#define HAS_GETGRNAM_R
#define HAS_GETPWUID_R
#define HAS_GETGRGID_R
#endif

#if !defined(AIX42)
#define HAS_LOCALTIME_R
#define HAS_CTIME_R
#endif

#endif /* __BASIS_TYPES_H */
