
The Gnuplot Plotting Utility
****************************

Gnuplot is a command-line driven interactive function plotting utility
for UNIX, MSDOS, VMS, and many other platforms.  The software is copyrighted
but freely distributed (i.e., you don't have to pay for it).  It was
originally intended as graphical program which would allow scientists
and students to visualize mathematical functions and data.  Gnuplot
supports many different types of terminals, plotters, and printers
(including many color devices, and pseudo-devices like LaTeX) and is
easily extensible to include new devices.  [ The "GNU" in gnuplot is
NOT related to the Free Software Foundation, the naming is just a
coincidence (and a long story; see the gnuplot FAQ for details). Thus
gnuplot is not covered by the GNU copyleft, but rather by its own
copyright statement, included in all source code files.]

Gnuplot handles both curves (2 dimensions) and surfaces (3
dimensions). Surfaces can be plotted as a mesh fitting the specified
function, floating in the 3-d coordinate space, or as a contour plot
on the x-y plane. For 2-d plots, there are also many plot styles,
including lines, points, lines with points, error bars, impulses and
(optionally filled) boxes.  Graphs may be labeled with arbitrary
labels and arrows, axis labels, a title, date and time, and a key.
The interface includes command-line editing and history on most
platforms.

The new gnuplot user should begin by reading the general information
available by typing `help` after running gnuplot. Then read about the
`plot` command (type `help plot`).  The manual for gnuplot (which is a
nicely formatted version of the on-line help information) can be
printed either with TeX, troff or nroff.  Look at the docs/Makefile
for the appropriate option.

 The gnuplot source code and executables may be copied and/or modified
freely as long as the copyright messages are left intact.

Copyright and Porting
=====================

 See the Copyright file for copyright conditions.

 See the ChangeLog and docs/old/History.old file for changes to gnuplot.

 Build instructions are in the INSTALL file.  Some additional
informations needed to port gnuplot to new platforms not covered by
GNU autoconf can be found in the PORTING file. 

 The code for gnuplot was written with portability in mind; however, the
new features incorporated from v3.7 on were developed with ANSI-C compliant
compilers, and it was deemed expedient to restrict the development effort
to that environment, with the consequence that systems that could build
previous gnuplot releases may require some source modifications.  gnuplot
has been tested on the following systems (incomplete), but be aware that
this list is not necessarily up to date with regard to the current version
of gnuplot:

 o Sun3, sun4c, sun4m, sun4u (SunOS 4.03, SunOS 4.1.x, Solaris 2.5.1/2.6/2.7)
 o uVAX 3100 (VMS 5.4, 5.5, 6.0, 6.1)
 o VAX 6410 (VMS 5.2)
 o DECStation 5000/200PXG (ULTRIX V4.1)
 o DEC AXP 3400 (VMS/OpenVMS 1.5, 6.2)
 o DEC AXP 600, A1000A, 3000 (DEC Unix 3.2 4.0, Windows NT 4.0 MSVC++ 4.0)
 o IBM PC and compatibles (MS-DOS 3.3/5.0 32-bit DJGPP,
   OS/2 2.x, Linux 2.x SVGA X11, Windows 9x, ME, Windows NT, 2K, XP)
 o IBM AIX 3.x and 4.x
 o CBM Amiga (AmigaOS 1.3, 2.x and 3.x, SAS/C 6.2 and better, Aztec C
   beta 5.2a, gcc; Linux 2.x, NetBSD-1.3.x)
 o IRIS 4D/70G and 4D/25G with MIPS C
 o NeXT with gnu C 1.34
 o AT&T 3B1 (version 3.51m with cc and gcc 1.39)
 o Apollo's (DomainOS SR10.3 BSD4.3 with C compiler 68K Rev 6.7(316))
 o HP 300 (m68k) and HP 700 (PA-RISC) (HP-UX 9.x 10.x)
 o SGI Challenge and O2 (Irix 6.2, 6.3)

 gnuplot has not been tested on Pyramid 90x.  You can expect that gnuplot
will compile more or less out of the box on any system which has a newer
(2.x) version of the GNU C compiler gcc, or is compliant with current
POSIX/Unix standards.


Help and Bug Reports
********************

 Your primary place to go searching for help with gnuplot should
be the project's webpage.  At the time of this writing, that's

	http://gnuplot.sourceforge.net

 It has links to a lot of material, including the project's development
page, also at SourceForge:

	http://sourceforge.net/projects/gnuplot/

 Note that since gnuplot has nothing to do with the GNU project, please
don't ask them for help or information about gnuplot; also, please
don't ask us about GNU stuff.

 Please note that all bug reports should include the machine you are
using, the operating system and it's version, plotting devices, and
the version of gnuplot that you are running.  If you could add such
information to any messages on the Usenet newsgroup or the other
mailing lists and trackers, that'd be nice, too.

 
Usenet
======

 Additional help can be obtained from the USENET newsgroup

        comp.graphics.apps.gnuplot.

 This newsgroup is the first place to ask for routine help.  It used to be
gatewayed bi-directionally to the info-gnuplot mailing list, but that
had to be stopped when we moved the mailing lists off Dartmouth.


Mailing Lists
=============

 As of gnuplot-4.0, the gnuplot mailing lists have moved away from
their old home at Dartmouth College (thanks, guys!) to the project's
general new development site provided by SourceForge.net.  To 
subscribe to these new mailing lists, use the Web Interface provided
by SourceForge.net:
	
	http://sourceforge.net/mail/?group_id=2055

 The main lists you may be interested in are "gnuplot-info" and 
"gnuplot-bugs".  "gnuplot-info" is for general discussion and
questions about how to use the program.  But as noted above,
using the Usenet newsgroup for this kind of communication is 
almost certainly better both for you and for us.

 "gnuplot-bugs" is NOT an appropriate place to ask questions on how to
solve a gnuplot problem or even to report a bug that you haven't
investigated personally.  It is far more likely you'll get the help
you need for this kind of problem from comp.graphics.apps.gnuplot
or the gnuplot-info mailing list.

 Using "gnuplot-bugs" is also slightly disfavoured, because it makes it
hard for us to keep track what bugs are currently under investigation,
and hard for you to check if maybe the bug you've found has already
been reported by somebody else before.  We would thus like to ask to
you please use the "Bug Tracker" system that is part of gnuplot's
development web site at SourceForge.net instead of this mailing list.

 If you found a fix already, pleast post it in "diff -c" or "diff -u"
format done against the most current official version of gnuplot or
the latest alpha or beta release of the next version.  All major
modifications should include documentation and, if new features were
added, a demo file.  Finally, it is much easier to integrate smaller
stepwise modifications rather than one gigantic diff file which
represented months of changes.  

 There are separate tracking systems for Feature Requests and proposed
patches that implement new features, also hosted at SourceForge.  

 Discussions about plans for new features or other significant changes
should be announced and discussed on the developers' mailing list,
gnuplot-beta, which is also hosted by SourceForge.net.


Where to get updates to gnuplot
===============================

 Congratulations on getting this version of gnuplot! Unfortunately, it
may not be the most recent version ("you never know where this version
has been!"). You can be sure that it IS the most recent version by
checking one of the official distribution sites, guaranteed to be kept
up to date (of course, if you got this file from one of those sites,
you don't need to check!).

 To hear automatically about future releases (and other gnuplot news),
read the newsgroup; see above.

Distribution sites
******************

 In general, gnuplot 4.0 is available as the file gnuplot-4.0.0.tar.gz.
Because of the numerous changes since version 3.7, no patch files will
be available to bring 3.7 to 4.0.  It will be made available at web
and ftp sites listed below, along with DOS (32-bit, 386 or higher)
Windows (32-bit, i.e. the 9x and NT families) and OS/2 2.0
executables.  Amiga binaries will be made available on Aminet.
Macintosh binaries are planned to appear on the official gnuplot sites
at a later time.

 Also, some sites will have a special documentation tarball which
contains PostScript and other versions of the manuals and tutorials
for printout or installation in your system.

Please obtain gnuplot from the site nearest to you.

NORTH AMERICA:

     Anonymous ftp to ftp.gnuplot.vt.edu:
     ftp://ftp.gnuplot.vt.edu/pub/gnuplot/gnuplot-4.0.0.tar.gz
     ftp://ftp.gnuplot.info/pub/gnuplot/gnuplot-4.0.0.tar.gz

     Alternatively, you can use your web browser to get gnuplot
     from the gnuplot web pages at 
     http://sourceforge.net/projects/gnuplot/files

     Anonymous ftp to ftp.dartmouth.edu. Please try the
     other sites first!
     ftp://ftp.dartmouth.edu/pub/gnuplot/gnuplot-4.0.0.tar.gz

AUSTRALIA:

     From the AARNet Mirror Project:
     ftp://mirror.aarnet.edu.au/pub/gnuplot/
     http://mirror.aarnet.edu.au/pub/gnuplot/

     Anonymous ftp to ftp.cc.monash.edu.au:
     ftp://ftp.cc.monash.edu.au/pub/gnuplot/gnuplot-4.0.0.tar.gz


EUROPE:

     Anonymous ftp to ftp.irisa.fr:
     ftp://ftp.irisa.fr/pub/gnuplot/gnuplot-4.0.0.tar.gz

     Anonymous ftp to ftp.ucc.ie:
     ftp://ftp.ucc.ie/pub/gnuplot/gnuplot-4.0.0.tar.gz

OTHER:

     The CTAN mirror network has gnuplot in its "graphics" area
     http://www.ctan.org/

     Source and binary distributions for the Amiga are on Aminet in
     http://ftp.wustl.edu/~aminet/gfx/misc/ and its mirrors.

----

                                        -Thomas Williams-
                                        -Alex Woo-
                                        -David Denholm-
                                        -Lars Hecking-
