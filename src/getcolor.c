#ifndef lint
static char *RCSid() { return RCSid("$Id: getcolor.c,v 1.10 2001/09/18 21:40:29 amai Exp $"); }
#endif

/* GNUPLOT - getcolor.c */

/*[
 *
 * Petr Mikulik, December 1998 -- June 1999
 * Copyright: open source as much as possible
 *
]*/

#include "syscfg.h"

#ifdef PM3D

#include "stdfn.h"

#include "color.h"
#include "getcolor.h"


double
GetColorValueFromFormula(int formula, double x)
{
/* the input gray x is supposed to be in interval [0,1] */
    if (formula < 0) {		/* negate the value for negative formula */
	x = 1 - x;
	formula = -formula;
    }
    switch (formula) {
    case 0:
	return 0;
    case 1:
	return 0.5;
    case 2:
	return 1;
    case 3:			/* x = x */
	break;
    case 4:
	x = x * x;
	break;
    case 5:
	x = x * x * x;
	break;
    case 6:
	x = x * x * x * x;
	break;
    case 7:
	x = sqrt(x);
	break;
    case 8:
	x = sqrt(sqrt(x));
	break;
    case 9:
	x = sin(90 * x * DEG2RAD);
	break;
    case 10:
	x = cos(90 * x * DEG2RAD);
	break;
    case 11:
	x = fabs(x - 0.5);
	break;
    case 12:
	x = (2 * x - 1) * (2.0 * x - 1);
	break;
    case 13:
	x = sin(180 * x * DEG2RAD);
	break;
    case 14:
	x = fabs(cos(180 * x * DEG2RAD));
	break;
    case 15:
	x = sin(360 * x * DEG2RAD);
	break;
    case 16:
	x = cos(360 * x * DEG2RAD);
	break;
    case 17:
	x = fabs(sin(360 * x * DEG2RAD));
	break;
    case 18:
	x = fabs(cos(360 * x * DEG2RAD));
	break;
    case 19:
	x = fabs(sin(720 * x * DEG2RAD));
	break;
    case 20:
	x = fabs(cos(720 * x * DEG2RAD));
	break;
    case 21:
	x = 3 * x;
	break;
    case 22:
	x = 3 * x - 1;
	break;
    case 23:
	x = 3 * x - 2;
	break;
    case 24:
	x = fabs(3 * x - 1);
	break;
    case 25:
	x = fabs(3 * x - 2);
	break;
    case 26:
	x = (1.5 * x - 0.5);
	break;
    case 27:
	x = (1.5 * x - 1);
	break;
    case 28:
	x = fabs(1.5 * x - 0.5);
	break;
    case 29:
	x = fabs(1.5 * x - 1);
	break;
    case 30:
	if (x <= 0.25)
	    return 0;
	if (x >= 0.57)
	    return 1;
	x = x / 0.32 - 0.78125;
	break;
    case 31:
	if (x <= 0.42)
	    return 0;
	if (x >= 0.92)
	    return 1;
	x = 2 * x - 0.84;
	break;
    case 32:
	if (x <= 0.42)
	    x *= 4;
	else
	    x = (x <= 0.92) ? -2 * x + 1.84 : x / 0.08 - 11.5;
	break;
    case 33:
	x = fabs(2 * x - 0.5);
	break;
    case 34:
	x = 2 * x;
	break;
    case 35:
	x = 2 * x - 0.5;
	break;
    case 36:
	x = 2 * x - 1;
	break;
	/*
	   IMPORTANT: if any new formula is added here, then:
	   (1) its postscript counterpart must be added into term/post.trm,
	   search for "ps_math_color_formulae[]"
	   (2) number of colours must be incremented in color.c: variable
	   sm_palette, first item---search for "t_sm_palette sm_palette = "
	 */
    default:
	fprintf(stderr, "Fatal: undefined color formula (can be 0--%i)\n", sm_palette.colorFormulae - 1);
	exit(1);
    }
    if (x <= 0)
	return 0;
    if (x >= 1)
	return 1;
    return x;
}


/* Implementation of pm3dGetColorValue() in the postscript way.
   Notice that the description, i.e. the part after %, is important
   since it is used in `show pm3d' for displaying the analytical formulae.
   The postscript formulae will be expanded into lines like:
	"/cF0 {pop 0} bind def\t% 0",
	"/cF4 {dup mul} bind def\t% x^2",
*/

const char *ps_math_color_formulae[] = {
    /* /cF0  */ "pop 0", "0",
	/* /cF1  */ "pop 0.5", "0.5",
	/* /cF2  */ "pop 1", "1",
	/* /cF3  */ " ", "x",
	/* /cF4  */ "dup mul", "x^2",
	/* /cF5  */ "dup dup mul mul", "x^3",
	/* /cF6  */ "dup mul dup mul", "x^4",
	/* /cF7  */ "sqrt", "sqrt(x)",
	/* /cF8  */ "sqrt sqrt", "sqrt(sqrt(x))",
	/* /cF9  */ "90 mul sin", "sin(90x)",
	/* /cF10 */ "90 mul cos", "cos(90x)",
	/* /cF11 */ "0.5 sub abs", "|x-0.5|",
	/* /cF12 */ "2 mul 1 sub dup mul", "(2x-1)^2",
	/* /cF13 */ "180 mul sin", "sin(180x)",
	/* /cF14 */ "180 mul cos abs", "|cos(180x)|",
	/* /cF15 */ "360 mul sin", "sin(360x)",
	/* /cF16 */ "360 mul cos", "cos(360x)",
	/* /cF17 */ "360 mul sin abs", "|sin(360x)|",
	/* /cF18 */ "360 mul cos abs", "|cos(360x)|",
	/* /cF19 */ "720 mul sin abs", "|sin(720x)|",
	/* /cF20 */ "720 mul cos abs", "|cos(720x)|",
	/* /cF21 */ "3 mul", "3x",
	/* /cF22 */ "3 mul 1 sub", "3x-1",
	/* /cF23 */ "3 mul 2 sub", "3x-2",
	/* /cF24 */ "3 mul 1 sub abs", "|3x-1|",
	/* /cF25 */ "3 mul 2 sub abs", "|3x-2|",
	/* /cF26 */ "1.5 mul .5 sub", "(3x-1)/2",
	/* /cF27 */ "1.5 mul 1 sub", "(3x-2)/2",
	/* /cF28 */ "1.5 mul .5 sub abs", "|(3x-1)/2|",
	/* /cF29 */ "1.5 mul 1 sub abs", "|(3x-2)/2|",
	/* /cF30 */ "0.32 div 0.78125 sub", "x/0.32-0.78125",
	/* /cF31 */ "2 mul 0.84 sub", "2*x-0.84",
	/* /cF32 */ "dup 0.42 le {4 mul} {dup 0.92 le {-2 mul 1.84 add} {0.08 div 11.5 sub} ifelse} ifelse",
	"4x;1;-2x+1.84;x/0.08-11.5",
	/* /cF33 */ "2 mul 0.5 sub abs", "|2*x - 0.5|",
	/* /cF34 */ "2 mul", "2*x",
	/* /cF35 */ "2 mul 0.5 sub", "2*x - 0.5",
	/* /cF36 */ "2 mul 1 sub", "2*x - 1",
"", ""};


#endif /* PM3D */

/* eof getcolor.c */
