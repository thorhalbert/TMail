/*----------------------------------------------------------------------*
 *									*
 *	Mailer and network parameters					*
 *									*
 *----------------------------------------------------------------------*/

/* Various configuration paramerers */

/* #define RSTS_B */
/* #define VAX_B */
/* #define RT11_B */
/* #define UNIX_B */
#define MSDOS_B

#define DRAG	/* Does office program support dragon mode */
#define NET_ON	/* Does mpost allow posting into net queues */

/* Parameters which are needed by mail */

#ifdef MSDOS

#define SYS_MRC "%_SYSD%\asl\mailsys.ini"

#else

#define SYS_MRC "~/tmail/mailsys.ini"

#endif

