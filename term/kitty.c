/*
 * Kitty terminal support using existing PNG routines.
 * This file is #included from term.h if either the gd or cairo
 * libraries are present.
 */

#ifdef TERM_BODY

#ifndef KITTY_C
#define KITTY_C

/* Kitty graphics spec says this must be <= 4096 */
#define KITTY_BUFSIZ 4096

#define KITTY_FORMAT_RGB 24
#define KITTY_FORMAT_RGBA 32
#define KITTY_FORMAT_PNG 100

static unsigned char kitty_format;
static unsigned char *kitty_buf = NULL; /* Will be initialized to kitty_buf[KITTY_BUFSIZE] */
static unsigned char *kitty_writepos;
static TBOOLEAN kitty_init_sent;

static void
kitty_init(unsigned char format)
{
    kitty_format = format;
    if (!kitty_buf)
	kitty_buf = gp_alloc(sizeof(unsigned char) * KITTY_BUFSIZ, "kitty buffer");
    kitty_writepos = kitty_buf;
    kitty_init_sent = FALSE;
}

static int
kitty_flush(TBOOLEAN more)
{
    /* TODO: return EOF if any writes error */

    if (!kitty_init_sent) {
        fprintf(gpoutfile, "\033_Ga=T,f=%d,m=%d;", kitty_format, more);
        kitty_init_sent = TRUE;
    } else {
        fprintf(gpoutfile, "\033_Gm=%d;", more);
    }
    fwrite(kitty_buf, sizeof(unsigned char), kitty_writepos - kitty_buf, gpoutfile);
    fprintf(gpoutfile, "\033\\");
    kitty_writepos = kitty_buf;
    return 0;
}

static int
kitty_writechar(void *closure, unsigned char ch)
{
    *(kitty_writepos++) = ch;
    if (kitty_writepos - kitty_buf >= KITTY_BUFSIZ) {
        return kitty_flush(TRUE);
    }
    return 0;
}

#endif /* KITTY_C */
#endif /* TERM_BODY */
