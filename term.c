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


#ifdef PC

static char near buf[80];	/* kludge since EGA.LIB is compiled SMALL */

static int pattern[] = {0xffff, 0x0f0f, 0xffff, 0xaaaa, 0x3333, 0x3f3f, 0x0f0f};

static int graphics_on = FALSE;
int startx, starty;


pause()								/* press any key to continue... */
{
	while (kbhit())
		(void) getch();				/* flush the keyboard buffer */
	while (!kbhit())
		;
}


PC_lrput_text(row,str)
int row;
char str[];
{
	PC_curloc(24-row,78-strlen(str));
	PC_puts(str);
}

PC_ulput_text(row,str)
int row;
char str[];
{
	PC_curloc(row+1,2);
	PC_puts(str);
}

PC_text()
{
	if (graphics_on) {
		graphics_on = FALSE;
		pause();
	}
	Vmode(3);
}

PC_reset()
{
}
#define CGA_XMAX 640
#define CGA_YMAX 200

#define CGA_XLAST (CGA_XMAX - 1)
#define CGA_YLAST (CGA_YMAX - 1)

#define CGA_VCHAR 8
#define CGA_HCHAR 8
#define CGA_VTIC 2
#define CGA_HTIC 3

CGA_init()
{
	PC_color(1);		/* monochrome */
}

CGA_graphics()
{
	graphics_on = TRUE;
	Vmode(6);
}

#define CGA_text PC_text

CGA_linetype(linetype)
{
	if (linetype >= 5)
		linetype %= 5;
	PC_mask(pattern[linetype+2]);
}

CGA_move(x,y)
{
	startx = x;
	starty = y;
}


CGA_vector(x,y)
{
	PC_line(startx,CGA_YLAST-starty,x,CGA_YLAST-y);
	startx = x;
	starty = y;
}

#define CGA_lrput_text PC_lrput_text
#define CGA_ulput_text PC_ulput_text


#define CGA_reset PC_reset


#define EGA_XMAX 640
#define EGA_YMAX 350

#define EGA_XLAST (EGA_XMAX - 1)
#define EGA_YLAST (EGA_YMAX - 1)

#define EGA_VCHAR 14
#define EGA_HCHAR 8
#define EGA_VTIC 5
#define EGA_HTIC 5

static int ega64color[] =  {1,1,5,4,3,5,4,3, 5, 4, 3, 5, 4, 3,5};
static int ega256color[] = {1,8,2,3,4,5,9,14,12,15,13,10,11,7,6};

static int *egacolor;


EGA_init()
{
	PC_mask(0xffff);
	egacolor = ega256color;		/* should be smarter */
}

EGA_graphics()
{
	graphics_on = TRUE;
	Vmode(16);
}

#define EGA_text PC_text

EGA_linetype(linetype)
{
	if (linetype >= 13)
		linetype %= 13;
	PC_color(egacolor[linetype+2]);
}

EGA_move(x,y)
{
	startx = x;
	starty = y;
}

EGA_vector(x,y)
{
	PC_line(startx,EGA_YLAST-starty,x,EGA_YLAST-y);
	startx = x;
	starty = y;
}

#define EGA_lrput_text PC_lrput_text
#define EGA_ulput_text PC_ulput_text


#define EGA_reset PC_reset



#ifdef EGALIB

#define EGALIB_XMAX 640
#define EGALIB_YMAX 350

#define EGALIB_XLAST (EGA_XMAX - 1)
#define EGALIB_YLAST (EGA_YMAX - 1)

#define EGALIB_VCHAR 14
#define EGALIB_HCHAR 8
#define EGALIB_VTIC 4
#define EGALIB_HTIC 5

#include "mcega.h"

EGALIB_init()
{
	GPPARMS();
	if (GDTYPE != 5) {
		term = 0;
		int_error("color EGA board not found",NO_CARET);
	}
	egacolor = (GDMEMORY < 256) ? ega64color : ega256color;
}

EGALIB_graphics()
{
	graphics_on = TRUE;
	GPINIT();
}

EGALIB_text()
{
	if (graphics_on) {
		graphics_on = FALSE;
		pause();
	}
	GPTERM();
}

EGALIB_linetype(linetype)
{
	if (linetype >= 13)
		linetype %= 13;
	GPCOLOR(egacolor[linetype+2]);
}

EGALIB_move(x,y)
{
	GPMOVE(x,GDMAXROW-y);
}


EGALIB_vector(x,y)
{
	GPLINE(x,GDMAXROW-y);
}


EGALIB_lrput_text(row,str)
int row;
char str[];
{
	strcpy((char far *)buf,str);
	GotoXY(78-strlen(str),24-row);
	gprintf(buf);
}

EGALIB_ulput_text(row,str)
int row;
char str[];
{
	strcpy((char far *)buf,str);
	GotoXY(2,row+1);
	gprintf(buf);
}

#define EGALIB_reset PC_reset

#endif /* EGALIB */


#ifdef HERCULES

#define HERC_XMAX 720
#define HERC_YMAX 348

#define HERC_XLAST (HERC_XMAX - 1)
#define HERC_YLAST (HERC_YMAX - 1)

#define HERC_VCHAR 8
#define HERC_HCHAR 8
#define HERC_VTIC 4
#define HERC_HTIC 4


HERC_init()
{
}

HERC_graphics()
{
	HVmode(1);
	graphics_on = TRUE;
}

HERC_text()
{
	if (graphics_on) {
		graphics_on = FALSE;
		pause();
	}
	HVmode(0);
}

HERC_linetype(linetype)
{
	if (linetype >= 5)
		linetype %= 5;
	H_mask(pattern[linetype+2]);
}

HERC_move(x,y)
{
	if (x < 0)
		startx = 0;
	else if (x > HERC_XLAST)
		startx = HERC_XLAST;
	else
		startx = x;

	if (y < 0)
		starty = 0;
	else if (y > HERC_YLAST)
		starty = HERC_YLAST;
	else
		starty = y;
}

HERC_vector(x,y)
{
	if (x < 0)
		x = 0;
	else if (x > HERC_XLAST)
		x = HERC_XLAST;
	if (y < 0)
		y = 0;
	else if (y > HERC_YLAST)
		y = HERC_YLAST;

	H_line(startx,HERC_YLAST-starty,x,HERC_YLAST-y);
	startx = x;
	starty = y;
}

HERC_lrput_text(row,str)
int row;
char str[];
{
	H_puts(str, 41-row, 87-strlen(str));
}

HERC_ulput_text(row,str)
int row;
char str[];
{
	H_puts(str, row+1, 2);
}

#define HERC_reset PC_reset

#endif /* HERCULES */


/* thanks to sask!macphed (Geoff Coleman and Ian Macphedran) for the
   ATT 6300 driver */ 


#ifdef ATT6300

#define ATT_XMAX 640
#define ATT_YMAX 400

#define ATT_XLAST (ATT_XMAX - 1)
#define ATT_YLAST (ATT_YMAX - 1)

#define ATT_VCHAR 8
#define ATT_HCHAR 8
#define ATT_VTIC 3
#define ATT_HTIC 3

#define ATT_init CGA_init

ATT_graphics()
{
	graphics_on = TRUE;
	Vmode(0x40);        /* 40H is the magic number for the AT&T driver */
}

#define ATT_text CGA_text

#define ATT_linetype CGA_linetype

#define ATT_move CGA_move

ATT_vector(x,y)
{
	PC_line(startx,ATT_YLAST-starty,x,ATT_YLAST-y);
	startx = x;
	starty = y;
}

#define ATT_lrput_text PC_lrput_text
#define ATT_ulput_text PC_ulput_text


#define ATT_reset CGA_reset

#endif  /* ATT6300 */


#ifdef CORONA

#define COR_XMAX 640
#define COR_YMAX 325

#define COR_XLAST (COR_XMAX - 1)
#define COR_YLAST (COR_YMAX - 1)

#define COR_VCHAR 13
#define COR_HCHAR 8
#define COR_VTIC 4
#define COR_HTIC 4


static int corscreen;		/* screen number, 0 - 7 */

COR_init()
{
register char *p;
	if (!(p = getenv("CORSCREEN")))
		int_error("must run CORPLOT for Corona graphics",NO_CARET);
	corscreen = *p - '0';
}

COR_graphics()
{
	graphics_on = TRUE;
	Vmode(3);				/* clear text screen */
	grinit(corscreen);
	grandtx();
}

COR_text()
{
	if (graphics_on) {
		graphics_on = FALSE;
		pause();
	}
	grreset();
	txonly();
	Vmode(3);
}

COR_linetype(linetype)
{
	if (linetype >= 5)
		linetype %= 5;
	Cor_mask(pattern[linetype+2]);
}

COR_move(x,y)
{
	if (x < 0)
		startx = 0;
	else if (x > COR_XLAST)
		startx = COR_XLAST;
	else
		startx = x;

	if (y < 0)
		starty = 0;
	else if (y > COR_YLAST)
		starty = COR_YLAST;
	else
		starty = y;
}

COR_vector(x,y)
{
	if (x < 0)
		x = 0;
	else if (x > COR_XLAST)
		x = COR_XLAST;
	if (y < 0)
		y = 0;
	else if (y > COR_YLAST)
		y = COR_YLAST;

	Cor_line(startx,COR_YLAST-starty,x,COR_YLAST-y);
	startx = x;
	starty = y;
}

#define COR_lrput_text PC_lrput_text
#define COR_ulput_text PC_ulput_text

#define COR_reset PC_reset

#endif /* CORONA */

#endif /* PC */



#ifdef AED

#define AED_XMAX 768
#define AED_YMAX 575

#define AED_XLAST (AED_XMAX - 1)
#define AED_YLAST (AED_YMAX - 1)

#define AED_VCHAR	13
#define AED_HCHAR	8
#define AED_VTIC	8
#define AED_HTIC	7

/* slightly different for AED 512 */
#define AED5_XMAX 512
#define AED5_XLAST (AED5_XMAX - 1)

AED_init()
{
	fprintf(outfile,
	"\033SEN3DDDN.SEC.7.SCT.0.1.80.80.90.SBC.0.AAV2.MOV.0.9.CHR.0.FFD");
/*   2            3     4                5     7    6       1
	1. Clear Screen
	2. Set Encoding
	3. Set Default Color
	4. Set Backround Color Table Entry
	5. Set Backround Color
	6. Move to Bottom Lefthand Corner
	7. Anti-Alias Vectors
*/
}


AED_graphics()
{
	fprintf(outfile,"\033FFD\033");
}


AED_text()
{
	fprintf(outfile,"\033MOV.0.9.SEC.7.XXX");
}



AED_linetype(linetype)
int linetype;
{
static int color[2+9] = { 7, 1, 6, 2, 3, 5, 1, 6, 2, 3, 5 };
static int type[2+9] = { 85, 85, 255, 255, 255, 255, 255, 85, 85, 85, 85 };

	if (linetype >= 10)
		linetype %= 10;
	fprintf(outfile,"\033SLS%d.255.",type[linetype+2]);
	fprintf(outfile,"\033SEC%d.",color[linetype+2]);
}



AED_move(x,y)
int x,y;
{
	fprintf(outfile,"\033MOV%d.%d.",x,y);
}


AED_vector(x,y)
int x,y;
{
	fprintf(outfile,"\033DVA%d.%d.",x,y);
}


AED_lrput_text(row,str) /* write text to screen while still in graphics mode */
int row;
char str[];
{
	AED_move(AED_XMAX-((strlen(str)+2)*AED_HCHAR),AED_VTIC+AED_VCHAR*(row+1));
	fprintf(outfile,"\033XXX%s\033",str);
}


AED5_lrput_text(row,str) /* same, but for AED 512 */
int row;
char str[];
{
	AED_move(AED5_XMAX-((strlen(str)+2)*AED_HCHAR),AED_VTIC+AED_VCHAR*(row+1));
	fprintf(outfile,"\033XXX%s\033",str);
}


AED_ulput_text(row,str) /* write text to screen while still in graphics mode */
int row;
char str[];
{
	AED_move(AED_HTIC*2,AED_YMAX-AED_VTIC-AED_VCHAR*(row+1));
	fprintf(outfile,"\033XXX%s\033",str);
}


#define hxt (AED_HTIC/2)
#define hyt (AED_VTIC/2)

AED_reset()
{
	fprintf(outfile,"\033SCT0.1.0.0.0.SBC.0.FFD");
}

#endif /* AED */



/* thanks to dukecdu!evs (Ed Simpson) for the BBN BitGraph driver */

#ifdef BITGRAPH

#define BG_XMAX			 	768 /* width of plot area */
#define BG_YMAX			 	768 /* height of plot area */
#define BG_SCREEN_HEIGHT	1024 /* full screen height */

#define BG_XLAST	 (BG_XMAX - 1)
#define BG_YLAST	 (BG_YMAX - 1)

#define BG_VCHAR	16
#define BG_HCHAR	 9
#define BG_VTIC		 8
#define BG_HTIC		 8	


#define BG_init TEK40init

#define BG_graphics TEK40graphics


#define BG_linetype TEK40linetype

#define BG_move TEK40move

#define BG_vector TEK40vector


BG_text()
{
	BG_move(0, BG_SCREEN_HEIGHT - 2 * BG_VCHAR);
	fprintf(outfile,"\037");
/*                   1
	1. into alphanumerics
*/
}


BG_lrput_text(row,str)
unsigned int row;
char str[];
{
	BG_move(BG_XMAX - BG_HTIC - BG_HCHAR*(strlen(str)+1),
		BG_VTIC + BG_VCHAR*(row+1));
	fprintf(outfile,"\037%s\n",str);
}


BG_ulput_text(row,str)
unsigned int row;
char str[];
{
	BG_move(BG_HTIC, BG_YMAX - BG_VTIC - BG_VCHAR*(row+1));
	fprintf(outfile,"\037%s\n",str);
}


#define BG_reset TEK40reset

#endif /* BITGRAPH */


/* thanks to hplvlch!ch (Chuck Heller) for the HP2623A driver */

#ifdef HP26

#define HP26_XMAX 512
#define HP26_YMAX 390

#define HP26_XLAST (HP26_XMAX - 1)
#define HP26_YLAST (HP26_XMAX - 1)

/* Assume a character size of 1, or a 7 x 10 grid. */
#define HP26_VCHAR	10
#define HP26_HCHAR	7
#define HP26_VTIC	(HP26_YMAX/70)		
#define HP26_HTIC	(HP26_XMAX/75)		

HP26_init()
{
	/*	The HP2623A needs no initialization. */
}


HP26_graphics()
{
	/*	Clear and enable the display */

	fputs("\033*daZ\033*dcZ",outfile);
}


HP26_text()
{
	fputs("\033*dT",outfile);	/* back to text mode */
}


HP26_linetype(linetype)
int linetype;
{
#define	SOLID	1
#define LINE4	4
#define LINE5	5
#define LINE6	6
#define LINE8	8
#define	DOTS	7
#define LINE9	9
#define LINE10	10

static int map[2+9] = {	SOLID,	/* border */
						SOLID,	/* axes */
						DOTS,	/* plot 0 */
						LINE4,	/* plot 1 */
						LINE5,	/* plot 2 */
						LINE6,	/* plot 3 */
						LINE8,	/* plot 4 */
						LINE9,	/* plot 5 */
						LINE10,	/* plot 6 */
						SOLID,	/* plot 7 */
						SOLID	/* plot 8 */ };

	if (linetype >= 9)
		linetype %= 9;
	fprintf(outfile,"\033*m%dB",map[linetype + 2]);
}


HP26_move(x,y)
int x,y;
{
	fprintf(outfile,"\033*pa%d,%dZ",x,y);
}


HP26_vector(x,y)
int x,y;
{
	fprintf(outfile,"\033*pb%d,%dZ",x,y);
}


HP26_lrput_text(row,str)
int row;
char str[];
{
	HP26_move(HP26_XMAX-HP26_HTIC*2,HP26_VTIC*2+HP26_VCHAR*row);
	HP26_move(HP26_XMAX-HP26_HTIC*2,HP26_VTIC*2+HP26_VCHAR*row);
	fputs("\033*dS",outfile);
	fprintf(outfile,"\033*m7Q\033*l%s\n",str);
	fputs("\033*dT",outfile);
}


HP26_ulput_text(row,str)
int row;
char str[];
{
	HP26_move(HP26_HTIC*2,HP26_YMAX-HP26_VTIC*2-HP26_VCHAR*row);
	fputs("\033*dS",outfile);
	fprintf(outfile,"\033*m3Q\033*l%s\n",str);
	fputs("\033*dT",outfile);
}


HP26_reset()
{
}

#endif /* HP26 */


#ifdef HP75

#define HP75_XMAX 6000
#define HP75_YMAX 6000

#define HP75_XLAST (HP75_XMAX - 1)
#define HP75_YLAST (HP75_XMAX - 1)

/* HP75_VCHAR, HP75_HCHAR  are not used */
#define HP75_VCHAR	(HP75_YMAX/20)	
#define HP75_HCHAR	(HP75_XMAX/20)		
#define HP75_VTIC	(HP75_YMAX/70)		
#define HP75_HTIC	(HP75_XMAX/75)		

HP75_init()
{
	fprintf(outfile,
	"IN;\033.P1:SC0,%d,0,%d;\n;IP;SI0.2137,0.2812;\n",
		HP75_XMAX,HP75_YMAX);
/*	 1      2  3       4             5    6  7
	1. turn on eavesdropping
	2. reset to power-up defaults
	3. enable XON/XOFF flow control
	4. set SCaling to 2000 x 2000
	5. rotate page 90 degrees
	6. ???
	7. set some character set stuff
*/
}


HP75_graphics()
{
/*         1
	fputs("\033.Y",outfile);
	1. enable eavesdropping
*/
}


HP75_text()
{
	fputs("NR;\033.Z",outfile);
/*         1  2
	1. go into 'view' mode
	2. disable plotter eavesdropping
*/
}


HP75_linetype(linetype)
int linetype;
{
	fprintf(outfile,"SP%d;\n",(linetype+3)%8);
}


HP75_move(x,y)
int x,y;
{
	fprintf(outfile,"PU%d,%d;\n",x,y);
}


HP75_vector(x,y)
int x,y;
{
	fprintf(outfile,"PD%d,%d;\n",x,y);
}


HP75_lrput_text(row,str)
int row;
char str[];
{
	HP75_move(HP75_XMAX-HP75_HTIC*2,HP75_VTIC*2+HP75_VCHAR*row);
	fprintf(outfile,"LO17;LB%s\003\n",str);
}

HP75_ulput_text(row,str)
int row;
char str[];
{
	HP75_move(HP75_HTIC*2,HP75_YMAX-HP75_VTIC*2-HP75_VCHAR*row);
	fprintf(outfile,"LO13;LB%s\003\n",str);
}

HP75_reset()
{
}

#endif /* HP75 */


/* thanks to richb@yarra.OZ (Rich Burridge) for the Postscript driver */
 
#ifdef POSTSCRIPT
  
#define PS_XMAX 540
#define PS_YMAX 720

#define PS_XLAST (PS_XMAX - 1)
#define PS_YLAST (PS_YMAX - 1)

#define PS_VCHAR (PS_YMAX/30)
#define PS_HCHAR (PS_XMAX/72)
#define PS_VTIC (PS_YMAX/80)
#define PS_HTIC (PS_XMAX/80)


PS_init()
{
  (void) fprintf(outfile,"%%!\n") ;
  (void) fprintf(outfile,"/off {36 add} def\n") ;
  (void) fprintf(outfile,"/mv {off exch off moveto} def\n") ;
  (void) fprintf(outfile,"/ln {off exch off lineto} def\n") ;
  (void) fprintf(outfile,"/Times-Roman findfont 12 scalefont setfont\n") ;
  (void) fprintf(outfile,"0.25 setlinewidth\n") ;
}


PS_graphics()
{
  (void) fprintf(outfile,"newpath\n") ;
}


PS_text()
{
  (void) fprintf(outfile,"stroke\n") ;
  (void) fprintf(outfile,"showpage\n") ;
}


PS_linetype(linetype)
int linetype ;
{
  (void) fprintf(outfile,"stroke [") ;
  switch ((linetype+2)%7)
    {
      case 0 :                                 /* solid. */
      case 2 : break ;
      case 1 :                                 /* longdashed. */
      case 6 : (void) fprintf(outfile,"9 3") ;
               break ;
      case 3 : (void) fprintf(outfile,"3") ;            /* dotted. */
               break ;
      case 4 : (void) fprintf(outfile,"6 3") ;          /* shortdashed. */
               break ;
      case 5 : (void) fprintf(outfile,"3 3 6 3") ;      /* dotdashed. */
    }
  (void) fprintf(outfile,"] 0 setdash\n") ;
}
 
 
PS_move(x,y)
unsigned int x,y ;
{
  (void) fprintf(outfile,"%1d %1d mv\n",y,x) ;
}
 
 
PS_vector(x,y)
unsigned int x,y ;
{
  (void) fprintf(outfile,"%1d %1d ln\n",y,x) ;
}
 
 
PS_lrput_text(row,str)
unsigned int row ;
char str[] ;
{
  PS_move(PS_XMAX - PS_HTIC - PS_HCHAR*(strlen(str)+1),
          PS_VTIC + PS_VCHAR*(row+1)) ;
  (void) fprintf(outfile,"(%s) show\n",str) ;
}


PS_ulput_text(row,str)
unsigned int row ;
char str[] ;
{
  PS_move(PS_HTIC, PS_YMAX - PS_VTIC - PS_VCHAR*(row+1)) ;
  (void) fprintf(outfile,"(%s) show\n",str) ;
}


PS_reset()
{
}

#endif /* POSTSCRIPT */


#ifdef QMS

#define QMS_XMAX 9000
#define QMS_YMAX 6000

#define QMS_XLAST (QMS_XMAX - 1)
#define QMS_YLAST (QMS_YMAX - 1)

#define QMS_VCHAR		120
#define QMS_HCHAR		75
#define QMS_VTIC		70
#define QMS_HTIC		70


QMS_init()
{
/* This was just ^IOL, but at Rutgers at least we need some more stuff */
  fprintf(outfile,"^PY^-\n^IOL\n^ISYNTAX00000^F^IB11000^IJ00000^IT00000\n");
}


QMS_graphics()
{
	fprintf(outfile,"^IGV\n");
}



QMS_text()
{
/* added ^-, because ^, after an ^I command doesn't actually print a page */
/* Did anybody try this code out?  [uhh...-cdk] */
	fprintf(outfile,"^IGE\n^-^,");
}


QMS_linetype(linetype)
int linetype;
{
static int width[2+9] = {7, 3, 3, 3, 3, 5, 5, 5, 7, 7, 7};
/*
 * I don't know about Villanova, but on our printer, using ^V without
 * previously setting up a pattern crashes the microcode.
 * [nope, doesn't crash here. -cdk]
 */
    static int type[2+9] =  {0, 0, 0, 2, 5, 0, 2, 5, 0, 2, 5};
	if (linetype >= 9)
		linetype %= 9;
    fprintf(outfile,"^PW%02d\n^V%x\n",width[linetype+2], type[linetype+2]); 
}


QMS_move(x,y)
int x,y;
{
	fprintf(outfile,"^U%05d:%05d\n", 1000 + x, QMS_YLAST + 1000 - y);
}


QMS_vector(x2,y2)
int x2,y2;
{
	fprintf(outfile,"^D%05d:%05d\n", 1000 + x2, QMS_YLAST + 1000 - y2);
}


QMS_lrput_text(row,str)
int row;
char str[];
{
	QMS_move(QMS_XMAX-QMS_HTIC-QMS_HCHAR*(strlen(str)+1),
		QMS_VTIC+QMS_VCHAR*(row+1));
	fprintf(outfile,"^IGE\n%s\n^IGV\n",str);
}

QMS_ulput_text(row,str)
int row;
char str[];
{
	QMS_move(QMS_HTIC*2,QMS_YMAX-QMS_VTIC-QMS_VCHAR*(row+1));
	fprintf(outfile,"^IGE\n%s\n^IGV\n",str);
}


QMS_reset()
{
/* add ^- just in case last thing was ^I command */
	fprintf(outfile,"^-^,\n");
}

#endif /* QMS */


#ifdef REGIS

#define REGISXMAX 800             
#define REGISYMAX 440

#define REGISXLAST (REGISXMAX - 1)
#define REGISYLAST (REGISYMAX - 1)

#define REGISVCHAR		20  	
#define REGISHCHAR		8		
#define REGISVTIC		8
#define REGISHTIC		6

REGISinit()
{
	fprintf(outfile,"\033[r\033[24;1H");
/*                   1     2
	1. reset scrolling region
	2. locate cursor on bottom line
*/
}


/* thanks to calmasd!dko (Dan O'Neill) for adding S(E) for vt125s */
REGISgraphics()
{
	fprintf(outfile,"\033[2J\033P1pS(C0)S(E)");
/*                   1      2      3	4
	1. clear screen
	2. enter ReGIS graphics
	3. turn off graphics diamond cursor
	4. clear graphics screen
*/
}


REGIStext()
{
	fprintf(outfile,"\033\\\033[24;1H");
/*	                   1    2
	1. Leave ReGIS graphics mode
 	2. locate cursor on last line of screen
*/
}


REGISlinetype(linetype)
int     linetype;
{
      /* This will change color in order G,R,B,G-dot,R-dot,B-dot */
static int in_map[9 + 2] = {2, 2, 3, 2, 1, 3, 2, 1, 3, 2, 1};
static int lt_map[9 + 2] = {1, 4, 1, 1, 1, 4, 4, 4, 6, 6, 6};

	if (linetype >= 9)
		linetype %= 9;
	fprintf(outfile, "W(I%d)", in_map[linetype + 2]);
	fprintf(outfile, "W(P%d)", lt_map[linetype + 2]);
}


REGISmove(x,y)
int x,y;
{
	fprintf(outfile,"P[%d,%d]",x,REGISYLAST-y,x,REGISYLAST-y);
}


REGISvector(x,y)
int x,y;
{
	fprintf(outfile,"v[]v[%d,%d]",x,REGISYLAST - y);
/* the initial v[] is needed to get the first pixel plotted */
}


REGISlrput_text(row,str)
int row;
char *str;
{
	REGISmove(REGISXMAX-REGISHTIC-REGISHCHAR*(strlen(str)+3),
		REGISVTIC+REGISVCHAR*(row+1));
	(void) putc('T',outfile); (void) putc('\'',outfile);
	while (*str) {
		(void) putc(*str,outfile);
		if (*str == '\'')
			(void) putc('\'',outfile);	/* send out another one */
		str++;
	}
	(void) putc('\'',outfile);
}


REGISulput_text(row,str)
int row;
char *str;
{
	REGISmove(REGISVTIC,REGISYMAX-REGISVTIC*2-REGISVCHAR*row);
	(void) putc('T',outfile); (void) putc('\'',outfile);
	while (*str) {
		(void) putc(*str,outfile);
		if (*str == '\'')
			(void) putc('\'',outfile);	/* send out another one */
		str++;
	}
	(void) putc('\'',outfile);
}


REGISreset()
{
	fprintf(outfile,"\033[2J\033[24;1H");
}

#endif /* REGIS */


/* thanks to sask!macphed (Geoff Coleman and Ian Macphedran) for the
   Selanar driver */

#ifdef SELANAR

SEL_init()
{
	fprintf(outfile,"\033\062");
/*					1
	1. set to ansi mode
*/
}


SEL_graphics()
{
	fprintf(outfile,"\033[H\033[J\033\061\033\014");
/*                   1           2       3
	1. clear ANSI screen
	2. set to TEK mode
	3. clear screen
*/
}


SEL_text()
{
	TEK40move(0,12);
	fprintf(outfile,"\033\062");
/*                   1
	1. into ANSI mode
*/
}

SEL_reset()
{
	fprintf(outfile,"\033\061\033\012\033\062\033[H\033[J");
/*                   1        2       3      4
1       set tek mode
2       clear screen
3       set ansi mode
4       clear screen
*/
}
#endif /* SELANAR */


#ifdef TEK

#define TEK40XMAX 1024
#define TEK40YMAX 780

#define TEK40XLAST (TEK40XMAX - 1)
#define TEK40YLAST (TEK40YMAX - 1)

#define TEK40VCHAR		25
#define TEK40HCHAR		14
#define TEK40VTIC		11
#define TEK40HTIC		11	

#define HX 0x20		/* bit pattern to OR over 5-bit data */
#define HY 0x20
#define LX 0x40
#define LY 0x60

#define LOWER5 31
#define UPPER5 (31<<5)


TEK40init()
{
}


TEK40graphics()
{
	fprintf(outfile,"\033\014");
/*                   1
	1. clear screen
*/
}


TEK40text()
{
	TEK40move(0,12);
	fprintf(outfile,"\037");
/*                   1
	1. into alphanumerics
*/
}


TEK40linetype(linetype)
int linetype;
{
}


TEK40move(x,y)
unsigned int x,y;
{
	(void) putc('\035', outfile);	/* into graphics */
	TEK40vector(x,y);
}


TEK40vector(x,y)
unsigned int x,y;
{
	(void) putc((HY | (y & UPPER5)>>5), outfile);
	(void) putc((LY | (y & LOWER5)), outfile);
	(void) putc((HX | (x & UPPER5)>>5), outfile);
	(void) putc((LX | (x & LOWER5)), outfile);
}


TEK40lrput_text(row,str)
unsigned int row;
char str[];
{
	TEK40move(TEK40XMAX - TEK40HTIC - TEK40HCHAR*(strlen(str)+1),
		TEK40VTIC + TEK40VCHAR*(row+1));
	fprintf(outfile,"\037%s\n",str);
}


TEK40ulput_text(row,str)
unsigned int row;
char str[];
{
	TEK40move(TEK40HTIC, TEK40YMAX - TEK40VTIC - TEK40VCHAR*(row+1));
	fprintf(outfile,"\037%s\n",str);
}


TEK40reset()
{
}

#endif /* TEK */


#ifdef V384

/*
 *  thanks to roland@moncskermit.OZ (Roland Yap) for this driver
 *
 *	Vectrix 384 driver - works with tandy color printer as well
 *  in reverse printing 8 color mode.
 *  This doesn't work on Vectrix 128 because it redefines the
 *  color table. It can be hacked to work on the 128 by changing
 *  the colours but then it will probably not print best. The color
 *  table is purposely designed so that it will print well
 *
 */

#define V384_XMAX 630
#define V384_YMAX 480

#define V384_XLAST (V384_XMAX - 1)
#define V384_YLAST (V384_YMAX - 1)

#define V384_VCHAR	12
#define V384_HCHAR	7
#define V384_VTIC	8
#define V384_HTIC	7


V384_init()
{
	fprintf(outfile,"%c%c  G0   \n",27,18);
	fprintf(outfile,"Q 0 8\n");
	fprintf(outfile,"0 0 0\n");
	fprintf(outfile,"255 0 0\n");
	fprintf(outfile,"0 255 0\n");
	fprintf(outfile,"0 0 255\n");
	fprintf(outfile,"0 255 255\n");
	fprintf(outfile,"255 0 255\n");
	fprintf(outfile,"255 255 0\n");
	fprintf(outfile,"255 255 255\n");
}


V384_graphics()
{
	fprintf(outfile,"%c%c E0 RE N 65535\n",27,18);
}


V384_text()
{
	fprintf(outfile,"%c%c\n",27,17);
}


V384_linetype(linetype)
int linetype;
{
static int color[]= {
		1 /* red */,
		2 /* green */,
		3 /* blue */,
		4 /* cyan */,
		5 /* magenta */,
		6 /* yellow */, /* not a good color so not in use at the moment */
		7 /* white */
	};
		
	if (linetype < 0)
		linetype=6;
	else
		linetype %= 5;
	fprintf(outfile,"C %d\n",color[linetype]);
}


V384_move(x,y)
unsigned int x,y;
{
	fprintf(outfile,"M %d %d\n",x+20,y);
}


V384_vector(x,y)
unsigned int x,y;
{
	fprintf(outfile,"L %d %d\n",x+20,y);
}


V384_lrput_text(row,str)
unsigned int row;
char str[];
{
	V384_move(V384_XMAX - V384_HTIC - V384_HCHAR*(strlen(str)+1),
		V384_VTIC + V384_VCHAR*(row+1));
	fprintf(outfile,"$%s\n",str);
}


V384_ulput_text(row,str)
unsigned int row;
char str[];
{
	V384_move(V384_HTIC, V384_YMAX - V384_VTIC - V384_VCHAR*(row+1));
	fprintf(outfile,"$%s\n",str);
}


V384_reset()
{
}

#endif /* V384 */


#ifdef UNIXPLOT

#define UP_XMAX 4096
#define UP_YMAX 4096

#define UP_XLAST (UP_XMAX - 1)
#define UP_YLAST (UP_YMAX - 1)

#define UP_VCHAR (UP_YMAX/30)
#define UP_HCHAR (UP_XMAX/72)	/* just a guess--no way to know this! */
#define UP_VTIC (UP_YMAX/80)
#define UP_HTIC (UP_XMAX/80)

UP_init()
{
	openpl();
	space(0, 0, UP_XMAX, UP_YMAX);
}


UP_graphics()
{
	erase();
}


UP_text()
{
}


UP_linetype(linetype)
int linetype;
{
static char *lt[2+5] = {"solid", "longdashed", "solid", "dotted","shortdashed",
	"dotdashed", "longdashed"};

	if (linetype >= 5)
		linetype %= 5;
	linemod(lt[linetype+2]);
}


UP_move(x,y)
unsigned int x,y;
{
	move(x,y);
}


UP_vector(x,y)
unsigned int x,y;
{
	cont(x,y);
}


UP_lrput_text(row,str)
unsigned int row;
char str[];
{
	move(UP_XMAX - UP_HTIC - UP_HCHAR*(strlen(str)+1),
		UP_VTIC + UP_VCHAR*(row+1));
	label(str);
}


UP_ulput_text(row,str)
unsigned int row;
char str[];
{
	UP_move(UP_HTIC, UP_YMAX - UP_VTIC - UP_VCHAR*(row+1));
	label(str);
}

UP_reset()
{
	closepl();
}

#endif /* UNIXPLOT */



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
#ifdef PC
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
	term_init = FALSE;
	return(t);
}
