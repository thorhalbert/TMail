/*----------------------------------------------------------------------* 
 *	UNIX Emulation Package for RSTS - UEP/RSTS			* 
 *									* 
 *	Copyright (C) 1984 by Scott Halbert				* 
 *----------------------------------------------------------------------*/ 

#include <stdio.h> 
#include <uem.h> 
#include <rsts.h> 

/*----------------------------------------------------------------------* 
 *									* 
 *	setid.c - Maintain ident.rc identity file			* 
 *									* 
 *----------------------------------------------------------------------*/ 

int $$proj,$$prog,$$kbnum; 
char fibuf[30]; 

$xidget(idfil) 
struct idrec *idfil; 
{ 
	FILE *idfile; 
	int	idsize,idread; 

	sprintf(&fibuf,"%s%03d%03d.id",ident_act,$$proj,$$prog); 

	idsize=sizeof (struct idrec); 

	idfile=fopen(&fibuf,"rnu"); 

	if (idfile==NULL) return(NULL); 

	idread=fget(idfil,idsize,idfile); 

	if (idread!=idsize) return(NULL); 

	if (idfil->magic!=magprime) return(NULL); 

/*	printf("Uid=%d, Gid=%d, Username=\"%s\"\n",idfil->uid,idfil->gid, 
		&(idfil->uname)); */ 

	fclose(idfile); 

	return(idfil->uid);		 

} 
	 
wrtid(idfil) 
struct idrec *idfil; 
{ 
	FILE *idfile; 
	int	idsize,idread; 

	idsize=sizeof (struct idrec); 

	idfile=fopen(&fibuf,"wnu"); 

	if (idfile==NULL) error("Could not write %s file\n",&fibuf); 

	idfil->magic=magprime; 

	fput(idfil,idsize,idfile); 

	if (ferr(idfile)==1) error("Some error was discovered writing %s\n", 
		&fibuf); 

	fclose(idfile); 

	return(0);		 
} 

main(argc,argv) 
char *argv[]; 
int argc; 
{ 

	struct idrec user; 
	int retflg,tuid,tgid,parsed,complete,a,b,ilen; 
	char buff[82]; 
	register int	errret; 

	clrfqx();		 
	 
	firqb.fqfun = UU_SYS;	/* Return job status call */ 
	firqb.fqsizm = 0;	/* Get the first table */ 
	firqb.fqfil = 0;	/* Get our job number */ 

	if ((errret=rstsys(_UUO))!=0) error("Cannot find identity\n"); 

/*	dumpfqb();	*/	/* Dump the firqb */ 

	$$proj=firqb.fqprot & 0377; 
	$$prog=firqb.fqpflg & 0377; 
	$$kbnum=firqb.fqsizm; 

	printf("Setid - Maintain ident files [%d,%d]\n\n",$$proj,$$prog); 

	if (argc!=1&&argc!=3) error("incorrect number of args"); 
	if (argc==3) { 
		$$proj=atoi(argv[1]); 
		$$prog=atoi(argv[2]); 
	} 

	retflg=$xidget(&user); 
	printf("Opening %s ident file\n\n",&fibuf); 

	if (retflg==NULL) { 
		printf("Ident not set up or invalid\n"); 
		user.uid=0; 
		user.gid=0; 
		user.uname[0]='\0'; 
		user.fulln[0]='\0'; 
		user.offic[0]='\0'; 
		user.oficp[0]='\0'; 
		user.homep[0]='\0'; 
	} 

	complete=FALSE; 

	while(complete==FALSE) { 

		tuid=0; 
		while(tuid<=0) { 

			printf("\nUser id number <%d> ",user.uid); 
			gets(&buff); 
			parsed=sscanf(&buff,"%d",&tuid); 
			if (parsed==0) tuid=user.uid; 
			if (tuid<=0) continue; 
			user.uid=tuid; 

		} 

		tgid=0; 
		while(tgid<=0) { 

			printf("Group id number <%d> ",user.gid); 
			gets(&buff); 
			parsed=sscanf(&buff,"%d",&tgid); 
			if (parsed==0) tgid=user.gid; 
			if (tgid<=0) continue; 
			user.gid=tgid; 

		} 

		buff[0]='\0'; 
		while(buff[0]=='\0') { 

			printf("User name <%s> ",&user.uname); 
			gets(&buff); 
			b=0; 
			ilen=strlen(&buff); 
			if (ilen==0) strcpy(&buff,&user.uname); 
			ilen=strlen(&buff); 
			if (ilen>=30) ilen=29; 
			for(a=0; a<=ilen; a++) { 
				buff[a]=tolower(buff[a]); 
				if (buff[a]=='-') buff[a]='_'; 
				if (((buff[a]>='a')&& 
				     (buff[a]<='z')) || 
				    ((buff[a]>='0')&& 
				     (buff[a]<='9'))|| 
				    (buff[a]=='_')) buff[b++]=buff[a]; 
			} 

			buff[b]='\0';			 
				 
			if (buff[0]=='\0') continue; 

			strcpy(&user.uname,&buff); 

		} 

		buff[0]='\0'; 
		while(buff[0]=='\0') { 

			printf("Full Name <%s> ",&user.fulln); 
			gets(&buff); 
			b=0; 
			ilen=strlen(&buff); 
			if (ilen==0) strcpy(&buff,&user.fulln); 
			ilen=strlen(&buff); 
			if (ilen>=50) ilen=49; 
			strcpy(&user.fulln,&buff); 

		} 

		buff[0]='\0'; 
		while(buff[0]=='\0') { 

			printf("Office <%s> ",&user.offic); 
			gets(&buff); 
			b=0; 
			ilen=strlen(&buff); 
			if (ilen==0) strcpy(&buff,&user.offic); 
			ilen=strlen(&buff); 
			if (ilen>=50) ilen=49; 
			strcpy(&user.offic,&buff); 

		} 

		buff[0]='\0'; 
		while(buff[0]=='\0') { 

			printf("Office Phone <%s> ",&user.oficp); 
			gets(&buff); 
			b=0; 
			ilen=strlen(&buff); 
			if (ilen==0) strcpy(&buff,&user.oficp); 
			ilen=strlen(&buff); 
			if (ilen>=30) ilen=29; 
			strcpy(&user.oficp,&buff); 

		} 

		buff[0]='\0'; 
		while(buff[0]=='\0') { 

			printf("Home Phone <%s> ",&user.homep); 
			gets(&buff); 
			b=0; 
			ilen=strlen(&buff); 
			if (ilen==0) strcpy(&buff,&user.homep); 
			ilen=strlen(&buff); 
			if (ilen>=30) ilen=29; 
			strcpy(&user.homep,&buff); 

		} 

		printf("\nUser id number %d\n",user.uid); 
		printf("Group id number %d\n",user.gid); 
		printf("Full Name \"%s\"\n",&user.fulln); 
		printf("Office \"%s\"\n",&user.offic); 
		printf("Office Phone \"%s\"\n",&user.oficp); 
		printf("Home Phone\"%s\"\n",&user.homep); 

		printf("\nAcceptable? [nyq] "); 

		gets(&buff); 
		if (tolower(buff[0])=='q')  
			error("Action aborted-no changes\n"); 

		if (tolower(buff[0])=='y') complete=TRUE; 

	} 

	wrtid(&user); 

	printf("[Done]\n"); 

	exit(); 

} 

