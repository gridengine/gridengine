%{
#if defined(LINUX) || defined(ALPHA4) || defined(ALPHA5) || defined(IRIX6) || defined(HP11)
#  define YY_DECL int yylex(void)
#else
#  define YY_DECL extern "C" int yylex(void)
#endif
#include "culltrans_y.tab.h"
#include <string.h>
%}

liststart          LISTDEF
listend            LISTEND
cod                COD_
booldef            BOOL
floatdef           FLOAT
doubledef          DOUBLE
ulongdef           ULONG
longdef            LONG
chardef            CHAR
intdef             INT
stringdef          STRING
listdef            LIST
objdef             OBJECT
idlstart           \/\*IDL
idlend             XIDL\*\/
uppercase          [A-Z]
spaces             [ \t]*

%start list idl
%p 4000
%n 550

%%
{spaces}I{liststart}                  {BEGIN list; return IDLGEN_ILISTSTART; }
{spaces}S{liststart}                  {BEGIN list; return IDLGEN_SLISTSTART; }
<list>{spaces}{listend}               {BEGIN 0; return IDLGEN_LISTEND; }
<list>{spaces}{cod}K{ulongdef}                  return IDLGEN_KULONG;
<list>{spaces}{cod}K{stringdef}                 return IDLGEN_KSTRING;
<list>{spaces}{cod}I{booldef}                   return IDLGEN_IBOOL;
<list>{spaces}{cod}I{floatdef}                  return IDLGEN_IFLOAT;
<list>{spaces}{cod}I{doubledef}                 return IDLGEN_IDOUBLE;
<list>{spaces}{cod}I{ulongdef}                  return IDLGEN_IULONG;
<list>{spaces}{cod}I{longdef}                   return IDLGEN_ILONG;
<list>{spaces}{cod}I{chardef}                   return IDLGEN_ICHAR;
<list>{spaces}{cod}I{intdef}                    return IDLGEN_IINT;
<list>{spaces}{cod}I{stringdef}                 return IDLGEN_ISTRING;
<list>{spaces}{cod}IR{booldef}                  return IDLGEN_IRBOOL;
<list>{spaces}{cod}IR{floatdef}                 return IDLGEN_IRFLOAT;
<list>{spaces}{cod}IR{doubledef}                return IDLGEN_IRDOUBLE;
<list>{spaces}{cod}IR{ulongdef}                 return IDLGEN_IRULONG;
<list>{spaces}{cod}IR{longdef}                  return IDLGEN_IRLONG;
<list>{spaces}{cod}IR{chardef}                  return IDLGEN_IRCHAR;
<list>{spaces}{cod}IR{intdef}                   return IDLGEN_IRINT;
<list>{spaces}{cod}IR{stringdef}                return IDLGEN_IRSTRING;
<list>{spaces}{cod}I{listdef}                   return IDLGEN_ILIST;
<list>{spaces}{cod}I{objdef}                    return IDLGEN_IOBJECT;
<list>{spaces}{cod}IR{listdef}                  return IDLGEN_IRLIST;
<list>{spaces}{cod}IR{objdef}                   return IDLGEN_IROBJECT;
<list>{spaces}{cod}{booldef}                    return IDLGEN_BOOL;
<list>{spaces}{cod}{floatdef}                   return IDLGEN_FLOAT;
<list>{spaces}{cod}{doubledef}                  return IDLGEN_DOUBLE;
<list>{spaces}{cod}{ulongdef}                   return IDLGEN_ULONG;
<list>{spaces}{cod}{longdef}                    return IDLGEN_LONG;
<list>{spaces}{cod}{chardef}                    return IDLGEN_CHAR;
<list>{spaces}{cod}{intdef}                     return IDLGEN_INT;
<list>{spaces}{cod}{stringdef}                  return IDLGEN_STRING;
<list>{spaces}{cod}R{booldef}                   return IDLGEN_RBOOL;
<list>{spaces}{cod}R{floatdef}                  return IDLGEN_RFLOAT;
<list>{spaces}{cod}R{doubledef}                 return IDLGEN_RDOUBLE;
<list>{spaces}{cod}R{ulongdef}                  return IDLGEN_RULONG;
<list>{spaces}{cod}R{longdef}                   return IDLGEN_RLONG;
<list>{spaces}{cod}R{chardef}                   return IDLGEN_RCHAR;
<list>{spaces}{cod}R{intdef}                    return IDLGEN_RINT;
<list>{spaces}{cod}R{stringdef}                 return IDLGEN_RSTRING;
<list>{spaces}{cod}T{listdef}                   return IDLGEN_LIST;
<list>{spaces}{cod}R{listdef}                   return IDLGEN_RLIST;
<list>{spaces}{cod}R{objdef}                    return IDLGEN_ROBJECT;
<list>{spaces}{cod}X?{listdef}                  return IDLGEN_XLIST;
<list>{spaces}{cod}X{objdef}                    return IDLGEN_XOBJECT;
<list>{spaces}{cod}X{uppercase}*                {strcpy(yylval.name, yytext); return IDLGEN_XELEM;}
<list>[_a-zA-Z][_a-zA-Z0-9]*               {strcpy(yylval.name, yytext); return IDLGEN_IDENTIFIER;}
<list>{spaces}\({spaces}                   return IDLGEN_OPENBRACE;
<list>{spaces}\,{spaces}                   return IDLGEN_COMMA;
<list>{spaces}\)                           return IDLGEN_CLOSEBRACE;
<list>{spaces}{idlstart}                   BEGIN idl;
<list>\n                                   {yylval.other = '\n'; return IDLGEN_OTHER;}
<list>.                                    {yylval.other = yytext[0]; return IDLGEN_OTHER;}
<idl>{spaces}{idlend}\n                    BEGIN list;
<idl>.*\n                                  {strcpy(yylval.name, yytext); return IDLGEN_IDL;}
\n                                         {yylval.other = '\n'; return IDLGEN_OTHER;}
.                                          {yylval.other = yytext[0]; return IDLGEN_OTHER;}
