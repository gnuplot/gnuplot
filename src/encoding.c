/* GNUPLOT - encoding.c */

/*[
* Copyright 2018   Thomas Williams, Colin Kelley
*
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

#include "syscfg.h"
#include "term_api.h"
#include "encoding.h"
#include "setshow.h"  /* init_special_chars */
#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#endif

static enum set_encoding_id map_codepage_to_encoding __PROTO((unsigned int cp));
static TBOOLEAN utf8_getmore __PROTO((unsigned long * wch, const char **str, int nbytes));


/*
 * encoding functions
 */

void
init_encoding(void)
{
    encoding = encoding_from_locale();
    if (encoding == S_ENC_INVALID)
	encoding = S_ENC_DEFAULT;
    init_special_chars();
}

enum set_encoding_id
encoding_from_locale(void)
{
    char *l = NULL;
    enum set_encoding_id encoding = S_ENC_INVALID;

#if defined(_WIN32) || defined(MSDOS)
#ifdef HAVE_LOCALE_H
    char * cp_str;

    l = setlocale(LC_CTYPE, "");
    /* preserve locale string, skip language information */
    if ((l != NULL) && (cp_str = strchr(l, '.')) != NULL) {
	unsigned cp;

	cp_str++; /* Step past the dot in, e.g., German_Germany.1252 */
	cp = strtoul(cp_str, NULL, 10);

	if (cp != 0)
	    encoding = map_codepage_to_encoding(cp);
    }
#endif
#ifdef _WIN32
    /* get encoding from currently active codepage */
    if (encoding == S_ENC_INVALID) {
#ifndef WGP_CONSOLE
	encoding = map_codepage_to_encoding(GetACP());
#else
	encoding = map_codepage_to_encoding(GetConsoleCP());
#endif
    }
#endif
#elif defined(HAVE_LOCALE_H)
    if (encoding == S_ENC_INVALID) {
	l = setlocale(LC_CTYPE, "");
	if (l && (strstr(l, "utf") || strstr(l, "UTF")))
	    encoding = S_ENC_UTF8;
	if (l && (strstr(l, "sjis") || strstr(l, "SJIS") || strstr(l, "932")))
	    encoding = S_ENC_SJIS;
	if (l && (strstr(l, "850") || strstr(l, "858")))
	    encoding = S_ENC_CP850;
	if (l && (strstr(l, "437")))
	    encoding = S_ENC_CP437;
	if (l && (strstr(l, "852")))
	    encoding = S_ENC_CP852;
	if (l && (strstr(l, "1250")))
	    encoding = S_ENC_CP1250;
	if (l && (strstr(l, "1251")))
	    encoding = S_ENC_CP1251;
	if (l && (strstr(l, "1252")))
	    encoding = S_ENC_CP1252;
	if (l && (strstr(l, "1254")))
	    encoding = S_ENC_CP1254;
	if (l && (strstr(l, "950")))
	    encoding = S_ENC_CP950;
	/* FIXME: "set encoding locale" has only limited support on non-Windows systems */
    }
#endif
    return encoding;
}


static enum set_encoding_id
map_codepage_to_encoding(unsigned int cp)
{
    enum set_encoding_id encoding;

    /* The code below is the inverse to the code found in WinGetCodepage().
       For a list of code page identifiers see
       http://msdn.microsoft.com/en-us/library/dd317756%28v=vs.85%29.aspx
    */
    switch (cp) {
    case 437:   encoding = S_ENC_CP437; break;
    case 850:
    case 858:   encoding = S_ENC_CP850; break;  /* 850 with Euro sign */
    case 852:   encoding = S_ENC_CP852; break;
    case 932:   encoding = S_ENC_SJIS; break;
    case 950:   encoding = S_ENC_CP950; break;
    case 1250:  encoding = S_ENC_CP1250; break;
    case 1251:  encoding = S_ENC_CP1251; break;
    case 1252:  encoding = S_ENC_CP1252; break;
    case 1254:  encoding = S_ENC_CP1254; break;
    case 20866: encoding = S_ENC_KOI8_R; break;
    case 21866: encoding = S_ENC_KOI8_U; break;
    case 28591: encoding = S_ENC_ISO8859_1; break;
    case 28592: encoding = S_ENC_ISO8859_2; break;
    case 28599: encoding = S_ENC_ISO8859_9; break;
    case 28605: encoding = S_ENC_ISO8859_15; break;
    case 65001: encoding = S_ENC_UTF8; break;
    default:
	encoding = S_ENC_DEFAULT;
    }
    return encoding;
}


TBOOLEAN contains8bit(const char *s)
{
    while (*s) {
	if ((*s++ & 0x80))
	    return TRUE;
    }
    return FALSE;
}


/*
 * UTF-8 functions
 */

#define INVALID_UTF8 0xfffful

/* Read from second byte to end of UTF-8 sequence.
used by utf8toulong() */
static TBOOLEAN
utf8_getmore (unsigned long * wch, const char **str, int nbytes)
{
    int i;
    unsigned char c;
    unsigned long minvalue[] = {0x80, 0x800, 0x10000, 0x200000, 0x4000000};

    for (i = 0; i < nbytes; i++) {
	c = (unsigned char) **str;

	if ((c & 0xc0) != 0x80) {
	    *wch = INVALID_UTF8;
	    return FALSE;
	}
	*wch = (*wch << 6) | (c & 0x3f);
	(*str)++;
    }

    /* check for overlong UTF-8 sequences */
    if (*wch < minvalue[nbytes-1]) {
	*wch = INVALID_UTF8;
	return FALSE;
    }
    return TRUE;
}

/* Convert UTF-8 multibyte sequence from string to unsigned long character.
Returns TRUE on success.
*/
TBOOLEAN
utf8toulong (unsigned long * wch, const char ** str)
{
    unsigned char c;

    c =  (unsigned char) *(*str)++;
    if ((c & 0x80) == 0) {
	*wch = (unsigned long) c;
	return TRUE;
    }

    if ((c & 0xe0) == 0xc0) {
	*wch = c & 0x1f;
	return utf8_getmore(wch, str, 1);
    }

    if ((c & 0xf0) == 0xe0) {
	*wch = c & 0x0f;
	return utf8_getmore(wch, str, 2);
    }

    if ((c & 0xf8) == 0xf0) {
	*wch = c & 0x07;
	return utf8_getmore(wch, str, 3);
    }

    if ((c & 0xfc) == 0xf8) {
	*wch = c & 0x03;
	return utf8_getmore(wch, str, 4);
    }

    if ((c & 0xfe) == 0xfc) {
	*wch = c & 0x01;
	return utf8_getmore(wch, str, 5);
    }

    *wch = INVALID_UTF8;
    return FALSE;
}


/*
* Convert unicode codepoint to UTF-8
* returns number of bytes in the UTF-8 representation
*/
int
ucs4toutf8(uint32_t codepoint, unsigned char *utf8char)
{
    int length = 0;

    if (codepoint <= 0x7F) {
	utf8char[0] = codepoint;
	length = 1;
    } else if (codepoint <= 0x7FF) {
	utf8char[0] = 0xC0 | (codepoint >> 6);            /* 110xxxxx */
	utf8char[1] = 0x80 | (codepoint & 0x3F);          /* 10xxxxxx */
	length = 2;
    } else if (codepoint <= 0xFFFF) {
	utf8char[0] = 0xE0 | (codepoint >> 12);           /* 1110xxxx */
	utf8char[1] = 0x80 | ((codepoint >> 6) & 0x3F);   /* 10xxxxxx */
	utf8char[2] = 0x80 | (codepoint & 0x3F);          /* 10xxxxxx */
	length = 3;
    } else if (codepoint <= 0x10FFFF) {
	utf8char[0] = 0xF0 | (codepoint >> 18);           /* 11110xxx */
	utf8char[1] = 0x80 | ((codepoint >> 12) & 0x3F);  /* 10xxxxxx */
	utf8char[2] = 0x80 | ((codepoint >> 6) & 0x3F);   /* 10xxxxxx */
	utf8char[3] = 0x80 | (codepoint & 0x3F);          /* 10xxxxxx */
	length = 4;
    }

    return length;
}

/*
* Returns number of (possibly multi-byte) characters in a UTF-8 string
*/
size_t
strlen_utf8(const char *s)
{
    int i = 0, j = 0;
    while (s[i]) {
	if ((s[i] & 0xc0) != 0x80) j++;
	i++;
    }
    return j;
}


/*
 * S-JIS functions
 */

TBOOLEAN
is_sjis_lead_byte(char c)
{
    unsigned int ch = (unsigned char) c;
    return ((ch >= 0x81) && (ch <= 0x9f)) || ((ch >= 0xe1) && (ch <= 0xee));
}


size_t
strlen_sjis(const char *s)
{
    int i = 0, j = 0;
    while (s[i]) {
	if (is_sjis_lead_byte(s[i])) i++; /* skip */
	j++;
	i++;
    }
    return j;
}
