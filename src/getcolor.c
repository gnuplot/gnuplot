/* GNUPLOT - getcolor.c */

/*[
 *
 * Petr Mikulik, December 1998 -- June 1999
 * Copyright: open source as much as possible
 *
]*/
#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif

#ifdef PM3D

#include <math.h>
#include "plot.h"

extern t_sm_palette sm_palette;

double
GetColorValueFromFormula (int formula, double x)
{
/* the input gray x is supposed to be in interval [0,1] */
if (formula < 0) { /* negate the value for negative formula */
  x = 1 - x;
  formula = -formula;
  }
switch (formula) {
  case 0:  return 0;
  case 1:  return 0.5;
  case 2:  return 1;
  case 3:  /* x = x */ break;
  case 4:  x = x*x; break;
  case 5:  x = x*x*x; break;
  case 6:  x = x*x*x*x; break;
  case 7:  x = sqrt(x); break;
  case 8:  x = sqrt(sqrt(x)); break;
  case 9:  x = sin(90*x *DEG2RAD); break;
  case 10: x = cos(90*x *DEG2RAD); break;
  case 11: x = fabs(x-0.5); break;
  case 12: x = (2*x-1)*(2.0*x-1); break;
  case 13: x = sin(180*x *DEG2RAD); break;
  case 14: x = fabs(cos(180*x *DEG2RAD)); break;
  case 15: x = sin(360*x *DEG2RAD); break;
  case 16: x = cos(360*x *DEG2RAD); break;
  case 17: x = fabs( sin(360*x *DEG2RAD) ); break;
  case 18: x = fabs( cos(360*x *DEG2RAD) ); break;
  case 19: x = fabs( sin(720*x *DEG2RAD) ); break;
  case 20: x = fabs( cos(720*x *DEG2RAD) ); break;
  case 21: x = 3*x; break;
  case 22: x = 3*x-1; break;
  case 23: x = 3*x-2; break;
  case 24: x = fabs(3*x-1); break;
  case 25: x = fabs(3*x-2); break;
  case 26: x = (1.5*x-0.5); break;
  case 27: x = (1.5*x-1); break;
  case 28: x = fabs(1.5*x-0.5); break;
  case 29: x = fabs(1.5*x-1); break;
  case 30: if (x <= 0.25) return 0;
	   if (x >= 0.57) return 1;
	   x = x/0.32-0.78125; break;
  case 31: if (x <= 0.42) return 0;
	   if (x >= 0.92) return 1;
	   x = 2*x-0.84; break;
  case 32: if (x <= 0.42) x *= 4;
	     else x = (x <= 0.92) ? -2*x+1.84 : x/0.08-11.5;
	   break;
  case 33: x = fabs( 2*x - 0.5 ); break;
  case 34: x = 2*x; break;
  case 35: x = 2*x - 0.5; break;
  case 36: x = 2*x - 1; break;
  /*
  IMPORTANT: if any new formula is added here, then:
	(1) its postscript counterpart must be added into term/post.trm,
	    search for "PostScriptColorFormulae[]"
	(2) number of colours must be incremented in color.c: variable
	    sm_palette, first item---search for "t_sm_palette sm_palette = "
  */
  default: fprintf(stderr,"Fatal: undefined color formula (can be 0--%i)\n", sm_palette.colorFormulae-1);
	   exit(1);
  }
if (x <= 0) return 0;
if (x >= 1) return 1;
return x;
}

#endif
