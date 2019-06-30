/*----------------------------------------------------------------------*
 *	tmail - Thor's Mail utility - similar to berkeley mail		*
 *									*
 *	Copyright (C) 1985 by Scott Halbert				*
 *----------------------------------------------------------------------*/

const char *pname = "tmail V2.1(14)";

/*----------------------------------------------------------------------*
 *									*
 *	Edit	  Date		Who	Description of Changes Made 	*
 *									*
 *	  1	27-Sep-84	SH	Documentation and begin history	*
 *	  2	24-Oct-84	SH	Add login attachments		*
 *	  3	25-Oct-84	SH	Add Help Command		*
 *	  4	25-Nov-84	SH	Remove Ecc Prompt because it	*
 *					 doesn't work yet		*
 *	  5	25-Nov-84	SH	Add the multi line addresses	*
 *	  6	27-Nov-84	SH	Abort if user not set up	*
 *	  7	27-Nov-84	SH	Remove all alias code		*
 *	  8	19-Apr-85	SH	Convert stuff for decus C#2	*
 *	  9	 9-May-85	SH	Remove some defs to mail.h	*
 *	 10	20-May-85	SH	Extract common subroutines	*
 *					Remove write code from c_quit,	*
 *					put in closup(). Implement 	*
 *					reply command.			*
 *	 11	11-Jul-85	SH	Add first time print flag so	*
 *					 increment functions will 	*
 *					 skip first message		*
 *	 12	11-Jul-85	SH	Add -? option to startup	*
 *	 13	12-Jul-85	SH	Complete rehash, add dispose	*
 *					 folders, del unused commands	*
 *	 14	 6-Apr-93	SH	8 years later -- V2.  Never	*
 *					 throw away those old tapes, 	*
 *					 you might need them. 		*
 *					Start on DOS version for PC	*
 *					 networks
 *									*
 *----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*
 *									*
 *	The following issues and features have been ommitted		*
 *	or sidestepped during implementation:  Please Fix		*
 *									*
 *	arglist() needs to implement ranges and alpha searches		*
 *	need to fix mpost so it doesn't need a ccl (core common stuff)	*
 *									*
 *----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "uem.h"
#include "mail.h"

char *bombprofile(char *class, char *var);
char *ParseFileContents(char *buff);
char *lowerit(char *);
char *eatwhite(char *);
#define streq(a,b) (strcmp(a,b)==0)

/*----------------------------------------------------------------------*
 *									*
 *	mail - mailbox editor and mail send function interface		*
 *									*
 *	This program has two basic modes, one is the edit mode, which	*
 *	happens when the program is run without an address.  The 	*
 *	mailer first does a directory of the mailbox, then it prints	*
 *	a header indicating the number messages, the user then may	*
 *	list the directory, print, or delete messages.  Upon exit,	*
 *	the program updates the main mailbox and mbox folder.		*
 *									*
 *	The other mode of the program is to accept a mail address,	*
 *	then to invoke the local editor and upon exit, the mail		*
 *	program posts the mail to the post program.			*
 *									*
 *	The program is very compatible with the unix mail program,	*
 *	and has many identical commands, but the code herein is		*
 *	completely original and proprietary to the author.		*
 *									*
 *----------------------------------------------------------------------*/

const char M_DEL = 'D';		/* Message is flaged deleted */
const char M_UNR = 'U';		/* Message has not been read yet */
const char M_PRE = 'P';		/* Message is preserved (in hold state) */
const char M_REA = 'R';		/* Message has been read */

const int DIR_NOF = 1;		/* The mailbox is not found */
const int DIR_GOOD = 0;		/* The mailbox has been directoried */

#define BMAXI 4096		/* The input buffer max */

int	messeq,cflag,sflag,fipos,fflag,eflag,	/* Various Flags */
	xflag,firflg;
char	*unme,buff[BMAXI+1],*sub,*mailto,		/* And buffers */
	*current_file;

#define MAXFOLNAM 8      	/* Folder name size */

char	trash_folder[MAXFOLNAM],
    seen_folder[MAXFOLNAM],
    current_folder[MAXFOLNAM],
    alt_box[20];


struct	dirfmt	{		/* Linked structure of mailbox directories */
	char	sel;		/* Character for the message status */
	int	mesnum;		/* The sequence number of this message */
	char	*fromtext;	/* Pointer to the "From" text */
	char 	*fromname;	/* Name of the from person */
	char 	*fromadr;	/* Address of who message from */
	int	hedlen;		/* Length of header in chars */
	int	bodlen;		/* Length of body in chars */
	int	hedlines;	/* Number of lines in header */
	int	bodlines;	/* Number of lines in body */
	char	*subject;	/* Pointer to message subject if any */
	struct dirfmt *nexdir;	/* Message to the next link in directory */
	char	dispos[MAXFOLNAM+1];	/* Where to dispose the message */
	int	kepmes;		/* Flag for copy or delete */
	char	*repadd;	/* Reply address if any */
};

struct dirfmt	
    *rootdir=NULL,
    *currdir=NULL;	/* Basic directory links */

const int dirsize = (sizeof (struct dirfmt));	/* For malloc */

#define SB_BOOL 1			/* Set is a boolean value */
#define SB_NUMB 2			/* Set is a number */

struct set_bools {			/* The set variables */
	char *sb_name;			/* Name of this set variable */
	int sb_type;			/* The variable type */
	int sb_value;			/* The default value */
} sbool[] = {				/* Array of above structs */
	{ "append",	SB_BOOL, TRUE },	/* Append to end */
	{ "dispose",	SB_BOOL, TRUE },	/* Ask for disposal */
	{ "debug",	SB_BOOL, FALSE },	/* Enter debug mode */
	{ "expert",	SB_BOOL, FALSE },	/* Expert (terse) mode */
	{ "metoo",	SB_BOOL, FALSE },	/* Don't expand self */
	{ "quiet",	SB_BOOL, FALSE },	/* Don't print startup */
	{ "trash",	SB_BOOL, TRUE },	/* Keep the trash folder */
	{ "verbose",	SB_BOOL, FALSE },	/* Enter verbose mode */
	{ NULL,		0,	 0 } } ;	/* Null termination */

BOOL sys_any();

/*----------------------------------------------------------------------*
 *									*
 *	varbool(variable name) - returns TRUE or FALSE depending on	*
 *	if the variable is legal (TRUE-Legal, FALSE-not legal)		*
 *									*
 *----------------------------------------------------------------------*/

BOOL varbool(const char *invar)
{

	int varac;

	for (varac=0; sbool[varac].sb_type!=0; varac++)

		if (strcmp(sbool[varac].sb_name,invar)==0) return(TRUE);

	return(FALSE);

}

/*----------------------------------------------------------------------*
 *									*
 *	paraddress(ofile, hed, rest) - break down an address, read	*
 *	sucessive lines of address and put them to the output file	*
 *									*
 *----------------------------------------------------------------------*/

BOOL paraddress(const FILE *ofile,const char *hed, char *rest)
{

	int any;
	char tcrb;

	any=FALSE;

	fprintf(ofile,"%-10s",hed);

	while(TRUE) {

		if (*rest==' '||*rest=='\t'||*rest==',') {
			rest++;
			continue;
		}

		if (*rest=='\0') {
			tcrb=getc(stdin);
			if (feof(stdin)) break;
			ungetc(tcrb, stdin);
			if (tcrb!=' '&&tcrb!='\t') break;
			if (fgetss(buff,BMAXI,stdin)==NULL) break;
			rest= buff;
			continue;
		}

		if (any==TRUE) fprintf(ofile,"          ");

		while(*rest!=' '&&*rest!='\t'&&*rest!='\0'&&*rest!=',') {
			if (*rest<' ') {
				rest++;
				continue;
			}
			*rest=tolower(*rest);
			putc(*rest, ofile);
			rest++;
			any=TRUE;
		}

		fprintf(ofile,"\n");

	}

	return(any);

}

/*----------------------------------------------------------------------*
 *									*
 *	getbool(variable name) - returns the value of the named 	*
 *	numeric variable.  This will die if passed an illegal 		*
 *	variable name, which is why there is varbool() above		*
 *									*
 *----------------------------------------------------------------------*/

BOOL getbool(const char *invar)
{

	int varac;

	if (varbool(invar)==FALSE) 
		error("getbool() got called with undefined value\n");

	for (varac=0; sbool[varac].sb_type!=0; varac++)

		if (strcmp(sbool[varac].sb_name,invar)==0) 
			return(sbool[varac].sb_value);

	error("getbool() got impossible\n");

}

/*----------------------------------------------------------------------*
 *									*
 *	putbool(variable name, value) - this set the value of the 	*
 *	variable to value.  This too croaks on illegal variables	*
 *									*
 *----------------------------------------------------------------------*/

VOID putbool(const char *invar,const int setit)
{

	int varac;

	if (varbool(invar)==FALSE) 
		error("putbool() got called with undefined value\n");

	for (varac=0; sbool[varac].sb_type!=0; varac++)

		if (strcmp(sbool[varac].sb_name,invar)==0) {
			sbool[varac].sb_value=setit;
			return;
		}

	error("putbool() got impossible\n");

}

/*----------------------------------------------------------------------*
 *									*
 *	showbools() - this dumps a pretty variable list out to stdout	*
 *	someday this will print alphas too when implemented		*
 *									*
 *----------------------------------------------------------------------*/

VOID showbools()
{

	int varac;

	printf(" Variable Settings: \n\n");

	for (varac=0; sbool[varac].sb_type!=0; varac++) {

		printf("%-10s ",sbool[varac].sb_name);
		if (sbool[varac].sb_type==SB_BOOL) 
			printf("is %s",sbool[varac].sb_value?"ON":"OFF");
		if (sbool[varac].sb_type==SB_NUMB) 
			printf("value %d",sbool[varac].sb_value);
		printf("\n");

	}

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *	getds() - mallocs and initializes a nice dirfmt node and	*
 *	returns us a pointer to it					*
 *									*
 *----------------------------------------------------------------------*/

struct dirfmt *getds()
{

	struct dirfmt *holding;

	if((holding = (struct dirfmt *) malloc(dirsize))==NULL) 
		error("getds ran out of memory\n");

	holding->sel=M_UNR;
	holding->mesnum=0;
	holding->fromtext=NULL;
	holding->fromname=NULL;
	holding->fromadr=NULL;
	holding->hedlen=0;
	holding->bodlen=0;
	holding->bodlines=0;
	holding->hedlines=0;
	holding->subject=NULL;
	holding->nexdir=NULL;
	strcpy(holding->dispos,current_folder);
	holding->kepmes=TRUE;
	holding->repadd=NULL;

	return(holding);

}

void ParseHeadFrom(struct dirfmt *dir, char* fromstr)
{

	char from[4096];

    /* Header from is in form 'From address date' */

    	char* sp;

    	sp = strchr(fromstr,' ');	/* Look for next space */
    	if (sp!=NULL) *sp = '\0';

	strcpy(from,fromstr);		/* Trunc at max of 25 chars */

	if (dir->fromname!=NULL) free(dir->fromname);
	dir->fromname = strdup(from);
	
}

void ParseFrom(struct dirfmt *dir, char* fromstr)
{

	char from[4096];

     /* Search for addresses in this form:
     User Name <user address>
     user address (user name)
     user address */

	char* ic;

	ic = strchr(fromstr, '<');
	if (ic!=NULL) *ic='\0';
	ic = strrchr(fromstr, ')');
        if (ic!=NULL) *ic='\0';
	ic = strchr(fromstr, '(');
	if (ic!=NULL) strcpy(from,ic+1);
        else
                strcpy(from,fromstr);

	if (dir->fromname!=NULL) free(dir->fromname);
	dir->fromname = strdup(from);
}

/*----------------------------------------------------------------------*
 *									*
 *	dirbox(boxfile) - open mailbox in boxfile and parse it, link	*
 *	successive dirfmt links onto the rootdir pointer, return	*
 *	DIR_NOF if the mailbox cannot be opened, otherwise return	*
 *	DIR_GOOD when done with directory, then close mailbox		*
 *									*
 *----------------------------------------------------------------------*/

int dirbox(const char *boxfile)
{

	FILE *box;
	int bline,bodyyet;
	struct dirfmt *dir,*lasdir;

	rootdir=NULL;
	lasdir=NULL;
	messeq=0;
	fipos=1;
	firflg=TRUE;

	box=fopen(boxfile,"r");

	if (box==NULL) {
		if (getbool("debug")) 
		 	printf("dirbox() boxfile %s was not there\n",boxfile);
		return(DIR_NOF);
	}

	bline=TRUE;
	bodyyet=TRUE;

	while(!feof(box)) {

		if(fgets(buff,BMAXI,box)==NULL) break;
		buff[strlen(buff)-1]='\0';

		if (buff[0]=='\0') {
			bline=TRUE;
			bodyyet=TRUE;
			if (rootdir!=NULL) dir->bodlines++;
			continue;
		}

		if(piece(buff,"From ")==TRUE&&bline==TRUE) {

			bodyyet=FALSE;

			dir=getds();

			dir->mesnum= ++messeq;
			dir->fromtext=strdup(&buff[5]);
			dir->hedlen+=strlen(buff)+1;
			dir->hedlines++;
			ParseHeadFrom(dir,&buff[5]);

			if(getbool("debug")) printf("Msg #%d, from \"%s\"\n",
					dir->mesnum,dir->fromtext);

			if(rootdir==NULL) rootdir=dir;
			else	lasdir->nexdir=dir;

			lasdir=dir;

			continue;

		}
			
		bline=FALSE;

		if(piece(buff,"From: ")==TRUE&&bodyyet==FALSE) 
			ParseFrom(dir,eatwhite(&buff[6]));

		if(piece(buff,"Subject: ")==TRUE&&bodyyet==FALSE) {
		
			dir->subject=strdup(eatwhite(&buff[9]));
			dir->hedlen+=strlen(buff)+1;
			dir->hedlines++;

			if(getbool("debug")) printf("Mes #%d, subject \"%s\"\n",
					dir->mesnum,dir->subject);

			continue;

		}

		if (bodyyet==FALSE) {

			dir->hedlen+=strlen(buff)+1;
			dir->hedlines++;
			continue;

		}

		if (rootdir==NULL) continue;
		
		dir->bodlen+=strlen(buff)+1;
		dir->bodlines++;

	}

	fclose(box);

	return(DIR_GOOD);

}

/*----------------------------------------------------------------------*
 *									*
 *	readbox(box, mess#, where, flag) - read through mailbox box	*
 *	until message # mess# is encountered, then print it out to	*
 *	FILE where.  Print the "Message #n" on top if the flag is TRUE	*
 *									*
 *----------------------------------------------------------------------*/

readbox(boxfile,mess,where,mesmes)
char *boxfile;
int mess,mesmes;
FILE *where;
{

	FILE *box;
	int bline,bodyyet,mesnm,disp;

	mesnm=0;
	disp=0;

	box=fopen(boxfile,"r");

	if (box==NULL) error("Cannot now find box %s\n",boxfile);

	bline=TRUE;
	bodyyet=TRUE;

	while(!feof(box)) {

		if(fgets(buff,BMAXI,box)==NULL) break;
		buff[strlen(buff)-1]='\0';

		if (buff[0]=='\0') {
			bline=TRUE;
			bodyyet=TRUE;
			if (disp==1) fprintf(where,"\n");
			continue;
		}

		if(piece(buff,"From ")==TRUE&&bline==TRUE) {

			bodyyet=FALSE;

			if (disp==1) {
				disp=2;
				break;
			}

			++mesnm;
			if (getbool("debug")) printf("readbox() saw mes %d\n",
					mesnm);

			if (mesnm==mess) {
				disp=1;
				if (mesmes)
					fprintf(where,"Message %d:\n",mesnm);
			}

			if (disp==1) fprintf(where,"%s\n",buff);

			continue;

		}
			
		bline=FALSE;

		if (bodyyet==FALSE) {

			if (disp==1) fprintf(where,"%s\n",buff);
			continue;

		}

		if (disp==1) fprintf(where,"%s\n",buff);

	}

	fclose(box);

	return(DIR_GOOD);

}

/*----------------------------------------------------------------------*
 *									*
 *	ateof() - check our current position with the mailbox directory	*
 *	and return if we are within it, else print message and set	*
 *	key positional variables 					*
 *									*
 *----------------------------------------------------------------------*/

VOID ateof()
{

	if (fipos<=messeq) return;

	printf("\n at EOF\n");

	currdir = NULL;

	fipos=messeq+1;
	firflg=FALSE;			/* [011] */

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *	showdir(dir) - start at dirfmt dir and print out a pretty 	*
 *	directory of the mailbox.  This is used by the "header" command	*
 *									*
 *----------------------------------------------------------------------*/

showdir(dirls)
struct dirfmt *dirls;
{

	char	weekday[4],mon[4],zone[16];
	int	day,hour,minute,second,year,count;
	struct dirfmt *dir;

/*	printf("Showdir() starting loop\n"); */

	for (dir=dirls; dir!=NULL; dir=dir->nexdir) {

		buff[0]=weekday[0]=mon[0]=zone[0]='\0';
		day=hour=minute=second=year=0;

/*	printf("showdir() sel=%s from=%s\n",dir->sel,dir->fromtext); */

		/*if (dir->sel!=M_DEL) {*/

			count = sscanf(dir->fromtext,
				"%150s%3s%3s%d%d%*[:]%d%*[:]%d%d%3s",
				buff,weekday,mon,&day,&hour,&minute,
				&second,&year,zone);

/*		printf("showdir() scan was sucessful %d\n",count); */

		  printf("%c%c%3d %-8s %s %s %2d %2d:%02d %-15.15s%4d/%-4d",
			 dir->mesnum==fipos?'>':' ',dir->sel,
			 dir->mesnum,dir->dispos,
			 weekday,mon,
			 day,hour,minute,dir->fromname,
			 dir->bodlines+dir->hedlines,
			 dir->hedlen+dir->bodlen,dir->bodlines);

			if (dir->subject!=NULL) printf(" \"%s\"",dir->subject);

			if (dir->fromadr==NULL) dir->fromadr=strdup(buff);

			printf("\n");

		/*}*/

	}

	ateof();
	return;

}

/*----------------------------------------------------------------------*
 *									*
 *	setpos(dir,dels) - starting at dir, set curpos to the		*
 *	message which is fipos.  If dels=0, jump over deletes		*
 *									*
 *----------------------------------------------------------------------*/

setpos(dir,dels)
struct dirfmt *dir;
int dels;
{

	currdir = dir;

	while(currdir!=NULL) {

		if (currdir->mesnum==fipos) {
			if (currdir->sel==M_DEL&&dels==0) {
				fipos++;
				return(setpos(dir,dels));
			}
			return;
		}

	currdir=currdir->nexdir;

	}

	currdir=NULL;

	fipos=messeq+1;

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *	usage(string) - print our user a fatal usage error and stop	*
 *									*
 *----------------------------------------------------------------------*/


usage(string)
char *string;
{

	printf("?Usage error: %s\n",string);
	if (xflag) stopto("$logini",-10000);
	exit(1);

}

iexit()
{

	int errrt;

	if (xflag) {
		errrt=stopto("$logini",10000);
		printf("Chain to $logini at 10000 got error %d\n",errrt);
	}

	exit(1);

}

char *folder_file(char *name)
{

	sprintf(buff,bombprofile("tmail","Folder File"),name);
	return((ParseFileContents(buff)));
}

char *folder_bak(char *name)
{

	sprintf(buff,bombprofile("tmail","Folder Bak"),name);
	return((ParseFileContents(buff)));
}

char *folder_new(char *name)
{

	sprintf(buff,bombprofile("tmail","Folder New"),name);
	return((ParseFileContents(buff)));
}

closup()
{

	struct dirfmt *dir;

	for (dir=rootdir; dir!=NULL; dir=dir->nexdir) {
		if (!(streq(current_folder,dir->dispos)||
		      streq(seen_folder,dir->dispos)||
		      streq(trash_folder,dir->dispos))) {

			if ((dir->dispos)==NULL) continue;
			close_dis(dir->dispos);

		}
	}

	if (!streq(seen_folder,current_folder)&&chk_act(seen_folder)>0) 
	    close_dis(seen_folder);

	if (getbool("trash")&&chk_act(trash_folder)>0) close_dis(trash_folder);

	if (chk_act(current_folder)>0) {
		if (sys_any()) return;	/* No system action nothing changed */
		close_dis(current_folder);
	}
	else delete(folder_file(current_folder));

	return;

}	

/*----------------------------------------------------------------------*
 *									*
 *	outmail(set,outf) - output either the mailbox batch of mail,	*
 *	or the mbox batch out to outf					*
 *									*
 *----------------------------------------------------------------------*/

VOID outmail(folset,outf)
char *folset;
FILE *outf;
{

	struct dirfmt *dir;

	for (dir=rootdir; dir!=NULL; dir=dir->nexdir) {

		if (streq(dir->dispos,folset)) {

			readbox(current_file,dir->mesnum,outf,FALSE);
			dir->dispos[0]='\0';	/* Remove it */

		}

	}

	return;

}

close_dis(folbox)
char *folbox;
{

	FILE *infile, *outfs;
	char *folnam;
	char *folbak;
	char *folnew;

	folnam=strdup(folder_file(folbox));
	folbak=strdup(folder_bak(folbox));

	printf("[Update %s]\n",folnam);

	if ((outfs=fopen(folbak,"w"))==NULL) 
		error("Cannot open backup file %s",folbak);

	if (!getbool("append")) {
		outmail(folbox,outfs);
		fprintf(outfs,"\n");
	}

	if (!streq(folbox,current_folder))
		if ((infile=fopen(folnam,"r"))!=NULL) {
			fcopy(infile,outfs);
			fclose(infile);
		}

	if (getbool("append")) {
		fprintf(outfs,"\n");
		outmail(folbox,outfs);
	}
	
	fclose (outfs);

	if ((infile=fopen(folbak,"r"))==NULL) 
		error("Cannot open backup file %s");
	
	if ((outfs=fopen(folnam,"w"))!=NULL) fcopy(infile,outfs);

	fclose(outfs);
	fclose(infile);
	delete(folbak);

	free(folnam);
	free(folbak);

	return;
}

/*----------------------------------------------------------------------*
 *									*
 *	d_read(target) - read message #target and mark it read if	*
 *	it was currently unread						*
 *									*
 *----------------------------------------------------------------------*/

d_read(target)
int target;
{

/*	printf("called d_read(%d)\n",target); */

	if (target<1) target=1;

	fipos = target;
	firflg=FALSE;			/* [011] */

	setpos(rootdir,0);

	if (fipos>messeq) {
		ateof();
		return;
	}

	if (target!=fipos) {
		printf("Message %d was non-existant or deleted.\n",target);
		return;
	}

	readbox(current_file,fipos,stdout,TRUE);

	if (currdir->sel==M_UNR) {
		currdir->sel=M_REA;
		strcpy(currdir->dispos,seen_folder);
		currdir->kepmes=FALSE;
	}

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *	d_del(target) - delete message #target				*
 *									*
 *----------------------------------------------------------------------*/

d_del(target)
int target;
{

/*	printf("called d_del(%d)\n",target); */

	if (target<1) target=1;

	fipos = target;
	firflg=FALSE;			/* [011] */

	setpos(rootdir,1);

	if (fipos>messeq) {
		ateof();
		return;
	}

	if (target!=fipos) {
		printf("Message %d was non-existant.\n",target);
		return;
	}

	currdir->sel=M_DEL;
	strcpy(currdir->dispos,trash_folder);
	currdir->kepmes=FALSE;

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *	d_hold(target) - mark message #target as preserved		*
 *									*
 *----------------------------------------------------------------------*/

d_hold(target)
int target;
{

/*	printf("called d_hold(%d)\n",target); */

	if (target<1) target=1;

	fipos = target;
	firflg=FALSE;			/* [011] */

	setpos(rootdir,0);

	if (fipos>messeq) {
		ateof();
		return;
	}

	if (target!=fipos) {
		printf("Message %d was non-existant or deleted.\n",target);
		return;
	}

	currdir->sel=M_PRE;
	strcpy(currdir->dispos,current_folder);
	currdir->kepmes=TRUE;

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *	d_undel(target) - undelete message #target			*
 *									*
 *----------------------------------------------------------------------*/

d_undel(target)
int target;
{

/*	printf("called d_undel(%d)\n",target); */

	if (target<1) target=1;

	fipos = target;
	firflg=FALSE;			/* [011] */

	setpos(rootdir,1);

	if (fipos>messeq) {
		ateof();
		return;
	}

	if (target!=fipos) {
		printf("Message %d was non-existant.\n",target);
		return;
	}

	currdir->sel=M_REA;
	strcpy(currdir->dispos,seen_folder);
	currdir->kepmes=FALSE;

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *	arglist(meslist,mesfun) - for each message in the string	*
 *	run function mesfun 						*
 *									*
 *	Note:   Currently only individual message numbers are		*
 *		supported, we wish to eventually support ranges and	*
 *		user name, and string occurance scans			*
 *									*
 *----------------------------------------------------------------------*/

arglist(meslist,mesfun)
char *meslist;
int (*mesfun)();
{

	char intbuf[20];
	int mpos,mmax,mestarg,istar;

	if (meslist==NULL||(meslist!=NULL&&*meslist=='\0')) 
		return((*mesfun)(fipos));

	mmax=strlen(meslist);
	istar=0;

	for(mpos=0; mpos<mmax; mpos++) {

		if (*(meslist+mpos)=='-') 
			return(printf("ranges not yet implemented %s\n",
				meslist));

		if (*(meslist+mpos)==' '||*(meslist+mpos)==',') {
			if (istar==0) continue;
			intbuf[istar]='\0';
			mestarg=atoi(intbuf);
/*			printf("arglist() parsed %s, %d\n",intbuf,mestarg); */
			if (mestarg==0) return(printf("Bad argument %s",
				meslist));
			(*mesfun)(mestarg);
			istar=0;
			continue;
		}

		intbuf[istar++]= *(meslist+mpos);

	}

	if (istar!=0) {
		intbuf[istar]='\0';
		mestarg=atoi(intbuf);
/*		printf("arglist() parsed %s, %d\n",intbuf,mestarg); */
		if (mestarg==0) return(printf("Bad argument %s",
			meslist));
		(*mesfun)(mestarg);
	}

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *	c_bprint(arguments) - the "Print" command (with big p), print	*
 *	arglist() the arguments each with d_read(), but don't omit 	*
 *	any headers (the set header command)				*
 *									*
 *----------------------------------------------------------------------*/

VOID c_bprint(argstr)
char *argstr;
{

	if (argstr!=NULL&&*argstr=='\0') argstr=NULL;

	arglist(argstr,d_read);

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *	c_breply(argument) - Command "Reply" (big R) - send mail to	*
 *	the author of the current message (if any), any argument is	*
 *	not allowed							*
 *									*
 *----------------------------------------------------------------------*/

VOID c_breply(argstr)
char *argstr;
{

	if (argstr!=NULL&&*argstr=='\0') argstr=NULL;

	printf("%%unimplemented: [Reply] %s\n",argstr);

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *	c_copy(argument) - copy the specified message to the 		*
 *	specified mail folder - do not mark them as deleted as		*
 *	the save command would						*
 *									*
 *----------------------------------------------------------------------*/

VOID c_copy(argstr)
char *argstr;
{

	if (argstr!=NULL&&*argstr=='\0') argstr=NULL;

	printf("%%unimplemented: [copy] %s\n",argstr);

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *	c_del(arguments) - delete each of the messages in the arg 	*
 *									*
 *----------------------------------------------------------------------*/

VOID c_del(argstr)
char *argstr;
{

	if (argstr!=NULL&&*argstr=='\0') argstr=NULL;

	arglist(argstr,d_del);

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *	c_dt(arg) - delete current and type next (arg not allowed)	*
 *									*
 *----------------------------------------------------------------------*/

VOID c_dt(argstr)
char *argstr;
{

	if (argstr!=NULL&&*argstr=='\0') argstr=NULL;

	if (argstr!=NULL) printf("%%argument \"%s\" ignored.\n",argstr);

	setpos(rootdir,0);

	if (currdir==NULL) {
		ateof();
		return;
	}

	currdir->sel=M_DEL;
	strcpy(currdir->dispos,trash_folder);
	currdir->kepmes=FALSE;

	if (!firflg) fipos++;
	firflg=FALSE;			/* [011] */

	setpos(rootdir,0);

	if (fipos>messeq) {
		ateof();
		return;
	}

	if (currdir==NULL) return;

	readbox(current_file,fipos,stdout,TRUE);

	if (currdir->sel==M_UNR) {
		currdir->sel=M_REA;
		strcpy(currdir->dispos,seen_folder);
		currdir->kepmes=FALSE;
	}

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *	c_exit(arg) - abort anything and exit now (arg ignored)		*
 *									*
 *----------------------------------------------------------------------*/

VOID c_exit(argstr)
char *argstr;
{

	if (argstr!=NULL&&*argstr=='\0') argstr=NULL;

	iexit();
}

/*----------------------------------------------------------------------*
 *									*
 *	c_setf(arg) - switch folders (close out current)		*
 *									*
 *----------------------------------------------------------------------*/

VOID c_setf(argstr)
char *argstr;
{

	if (argstr!=NULL&&*argstr=='\0') argstr=NULL;

	printf("%%unimplemented: [folder] %s\n",argstr);

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *	c_lfolds(args) - list current folder				*
 *									*
 *----------------------------------------------------------------------*/

VOID c_lfolds(argstr)
char *argstr;
{

	if (argstr!=NULL&&*argstr=='\0') argstr=NULL;

	printf("%%unimplemented: [folders] %s\n",argstr);

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *									*
 *----------------------------------------------------------------------*/

VOID c_from(argstr)
char *argstr;
{

	if (argstr!=NULL&&*argstr=='\0') argstr=NULL;

	printf("%%unimplemented: [from] %s\n",argstr);

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *									*
 *----------------------------------------------------------------------*/

VOID c_head(argstr)
char *argstr;
{

	if (argstr!=NULL&&*argstr=='\0') argstr=NULL;

	if (argstr!=NULL) printf("%%argument \"%s\" ignored.\n",argstr);

	showdir(rootdir);

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *									*
 *----------------------------------------------------------------------*/

VOID c_help(argstr)
char *argstr;
{

	FILE *help;

	if (argstr!=NULL&&*argstr=='\0') argstr=NULL;

	help=fopen(ParseFileContents(bombprofile("tmail","Mail Help")),"r");
	if (help==NULL) {
		printf("%Cannot find the %s mail file\n",
		       ParseFileContents(bombprofile("tmail","Mail Help")));
		return;
	}

	fcopy(help,stdout);		/* (10) Replace verbosity */

	fclose(help);

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *									*
 *----------------------------------------------------------------------*/

VOID c_pres(argstr)
char *argstr;
{

	if (argstr!=NULL&&*argstr=='\0') argstr=NULL;

	arglist(argstr,d_hold);

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *									*
 *----------------------------------------------------------------------*/

VOID c_ign(argstr)
char *argstr;
{

	if (argstr!=NULL&&*argstr=='\0') argstr=NULL;

	printf("%%unimplemented: [ignore] %s\n",argstr);

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *									*
 *----------------------------------------------------------------------*/

VOID c_mail(argstr)
char *argstr;
{

	if (argstr!=NULL&&*argstr=='\0') argstr=NULL;

	printf("%%unimplemented: [mail] %s\n",argstr);

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *									*
 *----------------------------------------------------------------------*/

VOID c_mbox(argstr)
char *argstr;
{

	if (argstr!=NULL&&*argstr=='\0') argstr=NULL;

	printf("%%unimplemented: [mbox] %s\n",argstr);

	return;

}

/*----------------------------------------------------------------------*
 *	c_move(arg) - move list of messages to folder			*
 *----------------------------------------------------------------------*/

VOID c_move(argstr)
char *argstr;
{

	if (argstr!=NULL&&*argstr=='\0') argstr=NULL;

	printf("%%unimplemented: [move] %s\n",argstr);

	return;

}

/*----------------------------------------------------------------------*
 *	c_open(arg) - close current folder and open new one		*
 *----------------------------------------------------------------------*/

VOID c_open(argstr)
char *argstr;
{

	if (argstr!=NULL&&*argstr=='\0') argstr=NULL;

	printf("%%unimplemented: [open] %s\n",argstr);

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *									*
 *----------------------------------------------------------------------*/

VOID c_next(argstr)
char *argstr;
{

	int seqnex;

	if (argstr!=NULL&&*argstr=='\0') argstr=NULL;

	seqnex = 1;

	if(argstr!=NULL) {
		seqnex=atoi(argstr);
		if (seqnex==0) {
			printf("%bad argument \"%s\" -- ignored.\n",
			argstr);
			seqnex=1;
		}
	}

	if (!firflg) fipos+=seqnex;
	firflg=FALSE;			/* [011] */

	setpos(rootdir,0);

	if (fipos>messeq) {
		ateof();
		return;
	}

	if (currdir==NULL) return;

	readbox(current_file,fipos,stdout,TRUE);

	if (currdir->sel==M_UNR) {
		currdir->sel=M_REA;
		strcpy(currdir->dispos,seen_folder);
		currdir->kepmes=FALSE;
	}

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *									*
 *----------------------------------------------------------------------*/

VOID c_last(argstr)
char *argstr;
{

	int seqnex;

	if (argstr!=NULL&&*argstr=='\0') argstr=NULL;

	seqnex = 1;

	if(argstr!=NULL) {
		seqnex=atoi(argstr);
		if (seqnex==0) {
			printf("%bad argument \"%s\" -- ignored.\n",
			argstr);
			seqnex=1;
		}
	}

	fipos-=seqnex;

	if (fipos<1) fipos=1;

	setpos(rootdir,0);

	if (fipos>messeq) {
		ateof();
		return;
	}

	if (currdir==NULL) return;

	readbox(current_file,fipos,stdout,TRUE);

	if (currdir->sel==M_UNR) {
		currdir->sel=M_REA;
		strcpy(currdir->dispos,seen_folder);
		currdir->kepmes=FALSE;
	}

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *									*
 *----------------------------------------------------------------------*/

VOID c_print(argstr)
char *argstr;
{

	if (argstr!=NULL&&*argstr=='\0') argstr=NULL;

	arglist(argstr,d_read);

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *	action(set) - using the set number from above, return the	*
 *	number of messages to write, out...  This keeps from 		*
 *	writing out into empty files and wasting lots of time		*
 *									*
 *----------------------------------------------------------------------*/

int action(set)
int set;
{

	struct dirfmt *dir;
	int howmany;

	howmany=0;

	for (dir=rootdir; dir!=NULL; dir=dir->nexdir) {

		if (((!fflag)&&set==1&&dir->sel==M_REA)||
		    (fflag&&set==2&&dir->sel!=M_DEL)||
		    ((!fflag)&&set==2&&(dir->sel==M_PRE||dir->sel==M_UNR))) {

			howmany++;

		}

	}

	return(howmany);

}
/*----------------------------------------------------------------------*
 *									*
 *	chk_act(foldnam) - Look through the directory and see how	*
 *	many messages are destined for this folder.			*
 *									*
 *----------------------------------------------------------------------*/

int chk_act(foldnam)
char *foldnam;
{

	struct dirfmt *dir;
	int howmany;

	howmany=0;

	for (dir=rootdir; dir!=NULL; dir=dir->nexdir) 
		if (streq(foldnam,dir->dispos))	howmany++;

	return(howmany);

}

BOOL sys_any()
{

	struct dirfmt *dir;

	for (dir=rootdir; dir!=NULL; dir=dir->nexdir) 
		if (!streq(current_folder,dir->dispos)) return(FALSE);

	return(TRUE);

}

/*----------------------------------------------------------------------*
 *									*
 *	c_quit(arg) - exit, and clean out files concerned		*
 *									*
 *----------------------------------------------------------------------*/

VOID c_quit(argstr)
char *argstr;
{

	if (argstr!=NULL&&*argstr=='\0') argstr=NULL;

	if (argstr!=NULL) printf("%%argument \"%s\" ignored.\n",argstr);

	closup();

	writeprofile(ParseFileContents(bombprofile("tmail","User Ini")));

	iexit();

}

/*----------------------------------------------------------------------*
 *									*
 *									*
 *----------------------------------------------------------------------*/

VOID c_reply(argstr)
char *argstr;
{

	if (argstr!=NULL&&*argstr=='\0') argstr=NULL;

	if (argstr!=NULL) printf("%%argument \"%s\" ignored.\n",argstr);

	setpos(rootdir,0);

	if (currdir==NULL) {
		ateof();
		return;
	}

	mailto=currdir->fromadr;
	sub=currdir->subject;

	printf("[Replying to %s subject %s]\n",mailto,sub);

	closup();

	mailout();

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *									*
 *----------------------------------------------------------------------*/

VOID c_save(argstr)
char *argstr;
{

	if (argstr!=NULL&&*argstr=='\0') argstr=NULL;

	printf("%%unimplemented: [save] %s\n",argstr);

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *									*
 *----------------------------------------------------------------------*/

VOID c_set(argstr)
char *argstr;
{

	char *varcop;

	if (argstr!=NULL&&*argstr=='\0') argstr=NULL;
	if (argstr==NULL) {
		showbools();
		return;
	}

	varcop=lowerit(strdup(argstr));
	if (varbool(varcop)==TRUE) {
		putbool(varcop,TRUE);
		free(varcop);
		return;
	}

	printf("%%Unknown: [set] %s\n",argstr);

	free(varcop);

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *									*
 *----------------------------------------------------------------------*/

VOID c_source(argstr)
char *argstr;
{

	if (argstr!=NULL&&*argstr=='\0') argstr=NULL;

	printf("%%unimplemented: [source] %s\n",argstr);

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *									*
 *----------------------------------------------------------------------*/

VOID c_top(argstr)
char *argstr;
{

	if (argstr!=NULL&&*argstr=='\0') argstr=NULL;

	printf("%%unimplemented: [top] %s\n",argstr);

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *									*
 *----------------------------------------------------------------------*/

VOID c_undel(argstr)
char *argstr;
{

	if (argstr!=NULL&&*argstr=='\0') argstr=NULL;

	arglist(argstr,d_undel);

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *									*
 *----------------------------------------------------------------------*/

VOID c_unset(argstr)
char *argstr;
{

	char *varcop;

	if (argstr!=NULL&&*argstr=='\0') argstr=NULL;
	if (argstr==NULL) {
		showbools();
		return;
	}

	varcop=lowerit(strdup(argstr));
	if (varbool(varcop)==TRUE) {
		putbool(varcop,FALSE);
		free(varcop);
		return;
	}

	printf("%%Unknown boolean variable: %s\n",argstr);

	free(varcop);

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *									*
 *----------------------------------------------------------------------*/

VOID c_z(argstr)
char *argstr;
{

	if (argstr!=NULL&&*argstr=='\0') argstr=NULL;

	printf("%%unimplemented: [z] %s\n",argstr);

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *									*
 *----------------------------------------------------------------------*/

#define PK_RC 0		/* Commands can come from mail.rc */
#define PK_NORC 1	/* Commands can only come from terminal */

struct parse_keywords {
	char	*pk_keyword;
	int	pk_unique;
	int	pk_type;
	VOID	(*pk_fun)();
} keylist[] = {
	{ "Print",	1, PK_NORC, c_bprint },
	{ "Reply",	1, PK_NORC, c_breply },
	{ "Type",	1, PK_NORC, c_bprint },
	{ "copy",	2, PK_NORC, c_copy   },
	{ "delete",	1, PK_NORC, c_del    },
	{ "dir",	3, PK_NORC, c_head   },
	{ "dirf",	4, PK_NORC, c_lfolds },
	{ "dispose",	2, PK_NORC, c_save   },
	{ "dt",		2, PK_NORC, c_dt     },
	{ "dp",		2, PK_NORC, c_dt     },
	{ "exit",	2, PK_NORC, c_exit   },
	{ "file",	2, PK_NORC, c_setf   },
	{ "folders",	7, PK_RC,   c_lfolds },
	{ "folder",	2, PK_NORC, c_setf   },
	{ "from",	1, PK_NORC, c_from   },
	{ "headers",	1, PK_NORC, c_head   },
	{ "help",	4, PK_NORC, c_help   },
	{ "hold",	2, PK_NORC, c_pres   },
	{ "ignore",	6, PK_RC,   c_ign    },
	{ "move",	1, PK_NORC, c_move   },
	{ "mail",	4, PK_NORC, c_mail   },
	{ "mbox",	4, PK_NORC, c_mbox   },
	{ "next",	1, PK_NORC, c_next   },
	{ "open",	1, PK_NORC, c_open   },
	{ "preserve",	3, PK_NORC, c_pres   },
	{ "print",	1, PK_NORC, c_print  },
	{ "quit",	1, PK_NORC, c_quit   },
	{ "reply",	1, PK_NORC, c_reply  },
	{ "respond",	7, PK_NORC, c_reply  },
	{ "save",	1, PK_NORC, c_save   },
	{ "set",	2, PK_RC,   c_set    },
	{ "source",	2, PK_NORC, c_source },
	{ "top",	3, PK_NORC, c_top    },
	{ "type",	1, PK_NORC, c_print  },
	{ "undelete",	1, PK_NORC, c_undel  },
	{ "unset",	5, PK_RC,   c_unset  },
	{ "write",	1, PK_NORC, c_save   },
	{ "xit",	1, PK_NORC, c_exit   },
	{ "z",		1, PK_NORC, c_z      },
	{ NULL, 0, 0 } } ;

/*----------------------------------------------------------------------*
 *									*
 *									*
 *----------------------------------------------------------------------*/

char *kbreak(istr,wordr)
char *istr, *wordr;
{

	int loper;

	for(loper=0; *(istr+loper)!='\0' && *(istr+loper)!=' ' && loper<20;
		loper++) *(wordr+loper)= *(istr+loper);

	*(wordr+loper)='\0';

	for(; *(istr+loper)!='\0' && *(istr+loper)==' '; loper++);

	if ((istr+loper)=='\0') return(NULL);

	return(istr+loper);

}

/*----------------------------------------------------------------------*
 *									*
 *									*
 *----------------------------------------------------------------------*/

doparse(istr,alowrc)
char *istr;
int alowrc;
{

	int keyloop;
	int msgnum;
	char word[22], *rest;

	if (alowrc==1) {

		if (strlen(istr)==0) {
			c_next(NULL);
			return;
		}
		if (piece(istr,"?")) {
			c_help(istr+1);
			return;

		}
		if (piece(istr,"-")) {
			c_last(istr+1);
			return;
		}
		if (piece(istr,"+")) {
			c_next(istr+1);
			return;
		}
	}

	rest=kbreak(istr,word);	

	for (keyloop=0; keylist[keyloop].pk_unique!=0; keyloop++) {

		if (keylist[keyloop].pk_unique>strlen(word)) continue;

		if (piece(keylist[keyloop].pk_keyword,word)) {
			if (alowrc!=0||keylist[keyloop].pk_type==PK_RC)
				(*(keylist[keyloop].pk_fun))(rest);
			else printf("%%Command %s not allowed in mail.rc\n",
				keylist[keyloop].pk_keyword);
			return;
		}

	}

	/* Special case if user just types in number at prompt */

	msgnum = atoi(istr);
	if (msgnum>0) {
		d_read(msgnum);
		return;
	}

	printf("?Unknown command \"%s\"\n",istr);

	return;

}

/*----------------------------------------------------------------------*
 *									*
 *									*
 *----------------------------------------------------------------------*/

ttyparse()
{

	char *ibufi;

	ibufi=malloc(BMAXI+2);
	if (ibufi==NULL) error("ttyparse() couldn't allocate io buffer");

	while(TRUE) {

		printf("& ");

		if (fgetss(ibufi,BMAXI,stdin)==NULL) break;

		doparse(ibufi,1);

	}

	free(ibufi);

}

/*----------------------------------------------------------------------*
 *									*
 *	fiparse(fname,flag) - open channel on flag and process as 	*
 *	command file until eof.						*
 *									*
 *	flag=0, file is mail.rc, don't give error or allow PK_NORC	*
 *	flag=1, give error, allow all commands				*
 *									*
 *----------------------------------------------------------------------*/

fiparse(fname,flag)
char *fname;
int flag;
{

	char *ibufi;
	FILE *fstat;

	ibufi=malloc(BMAXI+2);
	if (ibufi==NULL) error("fiparse() couldn't allocate io buffer");

	fstat=fopen(fname,"r");
	if (fstat==NULL) {
		if (flag!=0) printf("%%Command file %s cannot be opened\n",
				fname);
		return;
	}
	

	while(TRUE) {

		if (fgetss(ibufi,BMAXI,fstat)==NULL) break;
		if (feof(fstat)) break;

		doparse(ibufi,0);

	}

	free(ibufi);
	return;

}

/*----------------------------------------------------------------------*
 *									*
 *									*
 *----------------------------------------------------------------------*/

sendmail()
{

	int sect,tospec,body,ecc;
	FILE *sendfile;

	tospec=body=sect=ecc=0;

	if ((sendfile=fopen(ParseFileContents(bombprofile("tmail","Send Temp")),"w"))==NULL) 
	    error("Can't write output %s",ParseFileContents(bombprofile("tmail","Send Temp")));

	while(!feof(stdin)) {
			
		if (fgetss(buff,BMAXI,stdin)==NULL) break;
		
		if (!sect) {

			if (piece(buff,"-----")||strlen(buff)==0) {
				sect = TRUE;
				fprintf(sendfile,"\n");
				continue;
			}

			if (piece(buff,"To: ")) {
				if(!paraddress(sendfile,"To:",&buff[4]))
					error("Nobody to send message to");
				tospec++;
				continue;
			}

			if (piece(buff,"Ecc: ")&&!ecc) {
				paraddress(sendfile,"Ecc:",&buff[5]);
				continue;
			}	

			if (piece(buff,"Cc: ")&&!ecc) {
				paraddress(sendfile,"Ecc:",&buff[4]);
				continue;
			}	

			if (piece(buff,"Subject:")) {
				fprintf(sendfile,"%-10s%s\n","Subject:",
					eatwhite(&buff[8]));
				continue;
			}

			fprintf(stderr,"%Expected header, saw: \n\t\"%s\"\n",
				buff);
			fprintf(stderr,"----extraneous data ignored\n");

			continue;

		}
	
		if (!body) {
			if (strlen(buff)==0) continue;
			body++;

		}

		fprintf(sendfile,"%s\n",buff);

	}

	fclose(sendfile);
	
	if (!tospec) error("?No to address specified--no action");

	if (!body) error("?No message body seen--no action");

	sprintf(buff,"mpost %s%s%s<%s",
		getbool("metoo")? "-m ":"",
		getbool("verbose")? "-v ":"",
		getbool("debug")? "-d ":"",
		ParseFileContents(bombprofile("tmail","Send Temp")));

	execu(buff);

}

/*----------------------------------------------------------------------*
 *									*
 *									*
 *----------------------------------------------------------------------*/

mailout()
{
	FILE *tempbox;

	if((tempbox=   
	    fopen(ParseFileContents(bombprofile("tmail","Send Temp")),"w"))
	   ==NULL) 
		error("Couldn't open temporary mailbox %s",
		      ParseFileContents(bombprofile("tmail","Send Temp")));

	fprintf(tempbox,"To:      %s\n",lowerit(mailto));
/*	fprintf(tempbox,"Ecc:     \n"); */	/* [4] */
	fprintf(tempbox,"Subject: ");
	if (sub!=NULL) fprintf(tempbox,"%s",sub);
	fprintf(tempbox,"\n-------Enter Message Below This Line--------\n");

	fclose(tempbox);

/*	sprintf(buff,"edt %s,bin:mail.edt/ccl=?mail '%s' %s%s%s-e <%s?",
		MAILTEMP,mailto,
		getbool("metoo")? "-m ":"",
		getbool("verbose")? "-v ":"",
		getbool("debug")? "-d ":"",
		MAILTEMP);*/		/* (10) */

	if (getbool("verbose")) printf("\n\n%s\n",buff);

	execu(buff);
	
}

/*----------------------------------------------------------------------*
 *									*
 *									*
 *----------------------------------------------------------------------*/

main(argc,argv)
int argc;
char *argv[];
{
	int i, c;
	char *p;

	readprofile(ParseFileContents(SYS_MRC));
	readprofile(ParseFileContents(bombprofile("tmail","User Ini")));

	for (i=0; sbool[i].sbname!=NULL; i++) {
		p = findprofile("tmail",sbool[i].sbname,NULL);
		if (p==NULL) continue;
		c = atoi(p);
		if (sbool[i].sb_type==SB_BOOL)
		    c = (c!=FALSE);
		sbool[i].sb_value = c;
	}

	cflag=sflag=fflag=eflag=xflag=0;
	sub=NULL;

	mailto=NULL;

	for(i=1; i<argc; ++i) {
		p = argv[i];
		if(*p == '-') {
			++p;
			while(c = *p++)
				switch(c) {

				case 'e':
				case 'E':
					++eflag;
					break;

				case 'c':
				case 'C':
					++cflag;
					break;

				case 'v':
				case 'V':
					putbool("verbose",TRUE);
					break;

				case 'd':
				case 'D':
					putbool("debug",TRUE);
					break;

				case 'x':
				case 'X':
					xflag++;
					break;

				case 'f':
				case 'F':
					++fflag;
					if (++i!=argc-1)
						usage("Illegal -f use");
					strcpy(alt_box,argv[i]);
					break;

				case 's':
				case 'S':
					++sflag;
					if (++i!=argc-1)
						usage("Illegal -s use");
					sub=strdup(argv[i]);
					break;

				case '?':
					execu("more bin:mailiv.hlp");
					break;

				default:
					usage("Bogus flag in command line");
				}
		} else {
			if (mailto!=NULL) usage("Illegal address");
			mailto=strdup(argv[i]);
		}
	}

	if (sflag&&fflag) usage("the -f and -s flags are ambiguous together");
	if (mailto!=NULL&&fflag) usage("-f and address are ambigous");
	if (mailto==NULL&&sflag) usage("-s given with no address");

	if (getbool("debug")) {
		showbools();
		printf("File mode %s\n",fflag?"ON":"OFF");
		if (fflag) printf("File folder is %s\n",alt_box);
		if (sflag) printf("Subject seen \"%s\"\n",sub);
	}

	if (eflag) sendmail();
	if (mailto!=NULL) mailout();

	p = getenv("USERNAME");
	if (p==NULL) p = getenv("_UNAM");
	if (p==NULL) p = getenv("USER");
	if (p==NULL) p = "noname";

	unme=strdup(p);

	if (getbool("debug")) 
		printf("[Uname=%s]\n",unme);

	readprofile(ParseFileContents(SYS_MRC));
	readprofile(ParseFileContents(bombprofile("tmail","User Ini")));

	strcpy(trash_folder,bombprofile("tmail","Trash Folder"));
	strcpy(seen_folder,bombprofile("tmail","Seen Folder"));
	strcpy(current_folder,bombprofile("tmail","Main Folder"));
	current_file=strdup(folder_file(current_folder));

	if (fflag) {
		FILE *tst;

		alt_box[MAXFOLNAM] = '\0';
		strcpy(seen_folder,alt_box);
		strcpy(current_folder,alt_box);
		current_file=strdup(folder_file(current_folder));
		
		tst = fopen(current_file,"r");
		if (tst!=NULL)
			fclose(tst);
		else {
			fprintf("No such folder as %s (file %s)\n",
				alt_box,current_file);
			iexit();
		}
	}

	if (dirbox(current_file)==DIR_NOF) {
		printf("No mail for %s.\n",unme);
		if (cflag) iexit();
	}		
	
	if (messeq==0) {
		printf("\"%s\": mail folder is empty.  No mail for %s.\n\n",
		current_folder,unme);
		if (cflag) iexit(); 
	}

	if (!getbool("quiet")) {

		printf("%s\n",pname);

		printf("[Folder: %s] ",current_folder);
		switch(messeq) {
			  case 0: 
			printf("No messages in folder for %s\n",
					 unme);
			if (cflag) iexit();
			break;
			  case 1:
			printf("1 message in folder for %s\n",unme);
			break;
			  default:
			printf("%d messages in folder for %s\n",messeq,unme);
			break;
		}

		showdir(rootdir);

		if (!getbool("expert")) printf("Type in a ? for help\n");
	}

	ttyparse();

	c_quit(NULL);

}

execu(string)
char *string;
{
	printf("Execute: %s\n",string);
	exit(0);
}
