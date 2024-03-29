/*----------------------------------------------------------------------* 
 *	UNIX Emulation Package for RSTS - UEP/RSTS			* 
 *									* 
 *	Copyright (C) 1985 by Scott Halbert				* 
 *----------------------------------------------------------------------*/ 

char *pname = "Mpost V1.0(10)"; 

/*----------------------------------------------------------------------* 
 *									* 
 *	Edit	  Date		Who	Description of Changes Made 	* 
 *									* 
 *	  1	27-Sep-84	SH	Begin Coding			* 
 *	  2	26-Nov-84	SH	Add multiple address routing	* 
 *	  3	26-Nov-84	SH	Add Aliasing code		* 
 *	  4	27-Nov-84	SH	Abort if user not set up	* 
 *					Print out aliased "To:" list	* 
 *	  5	27-Nov-84	SH	Add local name checking		* 
 *	  6	20-Apr-85	SH	Fix bug in ALSTR, bring to C#2	* 
 *	  7	 7-May-85	SH	Fix bug in VALIDATE		* 
 *	  8	 9-May-85	SH	Allow posting to net queue	* 
 *	  9	20-May-85	SH	Take out common subroutines	* 
 *	 10	10-Jun-85	SH	Add full address parsing	* 
 *									* 
 *----------------------------------------------------------------------*/ 

/*)BUILD 
	$(PROGRAM) = mpost 
	$(BIN) = bin: 
	$(MP) = 1 
	$(STACK) = 6000 
	$(TKBOPTIONS) = { 
		STACK = 6000 
		TASK = ...mps 
	} 
	$(LIBS) = c:uem 
*/ 


#include <stdio.h> 
#include <uem.h> 
#include "src:mail.h" 
#include "src:adpar.h" 

extern char *self_nam; 

/* #define TRACE */ 
#ifdef TRACE 
extern FILE *$$flow; 
#endif 

/*----------------------------------------------------------------------* 
 *									* 
 *	mpost - post mail into delivery queue				* 
 *									* 
 *----------------------------------------------------------------------*/ 

#define MAXBUF 300 

int	metoo, debug, verbose, sequence, netflg; 

struct	idrec user; 

char	buff[MAXBUF+2],*subject,*mes_id,*rec_date,*pos_date,*ful_name, 
	*send_date,*for_id; 

struct adlist *tolist; 
struct adnode *forfrom,*retaddrs; 

validate() 
{ 

	FILE *scfile; 
	char guname[30],guact[50]; 
	int luid,lgid,par; 
	struct adlist *getit; 

	if (debug) printf("Now opening file %s\n",NAMEFILE); 
	scfile=fopen(NAMEFILE, "r"); 

	if (scfile==NULL) { 
		  error("?Cannot find the name list %s... This is bad.\n", 
			NAMEFILE); 

	} 

	while(!feof(scfile)) { 

		if (fgetss(&buff,MAXBUF,scfile)==NULL) break;  /* (7) */ 
		par=sscanf(&buff,"%d %d %s %s",&luid,&lgid,&guname,&guact); 
		if (debug) printf("Read in uid=%d,gid=%d,uname=%s,uact=%s\n", 
			luid,lgid,&guname,&guact); 
		if (par!=4) break; 

		getit=NULL; 
		if (guname[0]!='\0') getit=sernode(&guname,tolist); 

		if (getit!=NULL) getit->adntyp=AT_LOC; 

	} 

	if (debug) { 
		fgetname(scfile,&guname); 
		printf("Closing file %s\n",&guname); 
	} 
	fclose(scfile);  
	return; 

} 

/*----------------------------------------------------------------------* 
 *									* 
 *									* 
 *----------------------------------------------------------------------*/ 

sendmail() 
{ 

	int sect,body; 

	FILE *bodyfile; 

	sect =body= 0; 

	subject=tolist=forfrom=mes_id=for_id=retaddrs=NULL; 
	rec_date=pos_date=ful_name=send_date=NULL; 

	if ((bodyfile=fopen(BODYTEMP,"w"))==NULL) error("Can't write output"); 

	while(!feof(stdin)) { 
			 
		if (fgetss(&buff,MAXBUF,stdin)==NULL) break; 
		 
		if (!sect) { 

			if (piece(&buff,"-----")||strlen(&buff)==0) { 
				sect = TRUE; 
				continue; 
			} 

			if (piece(&buff,"To: ")) { 
				tolist=getmuladd(tolist,&buff[4],stdin); 
				continue; 
			} 

			if (piece(&buff,"Ecc: ")) { 
				tolist=getmuladd(tolist,&buff[5],stdin); 
				continue; 
			}	 

			if (piece(&buff,"Cc: ")) { 
				tolist=getmuladd(tolist,&buff[4],stdin); 
				continue; 
			}	 

			if (piece(&buff,"Subject:")) { 
				subject=alstr(eatwhite(&buff[8])); 
				if(streq(subject,""))subject=NULL; 
				continue; 
			} 

			if (piece(&buff,"Subj:")) { 
				subject=alstr(eatwhite(&buff[5])); 
				if(streq(subject,""))subject=NULL; 
				continue; 
			} 

			if (piece(&buff,"From")) { 
				continue; 
			} 

			if (piece(&buff,"Message-Id:")) { 
				if (!netflg) continue; 
				mes_id=alstr(eatwhite(&buff[11])); 
			} 

			if (piece(&buff,"Forward-From:")) { 
				forfrom=getsinadd(&buff[13],stdin); 
				continue; 
			} 

			if (piece(&buff,"Forward-Id:")) { 
				for_id=alstr(eatwhite(&buff[11])); 
				continue; 
			} 

			if (piece(&buff,"Return-Path:")) { 
				if (!netflg) continue; 
				retaddrs=getsinadd(&buff[12],stdin); 
				continue; 
			} 

			if (piece(&buff,"Recieved-Date:")) { 
				if (!netflg) continue; 
				rec_date=alstr(eatwhite(&buff[14])); 
				continue; 
			} 

			if (piece(&buff,"Posted-Date:")) { 
				if (!netflg) continue; 
				pos_date=alstr(eatwhite(&buff[12])); 
				continue; 
			} 

			if (piece(&buff,"Full-Name:")) { 
				if (!netflg) continue; 
				ful_name=alstr(eatwhite(&buff[10])); 
				continue; 
			} 

			if (piece(&buff,"Sent-Date:")) { 
				if (!netflg) continue; 
				send_date=alstr(eatwhite(&buff[10])); 
				continue; 
			} 

			fprintf(stderr,"%Expected header, saw: \n\t\"%s\"\n", 
				&buff); 
			fprintf(stderr,"----extraneous data ignored\n"); 

			continue; 

		} 
	 
		if (!body) { 
			if (strlen(&buff)==0) continue; 
			body++; 

		} 

		fprintf(bodyfile,"%s\n",&buff); 

	} 

	fclose(bodyfile); 
	 
	if (tolist==NULL) error("?No to address specified--no action"); 

	if (!body) error("?No message body seen--no action"); 

	return; 

} 
outto(toaddr,toadls,outplace) 
struct adnode *toaddr; 
struct adlist *toadls; 
FILE *outplace; 
{ 

	int outyet; 

	outyet = FALSE; 

	adpretty("To:       ",toaddr,outplace); 
	mulpretty(toadls,"Ecc:",10,outplace); 
	 
	return; 

} 

getseq(to,seqnam) 
char *to; 
char *seqnam; 
{ 

	int retry; 

	FILE *seqread; 

	sequence=retry=0; 

	for (retry=0; retry<10; retry++) { 
		seqread=fopen(seqnam, "r"); 
		if (seqread!=NULL) break; 
		sleep(5); 
	} 

	if (seqread==NULL) error("Cannot open sequence file %s",seqnam); 

	fscanf(seqread,"%d",&sequence); 

	if(debug) printf("Read sequence number of %d\n",sequence); 

	sequence++; 

	fclose(seqread); 

	seqread=fopen(seqnam, "w"); 

	if (seqread==NULL) error("Cannot write sequence file %s",seqnam); 

	fprintf(seqread,"%d\n",sequence); 

	fclose(seqread); 

	printf("[Mail sequence %d posted to %s]\n",sequence,to); 

	return; 

} 

postmail(toaddr,boxname,holdname,ntype) 
struct adnode *toaddr; 
char *boxname,*holdname; 
int ntype; 
{ 

	char *time; 
	FILE *message, *bodyfile, *holdfile; 

	char ebuff[30],*ztime(); 

	sprintf(&ebuff,holdname,sequence); 

	if (debug) printf("postmail() Opening %s\n",&ebuff); 

	holdfile=fopen(&ebuff,"w"); 

	if (holdfile==NULL) error("Cannot write holdfile %s",&ebuff); 

	if (debug) printf("postmail() sucessfull opening of %s\n",&ebuff); 

	sprintf(&ebuff,boxname,sequence); 

	if (debug) printf("postmail() Opening %s\n",&ebuff); 

	message=fopen(&ebuff,"w"); 

	if (message==NULL) error("Cannot write message %s",&ebuff); 

	if (debug) printf("postmail() sucessfull opening of %s\n",&ebuff); 

	if (debug) printf("postmail() Opening %s\n",BODYTEMP); 

	bodyfile=fopen(BODYTEMP,"r");	 

	if (bodyfile==NULL) error("Cannot find body %s",BODYTEMP); 
	 
	if (debug) printf("postmail() sucessfull opening of %s\n",BODYTEMP); 

	time=alstr(ztime(NULL)); 

	if (forfrom==NULL) for_id=NULL; 
	if (for_id==NULL) forfrom=NULL; 
	if (mes_id==NULL) { 
		sprintf(&ebuff,"%s.%06d",self_nam,sequence); 
		mes_id=alstr(&ebuff); 
	} 
	if (retaddrs==NULL) { 
		retaddrs=malloc(sizeof (struct adnode)); 
		if (retaddrs==NULL) error("malloced out of room"); 
		retaddrs->nodnam=alstr(&user.uname); 
		retaddrs->routad=0; 
		retaddrs->furptr=NULL; 
		retaddrs->bakptr=NULL; 
		retaddrs->nodtyp=NOD_DES; 
	} 
	if (ful_name==NULL) ful_name=alstr(&user.fulln); 
	if (send_date==NULL) send_date=time; 

	adpretty("To ",toaddr,message); 
	fprintf(message,"From "); 
	shtaddr(message,retaddrs); 
	fprintf(message," %s\n",send_date); 
	outto(toaddr,tolist,message); 
 	fprintf(message,"%-10s","From:"); 
	shtaddr(message,retaddrs); 
	fprintf(message," (%s)\n",ful_name); 
	if (subject!=NULL) fprintf(message,"%-10s%s\n","Subject:",subject); 
	if (ntype||netflg) { 
		adpretty("Return-Path:  ",retaddrs,message); 
		fprintf(message,"Sent-Date:  %s\n",send_date); 
		fprintf(message,"Full-Name:  %s\n",ful_name); 
		fprintf(message,"Message-Id: %s\n",mes_id); 
		if (forfrom!=NULL) { 
			adpretty("Forward-From:  ",forfrom,message); 
			fprintf(message,"Forward-Id:    %s\n",for_id); 
		} 
		if (rec_date!=NULL) fprintf(message,"Received-Date:  %s\n", 
				rec_date); 
		if (pos_date!=NULL) fprintf(message,"Posted-Date:    %s\n", 
				pos_date); 
	} 
	fprintf(message,"\n"); 

	while (!feof(bodyfile)) { 

		if (fgetss(&buff,MAXBUF,bodyfile)==NULL) break; 
		fprintf(message,"%s\n",&buff); 
		if (debug) fprintf(stdout,"%s\n",&buff); 

	} 

	fmkdl(holdfile); 
	fclose(bodyfile); 
	fclose(message); 

	return; 

} 

/*----------------------------------------------------------------------* 
 *									* 
 *									* 
 *----------------------------------------------------------------------*/ 

main(argc,argv) 
int argc; 
char *argv[]; 
{ 
	int i, c, rsg; 
	char *p; 
	int tmp_flg; 
	struct adlist *scanadd; 

#ifdef TRACE 
	$$flow=stderr; 
#endif 

	metoo=debug=verbose=0; 

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

				case 'm': 
				case 'M': 
					++metoo; 
					break; 

				case 'n': 
				case 'N': 
					++netflg; 
					break; 

				default: 
					error("post() Bogus flag in command"); 
				} 
		} else  
			error("post() illegal argument in command"); 
	} 

/*	debug=verbose=TRUE; */ 

	if (verbose) { 
		printf("Welcome to %s\n\n",pname); 
		printf("verbose is ON\n"); 
		printf("debug is %s\n",debug?"ON":"OFF"); 
		printf("metoo is %s\n",metoo?"ON":"OFF"); 
	} 

	rsg=$xidget(&user); 

	sprintf(&buff,"%s (s)",&user.uname,&user.fulln); 

	if (rsg==NULL) error("User is not set up!!!"); 

	net_rc(); 

	sendmail(); 

	alias(AF_LOCAL,tolist); 
	alias(AF_SYSTEM,tolist); 
	validate(); 

	for (scanadd=tolist; scanadd!=NULL; scanadd=scanadd->conadl) { 
		if (scanadd->adnflg==AD_DEL) continue; 
/*		if (scanadd->adnflg==AD_GATE) { 
			printf("%%Gateways not yet implemented %s\n", 
				scanadd->adlths->nodnam); 
			continue; 
		}	*/ 
		if (!metoo&&streq(scanadd->adlths->nodnam,&user.uname)&& 
			(scanadd->adnflg==AD_ALI)&& 
			scanadd->adlths->furptr==NULL) continue; 
	/* Don't send mail to self on alias expansions unless metoo */ 
		if (scanadd->adlths->furptr!=NULL) { 
			if (scanadd->adnflg==AD_DEL) continue; 
#ifndef NET_ON 
			printf("%%Networking not on\n"); 
			if (netflg) break; 
			continue; 
#else 
			tmp_flg=scanadd->adnflg; 
			scanadd->adnflg=AD_DEL; 
			getseq(scanadd->adlths->nodnam,NETSEQS); 
			postmail(scanadd->adlths,NETF_MSK,NTLK_MSK,1); 
			scanadd->adnflg=tmp_flg; 
			if (netflg) break; 
			continue; 
#endif 
		} 
		if (scanadd->adntyp!=AT_LOC) { 
			printf("%%User %s does not exist on this machine\n", 
				scanadd->adlths->nodnam); 
			if (netflg) break; 
			continue; 
		} 
		tmp_flg=scanadd->adnflg; 
		scanadd->adnflg=AD_DEL; 
		getseq(scanadd->adlths->nodnam,SEQFILE); 
		postmail(scanadd->adlths,MBOX_MSK,MHLD_MSK,0); 
		scanadd->adnflg=tmp_flg; 

		if (netflg) break; 

	} 

	exits(IO_SUCCESS); 

} 

