/*
 *	definitions and declarations for the 3COM ethernet driver
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

#ident "@(#)el.h	1.6 - 89/01/20"

#define EA3COM0	0x02		/* first byte of 3COM ethernet addresses */
#define EA3COM1	0x60		/* second byte */
#define EA3COM2	0x8c		/* third byte */

#define DMAIN	0x45		/* dma from IE buffer, single mode */
#define DMAOUT	0x49		/* dma to IE buffer, single mode */

/* bits in IE Control and Status Register */

#define IERST	0x80		/* reset (write only) */
#define IERIDE	0x40		/* request interrupt & dma enable */
#define IEDMAR	0x20		/* dma request */
#define IEDMAD	0x10		/* dma done */
#define IEECR	0x08		/* buffer to EDLC receive */
#define IEECX	0x04		/* buffer to EDLC transmit */
#define IEIRE	0x01		/* interrupt request enable */
#define IESYS	0x00		/* buffer to system bus */

#define XBUSY	0x80		/* transmit busy (still sending) */
#define RBUSY	0x01		/* receive busy (packet arriving) */

/* bits in EDLC Transmit Register */

#define IEXEOF	0x08		/* end-of-frame interrupt */
#define IEJ16	0x04		/* jam 16 interrupt */
#define IEJAM	0x02		/* jam interrupt */
#define IEUNDER	0x01		/* underflow interrupt */

/* bits in EDLC Receive Register */

#define IESTALE	0x80		/* stale receive status (read only) */
#define MULTI	0xc0		/* station address plus multicast */
#define OWN	0x80		/* station address plus broadcast */
#define ALL	0x40		/* all addresses */
#define RGOOD	0x20		/* receive good packets only */
#define RNOOF	0x10		/* receive any packet without overflow */
#define SHORTP	0x08		/* short packet */
#define ALIGN	0x04		/* allignment error */
#define FCS	0x02		/* frame check sequence error (CRC) */
#define OVER	0x01		/* overflow error */
#define NONE	0x00		/* disable receive mode */

#define RCVOK	0x1f		/* mask to start looking for valid packet */
#define RWFP	0x10		/* got a well formed packet */

/* IE register offsets - add IE base address (pnbase) to each offset */

#define IESADR	0x00		/* ethernet station address */
#define PNRCV	0x06		/* EDLC receive register */
#define PNXMT	0x07		/* EDLC transmit register */
#define IEGBP	0x08		/* general purpose buffer pointer */
#define IERBP	0x0a		/* receive buffer pointer */
#define IEPADR	0x0c		/* my ethernet address PROM */
#define IECSR	0x0e		/* control and status register */
#define IEMEM	0x0f		/* buffer access register */

/*
 *	streams related definitions
 */
#define PNVPKTSZ	(3*256)
#define PNHIWAT		(16*PNVPKTSZ)
#define PNLOWAT		(8*PNVPKTSZ)
#define PNMAXPKT	1500
#define PNMAXPKTLLC	1497	/* max packet when using LLC1 */
#define PNMINSEND	60	/* 64 - 4 bytes CRC */

/*
 *	debug bits
 */
#define PNTRACE		0x01
#define PNRECV		0x02
#define PNSEND		0x04
#define PNERRS		0x10
#define PNINT		0x20

#ifdef DEBUG
#define PNDEBUG
#endif

/*
 *	board state
 */
#define PNB_IDLE	0
#define PNB_WAITRCV	1
#define PNB_XMTBUSY	2
#define PNB_ERROR	3


/*
 *	3COM board statistics
 */

#define PN_NSTATS	16

struct pnstats {

       /* non-hardware */
   struct llcstats pns_llcs;	/* 0-3 */

       /* transmit */
   ulong	pns_xpkts;	/* 4 */
   ulong	pns_xbytes;	/* 5 */
   ulong	pns_excoll;	/* 6 */
   ulong	pns_coll;	/* 7 */
   ulong	pns_under;	/* 8 */
   ulong	pns_carrier;	/* 9 */

       /* receive */
   ulong	pns_rpkts;	/* 10 */
   ulong	pns_rbytes;	/* 11 */
   ulong	pns_fcs;	/* 12 */
   ulong	pns_align;	/* 13 */
   ulong	pns_overflow;	/* 14 */
   ulong	pns_short;	/* 15 */
};

/*
 *	3COM 3C501 board dependent variables.
 *	In a structure to use multiple boards.
 */

struct pnvar {
    int pn_begaddr;		/* start address for transmit */
    int pn_chprom;		/* flag for changing promiscuous mode */
};
