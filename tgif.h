/*
 * $Id: tgif.h%v 3.50 1993/07/09 05:35:24 woo Exp $
 */

/*
 * Author:	William Chia-Wei Cheng (william@cs.ucla.edu)
 *
 * Copyright (C) 1990, 1991, William Cheng.
 *
 */

#define TOOL_NAME "Tgif"

#ifndef NULL
#define NULL 0
#endif /* ~NULL */

#ifndef TRUE
#define FALSE 0
#define TRUE 1
#endif /* ~TRUE */

#define INVALID -1
#define BAD -2

#define SINGLECOLOR (FALSE)
#define MULTICOLOR (TRUE)

#define BUTTONSMASK ((Button1Mask)|(Button2Mask)|(Button3Mask))

#ifndef max
#define max(A,B) ((A) > (B) ? (A) : (B))
#define min(A,B) ((A) > (B) ? (B) : (A))
#endif /* ~max */

#ifndef round
#define round(X) (((X) >= 0) ? (int)((X)+0.5) : (int)((X)-0.5))
#endif /* ~round */

#define OFFSET_X(AbsX) (((AbsX) - drawOrigX) >> zoomScale)
#define OFFSET_Y(AbsY) (((AbsY) - drawOrigY) >> zoomScale)

#define ABS_X(OffsetX) (((OffsetX)<<zoomScale) + drawOrigX)
#define ABS_Y(OffsetY) (((OffsetY)<<zoomScale) + drawOrigY)

#define SetRecVals(R,X,Y,W,H) ((R).x=(X),(R).y=(Y),(R).width=(W),(R).height=(H))

#define MAXSTRING 256
#define MAXPATHLENGTH 256

/* object types */

#define OBJ_POLY 0
#define OBJ_BOX 1
#define OBJ_OVAL 2
#define OBJ_TEXT 3
#define OBJ_POLYGON 4
#define OBJ_GROUP 5
#define OBJ_SYM 6
#define OBJ_ICON 7
#define OBJ_ARC 8
#define OBJ_RCBOX 9
#define OBJ_XBM 10

/* drawing modes */

#define NOTHING 0
#define DRAWTEXT 1
#define DRAWBOX 2
#define DRAWCIRCLE 3
#define DRAWPOLY 4
#define DRAWPOLYGON 5
#define DRAWARC 6
#define DRAWRCBOX 7

#define MAXCHOICES 8

/* stipple patterns */

#define NONEPAT 0
#define SOLIDPAT 1
#define BACKPAT 2
#define SCROLLPAT 7
#define MAXPATTERNS 20

/* line stuff */

#define LINE_THIN 0
#define LINE_MEDIUM 1
#define LINE_THICK 2
#define LINE_CURVED 3 /* compatibility hack for fileVersion <= 3 */

#define MAXLINEWIDTHS 7

#define LT_STRAIGHT 0
#define LT_SPLINE 1

#define MAXLINETYPES 2

#define LS_PLAIN 0
#define LS_RIGHT 1
#define LS_LEFT 2
#define LS_DOUBLE 3

#define MAXLINESTYLES 4

#define MAXDASHES 5

#define NOCONT (FALSE)
#define CONT (TRUE)

#define NORB (FALSE)
#define RB (TRUE)

/* font stuff */

#define FONT_TIM 0
#define FONT_COU 1
#define FONT_HEL 2
#define FONT_CEN 3
#define FONT_SYM 4

#define MAXFONTS 5

#define STYLE_NR 0
#define STYLE_BR 1
#define STYLE_NI 2
#define STYLE_BI 3

#define MAXFONTSTYLES 4

#define FONT_DPI_75 0
#define FONT_DPI_100 1

#define MAXFONTDPIS 2

#define MAXFONTSIZES 6

#define JUST_L 0
#define JUST_C 1
#define JUST_R 2

#define MAXJUSTS 3

/* alignment */

#define ALIGN_N 0

#define ALIGN_L 1
#define ALIGN_C 2
#define ALIGN_R 3

#define ALIGN_T 1
#define ALIGN_M 2
#define ALIGN_B 3

#define MAXALIGNS 4

/* color */

#define MAXCOLORS 10

/* button stuff */

#define CONFIRM_YES 0
#define CONFIRM_NO 1
#define CONFIRM_CANCEL 2
#define MAX_CONFIRMS 3

#define BUTTON_INVERT 0
#define BUTTON_NORMAL 1

/* page layout */

#define PORTRAIT 0
#define LANDSCAPE 1
#define HIGHPORT 2
#define HIGHLAND 3
#define SLIDEPORT 4
#define SLIDELAND 5

#define MAXPAGESTYLES 6

/* where to print */

#define PRINTER 0
#define LATEX_FIG 1
#define PS_FILE 2
#define XBM_FILE 3

#define MAXWHERETOPRINT 4

/* measurement */

#define PIX_PER_INCH 128
#define ONE_INCH (PIX_PER_INCH)
#define HALF_INCH (PIX_PER_INCH/2)
#define QUARTER_INCH (PIX_PER_INCH/4)
#define EIGHTH_INCH (PIX_PER_INCH/8)

#define DEFAULT_GRID (EIGHTH_INCH)

/* text rotation */

#define ROTATE0 0
#define ROTATE90 1
#define ROTATE180 2
#define ROTATE270 3

/* arc */

#define ARC_CCW 0 /* counter-clock-wise */
#define ARC_CW 1 /* clock-wise */
