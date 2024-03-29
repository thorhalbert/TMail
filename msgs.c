/*----------------------------------------------------------------------* 
 *	UNIX Emulation Package for RSTS - UEP/RSTS			* 
 *									* 
 *	Copyright (C) 1984 by Scott Halbert				* 
 *----------------------------------------------------------------------*/ 

#include <stdio.h> 

/*----------------------------------------------------------------------* 
 *									* 
 *	msgs - system messages and junk mail program			* 
 *									* 
 *----------------------------------------------------------------------*/ 

#define	MSG_FILE	"msgs:bounds." 
#define RC_FILE		"sy:msgs.rc" 
/*#define DEBUG*/ 

#define	EX_RED	-1 
#define	EX_QUT	-2 

int	hi_bound,low_bound,rc_bound,anyout; 

bounds() 
{ 
	FILE *boundf; 
	int ans; 
	 
	if((boundf=fopen(MSG_FILE,"r"))==NULL) 
		error("Boundary file (%s) cannot be opened--Agony",MSG_FILE); 

	ans=fscanf(boundf,"%d %d",&low_bound,&hi_bound); 

	if (ans!=2)  
		error("Boundary file (%s) had an illegal format--Agony", 
			MSG_FILE); 

	if (hi_bound<low_bound||hi_bound<0||low_bound<0)  
		error("Boundary file (%s) had illegal values--Agony", 
			MSG_FILE); 

#ifdef DEBUG 
	printf("Bounds file contained Low %d and High %d\n",low_bound,hi_bound); 
#endif 

	fclose(boundf); 

} 

getrc() 
{ 
	FILE *rcfile; 
	int ans; 
	 
	rc_bound=0; 

	if((rcfile=fopen(RC_FILE,"r"))==NULL) return(0); 

	ans=fscanf(rcfile,"%d",&rc_bound); 

	if (ans!=1)  
		fprintf(stderr,"[msgs.rc file has bad format--ignoring\n]"); 

#ifdef DEBUG 
	printf("getrc file contained %d\n",rc_bound); 
#endif 

	fclose(rcfile); 

} 

wrtrc() 
{ 
	FILE *rcfile; 
	int ans; 
	 
	if((rcfile=fopen(RC_FILE,"w"))==NULL)  
		error("?Cannot open the %s file for output\n",RC_FILE); 

	fprintf(rcfile,"%d\n",rc_bound); 

#ifdef DEBUG 
	printf("Wrote new %s file with %d\n",RC_FILE,rc_bound); 
#endif 

	fclose(rcfile); 

} 

int 
exm_msg(mesnum) 
int mesnum; 
{ 
	FILE *mesage; 

	char fbuf[40]; 
	char ilbuf[202]; 
	int hedcnt,bodcnt,bodflg,more,gotsub; 

	sprintf(&fbuf,"msgs:%06d.msg",mesnum); 

#ifdef DEBUG 
	printf("Now opening message file %s\n",&fbuf); 
#endif 

	mesage=fopen(&fbuf,"r"); 
	if (mesage==NULL) return(0); 

	if (anyout==TRUE) printf("-----\n\n"); 

	anyout=TRUE; 

	printf("Message %d:\n",mesnum); 

	hedcnt=0; 
	bodcnt=0; 
	bodflg=FALSE; 
	gotsub=FALSE; 

	while(!feof(mesage)) { 

		if (fgetss(&ilbuf,200,mesage)==NULL) break; 
		if (bodflg==FALSE&&ilbuf[0]=='\0') { 
			bodflg=TRUE; 
			continue; 
		} 

		if (bodflg==FALSE) { 
			if (tolower(ilbuf[0])=='f'&& 
				tolower(ilbuf[1])=='r'&& 
				tolower(ilbuf[2])=='o'&& 
				tolower(ilbuf[3])=='m'&& 
				tolower(ilbuf[4])==' ') puts(&ilbuf, stdout); 
			if (tolower(ilbuf[0])=='s'&& 
				tolower(ilbuf[1])=='u'&& 
				tolower(ilbuf[2])=='b'&& 
				tolower(ilbuf[3])=='j') { 
					gotsub=TRUE; 
					puts(&ilbuf, stdout); 
			} 

		} 

		if (bodflg==FALSE) hedcnt++; 
		else	bodcnt++; 

	} 

#ifdef DEBUG 
	printf("Header was %d, body was %d\n",hedcnt,bodcnt); 
#endif 

	fclose(mesage); 
	mesage=fopen(&fbuf,"r"); 
	if (mesage==NULL) return(0); 

	for(more=0;more<=hedcnt;more++)  
		if (fgetss(&ilbuf,200,mesage)==NULL) break; 
	 
	printf("(%d lines) More? [ynq] ",bodcnt); 

	fgetss(&ilbuf,200,stdin); 

	if (tolower(ilbuf[0])=='q') return(EX_QUT); 
	if (tolower(ilbuf[0])=='n') return(0); 

	while(!feof(mesage)) { 

		if (fgetss(&ilbuf,200,mesage)==NULL) break; 
		puts(&ilbuf, stdout); 

	} 

	fclose(mesage); 

	return(0); 
} 

main() 
{ 

	int exm; 

	onlytty(); 

	anyout=FALSE; 

	bounds(); 
	getrc(); 
	if (rc_bound>hi_bound) { 
		fprintf(stderr,"No New Messages\n"); 
		rc_bound=hi_bound+1; 
		wrtrc(); 
		exit(1); 
	} 

	for(;rc_bound<=hi_bound;rc_bound++) { 

		if (rc_bound<low_bound) { 
			rc_bound=low_bound-1; 
			continue; 
		} 

		exm=exm_msg(rc_bound); 

		if (exm==EX_RED) { 
			rc_bound=rc_bound-2; 
			continue; 
		} 

		if (exm==EX_QUT) { 
			fprintf(stderr,"--Postponed--\n"); 
			wrtrc(); 
			exit(1);		 

		} 

		if (exm>0) { 
			rc_bound=exm-1; 
			continue; 
		} 

	} 

	fprintf(stderr,"-----\n"); 
	rc_bound=hi_bound+1; 
	wrtrc(); 
	exit(1); 

} 

