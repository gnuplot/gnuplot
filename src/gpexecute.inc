/* GNUPLOT - gpexecute.inc */
/* vim:set ft=c: */

/*[
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the complete modified source code.  Modifications are to
 * be distributed as patches to the released version.  Permission to
 * distribute binaries produced by compiling modified sources is granted,
 * provided you
 *   1. distribute the corresponding source modifications from the
 *    released version in the form of a patch file along with the binaries,
 *   2. add special version identification to distinguish your version
 *    in addition to the base release version number,
 *   3. provide your name and address as the primary contact for the
 *    support of your modified version, and
 *   4. retain our contact information in regard to use of the base
 *    software.
 * Permission to distribute the released version of the source code along
 * with corresponding source modifications in the form of a patch file is
 * granted with same provisions 2 through 4 for binary distributions.
 *
 * This software is provided "as is" without express or implied warranty
 * to the extent permitted by applicable law.
]*/

/*
 * AUTHORS
 * 
 *   Original Software (October 1999 - January 2000):
 *     Pieter-Tjerk de Boer <ptdeboer@cs.utwente.nl>
 *     Petr Mikulik <mikulik@physics.muni.cz>
 *     Johannes Zellner <johannes@zellner.org>
 */

#ifndef GPEXECUTE_INC
#define GPEXECUTE_INC

#if defined(USE_NONBLOCKING_STDOUT) && !defined(OS2)
#include <stdlib.h>
#include <assert.h>
#include "mousecmn.h"
typedef struct gpe_fifo_t {
    struct gpe_fifo_t* prev;
    struct gp_event_t ge;
    struct gpe_fifo_t* next;
} gpe_fifo_t;
static int buffered_output_pending = 0;
#endif


#if 0
/* Structure for the ruler: on/off, position,...
*/
static struct {
   int on;
   int x, y;  /* ruler position in the viewport units */
#if 0
   int prev_x, prev_y;  /* previous ruler position */
#endif
} ruler = { 0, -1,-1 };
#endif

#if defined(USE_NONBLOCKING_STDOUT) && !defined(OS2)
static void pipe_died_handler __PROTO((int signum));
static int pipe_died = 0;
#endif

/*
 * gp_execute functions
 */

#ifdef OS2

void gp_execute (char *command);

char mouseShareMemName[40];
PVOID input_from_PM_Terminal;
  /* pointer to shared memory for storing the command to be executed */
HEV semInputReady = 0;
  /* handle to event semaphore (post an event to gnuplot that the shared 
     memory contains a command to be executed) */
int pausing = 0;
  /* avoid passing data back to gnuplot in `pause' mode */
#ifdef GNUPMDRV
  extern ULONG ppidGnu;
#else /* gplt_x11.c */
  ULONG ppidGnu = 0;
#endif


/*
 * Let the command in the shared memory be executed.
 */
void gp_post_shared_mem(void)
{
    APIRET rc;
    if (semInputReady == 0) { /* but it must be open for the first time */
	char semInputReadyName[40];
	sprintf( semInputReadyName, "\\SEM32\\GP%i_Input_Ready", (int)ppidGnu );
	DosOpenEventSem( semInputReadyName, &semInputReady);
    }
    rc = DosPostEventSem(semInputReady);
    DosSleep(10); // dirty trick: wait a little bit; otherwise problems to 
		  // distinguish mouse button down and up, for instance
		  // (info sent to shared memory was too fast; maybe a blocking
		  // semaphore would help, but no fun to implement it...)
}

/* Copy the command (given by the input string) to the shared memory
   and let gnuplot execute it.
	If this routine is called during a 'pause', then the command is
   ignored (shared memory is cleared). Needed for actions launched by a
   hotkey.
	Firstly, the command is copied from shared memory to clipboard
   if this option is set on.
	Secondly, gnuplot is informed that shared memory contains a command
   by posting semInputReady event semaphore.

   OS/2 specific: if (!s), then the command has been already sprintf'ed to
   the shared memory.
*/
void gp_execute(char *s)
{
    if (input_from_PM_Terminal==NULL)
	return;
    if (s) /* copy the command to shared memory */
      strcpy(input_from_PM_Terminal,s);
    if (((char*)input_from_PM_Terminal)[0]==0)
	return;
    if (pausing) { /* no communication during pause */
	/* DosBeep(440,111); */
	((char*)input_from_PM_Terminal)[0] = 0;
	return;
    }
#ifdef GNUPMDRV
    /* write the command to clipboard */
    if (bSend2gp == TRUE)
	TextToClipboard ( input_from_PM_Terminal );
#endif
    gp_post_shared_mem();
}

#endif

#if defined(USE_NONBLOCKING_STDOUT) && !defined(OS2)
/* TODO: Prototypes */
gpe_fifo_t*
gpe_init(void)
{
    gpe_fifo_t* base = malloc(sizeof(gpe_fifo_t));
    /* fprintf(stderr, "(gpe_init) \n"); */
    assert(base);
    base->next = (gpe_fifo_t*) 0;
    base->prev = (gpe_fifo_t*) 0;
    return base;
}
void
gpe_push(gpe_fifo_t** base, struct gp_event_t* ge)
{
    buffered_output_pending++;
    if ((*base)->prev) {
	gpe_fifo_t* new = malloc(sizeof(gpe_fifo_t));
	/* fprintf(stderr, "(gpe_push) \n"); */
	assert(new);
	(*base)->prev->next = new;
	new->prev = (*base)->prev;
	(*base)->prev = new;
	new->next = (gpe_fifo_t*) 0;
    } else {
	/* first element, this is the case, if the pipe isn't clogged */
	(*base)->next = (gpe_fifo_t*) 0; /* tail */
	(*base)->prev = (*base); /* points to itself */
    }
    (*base)->prev->ge = *ge;
}
struct gp_event_t*
gpe_front(gpe_fifo_t** base)
{
    return &((*base)->ge);
}
int
gpe_pop(gpe_fifo_t** base)
{
    buffered_output_pending--;
    if ((*base)->prev == (*base)) {
	(*base)->prev = (gpe_fifo_t*) 0;
	return 0;
    } else {
	gpe_fifo_t* save = *base;
	/* fprintf(stderr, "(gpe_pop) \n"); */
	(*base)->next->prev = (*base)->prev;
	(*base) = (*base)->next;
	free(save);
	return 1;
    }
}
#endif

void
gp_exec_event ( char type, int mx, int my, int par1, int par2 )
{
    struct gp_event_t ge;
#if defined(USE_NONBLOCKING_STDOUT) && !defined(OS2)
    static struct gpe_fifo_t* base = (gpe_fifo_t*) 0;
#endif
#if 0
    char s[127];
    sprintf(s,"%%%c %d %d %d %d", type, mx, my, par1, par2);
    gp_execute(s);
#endif
    ge.type = type;
    ge.mx = mx;
    ge.my = my;
    ge.par1 = par1;
    ge.par2 = par2;
#ifndef OS2
    if (pipe_died)
	return;
#ifndef USE_NONBLOCKING_STDOUT
    write(1, &ge, sizeof(ge));
#else
    if (!base) {
	base = gpe_init();
    }
    if (GE_pending != type) {
	gpe_push(&base, &ge);
    } else if (!buffered_output_pending) {
	return;
    }
    do {
	int status = write(1, gpe_front(&base), sizeof(ge));
	if (-1 == status) {
	    switch (errno) {
		case EAGAIN:
		    /* do nothing */
#if 0
		    fprintf(stderr, "(gp_exec_event) EAGAIN\n");
#endif
		    break;
		default:
#if 0
		    fprintf(stderr, "(gp_exec_event) errno = %d\n", errno);
#endif
		    break;
	    }
	    break;
	}
    } while (gpe_pop(&base));

#endif /* USE_NONBLOCKING_STDOUT */

#else /* OS/2 communication via shared memory; coded according to gp_execute() */
    if (input_from_PM_Terminal==NULL)
	return;
    ((char*)input_from_PM_Terminal)[0] = '%'; /* flag that passing gp_event_t */
    memcpy(((char*)input_from_PM_Terminal)+1, &ge, sizeof(ge)); /* copy the command to shared memory */
    if (pausing) { /* no communication during pause */
	/* DosBeep(440,111); */
	((char*)input_from_PM_Terminal)[0] = 0;
	return;
    }
    gp_post_shared_mem();
#endif
}

#if 0
/*
 The following has been proposed for GE_cmd event with an optional text 
 parameter. This is not used at the moment; OS/2 PM terminal issues text 
 commands (which do not need mouse coords) using gp_execute(char*).
 */
void
gp_exec_event_text ( char type, int mx, int my, int par1, int par2, char *text )
{
}
#endif

#if defined(USE_NONBLOCKING_STDOUT) && !defined(OS2)
static RETSIGTYPE
pipe_died_handler(int signum)
{
    /* fprintf(stderr, "\n*******(pipe_died_handler)*******\n"); */
    close(1);
    pipe_died = 1;
}
#endif



#endif
