The modifications to gnuplot 3.2 in this directory add the
capability of plotting probability distributions.

This mod adds the following extra standard gnuplot functions:

ibeta   - incomplete beta function
igamma  - incomplete gamma function
lgamma  - natural log of gamma
gamma   - gamma function
erf     - error function (= trivial variation of infamous bell curve)
erfc    - 1.0 - error function (more accurate than 1.0 - erf yourself)
rand    - pseudo random number generator (in compliance with Knuth)

using these things as building blocks, a gnuplot command file 'stat.inc'
is provided with definitions of almost all statistical distributions.

Author: Jos van der Woude, jvdwoude@hut.nl

==========================================================
Directions to install gnuplot 3.3:
(diffs are relative to gnuplot 3.0, patchlevel 2.0)

If you have a BSD machine, add the following to
the makefile.unx:

OPTIONS = -DERF -DGAMMA

and specfun.c to the SOURCES and specfun.o to the OBJECTS.

For MSDOS machines (and non-BSD machines) do the following:

ASSUMPTIONS:
The diffs provided work for msdos based pc's, using turbo C, version 2.0,
and Borland C++, version 2.0 or 3.0.
If you use a different setup, you might have to make (some) changes.

GOAL:
To extend the standard function library of gnuplot 3.2 with the following
functions:

ibeta    - incomplete beta function
igamma   - incomplete gamma function
erf()    - error function (= trivial variation of normal distribution function)
erfc()   - 1.0 - erf() (more accurate than computing 1.0 - erf() yourself)
gamma()  - gamma function (for entire domain!)
lgamma() - natural logarithm of gamma function
rand()   - a statistical acceptable version of standard C rand() function

These additions to the repertoire of standard functions turn gnuplot into
an excellent tool for plotting statistical density and distribution functions.

DETAILED INSTRUCTIONS:
0. Split the bottom part of this file into the appropriate files:
   readme.p3, patch3.dif, stat.inc, nomo95.dem, random.dem, prob.dem,
   prob2.dem, and specfun.c using an editor or a PD PC shar utility.

1. Use the patch program to generate new versions of:
   gnuplot.doc, command.c, internal.c, misc.c, parse.c, plot.c, plot.h,
   standard.c, util.c and version.c

2. Make sure that file specfun.c is in the gnuplot directory.

3. Use the make program to create new version of gnuplot.

4. Test the new functions with commmand files prob.dem, prob2.dem,
   random.dem and nomo95.dem.

KNOWN BUG:
On MSDOS machines this modification can result in a DGROUP segment overflow
error at link time. The reason is that gnuplot uses the resources of the
large memory model right up to the limit.
There are two things you can do about this:

1. Compile gnuplot using the huge memory model.
   This was tested using Borland C++, version 2.0: Works fine, even with
   -DREADLINE enabled.

2. If you dont want to compile under the huge memory model you can try to
   disable unused terminal drivers in term.h. See instructions at top
   of file term.h.


Happy plotting,

Jos van der Woude
Amsterdam, The Netherlands
jvdwoude@hut.nl
