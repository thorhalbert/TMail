/*----------------------------------------------------------------------* 
 *	UNIX Emulation Package for RSTS - UEP/RSTS			* 
 *									* 
 *	Copyright (C) 1985 by Scott Halbert				* 
 *----------------------------------------------------------------------*/ 

char *pname = "Office V1.0(6)"; 

/*----------------------------------------------------------------------* 
 *									* 
 *	Edit	  Date		Who	Description of Changes Made 	* 
 *									* 
 *	  1	27-Sep-84	SH	Begin Coding			* 
 *	  2	11-Oct-84	SH	Steal from POST for OFFICE	* 
 *	  3	26-Nov-84	SH	Don't forget to close holdfiles	* 
 *	  4	 7-May-85	SH	Change IOVTOA to FGETNAME	* 
 *	  5	 9-May-85	SH	Extract params for mail.h	* 
 *	  6	20-May-85	SH	Extract common subroutines	* 
 *									* 
 *----------------------------------------------------------------------*/ 

#include <stdio.h> 
#include "src:mail.h" 

/*#define TRACE */ 
#ifdef TRACE 
extern FILE *$$flow; 
#endif 

/*----------------------------------------------------------------------* 
 *									* 
 *	office - service delivery queue and put local mail into local	* 
 *	user mailboxes - put export mail into export queue		* 
 *									* 
 *----------------------------------------------------------------------*/ 

#define MAXBUF 300 

int	metoo, debug, verbose, sequence, find; 
int	oneshot, dragon; 

char	buff[MAXBUF+2],*tim; 

FILE 	*mailin, *mailout, *mailtemp; 

int uid,gid; 
char uname[20],uact[40]; 

int black[BLAKLEN],blacki[BLAKLEN];	/* Letter and timeout lists */ 

stamp() 
{ 
	tim=ztime(NULL); 
	printf("....Time stamp %s....\n",tim); 
	return; 
} 

/*----------------------------------------------------------------------* 
 *									* 
 *	Routines to maintain the black list...  The black list is a	* 
 *	list of the message numbers which will not be considered	* 
 *	until the timeout timer runs out on them, every cycle the	* 
 *	list is decremented, and the zero cells are cleared, here	* 
 *	is a list of subroutines and what they do:			* 
 *									* 
 *	initblk() - clean and initialize the arrays			* 
 *	findblk(letter) - return TRUE if letter is blacklisted		* 
 *	putblk(letter) - put letter in the black list, if the list	* 
 *		is full, replace the blacklisted entry with the least	* 
 *		time left with letter					* 
 *	cycleblk() - cycle the blacklist, clean up timeouts		* 
 *									* 
 *----------------------------------------------------------------------*/ 

initblk() 
{ 
	int i; 

	for(i=0;i<BLAKLEN;i++) black[i]=blacki[i]=0; 

} 
findblk(letnam) 
int letnam; 
{ 

	int i; 
	for (i=0;i<BLAKLEN;i++)  
		if (black[i]==letnam) { 
			if (blacki[i]<=1) black[i]=blacki[i]=0; 
			else return(TRUE); 
		} 
	return(FALSE); 
} 

putblk(letnam) 
int letnam; 
{ 
	int i,ms; 
	ms=BLAKTIM+1; 
	for (i=0;i<BLAKLEN;i++) if (blacki[i]<ms) ms=i; 
	black[ms]=letnam; 
	blacki[ms]=BLAKTIM; 
} 
cycleblk() 
{ 

	int i; 
	for (i=0;i<BLAKLEN;i++) { 
			blacki[i]--; 
			if (blacki[i]<=0) black[i]=blacki[i]=0; 
		} 
	return; 

} 

/*----------------------------------------------------------------------* 
 *									* 
 *	getpiece() - search the mailbox for the first viable piece 	* 
 *	mail and return its number					* 
 *									* 
 *----------------------------------------------------------------------*/ 

getpiece()  
{ 

	FILE *ranfile, *gethold; 
	char filbuf[40]; 
	int renum,scn; 

	ranfile=fwild(BOX_WILD,"r"); 

	while(TRUE) { 

		if(fnext(ranfile)==NULL) return(0); 

		fgetname(ranfile,&filbuf); 

#ifdef rsx 
		for(scn=0; filbuf[scn]!=']' && scn<strlen(filbuf); scn++); 
#endif 
#ifdef RT11 
		for(scn=0; filbuf[scn]!=':' && scn<strlen(filbuf); scn++); 
#endif 

		sscanf(&filbuf[scn+1], "%d",&renum); 

		if (verbose) stamp(); 
		if (verbose) printf("[[Saw %s number %d]]\n",&filbuf,renum); 

		if(findblk(renum)) { 
			printf("[[This message is on the blacklist]]\n"); 
			continue; 
		} 

		sprintf(&filbuf,MHLD_MSK,renum); 

		if (debug) printf("<<Checking for holdfile presence %s", 
			&filbuf); 

		if ((gethold=fopen(&filbuf,"r"))==NULL) { 
			if (debug) printf(" - File not found>>\n"); 
			fclose(ranfile); 
			return(renum); 
		} 

		if (debug) printf(" - File found>>\n"); 

		fclose(gethold); 

		if (verbose) printf("[[POST has a holdfile on this]]\n"); 

	} 

} 

lookup(indata) 
char *indata; 
{ 

	FILE *scfile; 

	scfile=NULL; 

	while (scfile==NULL) { 

		scfile=fopen(NAMEFILE, "r"); 

		if (scfile==NULL) { 
		  stamp(); 
		  printf("?Cannot find the name list %s... This is bad.\n", 
			NAMEFILE); 
		  printf("---OFFICE will wait for 30 mins and then retry\n"); 
		  fflush(stdout); 
			sleep(30*60); 

		} 
	} 

	while(!feof(scfile)) { 

		fscanf(scfile,"%d %d %s %s",&uid,&gid,&uname,&uact); 

		if (streq(&uname,indata)==TRUE) { 
			fclose(scfile); 
			return(TRUE); 
		} 

	} 

	fclose(scfile); 
	return(FALSE); 

} 

nameget() 
{ 

	char infile[40],who[20],tfile[40]; 
#ifdef RSTS_B 
	int proj,prog,tm; 
#endif 

	sprintf(&infile, MBOX_MSK, find); 

	mailin=fopen(&infile, "r"); 

	if (mailin==NULL) { 
	   stamp(); 
	   printf("?Cannot open previously detected file %s with error %d\n", 
			&infile,$$ferr); 
		printf("---message %d will be black listed for %d secs\n", 
			find,CYCLE*BLAKTIM); 
		return(FALSE); 
	} 

	fgetss(&buff,MAXBUF,mailin); 

	if (debug) printf("<<%s>>\n", &buff); 

	if (piece(&buff, "To ")==FALSE) { 

		/* Dispose of illegal mail */ 

		stamp(); 
		printf("%Message %d had bad header, copied to .BAD\n"); 
		printf("---header was: %s\n",&buff); 

		sprintf(&infile, MBAD_MSK, find); 
		mailout=fopen(&infile,"w"); 
		fprintf(mailout, "%s\n", &buff); 
		fcopy(mailin, mailout); 

		fmkdl(mailin); 

		fclose(mailout); 

		return(FALSE); 

	} 

	cpystr(&who, &buff[3]); 
	 
	if (verbose) printf("[[Mail to be sent to %s]]\n", &who); 

	if (lookup(&who)==FALSE) { 

		/* Dispose of illegal mail */ 

		stamp(); 
		printf("%Dead mail on message %d, copied to dead mail queue\n", 
			find); 

		sprintf(&infile, MDED_MSK, find); 
		mailout=fopen(&infile,"w"); 
		fprintf(mailout, "%s\n", &buff); 
		fcopy(mailin, mailout); 

		fmkdl(mailin); 

		fclose(mailout); 

		return(FALSE); 

	} 

	if (verbose) printf("[[Sending mail to user %d]]\n",uid); 

	sprintf(&tfile, MTEM_MSK, uid); 
	mailtemp=fopen(&tfile,"w"); 

	if (mailtemp==NULL) { 
	   stamp(); 
	   printf("?Cannot open temporary file %s with error %d\n", 
			&tfile,$$ferr); 
		printf("---message %d will be black listed for %d secs\n", 
			find,CYCLE*BLAKTIM); 
		return(FALSE); 
	} 

	sprintf(&infile, USBX_MSK, uid); 
	mailout=fopen(&infile,"r"); 

	if (mailout!=NULL) { 
	 
		fcopy(mailout, mailtemp); 

		fclose(mailout); 

	} 

	fprintf(mailtemp,"\n"); 

	fcopy(mailin, mailtemp); 

	fclose(mailtemp); 
	 
	mailtemp=fopen(&tfile,"r"); 
	if (mailtemp==NULL) { 
	   stamp(); 
	   printf("?Cannot open previously temp file %s with error %d\n", 
			&tfile,$$ferr); 
		printf("---message %d will be black listed for %d secs\n", 
			find,CYCLE*BLAKTIM); 
		printf("---%s's may have been hurt because of this\n",&who); 
		return(FALSE); 
	} 


	mailout=fopen(&infile,"w"); 
	if (mailtemp==NULL) { 
	   stamp(); 
	   printf("?Cannot replace mailbox file %s with error %d\n", 
			&infile,$$ferr); 
		printf("---message %d will be black listed for %d secs\n", 
			find,CYCLE*BLAKTIM); 
		printf("---%s's may have been hurt because of this\n",&who); 
		return(FALSE); 
	} 

	 
	fcopy(mailtemp, mailout); 

	fmkdl(mailtemp); 

	fmkdl(mailin); 

	fclose(mailout); 

#ifdef RSTS_B 
	for (tm=0;tm<strlen(&uact)&&uact[tm]!=']';tm++) ; 
	tm++; 
	buff[0]=uact[tm++]; 
	buff[1]=uact[tm++]; 
	buff[2]=uact[tm++]; 
	buff[3]='\0'; 
	proj=atoi(&buff); 
	buff[0]=uact[tm++]; 
	buff[1]=uact[tm++]; 
	buff[2]=uact[tm++]; 
	prog=atoi(&buff); 
	sprintf(&buff,"\n\r\007\007*** You have new mail ***\n\r"); 
	if (verbose) printf("[[Send messages to all [%d,%d] jobs]\n", 
		proj,prog); 
	disjob(proj,prog,&buff); 
#endif	 

	return(TRUE); 

} 

/*----------------------------------------------------------------------* 
 *									* 
 *									* 
 *----------------------------------------------------------------------*/ 

main(argc,argv) 
int argc; 
char *argv[]; 
{ 
	int i, c; 
	char *p; 

#ifdef TRACE 
	$$flow=stderr; 
#endif 

	metoo=debug=verbose=0; 
	dragon=oneshot=0; 

	for(i=1; i<argc; ++i) { 
		p = argv[i]; 
		if(*p == '-') { 
			++p; 
			while(c = *p++) 
				switch(c) { 

				case 'd': 
				case 'D': 
					++debug; 
					break; 

				case 'v': 
				case 'V': 
					++verbose; 
					break; 

				case 'o': 
				case 'O': 
					++oneshot; 
					break; 

				case 'z': 
				case 'Z': 
					++dragon; 
					break; 

				default: 
					error("office() Bogus flag in command"); 
				} 
		} else  
			error("office() illegal argument in command"); 
	} 

	if(oneshot&&dragon) error("Ambiguous use of oneshot and dragon"); 

#ifndef DRAG 
	if (dragon) error("Dragons not supported by this system"); 
#endif 

	tim=ztime(NULL); 

	printf("\t%s\n\nMail queue servicer started %s\n\n",pname,tim); 

	if (debug) printf("DEBUG mode is ON (<<>> messages)\n"); 
	if (verbose) printf("VERBOSE mode is ON ([[]] messages)\n"); 
	if (oneshot) printf("ONESHOT mode is ON\n"); 
	if (dragon) printf("DRAGON mode is ON\n"); 
	printf("\n"); 

	if (dragon) { 
		if (ftty(stdout))  
			error("dragon mode must have log file as stdout"); 
		fclose(stdin); 
		/* The detach call will close stderr since C won't let us */ 
		fprintf(stderr, "[Detaching from terminal]\n"); 
		if (detach()) printf("!!Detach was sucessfull!!\n"); 
		else error("Could not detach from the terminal"); 
	} 

	initblk(); 

	while(TRUE) { 

		find=getpiece(); 

		if (find==0&&oneshot) break; 
		if (find==0) { 
			fflush(stdout); 
			sleep(CYCLE); 
			continue; 
		} 

		nameget(); 

		cycleblk(); 

	} 

	exit(IO_SUCCESS); 

} 

