/*
 * Only for building on VMS
 */
#ifndef GNUPLOT_VMS_H
#define GNUPLOT_VMS_H

#ifdef VAXC            /* replacement values suppress some messages */
#   ifdef  EXIT_FAILURE
#    undef EXIT_FAILURE
#   endif
#   ifdef  EXIT_SUCCESS
#    undef EXIT_SUCCESS
#   endif
#endif /* VAXC */
#ifndef  EXIT_FAILURE
#  define EXIT_FAILURE  0x10000002
#endif
#ifndef  EXIT_SUCCESS
#  define EXIT_SUCCESS  1
# endif

#ifdef FOPEN_BINARY
# undef FOPEN_BINARY
#endif
# define FOPEN_BINARY(file) fopen(file, "wb", "rfm=fix", "bls=512", "mrs=512")

/*
 * exported by vms.c
 */
extern int vms_vkid;	/* Virtual keyboard id */
extern int vms_ktid;	/* key table id, for translating keystrokes */

/*
 * Jun 2023: moved from plot.c when vms_init_screen moved to vms.c
 */
# ifndef __GNUC__
#  include <unixio.h>
# endif
# include <smgdef.h>
extern smg$create_virtual_keyboard();
extern smg$create_key_table();

/*
 * vms-specific prototypes
 */
char *vms_init();
void vms_reset();
void term_mode_tek();
void term_mode_native();
void term_pasthru();
void term_nopasthru();
void fflush_binary();
void done(int);
void vms_init_screen();

#ifdef PIPES
  FILE *popen(char *, char *);
  int pclose(FILE *);
#endif

#endif /* GNUPLOT_VMS_H */
