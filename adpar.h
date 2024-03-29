/*----------------------------------------------------------------------* 
 *									* 
 *	adpar.h - address parsing structures and constants		* 
 *									* 
 *----------------------------------------------------------------------*/ 

struct adnode { 
	char *nodnam;		/* Node name of this address */ 
	int routad;		/* True if a un-routable address */ 
	struct adnode *furptr;	/* Link to next node (NULL=End) */ 
	struct adnode *bakptr;	/* Link to previous node (NULL=Beginning */ 
	int nodtyp;		/* What is this node */ 
}; 

#define AD_ORG 1		/* Currently Unaliased Address */ 
#define AD_ALI 2		/* Once aliased Address */ 
#define AD_DEL 4		/* Address is deleted (aliased once) */ 

#define AT_NET 1		/* Address is a network address */ 
#define AT_GATE 2		/* Address is a legal gate */ 
#define AT_LOC 3		/* Address is a simple local node */ 
#define AT_BAD 4		/* Address is unknown local user */ 


struct adlist { 
	int adnflg;		/* Address flag word */ 
	int adntyp;		/* Address type word */ 
	struct adnode *adlths;	/* The node on this leg of the array */ 
	struct adlist *conadl;	/* Pointer to the next in the array */ 
}; 

#define NOD_ADR 1	/* Node is a node name in address */ 
#define NOD_DES 2	/* Node is the name of our destination */ 

#define ADR_MAX 15	/* Maximum permissible address (truncated) */ 
#define ADB_MAX 200	/* Maximum permissible input line size */ 

#define NET_RCN "net:net.rc" 

struct net_nabor { 
	char *naborn; 
	struct net_nabor *nexnab; 
}; 


