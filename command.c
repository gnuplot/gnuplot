#ifndef lint
static char    *RCSid = "$Id: command.c,v 1.123 1998/03/22 23:31:00 drd Exp $";
#endif

/* GNUPLOT - command.c */

/*[
 * Copyright 1986 - 1993, 1998   Thomas Williams, Colin Kelley
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

/*
 * Changes:
 * 
 * Feb 5, 1992	Jack Veenstra	(veenstra@cs.rochester.edu) Added support to
 * filter data values read from a file through a user-defined function before
 * plotting. The keyword "thru" was added to the "plot" command. Example
 * syntax: f(x) = x / 100 plot "test.data" thru f(x) This example divides all
 * the y values by 100 before plotting. The filter function processes the
 * data before any log-scaling occurs. This capability should be generalized
 * to filter x values as well and a similar feature should be added to the
 * "splot" command.
 * 
 * 19 September 1992  Lawrence Crowl  (crowl@cs.orst.edu)
 * Added user-specified bases for log scaling.
 */

#include "plot.h"
#include "setshow.h"
#include "fit.h"
#include "binary.h"

#if defined(MSDOS) || defined(DOS386)
# ifdef DJGPP
#  include <dos.h>
#  include <dir.h>            /* HBB: for setdisk() */
# else
#  include <process.h>
# endif /* DJGPP */

# ifdef __ZTC__
#  define HAVE_SLEEP 1
#  define P_WAIT 0
# else /* __ZTC__ */

# ifdef __TURBOC__
#  ifndef _Windows
#   define HAVE_SLEEP 1
#   include <conio.h>
#   include <dir.h>    /* setdisk() */
extern unsigned _stklen = 16394;/* increase stack size */
extern char HelpFile[80] ;      /* patch for do_help  - DJL */
#  endif /* _Windows */

# else				/* must be MSC */ 
#  if !defined(__EMX__) && !defined(DJGPP)
#   ifdef __MSC__
#    include <direct.h>		/* for _chdrive() */
#   endif /* __MSC__ */
#  endif /* !__EMX__ && !DJGPP */
# endif	/* TURBOC */
#endif /* ZTC */

#endif /* MSDOS */

#ifndef _Windows
# include "help.h"
#else
# define MAXSTR 255
#endif /* _Windows */

#if defined(ATARI) || defined(MTOS)
#ifdef __PUREC__
#include <ext.h>
#include <tos.h>
#include <aes.h>
/* #include <float.h> - already in plot.h */
#else
#include <osbind.h>
#include <aesbind.h>
#include <support.h>
#endif /* __PUREC__ */
#endif /* ATARI || MTOS */

#ifndef STDOUT
#define STDOUT 1
#endif

#ifndef HELPFILE
# if defined( MSDOS ) || defined( OS2 ) || defined(DOS386)
#  define HELPFILE "gnuplot.gih"
# else
#  if defined(AMIGA_SC_6_1) || defined(AMIGA_AC_5)
#   define HELPFILE "S:gnuplot.gih"
#  else
#   define HELPFILE "docs/gnuplot.gih"	/* changed by makefile */
#  endif /* AMIGA_SC_6_1 || AMIGA_AC_5 */
# endif /* MSDOS || OS2 || DOS386 */ 
#endif /* HELPFILE */

#ifdef _Windows
#include <windows.h>
#ifdef __MSC__
#include <malloc.h>
#else
#include <alloc.h>
#include <dir.h>    /* setdisk() */
#endif
#include "win/wgnuplib.h"
extern TW textwin;
extern LPSTR winhelpname;
extern void screen_dump(void);	/* in term/win.trm */
extern int Pause(LPSTR mess); /* in winmain.c */
#endif

#define inrange(z,min,max) ((min<max) ? ((z>=min)&&(z<=max)) : ((z>=max)&&(z<=min)) )

#ifdef OS2
 /* emx has getcwd, chdir that can handle drive names */
#define chdir  _chdir2
#endif /* OS2 */

#ifdef VMS
int             vms_vkid;	/* Virtual keyboard id */
#endif


/* static prototypes */

static void replotrequest __PROTO((void));
static int command __PROTO((void));
static int read_line __PROTO((char *prompt));
static void do_shell __PROTO((void));
static void do_help __PROTO((int toplevel));
static void do_system __PROTO((void));
static int changedir __PROTO((char *path));

/* input data, parsing variables */
#ifdef AMIGA_SC_6_1
 __far int             num_tokens, c_token;
#else
 int             num_tokens, c_token;
#endif

extern TBOOLEAN  do_load_arg_substitution;
struct lexical_unit *token=NULL;
int token_table_size=0;

char *input_line=NULL;
int input_line_len=0;
int inline_num = 0;	/* input line number */

/* things shared by plot2d.c and plot3d.c are here, for historical reasons */

double          min_array[AXIS_ARRAY_SIZE], max_array[AXIS_ARRAY_SIZE];
int             auto_array[AXIS_ARRAY_SIZE];
TBOOLEAN        log_array[AXIS_ARRAY_SIZE];
double          base_array[AXIS_ARRAY_SIZE];
double          log_base_array[AXIS_ARRAY_SIZE];
struct udft_entry *dummy_func=NULL;  /* NULL means no dummy vars active */

char            c_dummy_var[MAX_NUM_VAR][MAX_ID_LEN + 1];	/* current dummy vars */


/* support for replot command */
char *replot_line=NULL;
int      plot_token;	/* start of 'plot' command */

/* If last plot was a 3d one. */
TBOOLEAN         is_3d_plot = FALSE;



#define Inc_c_token if (++c_token >= num_tokens)	\
                        int_error ("Syntax error", c_token);


/* support for dynamic size of input line */

void extend_input_line()
{
  if(input_line_len==0) {
    /* first time */
    input_line=gp_alloc(MAX_LINE_LEN, "input_line");
    input_line_len=MAX_LINE_LEN;
    input_line[0]='\0';
  } else {
    input_line=gp_realloc(input_line, input_line_len+MAX_LINE_LEN, "extend input line");
    input_line_len+=MAX_LINE_LEN;
#ifdef DEBUG_STR
    fprintf(stderr, "extending input line to %d chars\n", input_line_len);
#endif
  }
}

void extend_token_table()
{
  if(token_table_size==0) {
    /* first time */
    token=(struct lexical_unit *)gp_alloc(MAX_TOKENS*sizeof(struct lexical_unit), "token table");
    token_table_size=MAX_TOKENS;
  } else {
    token=gp_realloc(token, (token_table_size+MAX_TOKENS)*sizeof(struct lexical_unit), "extend token table");
    token_table_size+=MAX_TOKENS;
#ifdef DEBUG_STR
    fprintf(stderr, "extending token table to %d elements\n", token_table_size);
#endif
  }
}

void init_memory()
{
  extend_input_line();
  extend_token_table();
  replot_line=gp_alloc(1, "string");
  *replot_line='\0';
}

int com_line()
{
  if (multiplot) {
  	/* calls int_error() if it is not happy */
	term_check_multiplot_okay(interactive);
	
   if (read_line("multiplot> "))
       return(1);
 } else {
   if (read_line(PROMPT))
       return(1);
 }

    /* So we can flag any new output: if false at time of error, */
    /* we reprint the command line before printing caret. */
    /* TRUE for interactive terminals, since the command line is typed. */
    /* FALSE for non-terminal stdin, so command line is printed anyway. */
    /* (DFK 11/89) */
    screen_ok = interactive;

    if (do_line())
        return(1);
     else
        return(0);
}

#ifdef OS2
int set_input_line( char *line, int nchar )
{
    strncpy( input_line, line, nchar ) ;
    input_line[nchar]='\0';
}
#endif

int do_line()
{				/* also used in load_file */
    if (is_system(input_line[0])) {
	do_system();
	if (interactive) /* 3.5 did it unconditionally */
		(void) fputs("!\n", stderr); /* why do we need this ? */
	return(0);
    }
    num_tokens = scanner(input_line);
    c_token = 0;
    while (c_token < num_tokens) {
	if (command())
           return(1);
	if (c_token < num_tokens)	/* something after command */
	    if (equals(c_token, ";"))
		c_token++;
	    else
		int_error("';' expected", c_token);
    }
    return(0);
}


void define()
{
    register int    start_token;/* the 1st token in the function definition */
    register struct udvt_entry *udv;
    register struct udft_entry *udf;

    if (equals(c_token + 1, "(")) {
	/* function ! */
	int             dummy_num = 0;
	char save_dummy[MAX_NUM_VAR][MAX_ID_LEN+1];
	memcpy(save_dummy, c_dummy_var, sizeof(save_dummy));
	start_token = c_token;
	do {
	    c_token += 2;	/* skip to the next dummy */
	    copy_str(c_dummy_var[dummy_num++], c_token, MAX_ID_LEN);
	} while (equals(c_token + 1, ",") && (dummy_num < MAX_NUM_VAR));
	if (equals(c_token + 1, ","))
	    int_error("function contains too many parameters", c_token + 2);
	c_token += 3;		/* skip (, dummy, ) and = */
	if (END_OF_COMMAND)
	    int_error("function definition expected", c_token);
	udf = dummy_func = add_udf(start_token);
	if (udf->at)		/* already a dynamic a.t. there */
	    free((char *) udf->at);	/* so free it first */
	if ((udf->at = perm_at()) == (struct at_type *) NULL)
	    int_error("not enough memory for function", start_token);
	memcpy(c_dummy_var, save_dummy, sizeof(save_dummy));
	m_capture(&(udf->definition), start_token, c_token - 1);
	dummy_func=NULL; /* dont let anyone else use our workspace */
    } else {
	/* variable ! */
	start_token = c_token;
	c_token += 2;
	udv = add_udv(start_token);
	(void) const_express(&(udv->udv_value));
	udv->udv_undef = FALSE;
    }
}


static int command()
{
    FILE *fp;
    int             i;
    char            sv_file[MAX_LINE_LEN + 1];
    /* string holding name of save or load file */
    
    for (i = 0; i < MAX_NUM_VAR; i++)
	c_dummy_var[i][0] = '\0';	/* no dummy variables */

    if (is_definition(c_token))
	define();
    else if (almost_equals(c_token, "h$elp") || equals(c_token, "?")) {
	c_token++;
	do_help(1);
	 } else if (equals(c_token, "testtime")) {
	 	/* given a format and a time string, exercise the time code */
	 	char format[160], string[160];
	 	struct tm tm;
	 	double secs;
	 	if (isstring(++c_token))
	 	{
	 		quote_str(format, c_token, 159);
	 		if (isstring(++c_token))
	 		{
				quote_str(string, c_token++, 159);
				memset(&tm, 0, sizeof(tm));
				gstrptime(string, format, &tm);
				secs = gtimegm(&tm);
				fprintf(stderr, "internal = %f - %d/%d/%d::%d:%d:%d , wday=%d, yday=%d\n",
				  secs, tm.tm_mday, tm.tm_mon+1, tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_wday, tm.tm_yday);
				memset(&tm, 0, sizeof(tm));
				ggmtime(&tm, secs);
				gstrftime(string, 159, format, secs);
				fprintf(stderr, "convert back \"%s\" - %d/%d/%d::%d:%d:%d , wday=%d, yday=%d\n",
				  string, tm.tm_mday, tm.tm_mon+1, tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_wday, tm.tm_yday);
			}
		}
    } else if (almost_equals(c_token, "test")) {
	c_token++;
	test_term();
    } else if (almost_equals(c_token, "scr$eendump")) {
	c_token++;
#ifdef _Windows
	screen_dump();
#else
	fputs("screendump not implemented\n",stderr);
#endif
    } else if (almost_equals(c_token, "pa$use")) {
	struct value    a;
	int             sleep_time, text = 0;
	char            buf[MAX_LINE_LEN + 1];

	c_token++;
	sleep_time = (int) real(const_express(&a));
	buf[0]='\0';
	if (!(END_OF_COMMAND)) {
	    if (!isstring(c_token))
		int_error("expecting string", c_token);
	    else {
		quote_str(buf, c_token, MAX_LINE_LEN);
		++c_token;
#ifdef _Windows
		if (sleep_time>=0)
#endif
#ifdef OS2
                if( strcmp(term->name, "pm" )!=0 || sleep_time >=0 )
#endif
#ifdef MTOS
		if( strcmp(term->name, "mtos" )!=0 || sleep_time >=0 )
#endif
		(void) fprintf(stderr, "%s", buf);
		text = 1;
	    }
	}
	if (sleep_time < 0)
#ifdef _Windows
        {
            if (!Pause(buf))
                 bail_to_command_line();
         }
#else
#ifdef OS2
        if( strcmp(term->name, "pm" )==0 && sleep_time < 0 )
        {
            int rc ;
            if( (rc=PM_pause( buf ))==0 ) bail_to_command_line();
            else if( rc==2 ) { 
		(void) fprintf(stderr, "%s", buf);
		text = 1;
                (void) fgets(buf, MAX_LINE_LEN, stdin);
                }	

	}
#else
#ifdef _Macintosh
	if ( strcmp(term->name, "macintosh" )==0 && sleep_time < 0)
	{
		Pause(sleep_time);
	}
#else 
#ifdef MTOS
	{
	if( strcmp(term->name, "mtos" )==0) {
	    	int MTOS_pause(char *buf);
	    	int rc;
		if ((rc = MTOS_pause(buf)) == 0) 
			bail_to_command_line();
	    	else if (rc == 2) {
			(void) fprintf(stderr, "%s", buf);
			text = 1;
			(void) fgets(buf, MAX_LINE_LEN, stdin);
		} 
	}
	else if( strcmp(term->name, "atari" )==0) {
		char *readline(char *);
		char *line=readline("");
		if(line) free(line);	    
        }
	else
		(void) fgets(buf, MAX_LINE_LEN, stdin);
	}
#else
#ifdef ATARI
	{
	else if( strcmp(term->name, "atari" )==0) {
		char *readline(char *);
		char *line=readline("");
		if(line) free(line);	    
        }
	else
		(void) fgets(buf, MAX_LINE_LEN, stdin);	   	    
	}
#else /* ATARI */
		(void) fgets(buf, MAX_LINE_LEN, stdin);
	/* Hold until CR hit. */
#endif /* MTOS */
#endif /* ATARI */
#endif
#endif /*OS2*/
#endif
	if (sleep_time > 0)
	    GP_SLEEP(sleep_time);

	if (text != 0 && sleep_time >= 0)
	    (void) fprintf(stderr, "\n");
	screen_ok = FALSE;
    }
    else if (almost_equals(c_token, "pr$int")) {
		int need_space=0; /* space printed between two expressions only*/
    	screen_ok = FALSE;
    	do {
    		++c_token;
    	   if (isstring(c_token)) {
    			char s[MAX_LINE_LEN];
    			quote_str(s, c_token, MAX_LINE_LEN);
    			fputs(s, stderr);
				need_space=0;
    			++c_token;
    		} else {
				struct value    a;
				(void) const_express(&a);
				if (need_space)
					putc(' ', stderr);
				need_space = 1;
				disp_value(stderr, &a);
			}
		} while (!END_OF_COMMAND && equals(c_token, ","));
		
		(void) putc('\n', stderr);
    } else if (almost_equals(c_token,"fit")) {
	++c_token;
	do_fit();
    }
    else if (almost_equals(c_token,"up$date")) {
		char tmps[80];
		char tmps2[80];
		/* Have to initialise tmps2, otherwise
		 * update() cannot decide whether a valid
		 * filename was given. lh
		 */
		tmps2[0] = '\0';
		if ( !isstring(++c_token) )
			int_error ("Parameter filename expected", c_token);
		quote_str (tmps, c_token++, 80);
		if (!(END_OF_COMMAND)) {
			if ( !isstring(c_token) )
				int_error ("New parameter filename expected", c_token);
		    else
				quote_str (tmps2, c_token++, 80);
		}
		update (tmps, tmps2);
	}
    else if (almost_equals(c_token, "p$lot")) {
	plot_token = c_token++;
#ifdef _Windows
	SetCursor(LoadCursor((HINSTANCE)NULL, IDC_WAIT));
#endif
	plotrequest();
#ifdef _Windows
	SetCursor(LoadCursor((HINSTANCE)NULL, IDC_ARROW));
#endif
    } else if (almost_equals(c_token, "sp$lot")) {
	plot_token = c_token++;
#ifdef _Windows
	SetCursor(LoadCursor((HINSTANCE)NULL, IDC_WAIT));
#endif
	plot3drequest();
#ifdef _Windows
	SetCursor(LoadCursor((HINSTANCE)NULL, IDC_ARROW));
#endif
    } else if (almost_equals(c_token, "rep$lot")) {
	if (replot_line[0] == '\0')
	    int_error("no previous plot", c_token);
	c_token++;
#ifdef _Windows
	SetCursor(LoadCursor((HINSTANCE)NULL, IDC_WAIT));
#endif
	replotrequest();
#ifdef _Windows
	SetCursor(LoadCursor((HINSTANCE)NULL, IDC_ARROW));
#endif
    } else if (almost_equals(c_token, "se$t"))
	set_command();
    else if (almost_equals(c_token, "res$et"))
        reset_command();
    else if (almost_equals(c_token, "sh$ow"))
	show_command();
    else if (almost_equals(c_token, "cl$ear")) {
    	term_start_plot();

		if (multiplot && term->fillbox) {
			unsigned int x1 = (unsigned int)(xoffset * term->xmax);
			unsigned int y1 = (unsigned int)(yoffset * term->ymax);
			unsigned int width = (unsigned int)(xsize * term->xmax);
			unsigned int height = (unsigned int)(ysize * term->ymax);
			(*term->fillbox)(0,x1,y1,width,height);
		}

		term_end_plot();
		
		screen_ok = FALSE;
		c_token++;
    } else if (almost_equals(c_token, "she$ll")) {
	do_shell();
	screen_ok = FALSE;
	c_token++;
    } else if (almost_equals(c_token, "sa$ve")) {
	if (almost_equals(++c_token, "f$unctions")) {
	    if (!isstring(++c_token))
		int_error("expecting filename", c_token);
	    else {
		quote_str(sv_file, c_token, MAX_LINE_LEN);
		save_functions(fopen(sv_file, "w"));
	    }
	} else if (almost_equals(c_token, "v$ariables")) {
	    if (!isstring(++c_token))
		int_error("expecting filename", c_token);
	    else {
		quote_str(sv_file, c_token, MAX_LINE_LEN);
		save_variables(fopen(sv_file, "w"));
	    }
	} else if (almost_equals(c_token, "s$et")) {
	    if (!isstring(++c_token))
		int_error("expecting filename", c_token);
	    else {
		quote_str(sv_file, c_token, MAX_LINE_LEN);
		save_set(fopen(sv_file, "w"));
	    }
	} else if (isstring(c_token)) {
	    quote_str(sv_file, c_token, MAX_LINE_LEN);
	    save_all(fopen(sv_file, "w"));
	} else {
	    int_error(
			 "filename or keyword 'functions', 'variables', or 'set' expected",
			 c_token);
	}
	c_token++;
    } else if (almost_equals(c_token, "l$oad")) {
	if (!isstring(++c_token))
	    int_error("expecting filename", c_token);
	else {
	    quote_str(sv_file, c_token, MAX_LINE_LEN);
	    load_file(fp=fopen(sv_file, "r"), sv_file, FALSE);
	    /* input_line[] and token[] now destroyed! */
	    c_token = num_tokens = 0;
	}
    } else if (almost_equals(c_token,"ca$ll")) {
	if (!isstring(++c_token))
	    int_error("expecting filename",c_token);
	else {
	    quote_str(sv_file,c_token, MAX_LINE_LEN);
	    load_file(fopen(sv_file,"r"), sv_file, TRUE);	/* Argument list follows filename */
	    /* input_line[] and token[] now destroyed! */
	    c_token = num_tokens = 0;
	}
    } else if (almost_equals(c_token,"if")) {
	double exprval;
	struct value    t;
	if (!equals(++c_token, "("))	/* no expression */
	    int_error("expecting (expression)", c_token);
        exprval = real(const_express(&t));
	if (exprval != 0.0) {
	    /* fake the condition of a ';' between commands */
	    int eolpos = token[num_tokens-1].start_index+token[num_tokens-1].length;
	    --c_token;
	    token[c_token].length = 1;
	    token[c_token].start_index = eolpos+2;
	    input_line[eolpos+2] = ';';
	    input_line[eolpos+3] = '\0';
	} else
	    c_token = num_tokens = 0;
    } else if (almost_equals(c_token,"rer$ead")) {
            fp = lf_top();
            if (fp != (FILE *)NULL) rewind(fp);
            c_token++;
    } else if (almost_equals(c_token, "cd")) {
	if (!isstring(++c_token))
	    int_error("expecting directory name", c_token);
	else {
	    quote_str(sv_file, c_token, MAX_LINE_LEN);
	    if (changedir(sv_file)) {
		int_error("Can't change to this directory", c_token);
	    }
	    c_token++;
	}
    } else if (almost_equals(c_token, "pwd")) {
	GP_GETCWD(sv_file, sizeof(sv_file) );
	fprintf(stderr, "%s\n", sv_file);
	c_token++;
    } else if (almost_equals(c_token, "ex$it") ||
	       almost_equals(c_token, "q$uit")) {
		/* graphics will be tidied up in main */
	return(1);
    } else if (!equals(c_token, ";")) {	/* null statement */
#ifdef OS2
        if (_osmode==OS2_MODE) {
        if( token[c_token].is_token ) { 
            int rc ; 
            rc=ExecuteMacro(input_line+token[c_token].start_index,
                                token[c_token].length);
            if(rc==0){
                c_token=num_tokens=0;
                return(0);
                }
            }
        }
#endif
	int_error("invalid command", c_token);
    }
    
    return(0);
}


void done(status)
    int             status;
{
	term_reset();
    exit(status);
}

static int changedir(path)
char *path;
{
#if defined(MSDOS) || defined(_Windows) || defined(ATARI) || defined(DOS386)
#if defined(__ZTC__)
    unsigned dummy; /* it's a parameter needed for dos_setdrive */
#endif

    /* first deal with drive letter */

    if (isalpha(path[0]) && (path[1]==':')) {
	int driveno=toupper(path[0])-'A'; /* 0=A, 1=B, ... */
#if defined(ATARI)
	(void)Dsetdrv(driveno);
#endif

#if defined(__ZTC__)
	(void)dos_setdrive(driveno + 1, &dummy);
#endif

#if defined(MSDOS) && defined(__EMX__)
	(void)_chdrive(driveno + 1);
#endif
#if defined(__MSC__)
	(void)_chdrive(driveno+1);
#endif
/* HBB: recent versions of DJGPP also have setdisk():, so I del'ed the special code */
#if ((defined(MSDOS) || defined(_Windows)) && defined(__TURBOC__)) || defined(DJGPP)
	(void) setdisk(driveno);
#endif
	path += 2; /* move past drive letter */
    }
    /* then change to actual directory */
    if (*path)
	if (chdir(path))
	    return 1;

    return 0; /* should report error with setdrive also */
#else /* MSDOS, ATARI etc. */
    return chdir(path);
#endif /* MSDOS, ATARI etc. */
}


static void replotrequest()
{
    if (equals(c_token, "["))
	int_error("cannot set range with replot", c_token);

    /* do not store directly into the replot_line string, until the
     * new plot line has been successfully plotted. This way,
     * if user makes a typo in a replot line, they do not have
     * to start from scratch. The replot_line will be committed
     * after do_plot has returned, whence we know all is well
     */
	if (END_OF_COMMAND)
	{
		/* it must already be long enough, but lets make sure */
		int len = strlen(replot_line) + 1;
		while (input_line_len < len)
			extend_input_line();
		strcpy(input_line, replot_line);
	}
	else
	{
     		char *replot_args=NULL; /* else m_capture will free it */
     		int last_token = num_tokens - 1;
	      /* length = length of old part + length of new part + ',' + \0 */
	      int newlen=strlen(replot_line)+token[last_token].start_index+token[last_token].length-token[c_token].start_index+2;
     		m_capture(&replot_args, c_token, last_token); /* might be empty */
     		while (input_line_len < newlen)
     			extend_input_line();
     		strcpy(input_line, replot_line);
     		strcat(input_line, ",");
     		strcat(input_line, replot_args);
     		free(replot_args);
     }
     plot_token=0; /* whole line to be saved as replot line */
    
    screen_ok = FALSE;
    num_tokens = scanner(input_line);
    c_token = 1;		/* skip the 'plot' part */
    if(is_3d_plot)
	  plot3drequest();
	else  plotrequest();
}


/* Support for input, shell, and help for various systems */

#ifdef VMS

#include <descrip.h>
#include <rmsdef.h>
#include <smgdef.h>
#include <smgmsg.h>

extern          lib$get_input(), lib$put_output();
extern          smg$read_composed_line();
extern		sys$putmsg();
extern		lbr$output_help();
extern		lib$spawn();

int             vms_len;

unsigned int    status[2] =
{1, 0};

static char     Help[MAX_LINE_LEN + 1] = "gnuplot";

$DESCRIPTOR(prompt_desc, PROMPT);
/* temporary fix until change to variable length */
struct dsc$descriptor_s line_desc = { 0, DSC$K_DTYPE_T, DSC$K_CLASS_S, NULL };

$DESCRIPTOR(help_desc, Help);
$DESCRIPTOR(helpfile_desc, "GNUPLOT$HELP");

/* please note that the vms version of read_line doesn't support variable line
   length (yet) */

static int read_line(prompt)
    char           *prompt;
{
    int             more, start = 0;
    char            expand_prompt[40];

    prompt_desc.dsc$w_length = strlen(prompt);
    prompt_desc.dsc$a_pointer = prompt;
    (void) strcpy(expand_prompt, "_");
    (void) strncat(expand_prompt, prompt, 38);
    do {
	line_desc.dsc$w_length = MAX_LINE_LEN - start;
	line_desc.dsc$a_pointer = &input_line[start];
	switch (status[1] = smg$read_composed_line(&vms_vkid, 0, &line_desc, &prompt_desc, &vms_len)) {
	case SMG$_EOF:
	    done(IO_SUCCESS);	/* ^Z isn't really an error */
	    break;
	case RMS$_TNS:		/* didn't press return in time */
	    vms_len--;		/* skip the last character */
	    break;		/* and parse anyway */
	case RMS$_BES:		/* Bad Escape Sequence */
	case RMS$_PES:		/* Partial Escape Sequence */
	    sys$putmsg(status);
	    vms_len = 0;	/* ignore the line */
	    break;
	case SS$_NORMAL:
	    break;		/* everything's fine */
	default:
	    done(status[1]);	/* give the error message */
	}
	start += vms_len;
	input_line[start] = '\0';
	inline_num++;
	if (input_line[start - 1] == '\\') {
	    /* Allow for a continuation line. */
	    prompt_desc.dsc$w_length = strlen(expand_prompt);
	    prompt_desc.dsc$a_pointer = expand_prompt;
	    more = 1;
	    --start;
	} else {
	    line_desc.dsc$w_length = strlen(input_line);
	    line_desc.dsc$a_pointer = input_line;
	    more = 0;
	}
    } while (more);
	return 0;
}


static void do_help(toplevel)
int toplevel; /* not used for VMS version */
{
    int first=c_token;
    while (!END_OF_COMMAND)
	++c_token;

    strcpy(Help, "GNUPLOT ");
    capture(Help+8, first, c_token-1, sizeof(Help)-9);
    help_desc.dsc$w_length = strlen(Help);
    if ((vaxc$errno = lbr$output_help(lib$put_output, 0, &help_desc,
			   &helpfile_desc, 0, lib$get_input)) != SS$_NORMAL)
	os_error("can't open GNUPLOT$HELP", NO_CARET);
}


static void do_shell()
{
    if ((vaxc$errno = lib$spawn()) != SS$_NORMAL) {
	os_error("spawn error", NO_CARET);
    }
}


static void do_system()
{
    input_line[0] = ' ';	/* an embarrassment, but... */

    if ((vaxc$errno = lib$spawn(&line_desc)) != SS$_NORMAL)
	os_error("spawn error", NO_CARET);

    (void) putc('\n', stderr);
}

#else				/* VMS */

#ifdef _Windows
static void do_help(toplevel)
int toplevel; /* not used for windows */
{
	if (END_OF_COMMAND)
		WinHelp(textwin.hWndParent,(LPSTR)winhelpname,HELP_INDEX,(DWORD)NULL);
	else {
		char buf[128];
		int start = c_token++;
		while (!(END_OF_COMMAND))
			c_token++;
		capture(buf, start, c_token-1, 128);
		WinHelp(textwin.hWndParent,(LPSTR)winhelpname,HELP_PARTIALKEY,(DWORD)buf);
	}
}
#else

/*
 * do_help: (not VMS, although it would work) Give help to the user. It
 * parses the command line into helpbuf and supplies help for that string.
 * Then, if there are subtopics available for that key, it prompts the user
 * with this string. If more input is given, do_help is called recursively,
 * with argument 0.  Thus a more
 * specific help can be supplied. This can be done repeatedly. If null input
 * is given, the function returns, effecting a backward climb up the tree.
 * David Kotz (David.Kotz@Dartmouth.edu) 10/89
 * drd - The help buffer is first cleared when called with
 * toplevel=1. This is to fix a bug where help is broken if ^C is pressed
 * whilst in the help.
 */

static void do_help(toplevel)
int toplevel;
{
    static char    *helpbuf = NULL;
    static char    *prompt = NULL;
    int             base;	/* index of first char AFTER help string */
    int             len;	/* length of current help string */
    TBOOLEAN         more_help;
    TBOOLEAN         only;	/* TRUE if only printing subtopics */
    int             subtopics;	/* 0 if no subtopics for this topic */
    int             start;	/* starting token of help string */
    char           *help_ptr;	/* name of help file */
#if defined(SHELFIND)
    static char    help_fname[256]=""; /* keep helpfilename across calls */
#endif
#if defined(MTOS) || defined(ATARI)
    char const *const ext[] = {NULL};
#endif

    if ((help_ptr = getenv("GNUHELP")) == (char *) NULL)
#ifndef SHELFIND
	/* if can't find environment variable then just use HELPFILE */

/* patch by David J. Liu for getting GNUHELP from home directory */
#if defined(__TURBOC__) && (defined(MSDOS) || defined(DOS386))
        help_ptr = HelpFile ;
#else
#if defined(MTOS) || defined(ATARI)
    {
	if ((help_ptr = findfile(HELPFILE,getenv("GNUPLOTPATH"),ext)) == NULL)
		help_ptr = findfile(HELPFILE,getenv("PATH"),ext);
	if (!help_ptr)	
		help_ptr = HELPFILE;
    }
#else
	help_ptr = HELPFILE;
#endif /* MTOS || ATARI */
#endif /* __TURBOC__ */
/* end of patch  - DJL */

#else
	/* try whether we can find the helpfile via shell_find. If not, just
	   use the default. (tnx Andreas) */

	if( !strchr( HELPFILE, ':' ) && !strchr( HELPFILE, '/' ) &&
	    !strchr( HELPFILE, '\\' ) ) {
	    if( strlen(help_fname)==0 ) {
		strcpy( help_fname, HELPFILE );
		if( shel_find( help_fname )==0 ) {
		    strcpy( help_fname, HELPFILE );
		}
	    }
	    help_ptr=help_fname;
	} else {
	    help_ptr=HELPFILE;
	}
#endif /* SHELFIND */

    /* Since MSDOS DGROUP segment is being overflowed we can not allow such  */
    /* huge static variables (1k each). Instead we dynamically allocate them */
    /* on the first call to this function...				     */
    if (helpbuf == NULL) {
	helpbuf = gp_alloc((unsigned long)MAX_LINE_LEN, "help buffer");
	prompt = gp_alloc((unsigned long)MAX_LINE_LEN, "help prompt");
	helpbuf[0] = prompt[0] = 0;
    }
	if (toplevel)
		helpbuf[0] = prompt[0] = 0; /* in case user hit ^c last time */

    len = base = strlen(helpbuf);

    /* find the end of the help command */
    for (start = c_token; !(END_OF_COMMAND); c_token++);
    /* copy new help input into helpbuf */
    if (len > 0)
	helpbuf[len++] = ' ';	/* add a space */
    capture(helpbuf + len, start, c_token - 1, MAX_LINE_LEN-len);
    squash_spaces(helpbuf + base);	/* only bother with new stuff */
    lower_case(helpbuf + base);	/* only bother with new stuff */
    len = strlen(helpbuf);

    /* now, a lone ? will print subtopics only */
    if (strcmp(helpbuf + (base ? base + 1 : 0), "?") == 0) {
	/* subtopics only */
	subtopics = 1;
	only = TRUE;
	helpbuf[base] = '\0';	/* cut off question mark */
    } else {
	/* normal help request */
	subtopics = 0;
	only = FALSE;
    }

    switch (help(helpbuf, help_ptr, &subtopics)) {
    case H_FOUND:{
	    /* already printed the help info */
	    /* subtopics now is true if there were any subtopics */
	    screen_ok = FALSE;

	    do {
		if (subtopics && !only) {
		    /* prompt for subtopic with current help string */
		    if (len > 0)
			(void) sprintf(prompt, "Subtopic of %s: ", helpbuf);
		    else
			(void) strcpy(prompt, "Help topic: ");
		    read_line(prompt);
		    num_tokens = scanner(input_line);
		    c_token = 0;
		    more_help = !(END_OF_COMMAND);
		    if (more_help)
			/* base for next level is all of current helpbuf */
			do_help(0);
		} else
		    more_help = FALSE;
	    } while (more_help);

	    break;
	}
    case H_NOTFOUND:{
	    printf("Sorry, no help for '%s'\n", helpbuf);
	    break;
	}
    case H_ERROR:{
	    perror(help_ptr);
	    break;
	}
    default:{			/* defensive programming */
	    int_error("Impossible case in switch", NO_CARET);
	    /* NOTREACHED */
	}
    }

    helpbuf[base] = '\0';	/* cut it off where we started */
}
#endif  /* _Windows */

#ifdef _Windows
/* this function is used before its definition */
int winsystem(char *s);
#endif

#ifdef AMIGA_AC_5
char            strg0[256];
#endif

static void do_system()
{
#ifdef AMIGA_AC_5
    char           *parms[80];
    void            getparms();

    getparms(input_line + 1, parms);
    if (fexecv(parms[0], parms) < 0)
#else
#if (defined(ATARI)&&defined(__GNUC__)) /* || (defined(MTOS)&&defined(__GNUC__)) */
    /* use preloaded shell, if available */
    short           (*shell_p) (char *command);
    void           *ssp;

    ssp = (void *) Super(NULL);
    shell_p = *(short (**) (char *)) 0x4f6;
    Super(ssp);

    /* this is a bit strange, but we have to have a single if */
    if ((shell_p ? (*shell_p) (input_line + 1) : system(input_line + 1)))
#else
#ifdef _Windows
    if (winsystem(input_line + 1))
#else
    if (system(input_line + 1))
#endif
#endif
#endif
	os_error("system() failed", NO_CARET);
}

#ifdef AMIGA_AC_5

/******************************************************************************/
/* */
/* Parses the command string (for fexecv use) and  converts the first token  */
/* to lower case                                                          */
/* */
/******************************************************************************/

void 
getparms(command, parms)
    char           *command;
    char          **parms;
{
    register int    i = 0;	/* A bunch of indices          */
    register int    j = 0;
    register int    k = 0;

    while (*(command + j) != '\0') {	/* Loop on string characters   */
	parms[k++] = strg0 + i;
	while (*(command + j) == ' ')
	    ++j;
	while (*(command + j) != ' ' && *(command + j) != '\0') {
	    if (*(command + j) == '"')	/* Get quoted string           */
		for (*(strg0 + (i++)) = *(command + (j++));
		     *(command + j) != '"';
		     *(strg0 + (i++)) = *(command + (j++)));
	    *(strg0 + (i++)) = *(command + (j++));
	}
	*(strg0 + (i++)) = '\0';/* NUL terminate every token   */
    }
    parms[k] = '\0';

    for (k = strlen(strg0) - 1; k >= 0; --k)	/* Convert to lower case       */
	*(strg0 + k) >= 'A' && *(strg0 + k) <= 'Z' ? *(strg0 + k) |= 32 : *(strg0 + k);
}

#endif				/* AMIGA_AC_5 */


#ifdef READLINE

/* keep some compilers happy */
char *rlgets __PROTO((char *s, int n, char *prompt));

char           *
rlgets(s, n, prompt)
    char           *s;
    int             n;
    char           *prompt;
{
    static char    *line = (char *) NULL;
    static int     leftover = -1; /* index of 1st char leftover from last call */

    if(leftover== -1) {
	/* If we already have a line, first free it */
	if (line != (char *) NULL)
	{
	    free(line);
	    line = NULL; /* so that ^C or int_error during readline() does */
	                 /* not result in line being free-ed twice         */
	}

	line = readline((interactive) ? prompt : "");
	leftover=0;
	/* If it's not an EOF */
	if (line && *line) 
	    add_history(line);
    }
    if(line) {
	strncpy(s, line+leftover, n);
	s[n-1]='\0';
	leftover+=strlen(s);
	if(line[leftover]=='\0')
	    leftover = -1;
	return s;
    }

    return NULL;
}
#endif				/* READLINE */

#if defined(MSDOS) || defined(_Windows) || defined(DOS386)



static void do_shell()
{
    register char  *comspec;
    if ((comspec = getenv("COMSPEC")) == (char *) NULL)
	comspec = "\\command.com";
#ifdef _Windows
    if (WinExec(comspec, SW_SHOWNORMAL) <= 32)
#else
#ifdef DJGPP
	if (system(comspec) == -1)
#else
    if (spawnl(P_WAIT, comspec, NULL) == -1)
#endif
#endif
	os_error("unable to spawn shell", NO_CARET);
}

#else				/* MSDOS */
/* plain old Unix */

#ifdef AMIGA_SC_6_1
static void do_shell()
{
    register char  *shell;
    if (!(shell = getenv("SHELL")))
	shell = SHELL;

    if (system(shell))
	os_error("system() failed", NO_CARET);

    (void) putc('\n', stderr);
}
#else				/* AMIGA_SC_6_1 */
#ifdef OS2
static void do_shell()
{
    register char  *shell;
    if (!(shell = getenv("SHELL"))&&!(shell = getenv("COMSPEC")))
	shell = SHELL;

    if (system(shell) == -1 )
	os_error("system() failed", NO_CARET);

    (void) putc('\n', stderr);
}
#else                           /* ! OS2 */
#define EXEC "exec "
static void do_shell()
{
    static char     exec[100] = EXEC;
    register char  *shell;
    if (!(shell = getenv("SHELL")))
	shell = SHELL;

    if (system(strncpy(&exec[sizeof(EXEC) - 1], shell,
		       sizeof(exec) - sizeof(EXEC) - 1)))
	os_error("system() failed", NO_CARET);

    (void) putc('\n', stderr);
}
#endif                          /* OS2 */
#endif				/* AMIGA_SC_6_1 */
#endif				/* MSDOS */

/* read from stdin, everything except VMS */

#ifndef READLINE
#if (defined(MSDOS) || defined(DOS386)) && !defined(_Windows) && !defined(__EMX__) && !defined(DJGPP)

/* if interactive use console IO so CED will work */

#define PUT_STRING(s) cputs(s)
#define GET_STRING(s,l) ((interactive) ? cgets_emu(s,l) : fgets(s,l,stdin))

#ifdef __TURBOC__ 
/* cgets implemented using dos functions */
/* Maurice Castro 22/5/91 */
char           *
doscgets(s)
    char           *s;
{
    long            datseg;

    /* protect and preserve segments - call dos to do the dirty work */
    datseg = _DS;

    _DX = FP_OFF(s);
    _DS = FP_SEG(s);
    _AH = 0x0A;
    geninterrupt(33);
    _DS = datseg;

    /* check for a carriage return and then clobber it with a null */
    if (s[s[1] + 2] == '\r')
	s[s[1] + 2] = 0;

    /* return the input string */
    return (&(s[2]));
}
#endif	/* __TURBOC__ */

#ifdef __ZTC__
void cputs(char *s)
{
   register int i = 0;
   while (s[i] != '\0')  bdos(0x02, s[i++], NULL);
}
char *cgets(char *s)
{
   bdosx(0x0A, s, NULL);

   if (s[s[1]+2] == '\r')
      s[s[1]+2] = 0;

   /* return the input string */
   return(&(s[2]));
}
#endif   /* __ZTC__ */

/* emulate a fgets like input function with DOS cgets */
char *cgets_emu(str, len)
char *str;
int len;
{
    static char buffer[128]="";
    static int leftover=0;

    if(buffer[leftover]=='\0') {
	buffer[0]=126;
#ifdef __TURBOC__
	doscgets(buffer);
#else
	cgets(buffer);
#endif
	fputc('\n', stderr);
	if(buffer[2]==26)
	    return NULL;
	leftover=2;
    }
    strncpy(str, buffer+leftover, len);
    str[len-1]='\0';
    leftover+=strlen(str);
    return str;
}
#else /* !plain DOS */

#define PUT_STRING(s) fputs(s, stderr)
#define GET_STRING(s,l) fgets(s, l, stdin)

#endif
#endif /* READLINE */

static int read_line(prompt)
    char           *prompt;
{
    int             start = 0;
    TBOOLEAN        more = FALSE;
    int             last = 0;

#ifndef READLINE
    if (interactive)
	PUT_STRING(prompt);
#endif				/* READLINE */
    do {
	/* grab some input */
#ifdef READLINE
	if (((interactive)
	     ? rlgets(&(input_line[start]), input_line_len - start,
		      ((more) ? "> " : prompt))
	     : fgets(&(input_line[start]), input_line_len - start, stdin))
	    == (char *) NULL) {
#else
	if (GET_STRING(&(input_line[start]), input_line_len - start)
	    == (char *) NULL) {
#endif				/* READLINE */
	    /* end-of-file */
	    if (interactive)
		(void) putc('\n', stderr);
	    input_line[start] = '\0';
	    inline_num++;
	    if (start > 0)	/* don't quit yet - process what we have */
		more = FALSE;
	    else
		return(1); /* exit gnuplot */
	} else {
	    /* normal line input */
	    last = strlen(input_line) - 1;
	    if(last >= 0)
	      {
		if (input_line[last] == '\n') {	/* remove any newline */
		  input_line[last] = '\0';
		  /* Watch out that we don't backup beyond 0 (1-1-1) */
		  if (last > 0)
		    --last;
		} else if (last + 2 >= input_line_len) {
		  extend_input_line();
		  start = last+1;
		  more = TRUE;
		  continue; /* read rest of line, don't print "> " */
		}
		
		if (input_line[last] == '\\') {	/* line continuation */
		  start = last;
		  more = TRUE;
		} else
		  more = FALSE;
	      }
	    else
	      more = FALSE;
	}
#ifndef READLINE
	if (more && interactive)
	    PUT_STRING("> ");
#endif
    } while (more);
    return(0);
}
#endif				/* VMS */

#ifdef _Windows
/* there is a system like call on MS Windows but it is a bit difficult to 
   use, so we will invoke the command interpreter and use it to execute the 
   commands */
int winsystem(char *s)
{
	LPSTR comspec;
	LPSTR execstr;
	LPSTR p;

	/* get COMSPEC environment variable */
#ifdef WIN32
	char envbuf[81];
	GetEnvironmentVariable("COMSPEC", envbuf, 80);
	if (*envbuf == '\0')
		comspec = "\\command.com";
	else
		comspec = envbuf;
#else
	p = GetDOSEnvironment();
	comspec = "\\command.com";
	while (*p) {
		if (!strncmp(p,"COMSPEC=",8)) {
			comspec=p+8;
			break;
		}
		p+=strlen(p)+1;
	}
#endif
	/* if the command is blank we must use command.com */
	p = s;
	while ((*p == ' ') || (*p == '\n') || (*p == '\r'))
		p++;
	if (*p == '\0')
	{
		WinExec(comspec, SW_SHOWNORMAL);
		}
	else
	{
		/* attempt to run the windows/dos program via windows */
		if (WinExec(s, SW_SHOWNORMAL) <= 32)
		{
			/* attempt to run it as a dos program from command line */
			execstr = (char *) malloc(strlen(s) + strlen(comspec) + 6);
			strcpy(execstr, comspec);
			strcat(execstr, " /c ");
			strcat(execstr, s);
			WinExec(execstr, SW_SHOWNORMAL);
			free(execstr);
			}
		}

	/* regardless of the reality return OK - the consequences of */
	/* failure include shutting down Windows */
	return(0);		/* success */
	}
#endif

#if 0 /* not used */
static int 
get_time_data(line,maxcol,cols,types,x,y,z)
char *line;
int maxcol, *cols, *types;
double *x, *y, *z;
{
	register int m, n, linestat;
	double val[3];
	char *s;
	static struct tm tm;

	linestat = 1;
	m = n = 0;
	s = line;
	while ((linestat == 1) && (n<maxcol)) {
		while (isspace(*s)) s++;
		if (*s == '\0') {
			linestat = 0;
			break;
		}
		n++;
		if (n == cols[m]) {
			if ( types[m] == TIME ) {
				if ((s = (char *) gstrptime(s,timefmt,&tm)) != NULL ) {
					val[m] = (double) gtimegm(&tm);
					m++;
				} else
					linestat = 2;
			} else if (isdigit(*s) || *s=='-' || *s=='+' || *s=='.') {
				val[m] = atof(s);
				m++;
			}
			else
				linestat = 2;	/* abort the line non-digit in req loc */
		}
		while ((!isspace(*s)) && (*s != '\0')) s++;
	}
	if ( linestat == 2 )
		return(0);
	if ( m > 0 )
		*x = val[0];
	if ( m > 1 )
		*y = val[1];
	if ( m > 2 )
		*z = val[2];
	return(m);
}
#endif /* not used */
