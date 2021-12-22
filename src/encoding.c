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
#include "util.h"

#ifdef HAVE_ICONV
#include <iconv.h>
#endif
#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#endif

#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#endif

#ifdef OS2
# define INCL_DOSNLS
# include <os2.h>
#endif

#if defined(_WIN32) || defined(MSDOS) || defined(OS2)
static enum set_encoding_id map_codepage_to_encoding(unsigned int cp);
#endif
static const char * encoding_micro(void);
static const char * encoding_minus(void);
static void set_degreesign(char *);
static TBOOLEAN utf8_getmore(unsigned long * wch, const char **str, int nbytes);


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

#if defined(_WIN32) || defined(MSDOS) || defined(OS2)
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
#ifdef OS2
    if (encoding == S_ENC_INVALID) {
	ULONG  cplist[4];
	ULONG  listsize = sizeof(cplist);
	ULONG  count;
	APIRET rc;

	rc = DosQueryCp(listsize, cplist, &count);
	if (rc == 0 && count > 0)
	    encoding = map_codepage_to_encoding(cplist[0]);
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


void
init_special_chars(void)
{
    /* Set degree sign to match encoding */
    char * l = NULL;
#ifdef HAVE_LOCALE_H
    l = setlocale(LC_CTYPE, "");
#endif
    set_degreesign(l);

    /* Set minus sign to match encoding */
    minus_sign = encoding_minus();

    /* Set micro character to match encoding */
    micro = encoding_micro();
}


/* Encoding-specific character enabled by "set micro" */
static const char *
encoding_micro()
{
    static const char micro_utf8[4] = {0xC2, 0xB5, 0x0, 0x0};
    static const char micro_437[2] = {0xE6, 0x0};
    static const char micro_latin1[2] = {0xB5, 0x0};
    static const char micro_default[2] = {'u', 0x0};
    switch (encoding) {
	case S_ENC_UTF8:	return micro_utf8;
	case S_ENC_CP1250:
	case S_ENC_CP1251:
	case S_ENC_CP1252:
	case S_ENC_CP1254:
	case S_ENC_ISO8859_1:
	case S_ENC_ISO8859_9:
	case S_ENC_ISO8859_15:	return micro_latin1;
	case S_ENC_CP437:
	case S_ENC_CP850:	return micro_437;
	default:		return micro_default;
    }
}


/* Encoding-specific character enabled by "set minussign" */
static const char *
encoding_minus()
{
    static const char minus_utf8[4] = {0xE2, 0x88, 0x92, 0x0};
    static const char minus_1252[2] = {0x96, 0x0};
    /* NB: This SJIS character is correct, but produces bad spacing if used	*/
    /*     static const char minus_sjis[4] = {0x81, 0x7c, 0x0, 0x0};		*/
    switch (encoding) {
	case S_ENC_UTF8:	return minus_utf8;
	case S_ENC_CP1252:	return minus_1252;
	case S_ENC_SJIS:
	default:		return NULL;
    }
}


static void
set_degreesign(char *locale)
{
#if defined(HAVE_ICONV) && !(defined _WIN32)
    char degree_utf8[3] = {'\302', '\260', '\0'};
    size_t lengthin = 3;
    size_t lengthout = 8;
    char *in = degree_utf8;
    char *out = degree_sign;
    iconv_t cd;

    if (locale) {
	/* This should work even if gnuplot doesn't understand the encoding */
#ifdef HAVE_LANGINFO_H
	char *cencoding = nl_langinfo(CODESET);
#else
	char *cencoding = strchr(locale, '.');
	if (cencoding)
	    cencoding++; /* Step past the dot in, e.g., ja_JP.EUC-JP */
#endif
	if (cencoding) {
	    if (strcmp(cencoding,"UTF-8") == 0)
		strcpy(degree_sign,degree_utf8);
	    else if ((cd = iconv_open(cencoding, "UTF-8")) == (iconv_t)(-1))
		int_warn(NO_CARET, "iconv_open failed for %s", cencoding);
	    else {
		if (iconv(cd, &in, &lengthin, &out, &lengthout) == (size_t)(-1))
		    int_warn(NO_CARET, "iconv failed to convert degree sign");
		iconv_close(cd);
	    }
	}
	return;
    }
#else
    (void)locale; /* -Wunused argument */
#endif

    /* These are the internally-known encodings */
    memset(degree_sign, 0, sizeof(degree_sign));
    switch (encoding) {
    case S_ENC_UTF8:	degree_sign[0] = '\302'; degree_sign[1] = '\260'; break;
    case S_ENC_KOI8_R:
    case S_ENC_KOI8_U:	degree_sign[0] = '\234'; break;
    case S_ENC_CP437:
    case S_ENC_CP850:
    case S_ENC_CP852:	degree_sign[0] = '\370'; break;
    case S_ENC_SJIS:	break;  /* should be 0x818B */
    case S_ENC_CP950:	break;  /* should be 0xA258 */
    /* default applies at least to:
       ISO8859-1, -2, -9, -15,
       CP1250, CP1251, CP1252, CP1254
     */
    default:		degree_sign[0] = '\260'; break;
    }
}


#if defined(_WIN32) || defined(MSDOS) || defined(OS2)
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
#endif


const char *
latex_input_encoding(enum set_encoding_id encoding)
{
    const char * inputenc = NULL;

    switch(encoding) {
    case S_ENC_DEFAULT:
	break;
    case S_ENC_ISO8859_1:
	inputenc = "latin1";
	break;
    case S_ENC_ISO8859_2:
	inputenc = "latin2";
	break;
    case S_ENC_ISO8859_9:	/* ISO8859-9 is Latin5 */
	inputenc = "latin5";
	break;
    case S_ENC_ISO8859_15:	/* ISO8859-15 is Latin9 */
	inputenc = "latin9";
	break;
    case S_ENC_CP437:
	inputenc = "cp437de";
	break;
    case S_ENC_CP850:
	inputenc = "cp850";
	break;
    case S_ENC_CP852:
	inputenc = "cp852";
	break;
    case S_ENC_CP1250:
	inputenc = "cp1250";
	break;
    case S_ENC_CP1251:
	inputenc = "cp1251";
	break;
    case S_ENC_CP1252:
	inputenc = "cp1252";
	break;
    case S_ENC_KOI8_R:
	inputenc = "koi8-r";
	break;
    case S_ENC_KOI8_U:
	inputenc = "koi8-u";
	break;
    case S_ENC_UTF8:
	/* utf8x (not utf8) is needed to pick up degree and micro signs */
	inputenc = "utf8x";
	break;
    case S_ENC_INVALID:
	int_error(NO_CARET, "invalid input encoding used");
	break;
    default:
	/* do nothing */
	break;
    }

    return inputenc;
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
 * This operation is used in several places to reduce 8 chars of
 * input to a single utf8 character used for PT_CHARACTER.
 * Replaces content of original string.
 * "AB" -> "A"
 * "\U+0041" -> "A"
 * "\316\261" -> utf8 "ɑ" (alpha) (unchanged from input)
 *               the scanner already replaced string "\316" with octal byte 0316
 */
void
truncate_to_one_utf8_char(char *orig)
{
    uint32_t codepoint;
    char newchar[9];
    int length = 0;

    safe_strncpy(newchar, orig, sizeof(newchar));

    /* Check for unicode escape */
    if (!strncmp("\\U+", newchar, 3)) {
	if (sscanf(&newchar[3], "%5x", &codepoint) == 1)
	    length = ucs4toutf8(codepoint, (unsigned char *)newchar);
	newchar[length] = '\0';
    }
    /* Truncate ascii text to single character */
    else if ((newchar[0] & 0x80) == 0)
	newchar[1] = '\0';
    /* Some other 8-bit sequence (we don't check for valid utf8) */
    else {
	newchar[7] = '\0';
	for (length=1; length<7; length++)
	    if ((newchar[length] & 0xC0) != 0x80) {
		newchar[length] = '\0';
		break;
	    }
    }

    /* overwrite original string */
    strcpy(orig, newchar);
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
