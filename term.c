/*
 *
 *    G N U P L O T  --  term.c
 *
 *  Copyright (C) 1986, 1987  Colin Kelley, Thomas Williams
 *
 *  You may use this code as you wish if credit is given and this message
 *  is retained.
 *
 *  Please e-mail any useful additions to vu-vlsi!plot so they may be
 *  included in later releases.
 *
 *  This file should be edited with 4-column tabs!  (:set ts=4 sw=4 in vi)
 */

#include <stdio.h>
#include "plot.h"

char *getenv();

extern FILE *outfile;
extern BOOLEAN term_init;
extern int term;

extern char input_line[];
extern struct lexical_unit token[];
extern struct termentry term_tbl[];

/* This is needed because the unixplot library only writes to stdout. */
FILE save_stdout;
int unixplot=0;

#define NICE_LINE		0
#define POINT_TYPES		6


do_point(x,y,number)
int x,y;
int number;
{
register int htic,vtic;
register struct termentry *t;

	number %= POINT_TYPES;
	t = &term_tbl[term];
	htic = (t->h_tic/2);	/* should be in term_tbl[] in later version */
	vtic = (t->v_tic/2);	

	if ( x < t->h_tic || y < t->v_tic || x >= t->xmax-t->h_tic ||
		y >= t->ymax-t->v_tic ) 
		return;				/* add clipping in later version maybe */

	switch(number) {
		case 0: /* do diamond */ 
				(*t->move)(x-htic,y);
				(*t->vector)(x,y-vtic);
				(*t->vector)(x+htic,y);
				(*t->vector)(x,y+vtic);
				(*t->vector)(x-htic,y);
				(*t->move)(x,y);
				(*t->vector)(x,y);
				break;
		case 1: /* do plus */ 
				(*t->move)(x-htic,y);
				(*t->vector)(x-htic,y);
				(*t->vector)(x+htic,y);
				(*t->move)(x,y-vtic);
				(*t->vector)(x,y-vtic);
				(*t->vector)(x,y+vtic);
				break;
		case 2: /* do box */ 
				(*t->move)(x-htic,y-vtic);
				(*t->vector)(x+htic,y-vtic);
				(*t->vector)(x+htic,y+vtic);
				(*t->vector)(x-htic,y+vtic);
				(*t->vector)(x-htic,y-vtic);
				(*t->move)(x,y);
				(*t->vector)(x,y);
				break;
		case 3: /* do X */ 
				(*t->move)(x-htic,y-vtic);
				(*t->vector)(x-htic,y-vtic);
				(*t->vector)(x+htic,y+vtic);
				(*t->move)(x-htic,y+vtic);
				(*t->vector)(x-htic,y+vtic);
				(*t->vector)(x+htic,y-vtic);
				break;
		case 4: /* do triangle */ 
				(*t->move)(x,y+(4*vtic/3));
				(*t->vector)(x-(4*htic/3),y-(2*vtic/3));
				(*t->vector)(x+(4*htic/3),y-(2*vtic/3));
				(*t->vector)(x,y+(4*vtic/3));
				(*t->move)(x,y);
				(*t->vector)(x,y);
				break;
		case 5: /* do star */ 
				(*t->move)(x-htic,y);
				(*t->vector)(x-htic,y);
				(*t->vector)(x+htic,y);
				(*t->move)(x,y-vtic);
				(*t->vector)(x,y-vtic);
				(*t->vector)(x,y+vtic);
				(*t->move)(x-htic,y-vtic);
				(*t->vector)(x-htic,y-vtic);
				(*t->vector)(x+htic,y+vtic);
				(*t->move)(x-htic,y+vtic);
				(*t->vector)(x-htic,y+vtic);
				(*t->vector)(x+htic,y-vtic);
				break;
	}
}


/*
 * general point routine
 */
line_and_point(x,y,number)
int x,y,number;
{
	/* temporary(?) kludge to allow terminals with bad linetypes 
		to make nice marks */

	(*term_tbl[term].linetype)(NICE_LINE);
	do_point(x,y,number);
}


#ifdef HPLJET
#define RASTER
#endif /* HPLJET */

#ifdef RASTER
/*
** General raster plotting routines.
** Raster routines written and copyrighted 1987 by
** Jyrki Yli-Nokari (jty@intrin.UUCP)
** Intrinsic, Ltd.
** 
** You may use this code for anything you like as long as
** you are not selling it and the credit is given and
** this message retained.
**
** The plotting area is defined as a huge raster.
** The raster is stored in a dynamically allocated pixel array r_p
**
** The raster is allocated (and initialized to zero) with
** r_makeraster(xsize, ysize)
** and freed with r_freeraster()
**
** Valid (unsigned) coordinates range from zero to (xsize-1,ysize-1)
**
** Plotting is done via r_move(x, y) and r_draw(x, y, value) functions,
** where the point (x,y) is the target to go from the current point
** and value is the value (of type pixel) to be stored in every pixel.
**
** Internally all plotting goes through r_setpixel(x, y, val).
** If you want different plotting styles (like OR, XOR...), use "value"
** in r_draw() to mark different styles and change r_setpixel() accordingly.
*/

#define IN(i,size)	((unsigned)i < (unsigned)size)
typedef char pixel;	/* the type of one pixel in raster */
typedef pixel *raster[];	/* the raster */

static raster *r_p;	/* global pointer to raster */
static unsigned r_currx, r_curry;	/* the current coordinates */
static unsigned r_xsize, r_ysize;	/* the size of the raster */

char *calloc();
void free();

/*
** set pixel (x, y, val) to value val (this can be 1/0 or a color number).
*/
void
r_setpixel(x, y, val)
unsigned x, y;
pixel val;
{
	if (IN(x, r_xsize) && IN(y, r_ysize)) {
		*(((*r_p)[y]) + x) = val;
	}
#ifdef RASTERDEBUG
	else {
		fprintf(stderr, "Warning: setpixel(%d, %d, %d) out of bounds\n", x, y, val);
	}
#endif
}

/*
** get pixel (x,y) value
*/
pixel
r_getpixel(x, y)
unsigned x, y;
{
	if (IN(x, r_xsize) && IN(y, r_ysize)) {
		return *(((*r_p)[y]) + x);
	} else {
#ifdef RASTERDEBUG
		fprintf(stderr, "Warning: getpixel(%d,%d) out of bounds\n", x, y);
#endif
		return 0;
	}
}

/*
** allocate the raster
*/
void
r_makeraster(x, y)
unsigned x, y;
{
	register unsigned j;
	
	/* allocate row pointers */
	if ((r_p = (raster *)calloc(y, sizeof(pixel *))) == (raster *)0) {
		fprintf(stderr, "Raster buffer allocation failure\n");
		exit(1);
	}
	for (j = 0; j < y; j++) {
		if (((*r_p)[j] = (pixel *)calloc(x, sizeof(pixel))) == (pixel *)0) {
			fprintf(stderr, "Raster buffer allocation failure\n");
			exit(1);
		}
	}
	r_xsize = x; r_ysize = y;
	r_currx = r_curry = 0;
}
	
/*
** plot a line from (x0,y0) to (x1,y1) with color val.
*/
void
r_plot(x0, y0, x1, y1, val)
unsigned x0, y0, x1, y1;
pixel val;
{
	unsigned hx, hy, i;
	int e, dx, dy;

	hx = abs((int)(x1 - x0));
	hy = abs((int)(y1 - y0));
	dx = (x1 > x0) ? 1 : -1;
	dy = (y1 > y0) ? 1 : -1;
	
	if (hx > hy) {
		/*
		** loop over x-axis
		*/
		e = hy + hy - hx;
		for (i = 0; i <= hx; i++) {
			r_setpixel(x0, y0, val);
			if (e > 0) {
				y0 += dy;
				e += hy + hy - hx - hx;
			} else {
				e += hy + hy;
			}
			x0 += dx;
		}
	} else {
		/*
		** loop over y-axis
		*/
		e = hx + hx - hy;
		for (i = 0; i <= hy; i++) {
			r_setpixel(x0, y0, val);
			if (e > 0) {
				x0 += dx;
				e += hx + hx - hy - hy;
			} else {
				e += hx + hx;
			}
			y0 += dy;
		}
	}
}

/*
** move to (x,y)
*/
void
r_move(x, y)
unsigned x, y;
{
	r_currx = x;
	r_curry = y;
}

/*
** draw to (x,y) with color val
** (move pen down)
*/
void
r_draw(x, y, val)
unsigned x, y;
pixel val;
{
	r_plot(r_currx, r_curry, x, y, val);
	r_currx = x;
	r_curry = y;
}

/*
** free the allocated raster
*/
void
r_freeraster()
{
	int y;

	for (y = 0; y < r_ysize; y++) {
		free((char *)(*r_p)[y]);
	}
	free((char *)r_p);
}
#endif /* RASTER */

#ifdef HPLJET
#include "hpljet.trm"
#endif /* HPLJET */

#ifdef PC
#include "pc.trm"
#endif /* PC */

#ifdef AED
#include "aed.trm"
#endif /* AED */

#ifdef BITGRAPH
#include "bitgraph.trm"
#endif /* BITGRAPH */

#ifdef HP26
#include "hp26.trm"
#endif /* HP26 */

#ifdef HP75
#include "hp75.trm"
#endif /* HP75 */

#ifdef IRIS4D
#include "iris4d.trm"
#endif /* IRIS4D */
 
#ifdef POSTSCRIPT
#include "postscpt.trm"
#endif /* POSTSCRIPT */


#ifdef QMS
#include "qms.trm"
#endif /* QMS */


#ifdef REGIS
#include "regis.trm"
#endif /* REGIS */


#ifdef SELANAR
#include "selanar.trm"
#endif /* SELANAR */


#ifdef TEK
#include "tek.trm"
#endif /* TEK */


#ifdef V384
#include "v384.trm"
#endif /* V384 */


#ifdef UNIXPLOT
/*
   Unixplot library writes to stdout.  A fix was put in place by
   ..!arizona!naucse!jdc to let set term and set output redirect
   stdout.  All other terminals write to outfile.
*/
#include "unixplot.trm"
#endif /* UNIXPLOT */

#ifdef UNIXPC     /* unix-PC  ATT 7300 or 3b1 machine */
#include "unixpc.trm"
#endif /* UNIXPC */


UNKNOWN_null()
{
}


/*
 * term_tbl[] contains an entry for each terminal.  "unknown" must be the
 *   first, since term is initialized to 0.
 */
struct termentry term_tbl[] = {
	{"unknown", 100, 100, 1, 1, 1, 1, UNKNOWN_null, UNKNOWN_null, UNKNOWN_null,
	UNKNOWN_null, UNKNOWN_null, UNKNOWN_null, UNKNOWN_null, UNKNOWN_null,
	UNKNOWN_null, UNKNOWN_null}
#ifdef HPLJET
	,{"laserjet1",HPLJETXMAX,HPLJETYMAX,HPLJET1VCHAR, HPLJET1HCHAR, HPLJETVTIC,
		HPLJETHTIC, HPLJET1init,HPLJETreset, HPLJETtext, HPLJETgraphics, 
		HPLJETmove, HPLJETvector,HPLJETlinetype,HPLJETlrput_text,
		HPLJETulput_text, line_and_point}
	,{"laserjet2",HPLJETXMAX,HPLJETYMAX,HPLJET2VCHAR, HPLJET2HCHAR, HPLJETVTIC, 
		HPLJETHTIC, HPLJET2init,HPLJETreset, HPLJETtext, HPLJETgraphics, 
		HPLJETmove, HPLJETvector,HPLJETlinetype,HPLJETlrput_text,
		HPLJETulput_text, line_and_point}
	,{"laserjet3",HPLJETXMAX,HPLJETYMAX,HPLJET3VCHAR, HPLJET3HCHAR, HPLJETVTIC, 
		HPLJETHTIC, HPLJET3init,HPLJETreset, HPLJETtext, HPLJETgraphics, 
		HPLJETmove, HPLJETvector,HPLJETlinetype,HPLJETlrput_text,
		HPLJETulput_text, line_and_point}
#endif

#ifdef PC
#ifdef __TURBOC__

	,{"egalib", EGALIB_XMAX, EGALIB_YMAX, EGALIB_VCHAR, EGALIB_HCHAR,
		EGALIB_VTIC, EGALIB_HTIC, EGALIB_init, EGALIB_reset,
		EGALIB_text, EGALIB_graphics, EGALIB_move, EGALIB_vector,
		EGALIB_linetype, EGALIB_lrput_text, EGALIB_ulput_text, line_and_point}

	,{"vgalib", VGA_XMAX, VGA_YMAX, VGA_VCHAR, VGA_HCHAR,
		VGA_VTIC, VGA_HTIC, VGA_init, VGA_reset,
		VGA_text, VGA_graphics, VGA_move, VGA_vector,
		VGA_linetype, VGA_lrput_text, VGA_ulput_text, line_and_point}

	,{"vgamono", VGA_XMAX, VGA_YMAX, VGA_VCHAR, VGA_HCHAR,
		VGA_VTIC, VGA_HTIC, VGA_init, VGA_reset,
		VGA_text, VGA_graphics, VGA_move, VGA_vector,
		VGAMONO_linetype, VGA_lrput_text, VGA_ulput_text, line_and_point}

	,{"mcga", MCGA_XMAX, MCGA_YMAX, MCGA_VCHAR, MCGA_HCHAR,
		MCGA_VTIC, MCGA_HTIC, MCGA_init, MCGA_reset,
		MCGA_text, MCGA_graphics, MCGA_move, MCGA_vector,
		MCGA_linetype, MCGA_lrput_text, MCGA_ulput_text, line_and_point}

	,{"cga", CGA_XMAX, CGA_YMAX, CGA_VCHAR, CGA_HCHAR,
		CGA_VTIC, CGA_HTIC, CGA_init, CGA_reset,
		CGA_text, CGA_graphics, CGA_move, CGA_vector,
		CGA_linetype, CGA_lrput_text, CGA_ulput_text, line_and_point}

	,{"hercules", HERC_XMAX, HERC_YMAX, HERC_VCHAR, HERC_HCHAR,
		HERC_VTIC, HERC_HTIC, HERC_init, HERC_reset,
		HERC_text, HERC_graphics, HERC_move, HERC_vector,
		HERC_linetype, HERC_lrput_text, HERC_ulput_text, line_and_point}

#else
	,{"cga", CGA_XMAX, CGA_YMAX, CGA_VCHAR, CGA_HCHAR,
		CGA_VTIC, CGA_HTIC, CGA_init, CGA_reset,
		CGA_text, CGA_graphics, CGA_move, CGA_vector,
		CGA_linetype, CGA_lrput_text, CGA_ulput_text, line_and_point}

	,{"egabios", EGA_XMAX, EGA_YMAX, EGA_VCHAR, EGA_HCHAR,
		EGA_VTIC, EGA_HTIC, EGA_init, EGA_reset,
		EGA_text, EGA_graphics, EGA_move, EGA_vector,
		EGA_linetype, EGA_lrput_text, EGA_ulput_text, do_point}

#ifdef EGALIB
	,{"egalib", EGALIB_XMAX, EGALIB_YMAX, EGALIB_VCHAR, EGALIB_HCHAR,
		EGALIB_VTIC, EGALIB_HTIC, EGALIB_init, EGALIB_reset,
		EGALIB_text, EGALIB_graphics, EGALIB_move, EGALIB_vector,
		EGALIB_linetype, EGALIB_lrput_text, EGALIB_ulput_text, do_point}
#endif

#ifdef HERCULES
	,{"hercules", HERC_XMAX, HERC_YMAX, HERC_VCHAR, HERC_HCHAR,
		HERC_VTIC, HERC_HTIC, HERC_init, HERC_reset,
		HERC_text, HERC_graphics, HERC_move, HERC_vector,
		HERC_linetype, HERC_lrput_text, HERC_ulput_text, line_and_point}
#endif /* HERCULES */

#ifdef ATT6300
	,{"att", ATT_XMAX, ATT_YMAX, ATT_VCHAR, ATT_HCHAR,
		ATT_VTIC, ATT_HTIC, ATT_init, ATT_reset,
		ATT_text, ATT_graphics, ATT_move, ATT_vector,
		ATT_linetype, ATT_lrput_text, ATT_ulput_text, line_and_point}
#endif

#ifdef CORONA
	,{"corona325", COR_XMAX, COR_YMAX, COR_VCHAR, COR_HCHAR,
		COR_VTIC, COR_HTIC, COR_init, COR_reset,
		COR_text, COR_graphics, COR_move, COR_vector,
		COR_linetype, COR_lrput_text, COR_ulput_text, line_and_point}
#endif /* CORONA */
#endif /* TURBOC */
#endif /* PC */

#ifdef AED
	,{"aed512", AED5_XMAX, AED_YMAX, AED_VCHAR, AED_HCHAR,
		AED_VTIC, AED_HTIC, AED_init, AED_reset, 
		AED_text, AED_graphics, AED_move, AED_vector, 
		AED_linetype, AED5_lrput_text, AED_ulput_text, do_point}
	,{"aed767", AED_XMAX, AED_YMAX, AED_VCHAR, AED_HCHAR,
		AED_VTIC, AED_HTIC, AED_init, AED_reset, 
		AED_text, AED_graphics, AED_move, AED_vector, 
		AED_linetype, AED_lrput_text, AED_ulput_text, do_point}
#endif

#ifdef UNIXPC
	,{"unixpc",uPC_XMAX,uPC_YMAX,uPC_VCHAR, uPC_HCHAR, uPC_VTIC, 
		uPC_HTIC, uPC_init,uPC_reset, uPC_text, uPC_graphics, 
		uPC_move, uPC_vector,uPC_linetype,uPC_lrput_text,
		uPC_ulput_text, line_and_point}
#endif

#ifdef BITGRAPH
	,{"bitgraph",BG_XMAX,BG_YMAX,BG_VCHAR, BG_HCHAR, BG_VTIC, 
		BG_HTIC, BG_init,BG_reset, BG_text, BG_graphics, 
		BG_move, BG_vector,BG_linetype,BG_lrput_text,
		BG_ulput_text, line_and_point}
#endif

#ifdef HP26
	,{"hp2623A",HP26_XMAX,HP26_YMAX, HP26_VCHAR, HP26_HCHAR,HP26_VTIC,HP26_HTIC,
		HP26_init,HP26_reset,HP26_text, HP26_graphics, HP26_move, HP26_vector,
		HP26_linetype, HP26_lrput_text, HP26_ulput_text, line_and_point}
#endif

#ifdef HP75
	,{"hp7580B",HP75_XMAX,HP75_YMAX, HP75_VCHAR, HP75_HCHAR,HP75_VTIC,HP75_HTIC,
		HP75_init,HP75_reset,HP75_text, HP75_graphics, HP75_move, HP75_vector,
		HP75_linetype, HP75_lrput_text, HP75_ulput_text, do_point}
#endif

#ifdef IRIS4D
	,{"iris4d", IRIS4D_XMAX, IRIS4D_YMAX, IRIS4D_VCHAR,
		IRIS4D_HCHAR, IRIS4D_VTIC, IRIS4D_HTIC,
		IRIS4D_init, IRIS4D_reset, IRIS4D_text,
		IRIS4D_graphics, IRIS4D_move, IRIS4D_vector,
		IRIS4D_linetype, IRIS4D_lrput_text, IRIS4D_ulput_text,
		do_point}
#endif

#ifdef POSTSCRIPT
	,{"postscript", PS_XMAX, PS_YMAX, PS_VCHAR, PS_HCHAR, PS_VTIC, PS_HTIC,
		PS_init, PS_reset, PS_text, PS_graphics, PS_move, PS_vector,
		PS_linetype, PS_lrput_text, PS_ulput_text, line_and_point}
#endif

#ifdef QMS
	,{"qms",QMS_XMAX,QMS_YMAX, QMS_VCHAR, QMS_HCHAR, QMS_VTIC, QMS_HTIC,
		QMS_init,QMS_reset, QMS_text, QMS_graphics, QMS_move, QMS_vector,
		QMS_linetype,QMS_lrput_text,QMS_ulput_text,line_and_point}
#endif

#ifdef REGIS
	,{"regis", REGISXMAX, REGISYMAX, REGISVCHAR, REGISHCHAR, REGISVTIC,
		REGISHTIC, REGISinit, REGISreset, REGIStext, REGISgraphics,
		REGISmove,REGISvector,REGISlinetype, REGISlrput_text, REGISulput_text,
		line_and_point}
#endif


#ifdef SELANAR
	,{"selanar",TEK40XMAX,TEK40YMAX,TEK40VCHAR, TEK40HCHAR, TEK40VTIC, 
		TEK40HTIC, SEL_init, SEL_reset, SEL_text, SEL_graphics, 
		TEK40move, TEK40vector, TEK40linetype, TEK40lrput_text,
		TEK40ulput_text, line_and_point}
#endif

#ifdef TEK
	,{"tek40xx",TEK40XMAX,TEK40YMAX,TEK40VCHAR, TEK40HCHAR, TEK40VTIC, 
		TEK40HTIC, TEK40init, TEK40reset, TEK40text, TEK40graphics, 
		TEK40move, TEK40vector, TEK40linetype, TEK40lrput_text,
		TEK40ulput_text, line_and_point}
#endif

#ifdef UNIXPLOT
	,{"unixplot", UP_XMAX, UP_YMAX, UP_VCHAR, UP_HCHAR, UP_VTIC, UP_HTIC,
		UP_init, UP_reset, UP_text, UP_graphics, UP_move, UP_vector,
		UP_linetype, UP_lrput_text, UP_ulput_text, line_and_point}
#endif

#ifdef V384
	,{"vx384", V384_XMAX, V384_YMAX, V384_VCHAR, V384_HCHAR, V384_VTIC,
		V384_HTIC, V384_init, V384_reset, V384_text, V384_graphics,
		V384_move, V384_vector, V384_linetype, V384_lrput_text,
		V384_ulput_text, do_point}
#endif
	};

#define TERMCOUNT (sizeof(term_tbl)/sizeof(struct termentry))


list_terms()
{
register int i;

	fprintf(stderr,"\navailable terminals types:\n");
	for (i = 0; i < TERMCOUNT; i++)
		fprintf(stderr,"\t%s\n",term_tbl[i].name);
	(void) putc('\n',stderr);
}


set_term(c_token)
int c_token;
{
register int i,t;

	if (!token[c_token].is_token)
		int_error("terminal name expected",c_token);
	t = -1;

	for (i = 0; i < TERMCOUNT; i++) {
   		if (!strncmp(input_line + token[c_token].start_index,term_tbl[i].name,
			token[c_token].length)) {
			if (t != -1)
				int_error("ambiguous terminal name",c_token);
			t = i;
		}
	}
	if (t == -1)
		int_error("unknown terminal type; type just 'set terminal' for a list",
			c_token);
	else if (!strncmp("unixplot",term_tbl[t].name,sizeof(unixplot))) {
		UP_redirect (2);  /* Redirect actual stdout for unixplots */
	}
	else if (unixplot) {
		UP_redirect (3);  /* Put stdout back together again. */
	}
	term_init = FALSE;
	return(t);
}


/*
   Routine to detect what terminal is being used (or do anything else
   that would be nice).  One anticipated (or allowed for) side effect
   is that the global ``term'' may be set.
*/
init()
{
char *term_name = NULL;
int t = -1, i;
#ifdef __TURBOC__
  int g_driver,g_mode;
  char far *c1,*c2;

/* Some of this code including BGI drivers is copyright Borland Intl. */
	g_driver=DETECT;
	      get_path();
        initgraph(&g_driver,&g_mode,path);
        c1=getdrivername();
        c2=getmodename(g_mode);
          switch (g_driver){
            case -2: fprintf(stderr,"Graphics card not detected.\n");
                     break;
            case -3: fprintf(stderr,"BGI driver file cannot be found.\n");
                     break;
            case -4: fprintf(stderr,"Invalid BGI driver file.\n");
                     break;
            case -5: fprintf(stderr,"Insufficient memory to load ",
                             "graphics driver.");
                     break;
            }
        closegraph();
	fprintf(stderr,"\tTC Graphics, driver %s  mode %s\n",c1,c2);
#endif
#ifdef VMS
/*
   Determine if we have a regis terminal.  If not use TERM 
   (tek40xx) as default.
*/
#include <descrip>
#include <dvidef>

extern int term;
char *term_str="tt:";
typedef struct
          {
          short cond_value;
          short count;
          int info;
          }  status_block;
typedef struct
          {
          short buffer_len;
          short item_code;
          int buffer_addr;
          int ret_len_addr;
          }  item_desc;
struct {
    item_desc dev_type;
      int terminator;
   }  dvi_list;
   int status, dev_type, zero=0;
   char buffer[255];
   $DESCRIPTOR(term_desc, term_str);

/* set up dvi item list */
	dvi_list.dev_type.buffer_len = 4;
	dvi_list.dev_type.item_code = DVI$_TT_REGIS;
	dvi_list.dev_type.buffer_addr = &dev_type;
	dvi_list.dev_type.ret_len_addr = 0;

	dvi_list.terminator = 0;

/* See what type of terminal we have. */
	status = SYS$GETDVIW (0, 0, &term_desc, &dvi_list, 0, 0, 0, 0);
	if ((status & 1) && dev_type) {
		term_name = "regis";
	}
#endif
	if (term_name != NULL) {
	/* We have a name to set! */
		for (i = 0; i < TERMCOUNT; i++) {
   			if (!strncmp("regis",term_tbl[i].name,5)) {
				t = i;
			}
		}
		if (t != -1)
			term = t;
	}
}


/*
	This is always defined so we don't have to have command.c know if it
	is there or not.
*/
#ifndef UNIXPLOT
UP_redirect(caller) int caller; {}
#else
UP_redirect (caller)
int caller;
/*
	Unixplot can't really write to outfile--it wants to write to stdout.
	This is normally ok, but the original design of gnuplot gives us
	little choice.  Originally users of unixplot had to anticipate
	their needs and redirect all I/O to a file...  Not very gnuplot-like.

	caller:  1 - called from SET OUTPUT "FOO.OUT"
			 2 - called from SET TERM UNIXPLOT
			 3 - called from SET TERM other
			 4 - called from SET OUTPUT
*/
{
	switch (caller) {
	case 1:
	/* Don't save, just replace stdout w/outfile (save was already done). */
		if (unixplot)
			*(stdout) = *(outfile);  /* Copy FILE structure */
	break;
	case 2:
		if (!unixplot) {
			fflush(stdout);
			save_stdout = *(stdout);
			*(stdout) = *(outfile);  /* Copy FILE structure */
			unixplot = 1;
		}
	break;
	case 3:
	/* New terminal in use--put stdout back to original. */
		closepl();
		fflush(stdout);
		*(stdout) = save_stdout;  /* Copy FILE structure */
		unixplot = 0;
	break;
	case 4:
	/*  User really wants to go to normal output... */
		if (unixplot) {
			fflush(stdout);
			*(stdout) = save_stdout;  /* Copy FILE structure */
		}
	break;
	}
}
#endif
