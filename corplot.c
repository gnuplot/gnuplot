#ifndef lint
static char *RCSid = "$Id: corplot.c,v 3.26 92/03/24 22:36:10 woo Exp Locker: woo $";
#endif

/* GNUPLOT - corplot.c */
/*
 * Copyright (C) 1986, 1987, 1990, 1991, 1992   Thomas Williams, Colin Kelley
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted, 
 * provided that the above copyright notice appear in all copies and 
 * that both that copyright notice and this permission notice appear 
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the modified code.  Modifications are to be distributed 
 * as patches to released version.
 *  
 * This software is provided "as is" without express or implied warranty.
 * 
 *
 * AUTHORS
 * 
 *   Original Software:
 *     Thomas Williams,  Colin Kelley.
 * 
 *   Gnuplot 2.0 additions:
 *       Russell Lang, Dave Kotz, John Campbell.
 *
 *   Gnuplot 3.0 additions:
 *       Gershon Elber and many others.
 * 
 * Send your comments or suggestions to 
 *  info-gnuplot@ames.arc.nasa.gov.
 * This is a mailing list; to join it send a note to 
 *  info-gnuplot-request@ames.arc.nasa.gov.  
 * Send bug reports to
 *  bug-gnuplot@ames.arc.nasa.gov.
 */
#include <stdio.h>
#include <process.h>
#include <dos.h>

#define BOUNDARY 32768
#define segment(addr) (FP_SEG(m) + ((FP_OFF(m)+15) >> 4));
#define round(value,boundary) (((value) + (boundary) - 1) & ~((boundary) - 1))

char *malloc(),*realloc();

char prog[] = "gnuplot";
char corscreen[] = "CORSCREEN=0";

main()
{
register unsigned int segm,start;
char *m;
	if (!(m = malloc(BOUNDARY))) {
		printf("malloc() failed\n");
		exit(1);
	}
	segm = segment(m);
	start = round(segm,BOUNDARY/16);

	if (realloc(m,BOUNDARY+(start-segm)*16) != m) {
		printf("can't realloc() memory\n");
		exit(2);
	}

	if ((segm = start >> 11) >= 8) {
		printf("not enough room in first 256K\n");
		exit(3);
	}

	corscreen[sizeof(corscreen)-2] = '0' + segm;
	if (putenv(corscreen))
		perror("putenv");

	if (spawnlp(P_WAIT,prog,prog,NULL))
		perror("spawnlp");
}
