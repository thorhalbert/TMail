/*----------------------------------------------------------------------* 
 *	UNIX Emulation Package for RSTS - UEP/RSTS			* 
 *									* 
 *	Copyright (C) 1985 by Scott Halbert				* 
 *----------------------------------------------------------------------*/ 

/*----------------------------------------------------------------------* 
 *	RSTS routine to scan for a certain PPN and its attached 	* 
 *	terminals, sending a given message to each for mail which	* 
 *	is delivered into the mailbox from office			* 
 *----------------------------------------------------------------------*/ 

#include <stdio.h> 
#include <rsts.h> 

struct rsts_firqb jobstat; 

broadcast(ttynum,text) 
int ttynum; 
char *text; 
{ 

	int left,err;		/* Number of bytes not yet sent out */ 

	left = strlen(text);	/* Start out here at this point */ 

	while (left>0) { 

		clrfqx(); 
		xrb.xrlen = 6;		/* send text to specified keyboard */ 
		xrb.xrbc = strlen(text); /* Length of string to send out */ 
		xrb.xrloc = text;	/* Location of the send to send */ 
		xrb.xrblkm = TTYHND;	/* TTY handler mode	*/ 
		xrb.xrblk = ttynum;	/* Unit number of terminal */ 
		if ((err=rstsys(_SPEC))!=0) return(err); 

		left=xrb.xrbc;		/* Number of bytes not yet done */ 
		text+=(strlen(text)-left)+1; /* Compute whats left */ 

	} 

	return(NULL); 

} 

uusystat(job) 
int job; 
{ 

	register int	errret; 

	clrfqx();		 
	 
	firqb.fqfun = UU_SYS;	/* RSTS return job status info (prived) */ 
	firqb.fqsizm = 0;	/* access table 0 */ 
	firqb.fqfil = job;	/* Job in question */ 

	if ((errret=rstsys(_UUO))!=0) return(errret); 

	getfqb(&jobstat); 

	return(0); 

} 

disjob(proj,prog,mess) 
int proj,prog; 
char *mess; 
{ 

	int job,ans; 

	for (job=1; job<64; job++) { 

		ans=uusystat(job); 
		if (ans==PRVIOL) continue; 
		if (ans==BADFUO) break; 

		if ((jobstat.fqfil&0377)/2!=job) { 
			printf("disjob doesn't have enough privs\n"); 
			return; 
		} 

/*		printf("Job %d [%d,%d] tty%d ctl=%d\n", 
			job,jobstat.fqprot&0377, 
			jobstat.fqpflg&0377, 
			jobstat.fqsizm,jobstat.fqppn&0377);	*/ 

		if ((jobstat.fqprot&0377)==proj&& 
		    (jobstat.fqpflg&0377)==prog&& 
		    (jobstat.fqppn&0377)==0&& 
	            jobstat.fqsizm>0) broadcast(jobstat.fqsizm&0377,mess); 

	} 

	return; 

}			 

