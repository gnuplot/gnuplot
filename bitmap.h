/*
 * $Id: bitmap.h,v 3.26 92/03/24 22:34:16 woo Exp Locker: woo $
 */

/* bitmap.h */

#ifdef __TURBOC__
#define FAR far
#else
#define FAR
#endif

/* allow up to 16 bit width for character array */
typedef unsigned int char_row;
typedef char_row * FAR char_box;

#define FNT_CHARS   96      /* Number of characters in the font set */

#define FNT5X9 0
#define FNT5X9_VCHAR 11 /* vertical spacing between characters */
#define FNT5X9_VBITS 9 /* actual number of rows of bits per char */
#define FNT5X9_HCHAR 7 /* horizontal spacing between characters */
#define FNT5X9_HBITS 5 /* actual number of bits per row per char */
extern char_row FAR fnt5x9[FNT_CHARS][FNT5X9_VBITS];

#define FNT9X17 1
#define FNT9X17_VCHAR 21 /* vertical spacing between characters */
#define FNT9X17_VBITS 17 /* actual number of rows of bits per char */
#define FNT9X17_HCHAR 13 /* horizontal spacing between characters */
#define FNT9X17_HBITS 9 /* actual number of bits per row per char */
extern char_row FAR fnt9x17[FNT_CHARS][FNT9X17_VBITS];

#define FNT13X25 2
#define FNT13X25_VCHAR 31 /* vertical spacing between characters */
#define FNT13X25_VBITS 25 /* actual number of rows of bits per char */
#define FNT13X25_HCHAR 19 /* horizontal spacing between characters */
#define FNT13X25_HBITS 13 /* actual number of bits per row per char */
extern char_row FAR fnt13x25[FNT_CHARS][FNT13X25_VBITS];


typedef unsigned char pixels;  /* the type of one set of 8 pixels in bitmap */
typedef pixels *bitmap[];  /* the bitmap */

extern bitmap *b_p;						/* global pointer to bitmap */
extern unsigned int b_currx, b_curry;	/* the current coordinates */
extern unsigned int b_xsize, b_ysize;	/* the size of the bitmap */
extern unsigned int b_planes;			/* number of color planes */
extern unsigned int b_psize;			/* size of each plane */
extern unsigned int b_rastermode;		/* raster mode rotates -90deg */
extern unsigned int b_linemask;			/* 16 bit mask for dotted lines */
extern unsigned int b_value;			/* colour of lines */
extern unsigned int b_hchar;			/* width of characters */
extern unsigned int b_hbits;			/* actual bits in char horizontally */
extern unsigned int b_vchar;			/* height of characters */
extern unsigned int b_vbits;			/* actual bits in char vertically */
extern unsigned int b_angle;			/* rotation of text */
extern char_box b_font[FNT_CHARS];		/* the current font */
extern unsigned int b_pattern[];
extern int b_maskcount;
extern unsigned int b_lastx, b_lasty;	/* last pixel set - used by b_line */


extern void b_makebitmap();
extern void b_freebitmap();
extern void b_setpixel();
extern unsigned int b_getpixel();
extern void b_line();
extern void b_setmaskpixel();
extern void b_putc();
extern void b_charsize();
extern void b_setvalue();

extern int b_setlinetype();
extern int b_move();
extern int b_vector();
extern int b_put_text();
extern int b_text_angle();

