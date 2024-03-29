#include <stdio.h> 
#include <uem.h> 

#ifdef decus 
int $$narg=1;		/* Don't care about argv at all */ 
#endif 

struct idrec user; 

#define OUTTEMP "sy:selfin.tmp" 

int myuid; 

FILE *ofile; 

#define BMAXI 300 

char buff[BMAXI+2]; 

int body; 

main() 
{ 

	body = FALSE; 

	myuid=$xidget(&user); 

	if (myuid==NULL) error("User not set up!!!"); 

	if ((ofile=fopen(OUTTEMP,"w"))==NULL) error("Cannot open temp %s", 
						OUTTEMP); 

	fprintf(ofile,"To:   %s\n",&user.uname); 

	if (ftty(stdin)) { 
		printf("Subject: "); 
		fflush(stdout); 
	}		/* Don't display this unless our tty is stdin */ 

	if (fgetss(&buff,BMAXI,stdin)==NULL) exit(1); 

	if (strlen(&buff)!=0) fprintf(ofile,"Subject:   %s\n",&buff); 

	fprintf(ofile,"\n"); 

	while(!feof(stdin)) { 

		if (fgetss(&buff,BMAXI,stdin)==NULL) break; 

		body=TRUE; 

		fprintf(ofile,"%s\n",&buff);	 

	} 

	if (body==FALSE) error("Empty body"); 

	fclose(ofile); 

	sprintf(&buff,"mpost <%s", 
		OUTTEMP); 

	execu(&buff); 

	exit(1); 

} 

