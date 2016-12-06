/*
 *	Streams-based 3COM ethernet driver
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1988 INTERACTIVE Systems Corporation
 *	All rights reserved.
 *
 *	RESTRICTED RIGHTS:
 *
 *	These programs are supplied under a license.  They may be used,
 *	disclosed, and/or copied only as permitted under such license
 *	agreement.  Any copy must contain the above copyright notice and
 *	this restricted rights notice.  Use, copying, and/or disclosure
 *	of the programs is strictly prohibited unless otherwise provided
 *	in the license agreement.
 */
#ident "@(#)Space.c	1.5 - 88/10/27"
#ident "@(#) (c) Copyright INTERACTIVE Systems Corporation 1988"

#include "sys/types.h"
#include "sys/stream.h"
#include "sys/bsdtypes.h"
#include "sys/llc.h"
#include "sys/el.h"


#ifdef MERGE
#include "config.h"	
#define Nel 		el_UNITS	/* Number of 3COM boards */
#define	NSTRS 		el_MAXSUB	/* Number of 3COM streams allowed */
#define el_IRQ0 	el_0_VECT	/* 3COM Interrupt level */
#define el_BASE0 	el_0_SIOA	/* 3COM base I/O port */
#define el_SIZE0 	2048		/* 3COM memory size */
#define el_MAJOR0 	el_CMAJ		/* 3COM major device number */

#else

#define Nel 1			/* Number of 3COM boards */
#define	NSTRS 5			/* Number of 3COM streams allowed */
#define el_IRQ0 3		/* 3COM Interrupt level */
#define el_BASE0 0x300		/* 3COM base I/O port */
#define el_SIZE0 2048		/* 3COM memory size */
#define el_MAJOR0 36		/* 3COM major device number */


#include "config.h"	/* to allow override of above defines */
#endif  /* MERGE */

/* el streams */
int el_cnt = Nel;			/* number of boards */
struct	llcdev	eldevs[NSTRS*Nel];	/* minor device structures */
struct elstats el_stats[Nel];		/* statistics structure */
struct elvar elvar[Nel];		/* board dependent variables */

struct	llcparam elparams[Nel] = {
	{	0,			/* board index */
		el_IRQ0,		/* interrupt level */
		el_BASE0,		/* I/O port base */
		16,			/* I/O port range */
		0,			/* base address of shared memory */
		el_SIZE0,		/* memory size */
		0,			/* mapped shared memory */
		0,			/* init status */
		0,			/* board state */
		el_MAJOR0,		/* major device number */
		NSTRS			/* number of minor devices */
	}
};

/* debug */
#ifdef elDEBUG
int eldebug = 0x10;
#endif
