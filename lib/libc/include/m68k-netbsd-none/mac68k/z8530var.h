/*	$NetBSD: z8530var.h,v 1.13 2008/03/29 19:15:34 tsutsui Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)zsvar.h	8.1 (Berkeley) 6/11/93
 */

/*
 * Copyright (c) 1994 Gordon W. Ross
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)zsvar.h	8.1 (Berkeley) 6/11/93
 */

#include <dev/ic/z8530sc.h>

/*
 * Clock source info structure, added here so xzs_chanstate works
 */
struct zsclksrc {
	long    clk;    /* clock rate, in MHz, present on signal line */
	int     flags;  /* Specifies how this source can be used
			   (RTxC divided, RTxC BRG, PCLK BRG, TRxC divided)
			   and also if the source is "external" and if it
			   is changeable (by an ioctl ex.). The
			   source usage flags are used by the tty
			   child. The other bits tell zsloadchannelregs
			   if it should call an md signal source
			   changing routine. ZSC_VARIABLE says if
			   an ioctl should be able to cahnge the
			   clock rate.*/
};
#define ZSC_PCLK        0x01
#define ZSC_RTXBRG      0x02
#define ZSC_RTXDIV      0x04
#define ZSC_TRXDIV      0x08
#define ZSC_VARIABLE    0x40
#define ZSC_EXTERN      0x80

#define ZSC_BRG         0x03
#define ZSC_DIV         0x0c


/*
 * These are the machine-dependent (extended) variants of
 * struct zs_chanstate and struct zsc_softc
 */
struct xzs_chanstate {
	/* machine-independent part (First!)*/
	struct zs_chanstate xzs_cs;
	/* machine-dependent extensions */
	int cs_hwflags;
	int	cs_chip;		/* type of chip */
	/* Clock source info... */
	int	cs_clock_count;		/* how many signal sources available */
	struct zsclksrc cs_clocks[4];	/* info on available signal sources */
	long	cs_cclk_flag;		/* flag for current clock source */
	long	cs_pclk_flag;		/* flag for pending clock source */
	int	cs_csource;		/* current source # */
	int	cs_psource;		/* pending source # */
};

struct zsc_softc {
	device_t zsc_dev;		/* required first: base device */
	struct	zs_chanstate *zsc_cs[2];	/* channel A and B soft state */
	/* Machine-dependent part follows... */
	struct xzs_chanstate xzsc_xcs_store[2];
	void *zsc_softintr_cookie;
};

/*
 * Functions to read and write individual registers in a channel.
 * The ZS chip requires a 1.6 uSec. recovery time between accesses,
 * and the Sun3 hardware does NOT take care of this for you.
 * MacII hardware DOES dake care of the delay for us. :-)
 * XXX - Then these should be inline functions! -gwr
 * Some clock-chirped macs loose serial ports. It could be that the
 * hardware delay is tied to the CPU speed, and that the minimum delay
 * no longer's respected. For them, ZS_DELAY might help.
 * XXX - no one seems to want to try and check this -wrs
 */

uint8_t zs_read_reg(struct zs_chanstate *, uint8_t);
uint8_t zs_read_csr(struct zs_chanstate *);
uint8_t zs_read_data(struct zs_chanstate *);

void  zs_write_reg(struct zs_chanstate *, uint8_t, uint8_t);
void  zs_write_csr(struct zs_chanstate *, uint8_t);
void  zs_write_data(struct zs_chanstate *, uint8_t);

/* XXX - Could define splzs() here instead of in psl.h */
#define IPL_ZS IPL_SERIAL

/* Hook for MD ioctl support */
int	zsmdioctl(struct zs_chanstate *, u_long, void *);
/* XXX - This is a bit gross... */
#define ZS_MD_IOCTL(cs, cmd, data) zsmdioctl(cs, cmd, data)

/* Callback for "external" clock sources */
void zsmd_setclock (struct zs_chanstate *);
#define ZS_MD_SETCLK(cs) zsmd_setclock(cs)