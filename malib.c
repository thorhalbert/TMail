/*----------------------------------------------------------------------*
 *	UNIX Emulation Package for RSTS - UEP/RSTS			*
 *									*
 *	Copyright (C) 1985 by Scott Halbert				*
 *----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*
 *									*
 *	malib - mail/network system library				*
 *									*
 *----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*
 *									*
 *	Edit	  Date		Who	Description of Changes Made 	*
 *									*
 *	  1	27-Sep-84	SH	Begin Coding			*
 *	  2	11-Oct-84	SH	Steal from POST for OFFICE	*
 *	  3	26-Nov-84	SH	Don't forget to close holdfiles	*
 *	  4	 7-May-85	SH	Change IOVTOA to FGETNAME	*
 *	  5	 9-May-85	SH	Extract params for mail.h	*
 *	  6	20-May-85	SH	Remove subs from OFFICE here	*
 *	  7	 6-Apr-93	SH	Create routines to parse	*
 *					 windows style .ini files and	*
 *					 do ~/ or $VAR for unix and 	*
 *					 %VAR% parsing from params	*
 *----------------------------------------------------------------------*/

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "uem.h"
#include "mail.h"

char *getenv(char *);

/*----------------------------------------------------------------------*
 *									*
 *	piece(pbuff, with) - just like streq, but only compares out	*
 *	to the length of "with", to perform partial comparisons		*
 *									*
 *----------------------------------------------------------------------*/

BOOL piece(pbuff,with)
char *pbuff,*with;
{

	int seq,len;
	len=strlen(with);

	for(seq=0;seq<len;seq++) if(*(pbuff+seq)!=*(with+seq)) return(FALSE);

	return(TRUE);

}

/*----------------------------------------------------------------------*
 *									*
 *	fcopy(in,out) - copy with getc file from in to out until eof	*
 *									*
 *----------------------------------------------------------------------*/

VOID fcopy(in,out)
FILE *in, *out;
{

	char cop;

	while (!feof(in)) {
		cop=getc(in);
		if (feof(in)) break;
		putc(cop, out);
	}
	return;

}

/*----------------------------------------------------------------------*
 *									*
 *	eatwhite(buffin) - scan down buffin until the first non-white	*
 *	char and return a pointer to that.				*
 *									*
 *----------------------------------------------------------------------*/

char *eatwhite(bufin)
char *bufin;
{

	for (;*bufin!='\0'&&isspace(*bufin);bufin++);

	return(bufin);

}

/*----------------------------------------------------------------------*
 *									*
 *	lowerit(string) - convert string to all lower in memory		*
 *									*
 *----------------------------------------------------------------------*/

char *lowerit(string)
char *string;
{

	char *org = string;

	for (; *string!='\0'; string++) *string = tolower(*string);

	return(org);

}


stopto(name,line)
char *name;
int line;
{
	printf("Stop to %s %d\n",name,line);
	exit(1);
}

delete(name)
char *name;
{

	unlink(name);
}

error(mask,d1,d2,d3,d4,d5,d6)
{
	fprintf(stderr,mask,d1,d2,d3,d4,d5,d6);
	exit(10);
}

fgetss(buff,len,chan)
char *buff;
int len;
FILE *chan;
{

	fgets(buff,len,chan);
	
	if (buff[strlen(buff)-1]=='\n') buff[strlen(buff)-1] = '\0';
}

/*-----------------------Parse .ini files--------------------------*/

VOID GetPrivateProfileString(const char *section_name, const char *var_name,
			     const char *default_value, char *ret_loc,
			     const int ret_loc_size, const char *prof_name)
{}

VOID WritePrivateProfileString(const char *section_name, const char *var_name,
			       const char *value, const char *prof_name)
{}

#ifdef UNIX
#include <pwd.h>
#endif

#define isident(a) (isalnum(a)||(a)=='_')

static char _sbuf[400];

char* ParseFileContents(char *incont)
{

#ifdef UNIX
	struct passwd *inpass;
	char copbuf[40],stbrk;
#endif
	char *p,*outcont = _sbuf;
	
	incont = eatwhite(incont);

#ifdef UNIX	/* Look for ~user and ~/ strings */
	
	if (*incont=='~') {
		incont++;
		if (*incont=='/') {
			incont++;
			inpass = getpwuid(getuid());
			strcpy(outcont, inpass->pw_dir);
			outcont = strchr(outcont, '\0');
			*outcont++ = '/';
		}
		else {
			for (p = copbuf; isident(*incont); p++,incont++) 
			    *p = *incont;
			*p = '\0';
			if (*incont=='/') incont++;
			inpass = getpwnam(copbuf);
			if (inpass!=NULL) {
				strcpy(outcont, inpass->pw_dir);
				outcont = strchr(outcont, '\0');
				*outcont++ = '/';
			}
		}
	}
#endif

	for(;*incont!='\0';) {

#ifdef UNIX	/* Look for embedded environment strings $str or $(env) or 
		   ${env} */

		if (*incont=='$') {
			incont++;
			stbrk=' ';
			if (*incont=='(') stbrk = ')';
			if (*incont=='{') stbrk = '}';
			if (stbrk!=' ') incont++;
			for (p = copbuf; isident(*incont); p++,incont++) 
			    *p = *incont;
			*p = '\0';
			if (*incont==stbrk) incont++;

			p = getenv(copbuf);
			if (p!=NULL) {
				strcpy(outcont, p);
				outcont = strchr(outcont, '\0');
			}
    		}
#endif

#ifdef UNIX	/* Take care of embedded backslashes */

		   if (*incont=='\\') {
			   incont++;
			   if (*incont!='\0') *outcont++ = *incont++;
		   }
#endif

#ifdef MSDOS	/* Take care of embedded %var% environment stuff */

		if (*incont=='%') {
			incont++;
			for (p = copbuf; isident(*incont); p++,incont++) 
			    *p = *incont;

			*p = '\0';
			if (*incont=='%') incont++;

			p = getenv(copbuf);
			if (p!=NULL) {
				strcpy(outcont, p);
				outcont = strchr(outcont, '\0');
			}
		}
#endif

		   *outcont++ = *incont++;

	   }

	*outcont++ = '\0';
	
	return(_sbuf);

}
