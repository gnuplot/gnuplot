#ifndef lint
static char *RCSid() { return RCSid("$Id: plot.c,v 1.31.2.4 2000/08/04 14:01:27 joze Exp $"); }
#endif

/* GNUPLOT - plot.c */

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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <signal.h>
#include <setjmp.h>

#ifdef HAVE_SIGSETJMP
# define SETJMP(env, save_signals) sigsetjmp(env, save_signals)
# define LONGJMP(env, retval) siglongjmp(env, retval)
#else
# define SETJMP(env, save_signals) setjmp(env)
# define LONGJMP(env, retval) longjmp(env, retval)
#endif

#include "plot.h"

#include "alloc.h"
#include "command.h"
#include "eval.h"
#include "fit.h"
#include "misc.h"
#include "setshow.h"
#include "term_api.h"
#include "util.h"
#ifdef USE_MOUSE
#   ifndef OS2
#       include "ipc.h"   /* for isatty_state */
#   endif
#   include "mouse.h" /* for mouse_setting */
#endif

/* Used nowhere else */
#ifdef HAVE_SYS_UTSNAME_H
# include <sys/utsname.h>
#endif

#if defined(MSDOS) || defined(DOS386) || defined(__EMX__)
# include <io.h>
#endif

#ifdef VMS
# ifndef __GNUC__
#  include <unixio.h>
# endif
# include <smgdef.h>
# include <ssdef.h>
extern int vms_vkid;
extern smg$create_virtual_keyboard();
extern int vms_ktid;
extern smg$create_key_table();
#endif /* VMS */

#ifdef AMIGA_SC_6_1
# include <proto/dos.h>
#endif

#ifdef _Windows
# include <windows.h>
# ifndef SIGINT
#  define SIGINT 2		/* for MSC */
# endif
#endif /* _Windows */

/* GNU readline
 * Only required by two files directly,
 * so I don't put this into a header file. -lh
 */
#ifdef HAVE_LIBREADLINE
# include <readline/readline.h>
# include <readline/history.h>
extern int rl_complete_with_tilde_expansion;

/* enable gnuplot history with readline */
# ifdef GNUPLOT_HISTORY
#  include <readline/tilde.h>
#  ifndef GNUPLOT_HISTORY_FILE
#   define GNUPLOT_HISTORY_FILE "~/.gnuplot_history"
#  endif
#  ifndef HISTORY_SIZE
/* Can be overriden with the environment variable 'GNUPLOT_HISTORY_SIZE' */
#   define HISTORY_SIZE 666
long int gnuplot_history_size = HISTORY_SIZE;
#  endif
/* 
 * The next variable is a pointer to the value returned from 'tilde_expand()'.
 * This function expands '~' to the user's home directory, or $HOME, with
 * UN*X, AmigaOS, MSDOS.
 * Depending on your OS you have to make sure that the "$HOME" environment
 * variable exitsts.  You are responsible for valid values.
 */
static char *expanded_history_filename;

static void wrapper_for_write_history __PROTO((void));

# endif				/* GNUPLOT_HISTORY */
#endif /* HAVE_LIBREADLINE */

TBOOLEAN interactive = TRUE;	/* FALSE if stdin not a terminal */
static TBOOLEAN noinputfiles = TRUE; /* FALSE if there are script files */

/* user home directory */
static const char *user_homedir = NULL;

/* user shell */
const char *user_shell = NULL;

#if defined(ATARI) || defined(MTOS)
const char *user_gnuplotpath = NULL;
#endif

/* flags to disable `replot` when some data are sent through stdin; used by
   mouse/hotkey capable terminals */
int plotted_data_from_stdin = 0;
int replot_disabled = 0;

#ifdef X11
extern int X11_args __PROTO((int, char **));
#endif

/* patch to get home dir, see command.c */
#if (defined (__TURBOC__) && (defined (MSDOS) || defined(DOS386))) || defined(DJGPP)
# include <dir.h>               /* MAXPATH */
char HelpFile[MAXPATH];
#endif /*   - DJL */

/* a longjmp buffer to get back to the command line */
#ifdef _Windows
static jmp_buf far command_line_env;
#else
static jmp_buf command_line_env;
#endif

static void load_rcfile __PROTO((void));
static void get_user_env __PROTO((void));
static RETSIGTYPE inter __PROTO((int anint));
static void init_memory __PROTO((void));

static int exit_status = EXIT_SUCCESS;

#ifdef OS2
# define INCL_DOS
# define INCL_REXXSAA
# ifdef USE_MOUSE
#  define INCL_DOSSEMAPHORES
# endif
# include <os2.h>
# include <process.h>
ULONG RexxInterface(PRXSTRING, PUSHORT, PRXSTRING);
int ExecuteMacro(char *, int);
TBOOLEAN CallFromRexx = FALSE;
void PM_intc_cleanup();
void PM_setup();
#endif /* OS2 */

#if defined(ATARI) || defined(MTOS)
/* For findfile () (?) */
# include <support.h>
void appl_exit(void);
void MTOS_open_pipe(void);
extern int aesid;
#endif

static RETSIGTYPE
inter(anint)
int anint;
{
#ifdef OS2
    (void) signal(anint, SIG_ACK);
#else
    (void) signal(SIGINT, (sigfunc) inter);
#endif

#ifndef DOSX286
    (void) signal(SIGFPE, SIG_DFL);	/* turn off FPE trapping */
#endif
#ifdef OS2
    PM_intc_cleanup();
#else
    term_reset();
    (void) putc('\n', stderr);
    LONGJMP(command_line_env, TRUE);	/* return to prompt */
#endif
}

#ifdef LINUXVGA
/* utility functions to ensure that setuid gnuplot
 * assumes root privileges only for those parts
 * of the code which require root rights.
 *
 * By "Dr. Werner Fink" <werner@suse.de>
 */
static uid_t euid, ruid;
static gid_t egid, rgid;
static int asked_privi = 0;

void
drop_privilege()
{
    if (!asked_privi) {
	euid = geteuid();
	egid = getegid();
	ruid = getuid();
	rgid = getgid();
	asked_privi = 1;
    }
    if (setegid(rgid) == -1)
	(void) fprintf(stderr, "setegid(%d): %s\n",
		       (int) rgid, strerror(errno));
    if (seteuid(ruid) == -1)
	(void) fprintf(stderr, "seteuid(%d): %s\n",
		       (int) ruid, strerror(errno));
}

void
take_privilege()
{
    if (!asked_privi) {
	euid = geteuid();
	egid = getegid();
	ruid = getuid();
	rgid = getgid();
	asked_privi = 1;
    }
    if (setegid(egid) == -1)
	(void) fprintf(stderr, "setegid(%d): %s\n",
		       (int) egid, strerror(errno));
    if (seteuid(euid) == -1)
	(void) fprintf(stderr, "seteuid(%d): %s\n",
		       (int) euid, strerror(errno));
}

#endif /* LINUXVGA */

/* a wrapper for longjmp so we can keep everything local */
void
bail_to_command_line()
{
    LONGJMP(command_line_env, TRUE);
}

#if defined(_Windows) || defined(_Macintosh)
int
gnu_main(argc, argv)
#else
int
main(argc, argv)
#endif
int argc;
char **argv;
{
#ifdef LINUXVGA
    LINUX_setup();		/* setup VGA before dropping privilege DBT 4/5/99 */
    drop_privilege();
#endif
/* make sure that we really have revoked root access, this might happen if
   gnuplot is compiled without vga support but is installed suid by mistake */
#ifdef __linux__
    setuid(getuid());
#endif
#if defined(MSDOS) && !defined(_Windows) && !defined(__GNUC__)
    PC_setup();
#endif /* MSDOS !Windows */
/* HBB: Seems this isn't needed any more for DJGPP V2? */
/* HBB: disable all floating point exceptions, just keep running... */
#if defined(DJGPP) && (DJGPP!=2)
    _control87(MCW_EM, MCW_EM);
#endif

#if defined(OS2)
    int rc;
    if (_osmode == OS2_MODE) {
#ifdef USE_MOUSE
	extern HEV semInputReady;
	char semInputReadyName[40];
	sprintf( semInputReadyName, "\\SEM32\\GP%i_Input_Ready", getpid() );
	rc = DosCreateEventSem(semInputReadyName,&semInputReady,0,0);
	if (rc != 0)
	    fputs("DosCreateEventSem error\n",stderr);
#endif
	PM_setup();
	rc = RexxRegisterSubcomExe("GNUPLOT", (PFN) RexxInterface, NULL);
    }
#endif

/* malloc large blocks, otherwise problems with fragmented mem */
#ifdef OSK
    _mallocmin(102400);
#endif

#ifdef MALLOCDEBUG
    malloc_debug(7);
#endif

/* get helpfile from home directory */
#ifndef DOSX286
# ifndef _Windows
#  if defined (__TURBOC__) && (defined (MSDOS) || defined(DOS386))
    strcpy(HelpFile, argv[0]);
    strcpy(strrchr(HelpFile, DIRSEP1), "\\gnuplot.gih");
#  endif			/*   - DJL */
# endif				/* !_Windows */
#endif /* !DOSX286 */
#ifdef __DJGPP__
    {
	char *s;
	strcpy(HelpFile, argv[0]);
	for (s = HelpFile; *s; s++)
	    if (*s == DIRSEP1)
		*s = DIRSEP2;	/* '\\' to '/' */
	strcpy(strrchr(HelpFile, DIRSEP2), "/gnuplot.gih");
    }			/* Add also some "paranoid" tests for '\\':  AP */
#endif /* DJGPP */

#ifdef VMS
    unsigned int status[2] = { 1, 0 };
#endif

#ifdef HAVE_LIBREADLINE
    using_history();
    /* T.Walter 1999-06-24: 'rl_readline_name' must be this fix name.
     * It is used to parse a 'gnuplot' specific section in '~/.inputrc' */
    rl_readline_name = "Gnuplot";
    rl_complete_with_tilde_expansion = 1;
    rl_terminal_name = getenv("TERM");
#endif

#ifdef X11
    {
	int n = X11_args(argc, argv);
	argv += n;
	argc -= n;
    }
#endif

#ifdef APOLLO
    apollo_pfm_catch();
#endif

/* moved to ATARI_init in atariaes.trm */
/* #ifdef ATARI
   void application_init(void);
   application_init();
   #endif */

#ifdef MTOS
    MTOS_open_pipe();
#endif

    setbuf(stderr, (char *) NULL);

#ifndef NO_SETVBUF
    /* this was once setlinebuf(). Docs say this is
     * identical to setvbuf(,NULL,_IOLBF,0), but MS C
     * faults this (size out of range), so we try with
     * size of 1024 instead. [SAS/C does that, too. -lh]
     * Failing this, I propose we just make the call and
     * ignore the return : its probably not a big deal
     */
    if (setvbuf(stdout, (char *) NULL, _IOLBF, (size_t) 1024) != 0)
	(void) fputs("Could not linebuffer stdout\n", stderr);
#endif

    gpoutfile = stdout;
    (void) Gcomplex(&udv_pi.udv_value, M_PI, 0.0);

    init_memory();

    interactive = FALSE;
    init_terminal();		/* can set term type if it likes */

#ifdef AMIGA_SC_6_1
    if (IsInteractive(Input()) == DOSTRUE)
	interactive = TRUE;
    else
	interactive = FALSE;
#else
# if (defined(__MSC__) && defined(_Windows)) || defined(__WIN32__)
    interactive = TRUE;
# else
    interactive = isatty(fileno(stdin));
#if defined(USE_MOUSE) && !defined(OS2)
    /* isatty_state is set here and nowhere else! (used in term/x11.trm) */
    isatty_state = interactive;
    if (!isatty_state) {
	/* stdin is not from a tty --> Turn mouse off.
	 * can be turned on again, e.g. if the user
	 * wants to write on a pipe to gnuplot */
	mouse_setting.on = 0;
    }
#endif
# endif
#endif /* !AMIGA_SC_6_1 */

    if (argc > 1)
	interactive = noinputfiles = FALSE;
    else
	noinputfiles = TRUE;

    /* Need this before show_version is called for the first time */

#ifdef HAVE_SYS_UTSNAME_H
    {
	struct utsname uts;

	/* something is fundamentally wrong if this fails ... */
	if (uname(&uts) > -1) {
# ifdef _AIX
	    strcpy(os_name, uts.sysname);
	    sprintf(os_name, "%s.%s", uts.version, uts.release);
# elif defined(SCO)
	    strcpy(os_name, "SCO");
	    strcpy(os_rel, uts.release);
# else
	    strcpy(os_name, uts.sysname);
	    strcpy(os_rel, uts.release);
# endif
	}
    }
#else /* ! HAVE_SYS_UTSNAME_H */

    strcpy(os_name, OS);
    strcpy(os_rel, "");

#endif /* HAVE_SYS_UTSNAME_H */

    if (interactive)
	show_version(stderr);

#ifdef VMS
    /* initialise screen management routines for command recall */
    if (status[1] = smg$create_virtual_keyboard(&vms_vkid) != SS$_NORMAL)
	done(status[1]);
    if (status[1] = smg$create_key_table(&vms_ktid) != SS$_NORMAL)
	done(status[1]);
#endif /* VMS */

    if (!SETJMP(command_line_env, 1)) {
	/* first time */
	interrupt_setup();
	/* should move this stuff another initialisation routine,
	 * something like init_set() maybe */
	get_user_env();
	init_loadpath();
	init_locale();
	load_rcfile();
	init_fit();		/* Initialization of fitting module */

	if (interactive && term != 0) {		/* not unknown */
#if defined(HAVE_LIBREADLINE) && defined(GNUPLOT_HISTORY)
	    FPRINTF((stderr, "Before read_history\n"));
	    expanded_history_filename = tilde_expand(GNUPLOT_HISTORY_FILE);
	    FPRINTF((stderr, "expanded_history_filename = %s\n", expanded_history_filename));
	    read_history(expanded_history_filename);
	    {
		/* BEGIN: Go local to get environment variable */
		const char *temp_env = getenv ("GNUPLOT_HISTORY_SIZE");
		if (temp_env)
		    gnuplot_history_size = strtol (temp_env, (char **) NULL, 10);
	    } /* END: Go local to get environment variable */

	    /* 
	     * It is safe to ignore the return values of 'atexit()' and
	     * 'on_exit()'. In the worst case, there is no history of your
	     * currrent session and you have to type all again in your next
	     * session.
	     * This is the default behaviour (traditional reasons), too.
	     * In case you don't have one of these functions, or you don't
	     * want to use them, 'write_history()' is called directly.
	     */
#if defined (HAVE_ATEXIT)
	    atexit(wrapper_for_write_history);
#elif defined (HAVE_ON_EXIT)
	    on_exit(wrapper_for_write_history);
#endif /* !HAVE_ATEXIT */

#endif /* HAVE_LIBREADLINE && GNUPLOT_HISTORY */

	    fprintf(stderr, "\nTerminal type set to '%s'\n", term->name);
	}			/* if (interactive && term != 0) */
    } else {
	/* come back here from int_error() */
	if (interactive == FALSE)
	    exit_status = EXIT_FAILURE;

#ifdef AMIGA_SC_6_1
	(void) rawcon(0);
#endif
	load_file_error();	/* if we were in load_file(), cleanup */
	SET_CURSOR_ARROW;

#ifdef VMS
	/* after catching interrupt */
	/* VAX stuffs up stdout on SIGINT while writing to stdout,
	   so reopen stdout. */
	if (gpoutfile == stdout) {
	    if ((stdout = freopen("SYS$OUTPUT", "w", stdout)) == NULL) {
		/* couldn't reopen it so try opening it instead */
		if ((stdout = fopen("SYS$OUTPUT", "w")) == NULL) {
		    /* don't use int_error here - causes infinite loop! */
		    fputs("Error opening SYS$OUTPUT as stdout\n", stderr);
		}
	    }
	    gpoutfile = stdout;
	}
#endif /* VMS */
	if (!interactive && !noinputfiles) {
	    term_reset();
#if defined(ATARI) || defined(MTOS)
	    if (aesid > -1)
		atexit(appl_exit);
#endif
	    exit(EXIT_FAILURE);	/* exit on non-interactive error */
	}
    }

    if (argc > 1) {
#ifdef _Windows
	int noend = 0;
#endif

	/* load filenames given as arguments */
	while (--argc > 0) {
	    ++argv;
	    c_token = NO_CARET;	/* in case of file not found */
#ifdef _Windows
	    if (stricmp(*argv, "-noend") == 0 || stricmp(*argv, "/noend") == 0)
		noend = 1;
	    else
#endif
	    if (strcmp(*argv, "-") == 0) {
		/* DBT 10-7-98  go interactive if "-" on command line */

		interactive = TRUE;
		/* will this work on all platforms? */

		while (!com_line());

/* interactive = FALSE; *//* should this be here? */

	    } else
		load_file(loadpath_fopen(*argv, "r"), *argv, FALSE);
	}
#ifdef _Windows
	if (noend) {
	    interactive = TRUE;
	    while (!com_line());
	}
#endif
    } else {
	/* take commands from stdin */
	while (!com_line());
    }

#if defined(HAVE_LIBREADLINE) && defined(GNUPLOT_HISTORY)
#if !defined(HAVE_ATEXIT) && !defined(HAVE_ON_EXIT)
    /* You should be here if you neither have 'atexit()' nor 'on_exit()' */
    * wrapper_for_write_history();
#endif /* !HAVE_ATEXIT && !HAVE_ON_EXIT */
#endif /* HAVE_LIBREADLINE && GNUPLOT_HISTORY */

    term_reset();

#ifdef OS2
    if (_osmode == OS2_MODE)
	RexxDeregisterSubcom("GNUPLOT", NULL);
#endif

#if defined(ATARI) || defined(MTOS)
    if (aesid > -1)
	atexit(appl_exit);
#endif
    exit(exit_status);
}

#if (defined(ATARI) || defined(MTOS)) && defined(__PUREC__)
int
purec_matherr(struct exception *e)
{
    char *c;
    switch (e->type) {
    case DOMAIN:
	c = "domain error";
	break;
    case SING:
	c = "argument singularity";
	break;
    case OVERFLOW:
	c = "overflow range";
	break;
    case UNDERFLOW:
	c = "underflow range";
	break;
    default:
	c = "(unknown error";
	break;
    }
    fprintf(stderr, "\
math exception : %s\n\
    name : %s\n\
    arg 1: %e\n\
    arg 2: %e\n\
    ret  : %e\n",
	    c,
	    e->name,
	    e->arg1,
	    e->arg2,
	    e->retval);

    return 1;
}
#endif /* (ATARI || MTOS) && PUREC */


/* Set up to catch interrupts */
void
interrupt_setup()
{
#ifdef __PUREC__
    setmatherr(purec_matherr);
#endif

    (void) signal(SIGINT, (sigfunc) inter);

#ifdef SIGPIPE
    /* ignore pipe errors, this might happen with set output "|head" */
    (void) signal(SIGPIPE, SIG_IGN);
#endif /* SIGPIPE */
}


/* Look for a gnuplot init file in current or home directory */
static void
load_rcfile()
{
    FILE *plotrc = NULL;
    char *rcfile = NULL;

    /* len of homedir + directory separator + len of file name + \0 */
    rcfile = (char *) gp_alloc((user_homedir ? strlen(user_homedir) : 0) + 1 + strlen(PLOTRC) + 1, "rcfile");

#ifdef NOCWDRC
    /* inhibit check of init file in current directory for security reasons */
#else
    plotrc = fopen(PLOTRC, "r");
#endif /* !NOCWDRC */

    if (plotrc == NULL) {
	if (user_homedir) {
	    strcpy(rcfile, user_homedir);
	    PATH_CONCAT(rcfile, PLOTRC);
	    plotrc = fopen(rcfile, "r");

#if defined(ATARI) || defined(MTOS)
	    if (plotrc == NULL) {
		char const *const ext[] = { NULL };
		char *ini_ptr = findfile(PLOTRC, user_gnuplotpath, ext);

		if (ini_ptr)
		    plotrc = fopen(ini_ptr, "r");
	    }
#endif /* ATARI || MTOS */
	}
    }
    if (plotrc)
	load_file(plotrc, rcfile, FALSE);

    free(rcfile);
}

static void
get_user_env()
{
    if (user_homedir == NULL) {
	const char *env_home;

#ifndef VMS
	if ((env_home = getenv(HOME)) || (env_home = getenv("HOME")))
#else
	if (env_home = HOME)
#endif
	    user_homedir = (const char *) gp_strdup(env_home);
	else
	    int_warn(NO_CARET, "no HOME found");
    }
    /* Hhm ... what about VMS? */
    if (user_shell == NULL) {
	const char *env_shell;

	if ((env_shell = getenv("SHELL")) == NULL)
#if defined(MSDOS) || defined(_Windows) || defined(DOS386) || defined(OS2)
	    if ((env_shell = getenv("COMSPEC")) == NULL)
#endif
		env_shell = SHELL;

	user_shell = (const char *) gp_strdup(env_shell);
    }
#if defined(ATARI) || defined(MTOS)
    if (user_gnuplotpath == NULL) {
	char *env_gpp;

	if (env_gpp = getenv("GNUPLOTPATH"))
	    user_gnuplotpath = (const char *) gp_strdup(env_gpp);
    }
#endif
}

/* expand tilde in path
 * path cannot be a static array!
 * tilde must be the first character in *pathp;
 * we may change that later
 */
void
gp_expand_tilde(pathp)
char **pathp;
{
    if (!*pathp)
	int_error(NO_CARET, "Cannot expand empty path");

    if ((*pathp)[0] == '~' && (*pathp)[1] == DIRSEP1) {
	if (user_homedir) {
	    size_t n = strlen(*pathp);

	    *pathp = gp_realloc(*pathp, n + strlen(user_homedir), "tilde expansion");
	    /* include null at the end ... */
	    memmove(*pathp + strlen(user_homedir) - 1, *pathp, n + 1);
	    strncpy(*pathp, user_homedir, strlen(user_homedir));
	} else
	    int_warn(NO_CARET, "HOME not set - cannot expand tilde");
    }
}


static void
init_memory()
{
    extend_input_line();
    extend_token_table();
    replot_line = gp_alloc(1, "string");
    *replot_line = NUL;
}


#ifdef OS2

int
ExecuteMacro(char *argv, int namelength)
{
    RXSTRING rxRc;
    RXSTRING rxArg[2];
    int rxArgCount = 0;
    char pszName[CCHMAXPATH];
    char *rxArgStr;
    short sRc;
    long rc;

    if (namelength >= sizeof(pszName))
	return 1;
    safe_strncpy(pszName, argv, namelength + 1);
    rxArgStr = &argv[namelength];
    RXSTRPTR(rxRc) = NULL;

#if 0
    /*
       C-like calling of function: program name is first
       parameter.
       In REXX you would have to use
       Parse Arg param0, param1
       to get the program name in param0 and the arguments in param1.

       Some versions before gnuplot 3.7pl1 used a similar approach but
       passed program name and arguments in a single string:
       (==> Parse Arg param0 param1)
     */

    MAKERXSTRING(rxArg[0], pszName, strlen(pszName));
    rxArgCount++;
    if (*rxArgStr) {
	MAKERXSTRING(rxArg[1], rxArgStr, strlen(rxArgStr));
	rxArgCount++;
    }
#else
    /*
       REXX standard calling (gnuplot 3.7pl1 and above):
       The program name is not supplied and so all actual arguments
       are in a single string:
       Parse Arg param
       We even handle blanks like cmd.exe when calling REXX programs.
     */

    if (*rxArgStr) {
	MAKERXSTRING(rxArg[0], rxArgStr, strlen(rxArgStr));
	rxArgCount++;
    }
#endif

    CallFromRexx = TRUE;
    rc = RexxStart(
		      rxArgCount,
		      rxArg,
		      pszName,
		      NULL,
		      "GNUPLOT",
		      RXCOMMAND,
		      NULL,
		      &sRc,
		      &rxRc);
    CallFromRexx = FALSE;

   /* am: a word WRT errors codes:
      the negative ones don't seem to have symbolic names, you can get
      them from the OREXX reference, they're not in REXX Programming Guide -
      no idea where to retrieve them from a Warp 3 reference ??
      The positive ones are somehow referenced in REXXPG
   */
    if (rc < 0) {
        /* REXX error */
    } else if (rc > 0) {
        /* Interpreter couldn't be started */
        if (rc == -4)
           /* run was cancelled, but don't give error message */
            rc = 0;  
    } else if (rc==0) {
        /* all was fine */
    }

/* We don't we try to use rxRc ?
   BTW, don't use free() instead since it's allocated inside RexxStart()
   and not in our executable using the EMX libraries */
   if (RXSTRPTR(rxRc))
       /* I guess it's NULL if something major went wrong,
          NULL strings are usually not part of the REXX language ... */
       DosFreeMem(rxRc.strptr);

   return rc;
}

ULONG
RexxInterface(PRXSTRING rxCmd, PUSHORT pusErr, PRXSTRING rxRc)
/*
   ** Rexx command line interface
 */
{
    int rc;
    static jmp_buf keepenv;
    int cmdlen;

    memcpy(keepenv, command_line_env, sizeof(jmp_buf));
    if (!SETJMP(command_line_env, 1)) {
	/* Set variable input_line.
	   Watch out for line length of NOT_ZERO_TERMINATED strings ! */
	cmdlen = rxCmd->strlength + 1;
	safe_strncpy(input_line, rxCmd->strptr, cmdlen);
	input_line[cmdlen] = NUL;
	rc = do_line();
	*pusErr = RXSUBCOM_OK;
	rxRc->strptr[0] = rc + '0';
	rxRc->strptr[1] = NUL;
	rxRc->strlength = strlen(rxRc->strptr);
    } else {
/*
   We end up here when bail_to_command_line() is called.
   Therefore sometimes this call should be avoided when
   executing a REXX program (e.g. 'Cancel' from
   PM GUI after a 'pause -1' command)
*/
	*pusErr = RXSUBCOM_ERROR;
	RexxSetHalt(getpid(), 1);
    }
    memcpy(command_line_env, keepenv, sizeof(jmp_buf));
    return 0;
}
#endif

#if defined(HAVE_LIBREADLINE) && defined(GNUPLOT_HISTORY)
static void
wrapper_for_write_history()
{
#if 1
    /* Alternative code, saves one disk access */
    if (history_is_stifled())
	unstifle_history();
    if (gnuplot_history_size >= 0)
	stifle_history (gnuplot_history_size);

    /* returns 0 on success */
    if (write_history(expanded_history_filename))
	fprintf (stderr, "Warning:  Could not write history file!!!\n");

    unstifle_history();
#else
    /* if writing was successful, truncate history
     *  to gnuplot_history_size lines
     */
    if (write_history(expanded_history_filename)) {
	if (gnuplot_history_size >= 0)
	    history_truncate_file(expanded_history_filename, gnuplot_history_size);
    }
#endif
}
#endif /* HAVE_LIBREADLINE && GNUPLOT_HISTORY */

int ignore_enhanced_text = 0;
