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
#include <string.h>

#include "sec_certtext.h"

/*
** NAME
**      sec_*
**
** SYNOPSIS
**      #include "sec_certtext.h"
**
**      int sec_*()
**
** DESCRIPTION
**	These functions generate the text for a certificate.
**
** RETURN VALUES
**      0       on success
**      -1      on failure
*/

int sec_certtext(
char *textfile,
char *defaultfile 
) {
	int	i,last;
	char	data[6][128];
	char	defaults[6][128];
	char	text[6][64];

	i = sec_read_defaults(defaults,defaultfile);
	if(i) sec_set_defaults(defaults);
	sec_set_text(text);

	last = sec_read_data(data,defaults,text);
	if(last<0){ 
		fprintf(stderr,"Not enough data!\n");
		return(-1);
	}
	
	i = sec_write_defaults(data,defaultfile);
	if(i) return(-1);
	i = sec_write_data(data,last,textfile);
	if(i) return(-1);
	return(0);
}

void sec_set_defaults(defaults)
char	defaults[6][128];
{
	strcpy(defaults[0],"de");
	strcpy(defaults[1],"Bayern");
	strcpy(defaults[2],"Regensburg");
	strcpy(defaults[3],"Grid Engine");
	strcpy(defaults[4],"SGE");
	strcpy(defaults[5],"Torsten Bieder");
}

void sec_set_text(text)
char	text[6][64];
{
        strcpy(text[0],"Country Name");
        strcpy(text[1],"State or Province");
        strcpy(text[2],"Locality Name");
        strcpy(text[3],"Organisation Name");
        strcpy(text[4],"Unit Name");
        strcpy(text[5],"Common Name");
}

int sec_read_data(data,defaults,text)
char    data[6][128];
char    defaults[6][128];
char    text[6][64];
{
	int	i,j,last=-1;
	char	buf[128];

	fprintf(stderr,"\nEnter 'enter' to use the default value in brackets [].\n");
	fprintf(stderr,"Enter '.'     to leave field blank.\n\n");
        for(i=0;i<6;i++){	
	      retry:
		fprintf(stderr,"%s [%s] : ",text[i],defaults[i]);
	        fflush(stderr);
        	buf[0]='\0';
        	fgets(buf,128,stdin);
        	if (buf[0] == '\0'){
			fprintf(stderr,"Bad input! Retry!\n");
			goto retry;
		}
        	else if (buf[0] == '\n'){
                	if ((defaults[i] == NULL) || (defaults[i][0] == '\0')){
				fprintf(stderr,"No default value! Retry!\n");
				goto retry;
			}
                	strcpy(buf,defaults[i]);
			last = i;
                }
        	else if ((buf[0] == '.') && (buf[1] == '\n')) buf[0] ='\0';
		else{
			j = strlen(buf);
        		if (buf[j-1] != '\n'){
                		fprintf(stderr,"weird input! Retry!\n");
				goto retry;
			}
        		buf[--j]='\0';
			last = i;
		}
		if(sec_printable((unsigned char *) buf)) goto retry;
		strcpy(data[i],buf);
		strcat(data[i],"\n");
	}
	return(last);
}

int sec_printable(
unsigned char *s 
        ) {
        int c;

        while (*s)
        {
                c= *(s++);
                if (!(  ((c >= 'a') && (c <= 'z')) ||
                        ((c >= 'A') && (c <= 'Z')) ||
                        (c == ' ') ||
                        ((c >= '0') && (c <= '9')) ||
                        (c == ' ') || (c == '\'') ||
                        (c == '(') || (c == ')') ||
                        (c == '+') || (c == ',') ||
                        (c == '-') || (c == '.') ||
                        (c == '/') || (c == ':') ||
                        (c == '=') || (c == '?'))){
				fprintf(stderr,"Bad character '%c'! Retry!\n",c);
                        	return(-1);
		}
        }
        return(0);
}

int sec_write_data(data,last,textfile)
char	data[6][128];
int	last;
char	*textfile;
{
	FILE	*fp;

	fp = fopen(textfile,"w");
	if(fp == NULL){
		fprintf(stderr,"Can't open certificate file '%s'\n",textfile);
		return(-1);
	}
	fprintf(fp,"X.509-Certificate begin\n");
	fprintf(fp,"CertificateInfo begin\n");
	fprintf(fp,"- 0F\n");
	fprintf(fp,"AlgorithmIdentifier begin\n");
        fprintf(fp,"= 1 2 840 113549 1 1 4\n");
        fprintf(fp,"= NULL\n");
        fprintf(fp,"AlgorithmIdentifier end\n");
	sec_write_name(data,last,fp);
        fprintf(fp,"Validity begin\n");
        fprintf(fp,"= 941202235444Z\n");
        fprintf(fp,"= 961201235444Z\n");
        fprintf(fp,"Validity end\n");
        sec_write_name(data,last,fp);
        fprintf(fp,"SubjectPublicKeyInfo begin\n");
        fprintf(fp,"AlgorithmIdentifier begin\n");
        fprintf(fp,"= 1 2 840 113549 1 1 1\n");
        fprintf(fp,"= NULL\n");
        fprintf(fp,"AlgorithmIdentifier end\n");
        fprintf(fp,"- 00\n");
        fprintf(fp,"SubjectPublicKeyInfo end\n");
        fprintf(fp,"CertificateInfo end\n");
        fprintf(fp,"AlgorithmIdentifier begin\n");
        fprintf(fp,"= 1 2 840 113549 1 1 4\n");
        fprintf(fp,"= NULL\n");
        fprintf(fp,"AlgorithmIdentifier end\n");
        fprintf(fp,"- 00\n");
        fprintf(fp,"X.509-Certificate end\n");
	fclose(fp);
	return(0);
}

void sec_write_name(data,last,fp)
char    data[6][128];
int     last;
FILE	*fp;
{
	fprintf(fp,"Name begin\n");
	if(data[0][0] != '\n'){
        	fprintf(fp,"= 2 5 4 6\n");
		if(last == 0) fprintf(fp,"- %s",data[0]);
		else fprintf(fp,"= %s",data[0]);
	}
        if(data[1][0] != '\n'){
                fprintf(fp,"= 2 5 4 8\n");
                if(last == 1) fprintf(fp,"- %s",data[1]);
                else fprintf(fp,"= %s",data[1]);
        }
        if(data[2][0] != '\n'){
                fprintf(fp,"= 2 5 4 7\n");
                if(last == 2) fprintf(fp,"- %s",data[2]);
                else fprintf(fp,"= %s",data[2]);
        }
        if(data[3][0] != '\n'){
                fprintf(fp,"= 2 5 4 10\n");
                if(last == 3) fprintf(fp,"- %s",data[3]);
                else fprintf(fp,"= %s",data[3]);
        }
        if(data[4][0] != '\n'){
                fprintf(fp,"= 2 5 4 11\n");
                if(last == 4) fprintf(fp,"- %s",data[4]);
                else fprintf(fp,"= %s",data[4]);
        }
        if(data[5][0] != '\n'){
                fprintf(fp,"= 2 5 4 3\n");
                if(last == 5) fprintf(fp,"- %s",data[5]);
                else fprintf(fp,"= %s",data[5]);
        }
        fprintf(fp,"Name end\n");
}

int sec_read_defaults(defaults,defaultfile)
char	defaults[6][128];
char	*defaultfile;
{
	int	i,j;
	FILE	*fp;

	fp = fopen(defaultfile,"r");
	if(fp == NULL) return(-1);
	for(i=0;i<6;i++){
		fgets(defaults[i],128,fp);
                j = strlen(defaults[i]);
                if (defaults[i][j-1] != '\n'){
			return(-1);
			if(fp) fclose(fp);
		}
                defaults[i][--j]='\0';
	}
	if(fp) fclose(fp);
	return(0);
}

int sec_write_defaults(defaults,defaultfile)
char    defaults[6][128];
char    *defaultfile;
{
        int     i;
        FILE    *fp;

        fp = fopen(defaultfile,"w");
        if(fp == NULL){
                fprintf(stderr,"Can't open default file '%s'\n",defaultfile);
                return(-1);
        }

        for(i=0;i<6;i++)
		fputs(defaults[i],fp);
	if(fp) fclose(fp);
	return(0);
}
