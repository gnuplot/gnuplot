/*
 *  Modified version of 1.1.0 gnuplot by Thomas Williams and Colin Kelley.
 *  "You may use this code as you wish if credit is given and this message
 *  is retained."
 *
 *  Our "use" of this code has been to add a terminal driver (att 3b1),
 *  improve the IBM-PC drivers using TURBO-C routines, throw in a
 *  different help system (the one we got didn't seem to work) and add
 *  the commands SET POLAR, SET OFFSETS, and PAUSE.
 *
 *  Files that were changed from the original 1.1.0 version:
 *  command.c, graphics.c, misc.c, plot.c, term.c and version.c.
 *
 *  The annoying problem with unixplot files not being redirected by
 *  the set output command was fixed and an ``init()'' routine was
 *  added to term.c to allow an implementation to detect the terminal
 *  type.  (Currently only used by TURBO-C and VMS.)  The file term.c
 *  was further changed by breaking it into a number of .TRM files
 *  (Makefile was changed accordingly).
 *
 *  A bug in do_plot() was fixed as well as a VMS bug that caused
 *  SET OUTPUT to fail.  A final departure from the original 1.1.0
 *  version was the addition of Jyrki Yli-Nokari's HP laser jet
 *  drivers to term.c.
 *
 *  John Campbell  CAMPBELL@NAUVAX.bitnet (3b1, polar, offsets, pause)
 *  Bill Wilson    WILSON@NAUVAX.bitnet   (TURBO-C IBM-PC drivers)
 *  Steve Wampler  ...!arizona!naucse!sbw (help system acquisition)
 */
char version[] = "1.1.0A (Polar)";
char date[] = "Thu May 18 21:57:24 MST 1989";
