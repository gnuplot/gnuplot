#ifndef lint
static char *RCSid = "$Id: readline.c,v 3.26 92/03/24 22:34:35 woo Exp Locker: woo $";
#endif

/* GNUPLOT - readline.c */
/*
 * Copyright (C) 1986, 1987, 1990, 1991, 1992   Thomas Williams, Colin Kelley
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted, 
 * provided that the above copyright notice appear in all copies and 
 * that both that copyright notice and this permission notice appear 
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the modified code.  Modifications are to be distributed 
 * as patches to released version.
 *  
 * This software is provided "as is" without express or implied warranty.
 * 
 *
 * AUTHORS
 *
 *   Original Software:
 *     Tom Tkacik
 *
 *   Msdos port and some enhancements:
 *     Gershon Elber and many others.
 * 
 * Send your comments or suggestions to 
 *  info-gnuplot@ames.arc.nasa.gov.
 * This is a mailing list; to join it send a note to 
 *  info-gnuplot-request@ames.arc.nasa.gov.  
 * Send bug reports to
 *  bug-gnuplot@ames.arc.nasa.gov.
 */

#ifdef READLINE

/* a small portable version of GNU's readline */

/* do not need any terminal capabilities except backspace,
/* and space overwrites a character */

/* NANO-EMACS line editing facility */
/* printable characters print as themselves (insert not overwrite) */
/* ^A moves to the beginning of the line */
/* ^B moves back a single character */
/* ^E moves to the end of the line */
/* ^F moves forward a single character */
/* ^K kills from current position to the end of line */
/* ^P moves back through history */
/* ^N moves forward through history */
/* ^H and DEL delete the previous character */
/* ^D deletes the current character, or EOF if line is empty */
/* ^L/^R redraw line in case it gets trashed */
/* ^U kills the entire line */
/* ^W kills last word */
/* LF and CR return the entire line regardless of the cursor postition */
/* EOF with an empty line returns (char *)NULL */

/* all other characters are ignored */

#include <stdio.h>
#include <ctype.h>
#include <signal.h>

/* SIGTSTP defines job control */
/* if there is job control then we need termios.h instead of termio.h */
#ifdef SIGTSTP
#define TERMIOS
#endif


#ifndef MSDOS

/* UNIX specific stuff */
#ifdef TERMIOS
#include <termios.h>
static struct termios orig_termio, rl_termio;
#else
#include <termio.h>
static struct termio orig_termio, rl_termio;
#endif /* TERMIOS */
static int term_set = 0;	/* =1 if rl_termio set */

#else

/* MSDOS specific stuff */
#define getc(stdin) msdos_getch()
static char msdos_getch();

#endif /* MSDOS */


/* is it <string.h> or <strings.h>?   just declare what we need */
extern int strlen();
extern char *strcpy();
extern char *malloc();

#define MAXBUF	1024
#define BACKSPACE 0x08	/* ^H */
#define SPACE	' '

struct hist {
	char *line;
	struct hist *prev;
	struct hist *next;
};

static struct hist *history = NULL;  /* no history yet */
static struct hist *cur_entry = NULL;

static char cur_line[MAXBUF];  /* current contents of the line */
static int cur_pos = 0;	/* current position of the cursor */
static int max_pos = 0;	/* maximum character position */


void add_history();
static void fix_line();
static void redraw_line();
static void clear_line();
static void clear_eoline();
static void copy_line();
static void set_termio();
static void reset_termio();

char *
readline(prompt)
char *prompt;
{

	char cur_char;
	char *new_line;

	/* set the termio so we can do our own input processing */
	set_termio();

	/* print the prompt */
	fputs(prompt, stderr);
	cur_line[0] = '\0';
	cur_pos = 0;
	max_pos = 0;
	cur_entry = NULL;

	/* get characters */
	for(;;) {
		cur_char = getc(stdin);
		if(isprint(cur_char)) {
			int i;
			for(i=max_pos; i>cur_pos; i--) {
				cur_line[i] = cur_line[i-1];
			}
			putc(cur_char, stderr);
			cur_line[cur_pos] = cur_char;
			cur_pos += 1;
			max_pos += 1;
			if (cur_pos < max_pos)
			    fix_line();
			cur_line[max_pos] = '\0';

		/* else interpret unix terminal driver characters */
#ifdef VERASE
		} else if(cur_char == orig_termio.c_cc[VERASE] ){  /* DEL? */
			if(cur_pos > 0) {
				int i;
				cur_pos -= 1;
				putc(BACKSPACE, stderr);
				for(i=cur_pos; i<max_pos; i++)
					cur_line[i] = cur_line[i+1];
				max_pos -= 1;
				fix_line();
			}
#endif /* VERASE */
#ifdef VEOF
		} else if(cur_char == orig_termio.c_cc[VEOF] ){   /* ^D? */
			if(max_pos == 0) {
				reset_termio();
				return((char *)NULL);
			}
			if((cur_pos < max_pos)&&(cur_char == 004)) { /* ^D */
				int i;
				for(i=cur_pos; i<max_pos; i++)
					cur_line[i] = cur_line[i+1];
				max_pos -= 1;
				fix_line();
			}
#endif /* VEOF */
#ifdef VKILL
		} else if(cur_char == orig_termio.c_cc[VKILL] ){  /* ^U? */
			clear_line(prompt);
#endif /* VKILL */
#ifdef VWERASE
		} else if(cur_char == orig_termio.c_cc[VWERASE] ){  /* ^W? */
			while((cur_pos > 0) &&
			      (cur_line[cur_pos-1] == SPACE)) {
				cur_pos -= 1;
				putc(BACKSPACE, stderr);
			}
			while((cur_pos > 0) &&
			      (cur_line[cur_pos-1] != SPACE)) {
				cur_pos -= 1;
				putc(BACKSPACE, stderr);
			}
			clear_eoline();
			max_pos = cur_pos;
#endif /* VWERASE */
#ifdef VREPRINT
		} else if(cur_char == orig_termio.c_cc[VREPRINT] ){  /* ^R? */
			putc('\n',stderr); /* go to a fresh line */
			redraw_line(prompt);
#else
#ifdef VRPRNT   /* on Ultrix VREPRINT is VRPRNT */
		} else if(cur_char == orig_termio.c_cc[VRPRNT] ){  /* ^R? */
			putc('\n',stderr); /* go to a fresh line */
			redraw_line(prompt);
#endif /* VRPRNT */
#endif /* VREPRINT */
#ifdef VSUSP
		} else if(cur_char == orig_termio.c_cc[VSUSP]) {
			reset_termio();
			kill(0, SIGTSTP);

			/* process stops here */

			set_termio();
			/* print the prompt */
			redraw_line(prompt);
#endif /* VSUSP */
		} else {
			/* do normal editing commands */
			/* some of these are also done above */
			int i;
			switch(cur_char) {
			    case EOF:
				reset_termio();
				return((char *)NULL);
			    case 001: /* ^A */
				while(cur_pos > 0) {
					cur_pos -= 1;
					putc(BACKSPACE, stderr);
				}
				break;
			    case 002: /* ^B */
				if(cur_pos > 0) {
					cur_pos -= 1;
					putc(BACKSPACE, stderr);
				}
				break;
			    case 005: /* ^E */
				while(cur_pos < max_pos) {
					putc(cur_line[cur_pos], stderr);
					cur_pos += 1;
				}
				break;
			    case 006: /* ^F */
				if(cur_pos < max_pos) {
					putc(cur_line[cur_pos], stderr);
					cur_pos += 1;
				}
				break;
			    case 013: /* ^K */
				clear_eoline();
				max_pos = cur_pos;
				break;
			    case 020: /* ^P */
				if(history != NULL) {
					if(cur_entry == NULL) {
						cur_entry = history;
						clear_line(prompt);
						copy_line(cur_entry->line);
					} else if(cur_entry->prev != NULL) {
						cur_entry = cur_entry->prev;
						clear_line(prompt);
						copy_line(cur_entry->line);
					}
				}
				break;
			    case 016: /* ^N */
				if(cur_entry != NULL) {
					cur_entry = cur_entry->next;
					clear_line(prompt);
					if(cur_entry != NULL) 
						copy_line(cur_entry->line);
					else
						cur_pos = max_pos = 0;
				}
				break;
			    case 014: /* ^L */
			    case 022: /* ^R */
				putc('\n',stderr); /* go to a fresh line */
				redraw_line(prompt);
				break;
			    case 0177: /* DEL */
			    case 010: /* ^H */
				if(cur_pos > 0) {
					cur_pos -= 1;
					putc(BACKSPACE, stderr);
					for(i=cur_pos; i<max_pos; i++)
						cur_line[i] = cur_line[i+1];
					max_pos -= 1;
					fix_line();
				}
				break;
			    case 004: /* ^D */
				if(max_pos == 0) {
					reset_termio();
					return((char *)NULL);
				}
				if(cur_pos < max_pos) {
					for(i=cur_pos; i<max_pos; i++)
						cur_line[i] = cur_line[i+1];
					max_pos -= 1;
					fix_line();
				}
				break;
			    case 025:  /* ^U */
				clear_line(prompt);
				break;
			    case 027:  /* ^W */
				while((cur_pos > 0) &&
				      (cur_line[cur_pos-1] == SPACE)) {
					cur_pos -= 1;
					putc(BACKSPACE, stderr);
				}
				while((cur_pos > 0) &&
				      (cur_line[cur_pos-1] != SPACE)) {
					cur_pos -= 1;
					putc(BACKSPACE, stderr);
				}
				clear_eoline();
				max_pos = cur_pos;
				break;
				break;
			    case '\n': /* ^J */
			    case '\r': /* ^M */
				cur_line[max_pos+1] = '\0';
				putc('\n', stderr);
				new_line = malloc(strlen(cur_line)+1);
				strcpy(new_line,cur_line);
				reset_termio();
				return(new_line);
			    default:
				break;
			}
		}
	}
}

/* fix up the line from cur_pos to max_pos */
/* do not need any terminal capabilities except backspace,
/* and space overwrites a character */
static void
fix_line()
{
	int i;

	/* write tail of string */
	for(i=cur_pos; i<max_pos; i++)
		putc(cur_line[i], stderr);

	/* write a space at the end of the line in case we deleted one */
	putc(SPACE, stderr);

	/* backup to original position */
	for(i=max_pos+1; i>cur_pos; i--)
		putc(BACKSPACE, stderr);

}

/* redraw the entire line, putting the cursor where it belongs */
static void
redraw_line(prompt)
char *prompt;
{
	int i;

	fputs(prompt, stderr);
	fputs(cur_line, stderr);

	/* put the cursor where it belongs */
	for(i=max_pos; i>cur_pos; i--)
		putc(BACKSPACE, stderr);
}

/* clear cur_line and the screen line */
static void
clear_line(prompt)
char *prompt;
{
	int i;
	for(i=0; i<max_pos; i++)
		cur_line[i] = '\0';

	for(i=cur_pos; i>0; i--)
		putc(BACKSPACE, stderr);

	for(i=0; i<max_pos; i++)
		putc(SPACE, stderr);

	putc('\r', stderr);
	fputs(prompt, stderr);

	cur_pos = 0;
	max_pos = 0;
}

/* clear to end of line and the screen end of line */
static void
clear_eoline(prompt)
char *prompt;
{
	int i;
	for(i=cur_pos; i<max_pos; i++)
		cur_line[i] = '\0';

	for(i=cur_pos; i<max_pos; i++)
		putc(SPACE, stderr);
	for(i=cur_pos; i<max_pos; i++)
		putc(BACKSPACE, stderr);
}

/* copy line to cur_line, draw it and set cur_pos and max_pos */
static void
copy_line(line)
char *line;
{
	strcpy(cur_line, line);
	fputs(cur_line, stderr);
	cur_pos = max_pos = strlen(cur_line);
}

/* add line to the history */
void
add_history(line)
char *line;
{
	struct hist *entry;
	entry = (struct hist *)malloc(sizeof(struct hist));
	entry->line = malloc((unsigned int)strlen(line)+1);
	strcpy(entry->line, line);

	entry->prev = history;
	entry->next = NULL;
	if(history != NULL) {
		history->next = entry;
	}
	history = entry;
}

#ifdef MSDOS

/* Convert Arrow keystrokes to Control characters: */
static  char
msdos_getch()
{
    char c = getch();

    if (c == 0) {
	c = getch(); /* Get the extended code. */
	switch (c) {
	    case 75: /* Left Arrow. */
		c = 002;
		break;
	    case 77: /* Right Arrow. */
		c = 006;
		break;
	    case 72: /* Up Arrow. */
		c = 020;
		break;
	    case 80: /* Down Arrow. */
		c = 016;
		break;
	    case 115: /* Ctl Left Arrow. */
	    case 71: /* Home */
		c = 001;
		break;
	    case 116: /* Ctl Right Arrow. */
	    case 79: /* End */
		c = 005;
		break;
	    case 83: /* Delete */
		c = 004;
		break;
	    default:
		c = 0;
		break;
	}
    }
    else if (c == 033) { /* ESC */
	c = 025;
    }


    return c;
}

#endif /* MSDOS */

/* set termio so we can do our own input processing */
static void
set_termio()
{
#ifndef MSDOS
	if(term_set == 0) {
#ifdef TERMIOS
#ifdef TCGETS
		ioctl(0, TCGETS, &orig_termio);
#else
		tcgetattr(0, &orig_termio);
#endif /* TCGETS */
#else
		ioctl(0, TCGETA, &orig_termio);
#endif /* TERMIOS */
		rl_termio = orig_termio;

		rl_termio.c_iflag &= ~(BRKINT|PARMRK|INPCK|IUCLC|IXON|IXOFF);
		rl_termio.c_iflag |=  (IGNBRK|IGNPAR);

		rl_termio.c_oflag &= ~(ONOCR);

		rl_termio.c_lflag &= ~(ICANON|ECHO|ECHOE|ECHOK|ECHONL|NOFLSH);
		rl_termio.c_lflag |=  (ISIG);

		rl_termio.c_cc[VMIN] = 1;
		rl_termio.c_cc[VTIME] = 0;

#ifdef VSUSP
		/* disable suspending process on ^Z */
		rl_termio.c_cc[VSUSP] = 0;
#endif /* VSUSP */

#ifdef TERMIOS
#ifdef TCSETSW
		ioctl(0, TCSETSW, &rl_termio);
#else
		tcsetattr(0, TCSADRAIN, &rl_termio);
#endif /* TCSETSW */
#else
		ioctl(0, TCSETAW, &rl_termio);
#endif /* TERMIOS */
		term_set = 1;
	}
#endif /* MSDOS */
}

static void
reset_termio()
{
#ifndef MSDOS
	if(term_set == 1) {
#ifdef TERMIOS
#ifdef TCSETSW
		ioctl(0, TCSETSW, &orig_termio);
#else
		tcsetattr(0, TCSADRAIN, &orig_termio);
#endif /* TCSETSW */
#else
		ioctl(0, TCSETAW, &orig_termio);
#endif /* TERMIOS */
		term_set = 0;
	}
#endif /* MSDOS */
}
#endif /* READLINE */
