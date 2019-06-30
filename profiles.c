#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "uem.h"

char *eatwhite(char *);

struct classvar {
	char* varname;

	char* varvalue;

	struct classvar* next;
};

struct iniclass {

	char* classname;

	struct iniclass* next;
	struct classvar* rootvar,*tailvar;

}* rootclass=NULL, *tailclass=NULL;

VOID sclean(char *iv)
{

	char *fp;

	fp = &iv[strlen(iv)];
	fp--;

	while (fp>=iv) {
		if (isspace(*fp)) {
			*fp='\0';
			fp--;
		} else break;
	}
}

struct iniclass* findclass(char* inbuf)
{

	struct iniclass *newclass;

	for (newclass=rootclass; newclass!=NULL; newclass=newclass->next) 
		if (strcmp(newclass->classname,inbuf)==0) return(newclass);

	return(NULL);
}

struct iniclass* makeclass(char* inbuf)
{

	char newval[strlen(inbuf)+1];
	char *fp;
	struct iniclass *newclass;

	strcpy(newval,eatwhite(&inbuf[1]));
	fp = strchr(newval,']');
	if (fp!=NULL) 
		*fp = '\0';

	sclean(newval);

	for (newclass=rootclass; newclass!=NULL; newclass=newclass->next) 
		if (strcmp(newclass->classname,newval)==0) return(newclass);

	newclass = (struct iniclass *) malloc(sizeof(struct iniclass));
	if (newclass==NULL) exit(200);
	      
	newclass->classname = strdup(newval);
	newclass->next = NULL;
	newclass->rootvar = NULL;
	newclass->tailvar = NULL;

	if (tailclass!=NULL) 		/* Link into linked list */
	    tailclass->next = newclass;
	else rootclass = newclass;

	tailclass = newclass;

	return(newclass);
}

VOID parsevar(struct iniclass *thisclass, char *_varnam,char *_varval)
{

	struct classvar *loop;
	
	char varnam[strlen(_varnam)+1];
	char varval[strlen(_varval)+1];

	char *eqs;

	if (thisclass==NULL) return;	/* No class declared at this point */

	strcpy(varnam,_varnam);
	strcpy(varval,eatwhite(_varval));

	sclean(varnam);
	sclean(varval);

	for (loop=thisclass->rootvar; loop!=NULL; loop=loop->next) {
		if (strcmp(varnam,loop->varname)==0) {
			if (loop->varvalue!=NULL)
			    free(loop->varvalue);
			loop->varvalue = strdup(varval);
			return;
		}
	}

	loop = (struct classvar *) malloc(sizeof(struct classvar));
	if (loop==NULL) exit(200);
	      
	loop->next = NULL;
	loop->varname = strdup(varnam);
	loop->varvalue = strdup(varval);

	if (thisclass->tailvar!=NULL)
	    thisclass->tailvar->next = loop;
	else thisclass->rootvar = loop;
	thisclass->tailvar = loop;

}

VOID readprofile(char* profilnam)
{

	FILE* prof;
	char ibuf[1024],*fb,*eqs;
	struct iniclass* thisclass=NULL;

	prof = fopen(profilnam,"r");
	if (prof==NULL) return;

	while (!feof(prof)) {
		fgetss(ibuf,1023, prof);
		fb = eatwhite(ibuf);
		if (fb[0]=='\0') continue;
		if (fb[0]==';') continue;	/* Kill comments for now */
		if (fb[0]=='[') {
			thisclass = makeclass(fb);
			continue;
		}
		eqs = strchr(fb,'=');
		if (eqs!=NULL) {
			*eqs++ = '\0';
			parsevar(thisclass,fb,eqs);
		}
	}
}

VOID writeprofile(char* profilnam)
{

	FILE *oprof;
	struct iniclass *cloop;
	struct classvar *vloop;

	oprof = fopen(profilnam,"w");   
	if (oprof==NULL) {
		fprintf(stderr,"Cannot open output profile %s\n",profilnam);
		return;
	}

	for (cloop=rootclass; cloop!=NULL; cloop=cloop->next) {
		fprintf(oprof,"[%s]\n",cloop->classname);
		for (vloop=cloop->rootvar; vloop!=NULL; vloop=vloop->next) {
			fprintf(oprof,"%s=%s\n",
				vloop->varname,vloop->varvalue);
		}
		fprintf(oprof,"\n");
	}

	fclose(oprof);

}

char *findprofile(char *iclass, char *varname, char *idefault)
{

	struct iniclass* thisclass;
	struct classvar *loop;

	thisclass = findclass(iclass);
	if (thisclass==NULL) return(idefault);

	for (loop=thisclass->rootvar; loop!=NULL; loop=loop->next) {
		if (strcmp(varname,loop->varname)==0) {
			if (loop->varvalue!=NULL)
			    return(loop->varvalue);
			return;
		}
	}

	return(idefault);
}

char *bombprofile(char *iclass, char *varname)
{

	struct iniclass* thisclass;
	struct classvar *loop;

	thisclass = findclass(iclass);
	if (thisclass==NULL) {

		fprintf(stderr,"Mandatory  profile class [%s]/%s missing\n",
			iclass,varname);
		exit(100);
	}

	for (loop=thisclass->rootvar; loop!=NULL; loop=loop->next) {
		if (strcmp(varname,loop->varname)==0) {
			if (loop->varvalue!=NULL)
			    return(loop->varvalue);
			return;
		}
	}

	fprintf(stderr,"Mandatory profile variable [%s]/%s missing\n",
		iclass,varname);
	exit(100);
}

