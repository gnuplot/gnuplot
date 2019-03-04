/*
 * Only for building on VMS
 */
#ifndef GNUPLOT_VMS_H
#define GNUPLOT_VMS_H

char *vms_init();
void vms_reset();
void term_mode_tek();
void term_mode_native();
void term_pasthru();
void term_nopasthru();
void fflush_binary();

#ifdef FOPEN_BINARY
# undef FOPEN_BINARY
#endif
# define FOPEN_BINARY(file) fopen(file, "wb", "rfm=fix", "bls=512", "mrs=512")

#endif /* GNUPLOT_VMS_H */
