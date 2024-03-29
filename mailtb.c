# 
#include <stdio.h> 
#include <uem.h> 

gident(idfil,ifile) 
struct idrec *idfil; 
char *ifile; 
{ 

	FILE *idfile; 
	int	idsize,idread; 

	idsize=sizeof (struct idrec); 

	idfile=fopen(ifile,"rnu"); 

	if (idfile==NULL) { 
		fprintf(stderr,"Cannot open error %d\n",$$ferr); 
		return(FALSE); 
	} 

	idread=fget(idfil,idsize,idfile); 

	fclose(idfile); 

	if (idread!=idsize) { 
		fprintf(stderr,"Wrong return size\n"); 
		return(FALSE); 
	} 

	if (idfil->magic!=magprime) { 
		fprintf(stderr,"Incorrect magic number\n"); 
		return(FALSE); 
	} 


/*	printf("Uid=%d, Gid=%d, Username=\"%s\"\n",idfil->uid,idfil->gid, 
		&(idfil->uname)); */ 

	fprintf(stderr,"Success\n"); 

	return(TRUE);		 

} 

char buffin[100]; 

main() 
{ 

	struct idrec iddata; 
	int ret; 

	while(!feof(stdin)) { 

		fgetss(&buffin,98,stdin); 
			 
		if (feof(stdin)) break; 

		fprintf(stderr,"Try %s -- ",&buffin); 

		ret=gident(&iddata,&buffin); 

		if (ret==TRUE) { 

			printf("%06d %06d %-12s %-30s\n",iddata.uid, 
				iddata.gid,&iddata.uname,&buffin); 

		} 

	} 

	exit(1); 

} 

