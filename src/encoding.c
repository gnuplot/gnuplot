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
#include "encoding.h"

TBOOLEAN contains8bit(const char *s)
{
    while (*s) {
	if ((*s++ & 0x80))
	    return TRUE;
    }
    return FALSE;
}

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
