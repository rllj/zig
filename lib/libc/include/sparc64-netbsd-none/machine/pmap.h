/*	$NetBSD: pmap.h,v 1.64 2020/09/06 10:48:21 mrg Exp $	*/

/*-
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
 * All rights reserved.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_MACHINE_PMAP_H_
#define	_MACHINE_PMAP_H_

#ifndef _LOCORE
#include <machine/pte.h>
#include <sys/queue.h>
struct vm_page;
#include <uvm/uvm_prot.h>
#include <uvm/uvm_pmap.h>
#include <uvm/uvm_object.h>
#ifdef _KERNEL
#include <machine/cpuset.h>
#ifdef SUN4V
#include <machine/hypervisor.h>
#endif
#endif
#endif

/*
 * This scheme uses 2-level page tables.
 *
 * While we're still in 32-bit mode we do the following:
 *
 *   offset:						13 bits
 * 1st level: 1024 64-bit TTEs in an 8K page for	10 bits
 * 2nd level: 512 32-bit pointers in the pmap for 	 9 bits
 *							-------
 * total:						32 bits
 *
 * In 64-bit mode the Spitfire and Blackbird CPUs support only
 * 44-bit virtual addresses.  All addresses between
 * 0x0000 07ff ffff ffff and 0xffff f800 0000 0000 are in the
 * "VA hole" and trap, so we don't have to track them.  However,
 * we do need to keep them in mind during PT walking.  If they
 * ever change the size of the address "hole" we need to rework
 * all the page table handling.
 *
 *   offset:						13 bits
 * 1st level: 1024 64-bit TTEs in an 8K page for	10 bits
 * 2nd level: 1024 64-bit pointers in an 8K page for 	10 bits
 * 3rd level: 1024 64-bit pointers in the segmap for 	10 bits
 *							-------
 * total:						43 bits
 *
 * Of course, this means for 32-bit spaces we always have a (practically)
 * wasted page for the segmap (only one entry used) and half a page wasted
 * for the page directory.  We still have need of one extra bit 8^(.
 */

#define HOLESHIFT	(43)

#define PTSZ	(PAGE_SIZE/8)			/* page table entry */
#define PDSZ	(PTSZ)				/* page directory */
#define STSZ	(PTSZ)				/* psegs */

#define PTSHIFT		(13)
#define	PDSHIFT		(10+PTSHIFT)
#define STSHIFT		(10+PDSHIFT)

#define PTMASK		(PTSZ-1)
#define PDMASK		(PDSZ-1)
#define STMASK		(STSZ-1)

#ifndef _LOCORE

#ifdef _LP64
int	sparc64_mmap_range_test(vaddr_t, vaddr_t);
#define	MD_MMAP_RANGE_TEST(MINVA, MAXVA)	sparc64_mmap_range_test(MINVA, MAXVA)
#endif

/*
 * Support for big page sizes.  This maps the page size to the
 * page bits.
 */
struct page_size_map {
	uint64_t mask;
	uint64_t code;
#if defined(DEBUG) || 1
	uint64_t use;
#endif
};
extern struct page_size_map page_size_map[];

/*
 * Pmap stuff
 */

#define va_to_seg(v)	(int)((((paddr_t)(v))>>STSHIFT)&STMASK)
#define va_to_dir(v)	(int)((((paddr_t)(v))>>PDSHIFT)&PDMASK)
#define va_to_pte(v)	(int)((((paddr_t)(v))>>PTSHIFT)&PTMASK)

#ifdef MULTIPROCESSOR
#define PMAP_LIST_MAXNUMCPU	CPUSET_MAXNUMCPU
#else
#define PMAP_LIST_MAXNUMCPU	1
#endif

struct pmap {
	unsigned int pm_refs;
	TAILQ_HEAD(, vm_page) pm_ptps;
	LIST_ENTRY(pmap) pm_list[PMAP_LIST_MAXNUMCPU];	/* per cpu ctx used list */

	struct pmap_statistics pm_stats;

	/*
	 * We record the context used on any cpu here. If the context
	 * is actually present in the TLB, it will be the plain context
	 * number. If the context is allocated, but has been flushed
	 * from the tlb, the number will be negative.
	 * If this pmap has no context allocated on that cpu, the entry
	 * will be 0.
	 */
	int pm_ctx[PMAP_LIST_MAXNUMCPU];	/* Current context per cpu */

	/*
	 * This contains 64-bit pointers to pages that contain
	 * 1024 64-bit pointers to page tables.  All addresses
	 * are physical.
	 *
	 * !!! Only touch this through pseg_get() and pseg_set() !!!
	 */
	paddr_t pm_physaddr;	/* physical address of pm_segs */
	int64_t *pm_segs;
};

/*
 * This comes from the PROM and is used to map prom entries.
 */
struct prom_map {
	uint64_t	vstart;
	uint64_t	vsize;
	uint64_t	tte;
};

#define PMAP_NC		0x001	/* Don't cache, set the E bit in the page */
#define PMAP_NVC	0x002	/* Don't enable the virtual cache */
#define PMAP_LITTLE	0x004	/* Map in little endian mode */
/* Large page size hints --
   we really should use another param to pmap_enter() */
#define PMAP_8K		0x000
#define PMAP_64K	0x008	/* Use 64K page */
#define PMAP_512K	0x010
#define PMAP_4M		0x018
#define PMAP_SZ_TO_TTE(x)	(((x)&0x018)<<58)
/* If these bits are different in va's to the same PA
   then there is an aliasing in the d$ */
#define VA_ALIAS_MASK   (1 << 13)
#define PMAP_WC		0x20	/* allow write combinimg */

#ifdef	_KERNEL
#ifdef PMAP_COUNT_DEBUG
/* diagnostic versions if PMAP_COUNT_DEBUG option is used */
int pmap_count_res(struct pmap *);
int pmap_count_wired(struct pmap *);
#define	pmap_resident_count(pm)		pmap_count_res((pm))
#define	pmap_wired_count(pm)		pmap_count_wired((pm))
#else
#define	pmap_resident_count(pm)		((pm)->pm_stats.resident_count)
#define	pmap_wired_count(pm)		((pm)->pm_stats.wired_count)
#endif

#define	pmap_phys_address(x)		(x)

void pmap_activate_pmap(struct pmap *);
void pmap_update(struct pmap *);
void pmap_bootstrap(u_long, u_long);

/* make sure all page mappings are modulo 16K to prevent d$ aliasing */
#define	PMAP_PREFER(fo, va, sz, td)	pmap_prefer((fo), (va), (td))
static inline void
pmap_prefer(vaddr_t fo, vaddr_t *va, int td)
{
	vaddr_t newva;
	vaddr_t m;

	m = 2 * PAGE_SIZE;
	newva = (*va & ~(m - 1)) | (fo & (m - 1));

	if (td) {
		if (newva > *va)
			newva -= m;
	} else {
		if (newva < *va)
			newva += m;
	}
	*va = newva;
}

#define	PMAP_GROWKERNEL         /* turn on pmap_growkernel interface */
#define PMAP_NEED_PROCWR

void pmap_procwr(struct proc *, vaddr_t, size_t);

/* SPARC specific? */
int             pmap_dumpsize(void);
int             pmap_dumpmmu(int (*)(dev_t, daddr_t, void *, size_t),
                                 daddr_t);
int		pmap_pa_exists(paddr_t);
void		switchexit(struct lwp *, int);
void		pmap_kprotect(vaddr_t, vm_prot_t);

/* SPARC64 specific */
void		pmap_copy_page_phys(paddr_t, paddr_t);
void		pmap_zero_page_phys(paddr_t);

#ifdef SUN4V
/* sun4v specific */
void		pmap_setup_intstack_sun4v(paddr_t);
void		pmap_setup_tsb_sun4v(struct tsb_desc*);
#endif

/* Installed physical memory, as discovered during bootstrap. */
extern int phys_installed_size;
extern struct mem_region *phys_installed;

#define	__HAVE_VM_PAGE_MD

/*
 * For each struct vm_page, there is a list of all currently valid virtual
 * mappings of that page.  An entry is a pv_entry_t.
 */
struct pmap;
typedef struct pv_entry {
	struct pv_entry	*pv_next;	/* next pv_entry */
	struct pmap	*pv_pmap;	/* pmap where mapping lies */
	vaddr_t		pv_va;		/* virtual address for mapping */
} *pv_entry_t;
/* PV flags encoded in the low bits of the VA of the first pv_entry */

struct vm_page_md {
	struct pv_entry mdpg_pvh;
};
#define	VM_MDPAGE_INIT(pg)						\
do {									\
	(pg)->mdpage.mdpg_pvh.pv_next = NULL;				\
	(pg)->mdpage.mdpg_pvh.pv_pmap = NULL;				\
	(pg)->mdpage.mdpg_pvh.pv_va = 0;				\
} while (/*CONSTCOND*/0)

#ifdef MULTIPROCESSOR
#define pmap_ctx_cpu(PM, C)	((PM)->pm_ctx[(C)])
#define pmap_ctx(PM)		pmap_ctx_cpu((PM), cpu_number())
#else
#define pmap_ctx(PM)		((PM)->pm_ctx[0])
#endif

#endif	/* _KERNEL */

#endif	/* _LOCORE */
#endif	/* _MACHINE_PMAP_H_ */