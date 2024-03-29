#include <stdio.h> 
#include "src:adpar.h" 

char *adsep[] = {	/* Address separator chars to corresp to above */ 
	"?","!",":" 
}; 

struct net_nabor *nab_list; 

char *self_nam; 

char linbf[ADB_MAX+2],nodbf[ADB_MAX+2],*lefptr; 
int rouflg,hopmax; 

nib_nod(curptr,iobuf) 
char *curptr; 
FILE *iobuf; 
{ 

	register char inch; 
	char *curbf; 

	curbf=&nodbf; 
	rouflg=FALSE; 

	while (*curptr!='\0') { 
		if (*curptr=='\\') { 
			curptr++; 
			if (*curptr=='\0') { 
				if(lod_nod(iobuf)==NULL) return(NULL); 
				curptr=&linbf; 
				while(isspace(*curptr)) curptr++; 
				continue; 
			} 
		} 
		inch=tolower(*curptr++); 
		if (inch=='*') { 
			rouflg=TRUE; 
			continue; 
		} 
		if (inch=='-') inch='_'; 
		if (isalnum(inch)||inch=='_') { 
			*curbf++=inch; 
			continue; 
		} 
		*curbf='\0'; 
		return(--curptr); 
	} 
	*curbf='\0'; 
	return(curptr); 
} 

lod_nod(iobuf) 
FILE *iobuf; 
{ 
	return(fgetss(&linbf,ADB_MAX,iobuf)); 
} 

eat_back(adn) 
struct adnode *adn; 
{ 
	if (adn==NULL) return; 
	eat_back(adn->bakptr); 
	if (adn->nodnam!=NULL) mfree(adn->nodnam); 
	mfree(adn); 
	return; 
} 

nod_wind(adn) 
struct adnode *adn; 
{ 
	if (adn==NULL) return; 
	nod_wind(adn->furptr); 
	if (adn->nodnam!=NULL) mfree(adn->nodnam); 
	mfree(adn); 
	return; 
} 

adr_par(iobuf,staptr) 
FILE *iobuf; 
char *staptr; 
{ 

	char *curptr; 
	struct adnode *newnod,*lasnod,*topnod; 
	 
	topnod=newnod=lasnod=NULL; 

	if (staptr==NULL) { 
		if(lod_nod(iobuf)==NULL) return(NULL); 
		curptr=&linbf; 
	} 
	else curptr=staptr; 

	while(isspace(*curptr)) curptr++; 

	while(TRUE) { 

		if ((curptr=nib_nod(curptr,iobuf))==NULL) { 
			nod_wind(topnod); 
			return(NULL); 
		} 

		if (nodbf[0]=='\0'&&(*curptr==':'||*curptr=='!')) { 
			curptr++; 
			continue; 
		} 
		if (!isalpha(nodbf[0])&&nodbf[0]!='\0') { 
			fprintf(stderr,"---%s\nBad Node name%s\n",&linbf, 
					&nodbf); 
			nod_wind(topnod); 
			lefptr=NULL; 
			return(NULL); 
		} 

		nodbf[ADR_MAX]='\0';	/* Limit addresses to ADR_MAX chars */ 
		if((newnod=malloc(sizeof(struct adnode)))==NULL) 
			error("Malloc ran out of room"); 
		newnod->nodnam=alstr(&nodbf); 
		newnod->routad=rouflg; 
		newnod->furptr=NULL; 
		newnod->bakptr=NULL; 
		newnod->nodtyp=0; 
		if (lasnod==NULL) { 
			topnod=lasnod=newnod; 
		} 
		else { 
			lasnod->furptr=newnod; 
			newnod->bakptr=lasnod; 
			lasnod=newnod; 
		} 

		switch(*curptr) { 

		case ':': 
		case '!': 
		case '\0': 
			break; 
/*		case '[': 
			if((curptr=lod_gate(++curptr,iobuf))==NULL) { 
				nod_wind(topnod); 
				return(NULL); 
			} 
			lefptr=curptr; 
			return(topnod); */ 
		default:  
			if (isspace(*curptr)) { 
				while(isspace(*curptr)) curptr++; 
				lefptr=curptr; 
				return(topnod); 
			} 

			fprintf(stderr,"---%s\nBad delimiter %c\n",&linbf, 
					*curptr); 
			nod_wind(topnod); 
			lefptr=NULL; 
			return(NULL); 

		} 
		if (*curptr=='\0') { 
			lefptr=NULL; 
			return(topnod); 
		} 
		curptr++; 
	} 
} 

struct adnode *red_add(inadn) 
struct adnode *inadn; 
{ 

	struct adnode *endnod,*lopnod; 
	struct net_nabor *nablop; 

	for (endnod=inadn;endnod->furptr!=NULL;endnod=endnod->furptr) ; 

	endnod->nodtyp=NOD_DES; 
	endnod->routad=FALSE; 
	if (endnod->bakptr!=NULL)  
		for (lopnod=endnod->bakptr;lopnod!=NULL; 
				lopnod=lopnod->bakptr) { 
			lopnod->nodtyp=NOD_ADR; 
			if (streq(self_nam,lopnod->nodnam)) { 
				endnod=lopnod->furptr; 
				endnod->bakptr=NULL; 
				eat_back(lopnod); /* Crunch from here back */ 
				return(endnod); 
			} 
			for (nablop=nab_list;nablop!=NULL; 
					nablop=nablop->nexnab) { 
				if (streq(lopnod->nodnam,nablop->naborn)) { 
					if(lopnod->bakptr!=NULL) { 
						eat_back(lopnod->bakptr); 
						lopnod->bakptr=NULL; 
					} 
					lopnod->routad=TRUE; 
					return(lopnod); 
				} 
			} 
		} 
	return(inadn); 
} 

	 

net_rc() 
{ 
	FILE *nrc; 
	struct adnode *gtnod; 
	struct net_nabor *lasnab,*newnab; 
	char tryone; 

	self_nam=NULL; 
	nab_list=lasnab=newnab=NULL; 

	if ((nrc=fopen(NET_RCN,"r"))==NULL)  
		error("Couldn't open net descriptor %s",NET_RCN); 

	while (!feof(nrc)) { 

		lod_nod(nrc); 

		if (piece(&linbf,"self ")) { 
			lefptr=NULL; 
			gtnod=adr_par(nrc,&linbf[5]); 
			if (gtnod==NULL)  
				error("%s file has bad self line",NET_RCN); 
			if (gtnod->furptr!=NULL) 
				error("%s file has bad self line",NET_RCN); 
			if (lefptr!=NULL)  
				error("%s file has bad self line",NET_RCN); 
			self_nam=alstr(gtnod->nodnam); 
			nod_wind(gtnod); 
			continue; 
		} 

		if (piece(&linbf,"links ")) { 
			lefptr=&linbf[6]; 
			while (TRUE) { 
				gtnod=adr_par(nrc,lefptr); 
				if (gtnod==NULL)  
				  error("%s file has bad links line",NET_RCN); 
				else { 
					if (gtnod->furptr!=NULL) 
				error("%s file has bad links line",NET_RCN); 
					if ((newnab=malloc(sizeof (struct 
						net_nabor)))==NULL) 
					   error("malloc failed in net_rc"); 
					newnab->naborn=alstr(gtnod->nodnam); 
					newnab->nexnab=NULL; 
					if (nab_list==NULL)  
						nab_list=lasnab=newnab; 
					else lasnab=lasnab->nexnab=newnab; 
					nod_wind(gtnod); 
				} 
				if (lefptr!=NULL) continue; 
				tryone=getc(nrc); 
				if (tryone==' '||tryone=='\t') continue; 
				ungetc(tryone,nrc); 
				break; 
			} 
		continue; 
		} 
		if (piece(&linbf,"hopmax ")) continue; 
		if (piece(&linbf,"local ")) continue; 
		fprintf(stderr,"Unknown %s line:\n---%s\n",NET_RCN,&linbf); 
	} 
	if (self_nam==NULL) error("Self node name never defined"); 
	printf("[Self node name is %s]\n",self_nam); 
	if (nab_list==NULL) printf("[No neighbor links]\n"); 
	else { 
		printf("[Neighbors:"); 
		for(newnab=nab_list;newnab!=NULL;newnab=newnab->nexnab) 
			printf(" %s",newnab->naborn); 
		printf("]\n"); 
	} 
	fclose(nrc); 
	return; 
} 

struct adlist *aladl() 
{ 
	struct adlist *retadl; 
	retadl=malloc(sizeof (struct adlist)); 
	if (retadl==NULL) error("aladl() ran out of memory"); 
	return(retadl); 
} 

/*----------------------------------------------------------------------* 
 *									* 
 *	adnlist - sto an adress away on the address list, eliminate 	* 
 *	dups before adding.						* 
 *									* 
 *----------------------------------------------------------------------*/ 

struct adlist *adnlist(oldroot,adress,adtype) 
struct adlist *oldroot; 
struct adnode *adress; 
int adtype; 
{ 
	struct adlist *lasadl,*tadl; 

	if (oldroot==NULL) { 
		oldroot=aladl(); 
		oldroot->adnflg=adtype; 
		oldroot->adlths=adress; 
		oldroot->conadl=NULL; 
	} 
	else { 
		for(lasadl=oldroot;lasadl!=NULL; 
				lasadl=lasadl->conadl) { 
			if (nodeq(adress,lasadl->adlths)) { 
				nod_wind(adress); 
				break; 
			} 
			if (lasadl->conadl==NULL) { 
				tadl=aladl(); 
				tadl->adnflg=adtype; 
				tadl->adntyp=0; 
				tadl->adlths=adress; 
				tadl->conadl=NULL; 
				lasadl->conadl=tadl; 
				break; 
			} 
		} 
	} 

	return(oldroot); 
} 

struct adlist *sernode(stfn,rotpt) 
char *stfn; 
struct adlist *rotpt; 
{ 
	struct adlist *lasadl; 
	for(lasadl=rotpt;lasadl!=NULL;lasadl=lasadl->conadl) 
		if (streq(stfn,lasadl->adlths->nodnam)&& 
			(lasadl->adlths->furptr==NULL)) return(lasadl); 
	return(NULL); 
} 


/*----------------------------------------------------------------------* 
 *									* 
 *	getmulad(oldroot,starstr,rdchan) - starting with string 	* 
 *	fragment starstr, begin parsing addresses from rdchan, and	* 
 *	return a root for a new structure containing a list of 		* 
 *	addresses.  If oldroot==NULL, create a new root, if it is not	* 
 *	append new addresses to the end.  If starstr==NULL, go ahead	* 
 *	and load up a buffer from rdchan and start reading.  Read	* 
 *	until there is a character in column one which is not a space	* 
 *	type character (uses ungetc to put it back)			* 
 *									* 
 *----------------------------------------------------------------------*/ 

struct adlist *getmulad(oldroot,starstr,rdchan,adflag) 
struct adlist *oldroot; 
char *starstr; 
int adflag; 
FILE *rdchan; 
{ 

	struct adnode *adress; 
	char tryone; 

	lefptr=starstr; 

	while (TRUE) { 
		adress=adr_par(rdchan,lefptr); 
		if (adress==NULL) printf("That address was bad\n"); 
		else { 
			adress=red_add(adress); 
			oldroot=adnlist(oldroot,adress,adflag); 
			 
		} 
		if (lefptr!=NULL) continue; 
		tryone=getc(rdchan); 
		ungetc(tryone,rdchan); 
		if (tryone==' '||tryone=='\t') continue; 
		break; 
	} 

	return(oldroot); 

} 
/*----------------------------------------------------------------------* 
 *									* 
 *	getsinadd(starstr,rdchan) - starting with string 		* 
 *	fragment starstr, begin parsing addresses from rdchan, and	* 
 *	return a root for a new structure containing 	 		* 
 *	a single address - give warning if extra addresses are given	* 
 *									* 
 *----------------------------------------------------------------------*/ 

struct adnode *getsinadd(starstr,rdchan) 
char *starstr; 
FILE *rdchan; 
{ 

	struct adnode *adress; 
	char tryone; 

	lefptr=starstr; 

	while (TRUE) { 
		if (adress==NULL) { 
			adress=adr_par(rdchan,lefptr); 
			if (adress==NULL) printf("That address was bad\n"); 
			else { 
				adress=red_add(adress); 
			} 
		} 
		if (lefptr!=NULL) { 
			fprintf(stderr,"%%Redundant address %s\n",lefptr); 
			lefptr=NULL; 
		} 
		tryone=getc(rdchan); 
		ungetc(tryone,rdchan); 
		if (tryone==' '||tryone=='\t') { 
			lod_nod(rdchan); 
			fprintf(stderr,"%%Redundant address %s\n", 
				eatwhite(&linbf)); 
			continue; 
		} 
		break; 
	} 
	return(adress); 
} 

nodeq(st1,st2) 
struct adnode *st1,*st2; 
{ 

	if (st1==NULL&&st2==NULL) return(TRUE); 
	if (st1==NULL||st2==NULL) return(FALSE); 
	if (streq(st1->nodnam,st2->nodnam))  
		return(nodeq(st1->furptr,st2->furptr)); 
	return(FALSE); 
} 

mulpretty(mulptr,str,maxln,out) 
struct adlist *mulptr; 
char *str; 
int maxln; 
FILE *out; 
{ 

	struct adlist *nexadd; 
	int lpr,ftm,tln; 
	char *bufs; 

	ftm=TRUE; 
	if((bufs=malloc(maxln+2))==NULL) error("mulpretty() out of room"); 

	for(nexadd=mulptr;nexadd!=NULL;nexadd=nexadd->conadl) { 

		if (ftm) { 
			tln=strlen(str); 
			for(lpr=1;lpr<=maxln;lpr++) { 
				if (lpr<=tln) *(bufs+lpr-1)=*(str+lpr-1); 
				else *(bufs+lpr-1)=' '; 
			} 
		} 
		else { 

			for(lpr=1;lpr<=maxln;lpr++) *(bufs+lpr-1)=' '; 
			*(bufs+lpr-1)=EOS; 
		} 

		if (nexadd->adnflg!=AD_DEL) { 
			adpretty(bufs,nexadd->adlths,out); 
			ftm=FALSE; 
		} 

	} 
	mfree(bufs); 

} 
			 
adpretty(ptstr,adres,outfl) 
char *ptstr; 
struct adnode *adres; 
FILE *outfl; 
{ 

	char stbf[80],*nxtr; 
	int clen; 
	struct adnode *node; 

	nxtr=cpystr(&stbf,ptstr); 
	clen=strlen(&stbf); 

	for(node=adres;node!=NULL;node=node->furptr) { 
		if (clen+strlen(node->nodnam)>75) { 
			fprintf(outfl,"%s\\\n",&stbf); 
			for (clen=1;clen<=strlen(ptstr)+3;clen++)  
				stbf[clen-1]=' '; 
			nxtr=&stbf[clen-1]; 
		} 
		if (node->bakptr!=NULL) nxtr=cpystr(nxtr, 
			adsep[node->nodtyp]); 
		if (node->routad) nxtr=cpystr(nxtr,"*"); 
		nxtr=cpystr(nxtr,node->nodnam); 
		clen=strlen(&stbf); 
	} 

	fprintf(outfl,"%s\n",&stbf); 
} 

/*----------------------------------------------------------------------* 
 *									* 
 *	alias(filename) - do as many alias passes as necessary until	* 
 *	no more matches							* 
 *									* 
 *----------------------------------------------------------------------*/ 

alias(fname,allist) 
char *fname; 
struct adlist *allist; 
{ 

	int hits,alp,first; 
	char alstore[ADR_MAX+2],buff[ADB_MAX+2]; 
	FILE *fptr; 
	struct adlist *fndlst; 

	first=TRUE; 

	hits=TRUE; 

	while(hits) { 

		hits = FALSE; 

		fptr=fopen(fname, "r"); 
		if (fptr==NULL) return; 

		if (first) printf("[Aliasing with %s]\n",fname); 

		first=FALSE;  

		while(!feof(fptr)) { 

			if (fgetss(&buff,ADB_MAX,fptr)==NULL) break; 

			if (buff[0]=='#') continue; 
			if (buff[0]==' ') continue; 
			if (buff[0]=='\t') continue; 
			if (buff[0]=='\0') continue; 

			alp = 0; 

			while(buff[alp]!=' '&&buff[alp]!='\t') { 
				if (buff[alp]=='\0') break; 
				if (alp<ADR_MAX) { 
					alstore[alp]=buff[alp]; 
					alp++; 
				} 
			} 

			if (buff[alp]=='\0') continue; 
			 
			alstore[alp]='\0'; 

			lowerit(&alstore); 

			fndlst=sernode(&alstore,allist); 

			if (fndlst==NULL) continue; 
			if (fndlst->adnflg==AD_DEL) continue; 

			fndlst->adnflg=AD_DEL; 

			getmulad(allist,&buff[alp],fptr,AD_ALI); 

			hits = TRUE; 

		} 

		fclose(fptr); 
		 
	} 

} 

shtaddr(ochan,adrls) 
FILE *ochan; 
struct adnode *adrls; 
{ 
	struct adnode *adstr; 

	for (adstr=adrls;adstr->furptr!=NULL;adstr=adstr->furptr); 

	if (adstr->bakptr==NULL) fprintf(ochan,"%s:",self_nam); 
	else fprintf(ochan,"%s:",adstr->bakptr->nodnam); 
	fprintf(ochan,"%s",adstr->nodnam); 

} 

