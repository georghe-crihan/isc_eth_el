#ident "@(#)Driver.o: INTERACTIVE UNIX System, INTERACTIVE TCP/IP Version 1.2.0"

#include <sys/param.h> /* for NULL et al. */
#include <sys/types.h>
#include <sys/bsdtypes.h>
#include <sys/signal.h> /* for struct user */
#include <sys/fs/s5dir.h> /* for struct user */
#include <sys/user.h>
#include <sys/sysmacros.h> /* for major() et al. */
#include <sys/immu.h> /* for sptalloc() */
#include <sys/stream.h>
#include <sys/llc.h>
 
#include <sys/inline.h> /* intr_disable() et al. */
#include <sys/el.h>

extern struct llcparam elparams[1];
extern struct elstats el_stats[1];
extern struct llcdev eldevs[1];
extern struct elvar elvar[1];
extern int el_cnt;

/* From 0 to 5 */
int irq_val_tbl[6] = { 0, 0, 0x10, 0x20, 0x40, 0x80 };

/* FIXME */
#define LOBYTE(i) i

/* -------------------------------------------------------------------------
 * Function declarations
 */

int el_open(/* queue_t *queue, unsigned int dev, int flag, int sflag */);
void el_close(/* queue_t *queue */);
void elinit();
void el_init_board(/* struct llcparam *llcparam */);
void el_reset(/* struct llcparam *llcparam */);
void el_saddr(/* struct llcparam *llcparam */);
void elintr(/* int irq */);
void el_getp(/* int *count, struct llcparam *llcparam */);
void el_gstat(/* struct llcparam *llcparam */);
int el_send(/* struct llcdev *llcdev, struct mblk_t *mblk */);
int el_canwrite(/* struct llcdev *llcdev */);
void el_prom(/* struct llcparam *llcparam */);
void el_start_board(/* struct llcparam *llcparam */);
void el_stop_board(/* struct llcparam *llcparam */);
void el_NIC_reset(/* struct llcparam *llcparam */);

typedef int (*int_callback_fn_t)();

extern int llc_rsrv();
extern int llc_wput();
extern int llc_wsrv();

/*
int sptalloc(unsigned int count_pages, int mode, unsigned int base, int memflags);
int printf(const char *Format, ...);
int splhi();
int splx(int previous);
void llc_sched(struct llcdev *llcdev);
mblk_t allocb(size_t size, unsigned int pri);
void llc_recv(struct mblk_t *mblk, struct llcdev *llcdev);
*/


/* ----- (000000D0) ----- ARGSUSED */
int el_open(queue, dev, flag, sflag)
	queue_t *queue;
	unsigned int dev;
	int flag;
	int sflag;
{
  struct llcparam *llcparam;
  int i;
  int result;
  short tflags;
  struct llcdev *llcdev;
  short dev_minor;

  llcparam = elparams;

  /* Find our major number in elparams */
  for ( i = el_cnt; i && llcparam->llcp_major != emajor(dev); --i )
    ++llcparam;

  if ( !i ) /* None found! */
    goto error;

  /* Get device minor, if it is a CLONing open */
  if ( sflag == CLONEOPEN )
  {
    for ( dev_minor = 0; dev_minor < llcparam->llcp_minors &&
		eldevs[llcparam->llcp_firstd + dev_minor].llc_qptr;
		++dev_minor )
      ;
  }
  else
  {
    dev_minor = dev;
  }

  if ( dev_minor < llcparam->llcp_minors )
  {
    /* Initialize the device structure */
    llcdev = &eldevs[llcparam->llcp_firstd + dev_minor];
    queue[1].q_ptr = (caddr_t)llcdev;
    queue->q_ptr = (caddr_t)llcdev;
    llcdev->llc_qptr = queue + 1;
    llcdev->llc_reset = (int_callback_fn_t)el_reset;
    llcdev->llc_saddr = (int_callback_fn_t)el_saddr;
    llcdev->llc_send = (int_callback_fn_t)el_send;
    llcdev->llc_prom = (int_callback_fn_t)el_prom;
    llcdev->llc_gstat = (int_callback_fn_t)el_gstat;
    llcdev->llc_state = 0;
    llcdev->llc_type = 0;
    llcdev->llc_sap = 0;
    llcdev->llc_no = dev_minor;
    llcdev->llc_stats = (struct llcstats *)&el_stats[llcparam->llcp_index];
    llcdev->llc_macpar = llcparam;
    llcdev->llc_flags = LLC_OPEN;

    /* Is it root, who opens the device? */
    if ( u.u_uid && u.u_ruid )
      tflags = 0;
    else
      tflags = LLC_SU;
    llcdev->llc_flags |= tflags;

    if ( !llcparam->llcp_running ) /* Start the board, if it is not running */
      el_start_board(llcparam);
    ++llcparam->llcp_running; /* Mark as running */

    return dev_minor;
  } else { /* No minor found! */

error:
    return OPENFAIL;
  }
}

/* ----- (00000234) -------------------------------------------------------- */
void el_close(queue)
	queue_t *queue;
{
  struct llcdev *llcdev;
  struct llcparam *llcparam;
  u_short flags;
  int v4;
  int v5;
  unsigned char v6;

  llcdev = (struct llcdev *)queue->q_ptr;
  llcparam = llcdev->llc_macpar;
  flags = llcdev->llc_flags;

  /* Is it in promiscious mode? */
  if ( flags & LLC_PROM )
  {
    /* Garbage follows */
    v6 = __OFSUB__(llcparam->llcp_proms, 1);
    v4 = llcparam->llcp_proms == 1;
    v5 = llcparam->llcp_proms-- - 1 < 0;

    if ( (unsigned char)(v5 ^ v6) | v4 )
    {
      llcparam->llcp_proms = 0;
      llcparam->llcp_devmode &= -(LLC_PROM|LLC_OPEN); /* This is !LLC_PROM */
      el_prom(llcparam);
    }
    /* End of garbage */
  }

  llcdev->llc_qptr = NULL;
  llcdev->llc_state = ELB_IDLE;
  llcdev->llc_sap = 0;
  llcdev->llc_type = 0;
  llcdev->llc_flags = 0;
  llcdev->llc_stats = 0;
  llcdev->llc_macpar = NULL;

  if ( llcparam->llcp_running > 0 )
    --llcparam->llcp_running; /* Decrease the counter of running instances */

  if ( !llcparam->llcp_running )
    el_stop_board(llcparam);
}

/* ----- (000002B4) -------------------------------------------------------- */
void elinit()
{
  struct llcparam *llcparam;
  int i;
  unsigned char ctrlval;

  llcparam = elparams;

  for ( i = 0; i < el_cnt; ++i )
  {
    llcparam->llcp_index = i;
    llcparam->llcp_firstd = i * llcparam->llcp_minors;

    if ( llcparam->llcp_state )
      ctrlval = ELAXCVR;
    else
      ctrlval = 0;
    elvar[i].el_onbd = ctrlval;

    llcparam->llcp_state = ELB_IDLE;
    llcparam->llcp_running = 0;
    llcparam->llcp_nextq = -1;
    llcparam->llcp_maxpkt = ELMAXPKT;
    llcparam->llcp_maxpktllc = ELMAXPKTLLC;
    llcparam->llcp_proms = 0;
    llcparam->llcp_devmode = 0;
    el_stats[i].els_llcs.llcs_nstats = EL_NSTATS;

    /* Allocate and lock the memory-mapped buffer */
    llcparam->llcp_memp = (caddr_t)sptalloc(
                                     (llcparam->llcp_memsize + 4095) >> 12,
                                     PG_P,
                                     (llcparam->llcp_base + 4095) >> 12,
                                     0);
    elvar[i].el_nxtpkt = RCV_START;
    el_init_board(llcparam);
    el_saddr(llcparam);
    ++llcparam;
  }
}

/* ----- (00000384) -------------------------------------------------------- */
void el_init_board(llcparam)
	struct llcparam *llcparam;
{
  short port;
  int i;
  int index;

  port = llcparam->llcp_port;
  index = llcparam->llcp_index;
  /* Several INs skipped! */
  outb(port + ELCTRL, elvar[index].el_onbd | ELARST);
  outb(port + ELCTRL, elvar[index].el_onbd);
  outb(port + ELCTRL, elvar[index].el_onbd | ELAEALO);

  /* Get the MAC address */
  for ( i = 0; i < LLC_ADDR_LEN; ++i )
    llcparam->llcp_macaddr[i] = inb(i + port);

  /* Check if we have proper MAC */
  if ( llcparam->llcp_macaddr[0] != EA3COM0
    || llcparam->llcp_macaddr[1] != EA3COM1
    || llcparam->llcp_macaddr[2] != EA3COM2 )
  {
    (void)printf("elinit: 3C503 board returned bad ethernet address\n");
    llcparam->llcp_state = ELB_ERROR;
  }

  outb(port + ELCTRL, elvar[index].el_onbd);
  outb(port + ELPSTR, RCV_START);
  outb(port + ELPSPR, RCV_STOP);

  /* Check IRQ */
  if ( llcparam->llcp_int <= ELMAXIRQ && llcparam->llcp_int >= ELMINIRQ )
  {
    outb(port + ELIDCFR, irq_val_tbl[4 * llcparam->llcp_int]);
    outb(port + ELGACFR, 9);
  } else {
    (void)printf("elinit: bad IRQ for 3C503 board: %d\n", llcparam->llcp_int);
    llcparam->llcp_state = ELB_ERROR;
  }

  /* Finish initialisation */
  /* SKIPPED an OUT! */
  outb(port, ELCDMA|ELCSTP);
  outb(port + ELDCR, 0x48);
  outb(port + ELTCR, 2);
  outb(port + ELRCR, 0x20);
}

/* ----- (000006DC) -------------------------------------------------------- */
void elintr(irq)
	int irq;
{
  int i;
  unsigned char t;
  short port;
  int index;
  unsigned char trnstat, isrval;
  int count;
  struct llcparam *llcparam;

  llcparam = elparams;
  count = 0;

  /* Make sure the IRQ belongs to our device */
  for ( i = el_cnt; i && llcparam->llcp_int != irq; --i )
    ++llcparam;

  if ( i ) /* It's ours */
  {
    index = llcparam->llcp_index;
    port = llcparam->llcp_port;
    outb(port + ELCTRL, elvar[index].el_onbd);
    t = inb(port);
    outb(port, t & ELPGMSK);
    isrval = inb(port + ELISR);
    outb(port + ELCNTR2, 0);
    outb(port + ELISR, isrval);

    if ( isrval & ELPRXE ) /* Get the packet */
      el_getp(&count, llcparam);

    if ( isrval & ELPTXE )
    {
      llcparam->llcp_state = ELB_WAITRCV;
      ++count;
    }

    if ( isrval & ELRXEE && inb(port + ELRCR) & ELTABT )
      ++el_stats[index].els_overflow;

    if ( isrval & ELTXEE )
    {
      trnstat = inb(port + ELTSR);
      llcparam->llcp_state = ELB_WAITRCV;

      if ( trnstat & ELTABT )
        ++el_stats[index].els_excoll;

      if ( trnstat & ELTFU )
        ++el_stats[index].els_under;

      if ( trnstat & ELTCRS )
        ++el_stats[index].els_carrier;

      if ( trnstat & 4 )
        el_stats[index].els_coll += inb(port + ELNCR);
    }

    if ( isrval & ELOVWE )
      el_NIC_reset(llcparam);

    if ( isrval & ELCNTE )
      el_gstat(llcparam);

    outb(port + ELCNTR2, 0x3B);

    if ( count )
      llc_sched(&eldevs[llcparam->llcp_firstd]);
  } else { /* Wrong IRQ arrived! */
    (void)printf("elintr: irq wrong: %x\n", irq);
  }
}

/* ----- (00000900) -------------------------------------------------------- */
void el_getp(count, llcparam)
	int *count;
	struct llcparam *llcparam;
{
  int port;
  int page_num;
  unsigned int page_number;
  unsigned int v5;
  int v6;
  unsigned *v7, *v8;
  int i, j;
  unsigned short *v10, *v11;
  int curpage;
  int v14;
  int v15;
  int v16;
  unsigned char *onboard_mem_end;
  char *v18;
  unsigned char *rcv_buf;
  struct rcv_buf *pPacket_buf;
  mblk_t *mblk;
  size_t size;
  int index;
  unsigned char crval, crvala;

  index = llcparam->llcp_index;
  port = llcparam->llcp_port;
  crval = inb(port + ELRSR);

  if ( crval & ELRPRX )
  {
    crvala = inb(port) & ELPGMSK;
    outb(port, crvala | ELCPG1);

    while ( 1 )
    {
      curpage = 0;
      /* Get the current page, if stays same, exit the loop */
      LOBYTE(curpage) = inb(port + ELCURR);

      if ( elvar[index].el_nxtpkt == curpage )
        break;

      onboard_mem_end = &(llcparam->llcp_memp[llcparam->llcp_memsize]);
      pPacket_buf = (struct rcv_buf *)&llcparam->llcp_memp[256 * (elvar[index].el_nxtpkt & 0x1F)]; /* The NIC has 0x1F * 256 onboard memory bytes */
      page_num = pPacket_buf->datalen >> 8; /* Get the page number */
      page_number = page_num + elvar[index].el_nxtpkt + 1;

      if ( page_number >= RCV_STOP )
        page_number = page_num + elvar[index].el_nxtpkt - 25; /* Page number of next expected packet - 25 */

      if ( page_number != pPacket_buf->nxtpg )
      {
        v5 = page_number + 1;

        if ( v5 >= RCV_STOP )
          v5 -= 26;

        if ( v5 != pPacket_buf->nxtpg )
        {
          elvar[index].el_nxtpkt = RCV_START;
          el_NIC_reset(llcparam);
          break;
        }
      }

      if ( pPacket_buf->status & 0xC0 )
      {
        elvar[index].el_nxtpkt = pPacket_buf->nxtpg;
        outb(port, crvala);

        if ( elvar[index].el_nxtpkt > RCV_START )
          outb(port + ELBNDY, LOBYTE(elvar[index].el_nxtpkt) - 1);
        else
          outb(port + ELBNDY, 0x3F);
        outb(port, crvala | ELCPG1);
      }
      else
      {
        v6 = pPacket_buf->nxtpg;
        elvar[index].el_nxtpkt = v6;

        if ( v6 > 63 )
          break;
        size = pPacket_buf->datalen - 4;

        if ( size >= ELMINSEND && size <= 1514 )
        {
          mblk = allocb(size, BPRI_MED); /* Allocate the streams messge block */

          if ( mblk || (mblk = allocb(2 * (size + 64), BPRI_MED)) != 0 )
          {
            rcv_buf = mblk->b_wptr;
            v18 = (char *)&pPacket_buf->pkthdr;
            mblk->b_wptr = &mblk->b_rptr[size];

            if ( (char *)&pPacket_buf->pkthdr + size >= onboard_mem_end )
            {
              v16 = onboard_mem_end - v18;
              v14 = port;
              v7 = &pPacket_buf->pkthdr;
              v8 = rcv_buf;

              for ( i = (onboard_mem_end - v18 + 1) >> 1; i; --i )
              {
                *v8 = *v7;
                ++v7;
                ++v8;
              }
              port = v14;
              size -= v16;
              v18 = llcparam->llcp_memp + 1536;
              rcv_buf += v16;
            }
            v15 = port;
            v10 = v18;
            v11 = rcv_buf;

            for ( j = (size + 1) >> 1; j; --j )
            {
              *v11 = *v10;
              ++v10;
              ++v11;
            }

            port = v15;
            ++el_stats[index].els_rpkts;
            el_stats[index].els_rbytes += size;
            llc_recv(mblk, &eldevs[llcparam->llcp_firstd]);
            outb(port, crvala);

            if ( elvar[index].el_nxtpkt > RCV_START )
              outb(port + ELBNDY, LOBYTE(elvar[index].el_nxtpkt) - 1);
            else
              outb(port + ELBNDY, 0x3F);
            outb(port, crvala | ELCPG1);
          } else {
            outb(port, crvala);

            if ( elvar[index].el_nxtpkt > RCV_START )
              outb(port + ELBNDY, LOBYTE(elvar[index].el_nxtpkt) - 1);
            else
              outb(port + ELBNDY, 0x3F);

            outb(port, crvala | ELCPG1);
            ++el_stats[index].els_llcs.llcs_nobuffer;
            ++*count;
          }
        } else {
          outb(port, crvala);

          if ( elvar[index].el_nxtpkt > RCV_START )
            outb(port + ELBNDY, LOBYTE(elvar[index].el_nxtpkt) - 1);
          else
            outb(port + ELBNDY, 0x3F);

          outb(port, crvala | ELCPG1);
          ++el_stats[index].els_short;
        }
      }
    }
    outb(port, crvala);
  } else {
    if ( crval & ELRFO )
      ++el_stats[index].els_overflow;
  }
}

/* ----- (00000D10) -------------------------------------------------------- */
void el_gstat(llcparam)
	struct llcparam *llcparam;
{
  int index;
  short port;
  int tpl;
  unsigned char t;
  int el_align;
  int el_fcs;
  int el_overflow;

  index = llcparam->llcp_index;
  port = llcparam->llcp_port;

  tpl = splhi();
  outb(port + ELCTRL, elvar[index].el_onbd);
  t = inb(port + ELCR);
  outb(port, t & ELPGMSK);
  el_align = 0;
  LOBYTE(el_align) = inb(port + ELCNTR0); /* Tally counter register 0 (Frame alignment) */
  el_stats[index].els_align += el_align;
  el_fcs = 0;
  LOBYTE(el_fcs) = inb(port + ELCNTR1); /* Tally counter register 1 (CRC) */
  el_stats[index].els_fcs += el_fcs;
  el_overflow = 0;
  LOBYTE(el_overflow) = inb(port + ELCNTR2); /* Tally counter register 2 (Missed packets) */
  el_stats[index].els_overflow += el_overflow;

  splx(tpl);
}

/* ----- (00000DC0) -------------------------------------------------------- */
int el_send(llcdev, mblk)
	struct llcdev *llcdev;
	struct msgb *mblk;
{
  unsigned short *v3, *v4;
  int i;
  unsigned int bytes_sent;
  unsigned short *v7, *v8;
  int j;
  int tpl;
  unsigned char v11;
  unsigned int wptr_rptr_diff;
  int wptr_rptr_diff2;
  short port;
  int index;
  struct llcparam *llcparam;
  caddr_t tmemp;
  struct msgb *nxt_mblk;

  llcparam = llcdev->llc_macpar;
  index = llcparam->llcp_index;
  port = llcparam->llcp_port;

  if ( ! el_canwrite(llcdev) )
    return 1;

  tmemp = llcparam->llcp_memp;
  wptr_rptr_diff = mblk->b_wptr - mblk->b_rptr;
  v3 = mblk->b_rptr;
  v4 = llcparam->llcp_memp;

  for ( i = (mblk->b_wptr - mblk->b_rptr + 1) >> 1; i; --i )
  {
    *v4 = *v3;
    ++v3;
    ++v4;
  }

  bytes_sent = wptr_rptr_diff;
  nxt_mblk = mblk->b_cont;

  do {
    wptr_rptr_diff2 = nxt_mblk->b_wptr - nxt_mblk->b_rptr;
    v7 = nxt_mblk->b_rptr;
    v8 = &tmemp[bytes_sent];

    for ( j = (nxt_mblk->b_wptr - nxt_mblk->b_rptr + 1) >> 1; j; --j )
    {
      *v8 = *v7;
      ++v7;
      ++v8;
    }

    bytes_sent += wptr_rptr_diff2;
    nxt_mblk = nxt_mblk->b_cont;
  } while ( nxt_mblk );

  if ( bytes_sent < ELMINSEND )
    bytes_sent = ELMINSEND;

  tpl = splhi();
  outb(port + ELCTRL, elvar[index].el_onbd);
  v11 = inb(port) & (ELCDMA|ELCTXP|ELCSTA|0x18) | ELCSTA;

  outb(port, v11);
  outb(port + ELTCR, 0);
  outb(port + ELTPSR, ELTFU);
  outb(port + ELTBCR0, bytes_sent);
  outb(port + ELTBCR1, BYTE1(bytes_sent));
  outb(port, v11 | ELCTXP);

  splx(tpl);

  ++el_stats[index].els_xpkts;
  el_stats[index].els_xbytes += bytes_sent;

  return 0;
}

/* ----- (00000F60) -------------------------------------------------------- */
int el_canwrite(llcdev)
	struct llcdev *llcdev;
{
  struct llcparam *llcparam;

  llcparam = llcdev->llc_macpar;

#if 0 /* Begin GARBAGE! */
  _CF = llcparam->llcp_state < (unsigned int)ELB_ERROR;
  _OF = __OFSUB__(llcparam->llcp_state, ELB_ERROR);
  _ZF = llcparam->llcp_state == ELB_ERROR;
  _SF = llcparam->llcp_state - ELB_ERROR < 0;
#endif /* End GARBAGE! */

  if ( llcparam->llcp_state == ELB_ERROR )
    return 0;

  intr_disable();

  if ( llcparam->llcp_state == ELB_XMTBUSY )
  {
    llcdev->llc_flags |= LLC_XWAIT;
    intr_restore();
    return 0;
  }

  llcparam->llcp_state = ELB_XMTBUSY;
  intr_restore();
  return 1;
}

/* ----- (00000FB0) -------------------------------------------------------- */
void el_prom(llcparam)
	struct llcparam *llcparam;
{
  short port;
  int tpl;
  unsigned char t;

  port = llcparam->llcp_port;

  /* Set promiscous if not in error */
  if ( llcparam->llcp_state != ELB_ERROR )
  {
    tpl = splhi();
    outb(port + ELCTRL, elvar[llcparam->llcp_index].el_onbd);
    t = inb(port);
    outb(port, t & ELPGMSK);

    if ( llcparam->llcp_devmode & 2 )
      outb(port + ELRCR, 0x1C);
    else
      outb(port + ELRCR, 0xC);

    splx(tpl);
  }
}

/* ----- (00001048) -------------------------------------------------------- */
void el_start_board(llcparam)
	struct llcparam *llcparam;
{
  short port;
  int tpl;

  port = llcparam->llcp_port;

  /* Check if in error */
  if ( llcparam->llcp_state == ELB_ERROR )
  {
    (void)printf("3C503 board not present or in error\n");
  }
  else
  {
    tpl = splhi();

    llcparam->llcp_state = ELB_WAITRCV;
    outb(port + ELCTRL, elvar[llcparam->llcp_index].el_onbd);
    outb(port, ELCDMA|ELCSTP);
    outb(port + ELISR, 0xFF);
    outb(port + ELIMR, 0x3B);
    /* Some INs skipped! */
    outb(port, ELCDMA);
    outb(port, ELCDMA|ELCSTA);
    outb(port + ELTCR, 0);

    if ( llcparam->llcp_devmode & 2 )
      outb(port + ELRCR, 0x1C);
    else
      outb(port + ELRCR, 0xC);

    splx(tpl);
  }
}

/* ----- (00001138) -------------------------------------------------------- */
void el_stop_board(llcparam)
	struct llcparam *llcparam;
{
  short port;
  int tpl;

  port = llcparam->llcp_port;

  tpl = splhi();
  outb(port + ELCTRL, elvar[llcparam->llcp_index].el_onbd);
  outb(port, ELCDMA|ELCSTP);
  outb(port + ELRCR, ELRMON);

  if ( llcparam->llcp_state != ELB_ERROR )
    llcparam->llcp_state = ELB_IDLE;

  splx(tpl);
}

/* ----- (000011A0) -------------------------------------------------------- */
void el_NIC_reset(llcparam)
	struct llcparam *llcparam;
{
  short port;

  port = llcparam->llcp_port;

  el_stop_board(llcparam);

  outb(port, ELCDMA|ELCSTP);
  outb(port + ELPSTART, RCV_START);
  outb(port + ELPSTOP, RCV_STOP);
  outb(port + ELBNDY, 0x3F);
  outb(port, ELCPG1|ELCDMA|ELCSTP);
  outb(port + ELISR, 0x26);

  el_start_board(llcparam);
}

struct module_info minfo = {
  0,		/* mi_idnum */
  "el",		/* mi_idname */
  0,		/* mi_minpsz */
  0xFFFF,	/* mi_maxpsz */
  ELHIWAT,	/* mi_hiwat */
  ELLOWAT	/* mi_lowat */
};

struct qinit rinit = {
  NULL,		/* qi_putp */
  llc_rsrv,	/* qi_srvp */
  el_open,	/* qi_qopen */
  (int_callback_fn_t)el_close,	/* qi_qclose */
  NULL,		/* qi_qadmin */
  &minfo,	/* qi_minfo */
  NULL		/* qi_mstat */
};

struct qinit winit = {
  llc_wput,	/* qi_putp */
  llc_wsrv,	/* qi_srvp */
  NULL,		/* qi_qopen */
  NULL,		/* qi_qclose */
  NULL,		/* qi_qadmin */
  &minfo,	/* qi_minfo */
  NULL		/* qi_mstat */
};

struct streamtab elinfo = {
  &rinit,	/* st_rdinit */
  &winit,	/* st_wrinit */
  NULL,		/* st_muxrinit */
  NULL		/* st_muxwinit */
};
