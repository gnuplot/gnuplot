/*
   This code was originally written by kmiya@culti and obtained as a
   tar archive sixel.tar.gz dated 06-Dec-2014 from
   http://nanno.dip.jp/softlib/man/rlogin/#REGWIND

   Original license:

    このプログラム及びソースコードの使用について個人・商用を問わず
    ご自由に使用していただいで結構です。

    また、配布・転載・紹介もご連絡の必要もありません。

    ソースの改変による配布も自由ですが、どのバージョンの改変かを
    明記されることを希望します。

    バージョン情報が無い場合は、配布物の年月日を明記されることを
    希望します。

					2014/10/05	kmiya

    Translation:

     Anyone is free to use this program for any purpose,
     commercial or non-commercial, without any restriction.
    
     Anyone is free to distribute, copy, publish, or
     advertise this software, without any contact.

     Anyone is free to distribute modifications of the
     source code, but I prefer that the version it is based
     on is clearly stated.  If there is no specific version
     number please give the full date of the original.

                                        2014/10/05 kmiya

   This version includes support for high-compression mode
   (v20141201) and true color extension (v20141206), see
   also the inofficial mirror at
   https://github.com/saitoha/sixel
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <gd.h>

/* xterm sixel compatible ? */
#define USE_SIXEL_INITPAL 1
/* sixel true color extension */
/* #define USE_SIXEL_TRUECOLOR */
/* vt240 sixel compatible ? */
/* #define USE_SIXEL_VT240 */
/* simple histogram */
/* #define USE_SIXEL_HISMAP */

#define PALMAX		1024
#define HASHMAX 	8

#define	PALVAL(n,a,m)	(((n) * (a) + ((m) / 2)) / (m))

#ifdef USE_SIXEL_TRUECOLOR
#define RGBMASK 	0xFFFFFFFF
#else
#define RGBMASK 	0xFFFCFCFC
#endif

typedef unsigned char BYTE;

typedef struct _PalNode {
	struct _PalNode *next;
	int	idx;
	int	rgb;
	int	init;
} PalNode;

typedef struct _SixNode {
	struct _SixNode *next;
	int	pal;
	int	sx;
	int	mx;
	BYTE	*map;
} SixNode;

static FILE *out_fp = NULL;

static SixNode *node_top = NULL;
static SixNode *node_free = NULL;

static int save_pix = 0;
static int save_count = 0;

static int palet_max = 0;
static int palet_act = -1;
static int palet_count[PALMAX];

static int palet_hash = HASHMAX;
static PalNode *palet_top[HASHMAX];
static PalNode palet_tab[PALMAX];

static int map_width = 0;
static int map_height = 0;
static BYTE *map_buf = NULL;


/*********************************************************/


static void
PutFlash(void)
{
    int n;

#ifdef USE_SIXEL_VT240
    while ( save_count > 255 ) {
	fprintf(out_fp, "!%d%c", 255, save_pix);
	save_count -= 255;
    }
#endif

    if ( save_count > 3 ) {
	/* DECGRI Graphics Repeat Introducer ! Pn Ch */
	fprintf(out_fp, "!%d%c", save_count, save_pix);
    } else {
	for ( n = 0 ; n < save_count ; n++ )
	    fputc(save_pix, out_fp);
    }

    save_pix = 0;
    save_count = 0;
}


static void
PutPixel(int pix)
{
    if ( pix < 0 || pix > 63 )
	pix = 0;

    pix += '?';

    if ( pix == save_pix ) {
	save_count++;
    } else {
	PutFlash();
	save_pix = pix;
	save_count = 1;
    }
}


static void
PutPalet(gdImagePtr im, int idx)
{
    /* DECGCI Graphics Color Introducer # Pc ; Pu; Px; Py; Pz */

    if ( (palet_tab[idx].init & 001) == 0 ) {
 #ifdef USE_SIXEL_TRUECOLOR
	fprintf(out_fp,
		"#%d;3;%d;%d;%d", palet_tab[idx].idx,
		gdTrueColorGetRed  (palet_tab[idx].rgb), 
		gdTrueColorGetGreen(palet_tab[idx].rgb), 
		gdTrueColorGetBlue (palet_tab[idx].rgb));
#else
	fprintf(out_fp,
		"#%d;2;%d;%d;%d", palet_tab[idx].idx,
		PALVAL(gdTrueColorGetRed  (palet_tab[idx].rgb), 100, gdRedMax  ), 
		PALVAL(gdTrueColorGetGreen(palet_tab[idx].rgb), 100, gdGreenMax), 
		PALVAL(gdTrueColorGetBlue (palet_tab[idx].rgb), 100, gdBlueMax));
#endif
	palet_tab[idx].init |= 1;

    } else if ( palet_act != idx )
	fprintf(out_fp, "#%d", palet_tab[idx].idx);

    palet_act = idx;
}


static void
PutCr(void)
{
    fputs("$\n", out_fp);
    /* x = 0; */
}


static void
PutLf(void)
{
    /* DECGNL Graphics Next Line */
    fputs("-\n", out_fp);
    /* x = 0; */
    /* y += 6;*/
}


/*********************************************************/


static void
NodeFree(void)
{
    SixNode *np;

    while ( (np = node_free) != NULL ) {
	node_free = np->next;
	free(np);
    }
}


static void
NodeDel(SixNode *np)
{
    SixNode *tp;

    if ( (tp = node_top) == np )
	node_top = np->next;

    else {
	while ( tp->next != NULL ) {
	    if ( tp->next == np ) {
		tp->next = np->next;
		break;
	    }
	    tp = tp->next;
	}
    }

    np->next = node_free;
    node_free = np;
}


static void
NodeAdd(int pal, int sx, int mx, BYTE *map, int cmp)
{
    SixNode *np, *tp, top;

    if ( (np = node_free) != NULL )
	node_free = np->next;
    else if ( (np = (SixNode *)malloc(sizeof(SixNode))) == NULL )
	return;

    np->pal = pal;
    np->sx = sx;
    np->mx = mx;
    np->map = map;

    top.next = node_top;
    tp = &top;

    while ( tp->next != NULL ) {
	if ( cmp && np->pal != tp->next->pal )
	    break;
	if ( np->sx < tp->next->sx )
	    break;
	else if ( np->sx == tp->next->sx && np->mx > tp->next->mx )
	    break;
	tp = tp->next;
    }

    np->next = tp->next;
    tp->next = np;
    node_top = top.next;
}


static int
NodeLine(int pal, BYTE *map, int cmp)
{
    int sx, mx, n;
    int count = 0;

    for ( sx = 0 ; sx < map_width ; sx++ ) {
	if ( map[sx] == 0 )
	    continue;

	for ( mx = sx + 1 ; mx < map_width ; mx++ ) {
	    if ( map[mx] != 0 )
		continue;

	    for ( n = 1 ; (mx + n) < map_width ; n++ ) {
		if ( map[mx + n] != 0 )
		    break;
	    }

	    if ( n >= 10 || (mx + n) >= map_width )
		break;
	    mx = mx + n - 1;
	}

	NodeAdd(pal, sx, mx, map, cmp);
	sx = mx - 1;
	count++;
    }
    return count;
}


/*********************************************************/


static int
NodePut(gdImagePtr im, int x, SixNode *np)
{
    PutPalet(im, np->pal);

    for ( ; x < np->sx ; x++ )
	PutPixel(0);

    for ( ; x < np->mx ; x++ )
	PutPixel(np->map[x]);

    PutFlash();

    return x;
}


static void
NodeFlush(gdImagePtr im, int optFill)
{
    int n, x, idx;
    SixNode *np, *next;
    BYTE *src, *dis;

    for ( n = 0 ; n < palet_max ; n++ )
	NodeLine(n, map_buf + map_width * n, 0);

    if ( optFill ) {
	memset(palet_count, 0, sizeof(palet_count));

	for ( np = node_top ; np != NULL ; np = np->next ) {
	    for ( x = np->sx + 1 ; x < np->mx ; x++ ) {
		if ( np->map[x - 1] != np->map[x] )
		    palet_count[np->pal]++;
	    }
	}

	for ( idx = 0, n = 1 ; n < palet_max ; n++ ) {
	    if ( palet_count[idx] < palet_count[n] )
		idx = n;
	}

	dis = map_buf + map_width * idx;

	for ( np = node_top ; np != NULL ; np = next ) {
	    next = np->next;
	    if ( np->pal == idx )
		NodeDel(np);
	}

	for ( n = 0 ; n < palet_max ; n++ ) {
	    if ( n == idx )
		continue;
	    src = map_buf + map_width * n;
	    for ( x = 0 ; x < map_width ; x++ )
		dis[x] |= src[x];
	}

	NodeLine(idx, dis, 1);
    }

    for ( x = 0 ; node_top != NULL ; ) {
	if ( x > node_top->sx ) {
	    PutCr();
	    x = 0;
	}

	x = NodePut(im, x, node_top);
	NodeDel(node_top);

	for ( np = node_top ; np != NULL ; np = next ) {
	    next = np->next;
	    if ( np->sx < x )
		continue;
	    x = NodePut(im, x, np);
	    NodeDel(np);
	}
    }

    for ( n = 0 ; n < palet_max ; n++ )
	palet_tab[n].init &= ~002;

    memset(map_buf, 0, palet_max * map_width);
}


/*********************************************************/


static void
PalInit(gdImagePtr im, int max)
{
    int n, hs = 0;

    for ( n = 0 ; n < HASHMAX ; n++ )
	palet_top[n] = NULL;

    if ( max > PALMAX )
	max = PALMAX;

    for ( palet_hash = HASHMAX ; palet_hash > 1 && (max / palet_hash) < 16 ; )
	palet_hash /= 2;

    for ( n = max - 1 ; n >= 0 ; n-- ) {
	palet_tab[n].idx = n;
	palet_tab[n].init = 0;

	if ( im != NULL )
	   palet_tab[n].rgb = ((gdImageRed  (im, n) << 16) |
			       (gdImageGreen(im, n) <<  8) |
				gdImageBlue (im, n));
	else
	    palet_tab[n].rgb = 0xFF000000;

	palet_tab[n].next = palet_top[hs];
	palet_top[hs] = &(palet_tab[n]);

	if ( ++hs >= palet_hash )
	    hs = 0;
    }
}


static int
PalAdd(gdImagePtr im, int x, int y)
{
    int hs, rgb;
    PalNode *bp, *tp, tmp;

    if ( !gdImageTrueColor(im) )
	return gdImagePalettePixel(im, x, y);

    rgb = gdImageGetTrueColorPixel(im, x, y);

    if ( gdTrueColorGetAlpha(rgb) != gdAlphaOpaque )
	return -1;

    rgb &= RGBMASK;
    hs = (rgb * 31) & (palet_hash - 1);

    bp = &tmp;
    tp = bp->next = palet_top[hs];

    for ( ; ; ) {
	if ( tp->rgb == rgb )
	    goto ENDOF;

	if ( tp->next == NULL )
	    break;

	bp = tp;
	tp = tp->next;
    }

    if ( (tp->init & 002) != 0 ) {
	NodeFlush(im, 0);
	PutCr();
    }

    tp->rgb = rgb;
    tp->init = 0;

ENDOF:
    tp->init |= 002;
    bp->next = tp->next;
    tp->next = tmp.next;
    palet_top[hs] = tp;
    return tp->idx;
}


/*********************************************************/


static int
ListCountCmp(const void *src, const void *dis)
{
    return palet_count[*((int *)dis)] - palet_count[*((int *)src)];
}


static void
Histogram(gdImagePtr im, int back)
{
    int n, i, x, y, idx;
    int skip = 6;
    int list[PALMAX];

    memset(palet_count, 0, sizeof(palet_count));

#ifdef USE_SIXEL_HISMAP
    for ( y = 0 ; y < map_height ; y++ ) {
	for ( x = 0 ; x < map_width ; x++ ) {
	    idx = gdImagePalettePixel(im, x, y);
	    if ( idx != back )
		palet_count[idx]++;
	}
    }
#else
    while ( (map_height / skip) > 240 )
	skip *= 2;

    for ( y = 0 ; y < map_width ; y += skip ) {
	for ( x = 0 ; x < map_width ; x++ ) {
	    for ( i = 0 ; i < 6 && (y + i) < map_height ; i++ ) {
		idx = gdImagePalettePixel(im, x, y + i);
		if ( idx >= 0 && idx < palet_max && idx != back )
		    map_buf[idx * map_width + x] |= (1 << i);
	    }
	}

	for ( n = 0 ; n < palet_max ; n++ )
	    palet_count[n] += NodeLine(n, map_buf + map_width * n, 0);

	while ( node_top != NULL )
	    NodeDel(node_top);

	memset(map_buf, 0, palet_max * map_width);
    }
#endif

    for ( n = 0 ; n < palet_max ; n++ )
	list[n] = n;

    qsort(list, palet_max, sizeof(int), ListCountCmp);

    for ( n = 0 ; n < palet_max ; n++ )
	palet_tab[list[n]].idx = n;
}


/*********************************************************/


void
gdImageSixel(gdImagePtr im, FILE *out, int maxPalet, int optTrue, int optFill)
{
    int n, i, x, y;
    int idx;
    int back = -1;

    out_fp = out;

    map_width  = gdImageSX(im);
    map_height = gdImageSY(im);

    if ( maxPalet <= 0 )
	maxPalet = gdMaxColors;

#if !GD_MIN_VERSION(2,1,0)
	optTrue = 0;
#endif

    if ( optTrue ) {
#if GD_MIN_VERSION(2,1,0)
	if ( !gdImageTrueColor(im) )
	    gdImagePaletteToTrueColor(im);
#endif

	palet_max = maxPalet; 
	back = -1;

    } else {
	if ( maxPalet > gdMaxColors )
	    maxPalet = gdMaxColors;

	if ( !gdImageTrueColor(im) && gdImageColorsTotal(im) > maxPalet ) {
#if GD_MIN_VERSION(2,1,0)
	    int red, green, blue;
	    back = gdImageGetTransparent(im);
	    if (back >= 0) {
		red = im->red[back];
		green = im->green[back];
		blue = im->blue[back];
	    }
	    gdImageColorTransparent(im, -1);
	    gdImagePaletteToTrueColor(im);
	    if (back >= 0) {
		back = gdImageColorAllocate(im, red, green, blue);
		gdImageColorTransparent(im, back);
	    }
#else
	    fprintf(stderr, "sixelgd error:  Too many palette entries in output and your gdlib is too old. Expect incorrect output.\n");
#endif
	}

	if ( gdImageTrueColor(im) ) {
#if GD_MIN_VERSION(2,1,0)
	    /* poor ... but fast */
	    /* gdImageTrueColorToPaletteSetMethod(im, GD_QUANT_JQUANT, 0); */
	    /* debug version ? */
	    /* gdImageTrueColorToPaletteSetMethod(im, GD_QUANT_NEUQUANT, 9); */

	    /* used libimagequant/pngquant2 best !! */
	    gdImageTrueColorToPaletteSetMethod(im, GD_QUANT_LIQ, 0);
#endif
	    gdImageTrueColorToPalette(im, 1, maxPalet);
	}

	palet_max = gdImageColorsTotal(im);
	back = gdImageGetTransparent(im);
    }

    if ( (map_buf = (BYTE *)malloc(palet_max * map_width)) == NULL )
	return;

    palet_act = -1;
    memset(map_buf, 0, palet_max * map_width);

    PalInit(optTrue ? NULL : im, palet_max);

    if ( !optTrue )
	Histogram(im, back);

    fprintf(out_fp, "\033Pq\"1;1;%d;%d\n", map_width, map_height);

#ifdef USE_SIXEL_INITPAL
    if ( !optTrue ) {
	for ( n = i = 0 ; n < palet_max ; n++ ) {
	    if ( n == back )
		continue;

#ifdef USE_SIXEL_TRUECOLOR
	    fprintf(out_fp,
		"#%d;3;%d;%d;%d", palet_tab[n].idx,
		gdTrueColorGetRed  (palet_tab[n].rgb), 
		gdTrueColorGetGreen(palet_tab[n].rgb), 
		gdTrueColorGetBlue (palet_tab[n].rgb));
#else
	    fprintf(out_fp,
		"#%d;2;%d;%d;%d", palet_tab[n].idx,
		PALVAL(gdTrueColorGetRed  (palet_tab[n].rgb), 100, gdRedMax  ), 
		PALVAL(gdTrueColorGetGreen(palet_tab[n].rgb), 100, gdGreenMax), 
		PALVAL(gdTrueColorGetBlue (palet_tab[n].rgb), 100, gdBlueMax));
#endif
	    palet_tab[n].init |= 1;

	    if ( ++i > 4 ) {
		fputc('\n', out_fp);
		i = 0;
	    }
	}

	if ( i > 0 )
	    fputc('\n', out_fp);
    }
#endif

    for ( y = 0 ; y < map_height ; y += 6 ) {
	for ( x = 0 ; x < map_width ; x++ ) {
	    for ( i = 0 ; i < 6 && (y + i) < map_height ; i++ ) {
		idx = PalAdd(im, x, y + i);
		if ( idx >= 0 && idx < palet_max && idx != back )
		    map_buf[idx * map_width + x] |= (1 << i);
	    }
	}
	NodeFlush(im, optTrue ? 0 : optFill);
	PutLf();
    }

    fputs("\033\\", out_fp);

    NodeFree();
    free(map_buf);
}
