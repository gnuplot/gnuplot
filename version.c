#ifndef lint
static char *RCSid = "$Id: version.c,v 1.347 1998/06/22 12:24:56 ddenholm Exp $";
#endif

/* GNUPLOT - version.c */

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
 * The rcs log will be removed for `real' releases
 *
 * A note about version numberering: Since RCS is notable faster when using
 * trunk revisions (x.y), I have decided simply to start with revs 1.x
 *
 * There is a small bug in CVS that prevents the commit -r option from
 * working. This means that the revision numbers in the individual files
 * do not agree with the one in the log. I hope to fix this some time soon.
 *
 * $Log: version.c,v $
 * Revision 1.347  1998/06/22 12:24:56  ddenholm
 * dd : retrieve fit.*, matrix.*, docs/latextut/makefile from earlier vsns
 *      add  set noxdtics etc. (behave same as  set xtics)
 *
 * Revision 1.346  1998/06/18 14:55:21  ddenholm
 * Lars : compilation of many patches posted to the net.
 *
 * Revision 1.345  1998/05/19 18:05:03  ddenholm
 * drd: add missing stdfn.c file
 *
 * Revision 1.344  1998/04/14 00:38:57  drd
 * drd: oops - forgot to bump version number with last checkin.
 *
 * Revision 1.343  1998/03/22  23:31:31  drd
 * hbb: compile on win16 ; latest and greatest hidden3d stuff
 *
 * Revision 1.342  1998/03/22  22:32:24  drd
 * drd: update copyright statements on most files. There are still a
 *      few files (obscure terminal drivers) copyright others. Maybe
 *      we have to drop these, or at least list the files which
 *      cannot be redistributed with a modified gnuplot.
 *      (Or maybe it is okay to include these files unchanged in an
 *       otherwise modified gnuplot ?)
 *
 * Revision 1.341  1997/12/16  23:01:05  drd
 * Lars Hecking: 0PORTING, 0README, 0INSTALL
 * HBB: latest hidden3d stuff
 *      off-by-one error in long-line datafiles
 * gas@SLAC.stanford.edu : fix for \" inside "strings"
 * Hans-Martin Keller : fix for fault in contour stuff
 * Niels Provos : VMS stuff
 *
 * Revision 1.340  1997/11/25  23:03:09  drd
 * Alexander Mai: fix show output for non-auto x range
 * Konrad Bernloehr: fixes for logscale tics
 *                   pointsize for fig
 * Thomas Witelski: set border  in save output
 * HBB: key entry for impulse plot
 * Dick Crawford: recent (latest ?) docs
 * drd: set cntrparam ; ...
 *      fix y2label placement (I hope)
 *
 * Revision 1.339  1997/10/10  06:51:56  drd
 * drd: very minor tweaks : fix the longjmp on os/2 ; fix the problem
 *      with time on last column in data file ; make lmargin and tmargin
 *      of 0 actually work
 *
 * Revision 1.338  1997/07/23  23:16:36  drd
 * drd: fix autodetection of libgd - inverted logic meant that !=no meant
 *        yes. Now !=yes means no, since it returns blank for no
 *      swap  axes  and  smooth  keywords on plot line, since smooth
 *        is really a data modifier
 *      fix the   set key width  stuff so that the text is within a
 *        key box for any combination of key reverse Left|Right
 *
 * Revision 1.337  1997/07/22  23:20:54  drd
 * erik@tntnhb3.tn.tudelft.nl : conform to postscript DSC
 * HBB, Lars Hecking : -Xc for solaris compile and other config changes
 * ueda@syslab.lias.flab.fujitsu.co.jp : gplt_x11.c includes config.h
 *       fix script line number after inline data
 * Roger Fearick : os/2 patches
 * jgrant@traveller.com: gplt_x11.c fix to drain _all_ X events after a select()
 *       fix endianness problem in cgm driver
 * Carsten Steger : fixes for clean build on amiga
 * David Schooley : ai term updates
 * M.N.Schipper@IRI.TUDelft.NL : workaround for broken sscanf(%n) on os9
 * William Kirby : double->coordtype in interpol.c
 * cb203@cus.cam.ac.uk: documentation : -noraise instead of +raise
 * drd: localise use of <setjmp.h> to plot.c and parse.c, for png on linux
 *
 * Revision 1.336  1997/06/04  05:48:01  drd
 * drd: put back setvbuf(...,1024)
 *      (trivial) make show at $3 work properly
 *      table output uses %g not %f for contours
 *
 * Revision 1.335  1997/05/27  01:29:03  drd
 * HBB: change number of colours in pbm.trm
 *      new hidden3d.c and fit demo/docs
 * Alexander Mai: os/2 patches (sorry I keep missing os/2 patches)
 * Lars Hecking: autoconf changes for png, and more configure improvements
 * drd: space in timefmt means 0 or more spaces. Add read of %b / %B
 *      tweak the windows /noend stuff
 *
 * Revision 1.334  1997/05/19  08:10:25  drd
 * Dick Crawford : set key width  [but I still think it should be absolute,not relative]
 * drd: review the splot key stuff :
 *        set key box ; set contour ; splot x,y  overran the key box
 *        splot x title ''  was not same as  splot x notitle
 *
 * Revision 1.333  1997/05/18  21:22:14  drd
 * Dick Crawford : rmargin/y2label bug
 *                 Latest docs
 * Joost Witteveen: configure bug
 * Lars Hecking: setvbuf workaround for win32
 * HBB : don't fit when ndata < nparams
 * HBB, Berthold Hoellmann : docs/makefile deps
 * Novak Levente: tgif upgrade
 * drd: fix pslatex aux again
 *      fix  show xdata
 *      support  %02H in time formats : allows suppression of leading 0's
 *
 * Revision 1.332  1997/04/12  05:44:01  drd
 * HBB: FIT_MAXITER patch   ; drd added it to fit.dem and gnuplot.doc
 * drd: change hidden3d.c to not draw diagonals by default
 *      trap seg.fault in  set term pslatex aux ; set out ; plot x
 *      Mention gif and png in 0INSTAL. Output a brief message when
 *       configure fails to detect the libraries.
 *      Add mention of AIX 4 NaNq bug to gnuplot.doc
 *
 * Revision 1.331  1997/04/10  02:33:29  drd
 * Dick Crawford: fix broken   set label ... font ...
 * Lars Hecking: many cleanups
 * Alexander Mai: os/2 changes
 * drd: trap EOF in fit intr. handler
 *      warn if udf shares name with internal func
 *      doc and copyrights
 *
 * Revision 1.330  1997/04/08  06:14:18  drd
 * Lars Hecking, drd: update command takes optional second parameter
 * Roger Fearick: os2 patches ("os2237")
 *
 * Revision 1.329  1997/04/05  19:44:29  drd
 * Ian MacPhedran, Konrad Bernloehr: fig driver patch
 * Berthold Hoellmann: support --with-gd etc in configure
 *                     patches for hidden demos
 * John Grant : fix unitialised variable in gplt_x11
 * HBB : latest hidden3d (25/3/97)
 * Nelson Beebe : fix some typos
 * Gary Holt: guard configure --with-gnu-readline=/bogus/path
 * drd: make non-rotated labels the default
 *      fix up multilined title and x2label
 *      print extended-printf %c as e+/-nn for outrange powers
 *      (forgot to record in 322) : trap top-bit-set chars in bitmap.c
 *      configure --with-gnu-readline  should not assume -ltermcap (Debian Linux)
 *
 * Revision 1.328  1997/03/09  23:50:19  drd
 * drd: detect top-bit-set characters in bitmap.c
 *      detect gd lib in autoconf
 *      put writeback back into plot2d.c : deleted for some reason..?
 * AL:  NeXT fixes, plus some tidying.
 * Roger Fearick, HBB : column check in fit.c
 * HBB: latest hidden3d.c (wasn't latest last time)
 * Stefan Bodewig: doc2xxxx for multiple top-level entries
 * Alexander Jolk: error in %c in extended printfs
 *
 * Revision 1.327  1997/03/06  06:04:14  drd
 * drd: add support for dates in user-tics ie set xtics ('format' 'place',...)
 *
 * Revision 1.326  1997/03/04  07:58:15  drd
 * Alexander Mai : gif to os2 stdout
 * Carson Wilson,drd: fix off-by-one in days for xdtics and %a
 * CMWestern,William Kirby: win32 build with bc
 * Dick Crawford : rotate patch
 * HBB: latest hidden3d code (with drd's OUTRANGE tweak)
 * James Obrien,drd : don't write to readonly string returned by xlib
 * drd: change message when range is less than zero
 *      show as comment the current range hidden by auto
 *      fix output to PRN in windows
 *      latex multiplot uses original size for bounding box
 *      temporary fix for VMS help : allow only one toplevel help entry
 *
 * Revision 1.325  1997/01/05  22:02:42  drd
 * Johannes.Hoffstadt,drd: free() bug in fit.c
 * William Kirby: undefined tmpn in fit.c
 * Carsten Steger: clean compile on HP-UX & Irix 6.2
 * Robert Lutwak: avoid ObjC keywords in cgm.trm
 * Jim Van Zandt: linwidth for cgm
 * Alexander Mai: readline fix for os/2
 *                makefile.os2 update
 * bjr@ln.nimh.nih.gov : take out redundant cast which upsets SGI
 * Lucas Hart : titles and crosshair in multiple X11 windows
 *              nameless mailbox for VMS x11
 *              mgr.dem update
 * C.M.Western@bristol.ac.uk : win95 fixes
 * drd: (trivial) accept vectors (plural) as line style
 *
 * Revision 1.324  1996/12/19  20:25:30  drd
 * Matthew Jackson (and others): segv on linux with  set locale
 * William Kirby: win32 has long filename, but win32s does not : fix update
 * Jim Van Zandt : allow more contour levels
 * G.Allen Morris III: texdraw fix
 * Alexander Mai: makefile.os2
 * David Schooley: mac patches
 *
 * Revision 1.323  1996/12/19  19:12:50  drd
 * Roger Fearick/Alexander Mai : doc2ipf changes
 * HBB: pointsize bug in hidden splot (?)
 * Carsten Steger: tweaks for compile on amiga
 * drd: eliminate some gcc -W... warnings (mostly prototypes)
 *      tidy use of lp in plot3d.c and set.c
 *      add some comments to term.c, and make term_suspend static
 *      set missing  was completely broken (df_column->good is not boolean)
 *      show contour says   approximately n levels
 *      off-by-1 error in incremental contour levels : last one was not shown
 *
 * Revision 1.322  1996/12/09  15:57:59  drd
 * drd: config tweaks
 *      gif output on stdout
 *      invnorm(0) on aix
 *
 * Revision 1.321  1996/12/08  13:08:56  drd
 * Lars Hecking : extensive changes to autoconf and various portability
 *                problems addressed. But is  ifdef __STDC__ enough,
 *                or do we need if defined(__STDC__) && __STDC__  ?
 *
 * Revision 1.320  1996/12/08  12:28:48  drd
 * Alexander Mai, Roger Fearick : more os/2 changes
 * Joerg, drd : font 'name,size' for enhanced postscript
 * drd : more checking in checkdoc, with file/line recording
 *       latest FAQ
 *
 * Revision 1.319  1996/11/26  07:15:19  drd
 * Alex Jolk: fix scientific formats %s/%S for -ve powers
 * Roger Fearick: os/2 fixes
 *
 * Revision 1.318  1996/11/25  22:37:40  drd
 * drd: accept '% g' for format
 *      tolerate pressing ^c while in help
 *      both x11 and X11 terminals accept options
 *      set nomxtics ; save 'all.gnu' no longer outputs 'set no mxtics'
 *      move reopen of output file into term_init so that cgm output is not
 *        truncated when terminal is subsequently changed
 *      $0 in plot 'file' resets to 0 after an index change
 *      column(-1) works same as  plot 'file' using -1:1
 *       - so plot 'file' using ($0+column(-1)):1 skips an x at blank lines
 *      set contour ; splot 0  no longer faults
 *      splot '/dev/null' binary   no longer faults
 *      bf_test opens files "wb" on all platforms
 *
 * Revision 1.317  1996/11/24  16:47:13  drd
 * drd: fix polar-mode tics when rmin != 0
 *      splot f(x,y)=x*y, f(x,y) faulted
 *       - scoping problem in plot3d.c : I blame the inconsistent indentation
 *      add tkcanvas driver (see file for authors)
 * Ed.Breen@dms.csiro.au clear for x11
 * Dick Crawford: latest doc changes
 * dkh: buildvms.com fix
 *
 * Revision 1.316  1996/10/21  19:51:18  drd
 * drd: fix radial grid lines in polar mode
 *      fix seg fault with  set dgrid ; splot '/dev/null'
 *      pointsize and wide lines for pslatex
 *      plot ... linestyle was not being accepted
 * HBB: remove 32k limit for DOS32
 * Emmanuel Bigler : vector demo (for x11 only)
 * Avinoam Kalma, drd:  show action_table
 * Joerg : more consistent linewidth stuff
 *         save cleanup ; add comments
 *
 * Revision 1.315  1996/09/29  21:12:41  drd
 * drd: no ! after a shell command, except in interactive mode
 *      fix polar mode bug
 *      use fancy formats in user-defined tics
 *
 * Revision 1.314  1996/09/23  20:55:03  drd
 * drd: try acos again : acos(-2) _is_ defined (but complex)
 *      post.trm was using pointsize directly, hence everything scaled twice
 *      - next.trm looks like it was doing the same
 *      define null pointsize routine to set scale for generic do_point
 *      test now fflushes (actually, uses term_start_plot and term_end_plot)
 *
 * Revision 1.313  1996/09/22  21:08:28  drd
 * drd: include time.c in buildvms.com
 *      fix latex character width (again)
 *      fix problem with linetypes not matching key on post.trm
 *      anticipate -ve log in acos()
 * Carsten Steger: amiga fixes
 *
 * Revision 1.312  1996/09/18  23:15:09  drd
 * drd: fix font sizes in latex term
 *      try to work around assertion failure with x1y2 axes in parametric
 * Roger Fearick: pm.trm tweaks
 * Martijn Schipper: os9 changes
 *
 * Revision 1.311  1996/09/16  20:22:18  drd
 * drd: fix seg fault with fig term on sunos
 *      -DNOGAMMA if neither lgamma nor gamma defined
 *      truncate term_options before call term->options
 * Gary Holt, drd: outstr stored twice in save : now only a comment is output
 * W.Geppert: doc changes
 *
 * Revision 1.310  1996/09/15  13:04:06  drd
 * HBB: hidden3d fix
 * gary Holt: configure --with-gnu-readline[=path]
 * Dick Crawford: latest docs
 * drd: file copyrights, comp.graphics.apps.gnuplot
 *
 * Revision 1.309  1996/09/11  23:39:56  drd
 * drd: try again to get the line property keywords right
 *      remove duplicate pointsize entry points from term structure, and
 *      reimplement set pointsize via line property stuff
 *      implement line width for x11
 *      fix "unknown type in real()" from  plot '<echo 0 0' thru 1/x
 *
 * Revision 1.308  1996/09/10  22:08:28  drd
 * Alexander Jolt: relative errors displayed for fit
 * Roger Fearick: more os2 changes
 * drd: change "" to '' in latextut/*.plt
 *      fix typo in hidden3d.c
 *      tweaks to term/*.trm to make gnuplot.tex build cleanly
 *
 * Revision 1.307  1996/09/10  21:07:11  drd
 * drd: de-ANSI-fication for sun cc
 *      fix problem in df_readline as shown up by 'purify'
 *      linewidth tweaks
 *      make next.trm conformant with termdoc.c
 *      also, remove some unbalanced ' in strings that were upsetting cpp ?
 *      move .NOEXPORT from top of docs/Makfile.in and latextut/...
 * Steve Stuart: typo in makefile.unx
 *
 * Revision 1.306  1996/09/08  20:03:33  drd
 * drd: tweaks to lp patch (parsing as #defines in plot.h, not lp_parse.inc)
 *      try again to tolerate "{/Symbol hello\nworld}" in enhpost
 * Topher Cawlfield: histeps style (at last)
 *
 * Revision 1.305  1996/09/05  21:16:21  drd
 * Joerg: linewidth patch (with some gratuitous drd tweaks !)
 *
 * Revision 1.304  1996/09/05  19:45:50  drd
 * drd: run configure.in through autoconf 2.10
 *      fix autoranging bug in binary splot
 * HBB: fix 'update' and demo
 *
 * Revision 1.303  1996/09/04  21:15:35  drd
 * drd: comments in .doc file, so I can add an RCS ID
 *      don't assert fail with  plot 'file' binary
 * HBB: FSF-compliant Makefile.in, but not yet since my autoconf is old
 *      hidden-friendly demo/whale.dat
 *      multiplot clear  for djsvga
 *      detect candlesticks/financebars in splot
 * Konrad Berloehr: fig.trm revisions
 * Thomas Koenig: on-the-fly postscript non-reencoding
 *
 * Revision 1.302  1996/09/02  22:36:55  drd
 * drd, William Kirby: more problems with change to outstr
 * drd: save of xdata time was broken
 * Dick Crawford: set key samplen and spacing
 * drd: axis alias for axes in  plot ... axes ...
 * HBB: hidden3d typo fix
 * Konrad Bernloehr: fig driver changes
 * Alex Woo: gif driver - user-specified sizes
 * drd: oops - put in pm.trm.rej that I missed last time
 *
 * Revision 1.301  1996/08/15  00:04:44  drd
 * drd: use AC_CHECK_FUNCS for strnicmp, ..., and update makefiles
 *      tolerate no closing backquote at end of line
 *      no limit on output filename
 *      main is int in doc2* funcs
 * Robert Lutwak: NeXT stuff (at last - sorry !)
 *
 * Revision 1.300  1996/08/13  21:48:13  drd
 * drd: dont check for #warn - solaris cant do it, but we dont use it
 *      *at last* - plot ... axes x1y2 ...
 *      typo in recent change to parse.c :-(
 *      (de-ansify macros in hidden3d.c in 299 but I forgot to note it)
 *
 * Revision 1.299  1996/08/12  21:36:10  drd
 * Steve Turner, drd : eliminate warnings from enthusiastic SGI compiler
 * Santiago Vila Docel: more VPATH makefile tweaks
 * Alex Woo,Roger Fearick: gnuplot.doc error
 * Roger Fearick: os/2 patches
 * drd: infinite loop in enhpost with spurious '}' in text
 *
 * Revision 1.298  1996/08/11  21:56:20  drd
 * Pieter Vosbeek: gnu readline
 *                 pslatex bounding box
 * David Schooley: mac patches
 * Santiago Vila Doncel: mkinstalldirs
 * Alexander Lehmann: PNG terminal changes
 * HBB: hidden3d tweaks
 *      fit sanity checks
 *      djsvga terminal changes
 * drd: 3d fitting ; eliminate FIT_INDEX
 *      support for ... using -1  and  -2  (line and index)
 *
 * Revision 1.297  1996/08/08  21:24:48  drd
 * drd: financebars / candlesticks
 *      plot 'file' using 0:1 gives x=0,1,...
 *      optional keyword 'font' in set xlabel etc
 * Rolf Niepraschk: new descrip.mms
 * Jens Emmerich: fix undefined points in splines
 *
 * Revision 1.296  1996/08/05  20:05:02  drd
 * drd: latest FAQ
 *      provide a default location for HELPFILE in Makefile.in
 *      include stdfn.h in gplt_x11.c, for index() on solaris
 *      back out size/font-dependent tics {also fixes  set format y '' bug}
 *      save \'s in xtics properly
 *      fix typo in djsvga.trm
 *      increase pslatex aux file from MAX_ID_LEN to MAX_LINE_LEN
 *      dont use PS_set_font from pslatex driver
 * Santiago Vila Doncel : some Makefile.in tweaks
 *
 * Revision 1.295  1996/07/14  15:13:44  drd
 * drd: update Projects and WhatsNew
 *      restore set size nosquare : it does no harm
 *      include stdlib.h in gplt_x11.c for malloc()
 *      rename vfree() in hidden3d.c to avoid name clash on next
 * Carsten Steger: atan2() for real args
 * Chuck McCorvey: fix for windows 95 printing
 * Rolf Niepraschk: descrip.mms etc for VMS
 *
 * Revision 1.294  1996/07/14  11:51:55  drd
 * drd: max_params typo in fit.c
 *
 * Revision 1.293  1996/06/27  22:04:48  drd
 * drd: print "this", z, "that", f(2), ...
 *      ICCCM export of X11 pixmap
 *
 * Revision 1.292  1996/06/27  21:07:55  drd
 * drd: add set size ratio ...
 *      take David Kotz' name out of latextut [at his request]
 *      try to fix the linux/x11 plot with dots problem
 * Hans-Bernhard Broeker: extra checks in fit.c
 * Scott Lurndal: variable scope in help.c [reported against 3.5]
 * Alex Woo: Makefile for docs
 * William Kirby: colours in win32 [again !]
 * Carsten Steger: amiga fixes
 * Dick Crawford: yet more postscript symbols
 * Joerg Fischer: makefile.all fixes
 * Neil Mathews: gplt_x11 tweaks
 * Rolf Niepraschk: SQRTPI wrong in specfun.c !
 * Joerg Fischer: back out a personal-preference change made earlier
 *
 * Revision 1.291  1996/06/05  21:39:01  drd
 * pl291
 * drd: term_reset() didn't check that term had been initialised
 *      enhance the heap checking with CHECK_HEAP_USE build
 *      trap access violation in hidden3d
 *      fit was bypassing alloc routines
 *      add leak-check calls around the plot calls
 *
 * Revision 1.290  1996/06/03  19:22:46  drd
 * pl290
 * drd: get rid of some warnings on NT
 *      fix access violation in hidden3d
 *      define GP_INLINE for inline fns if directive available
 * Martijn Schipper: os-9 changes for x11
 * Lars Hecking,drd : better support for on_exit vs atexit
 *
 * Revision 1.289  1996/06/02  21:44:55  drd
 * pl289
 * Hans-Bernhard Br"oker : many changes integrated (at last !)
 *
 * Revision 1.288  1996/06/02  20:26:42  drd
 * drd: revisit time code again.
 *      add undocumented command: testtime 'format' 'string'
 *
 * Revision 1.287  1996/05/16  21:25:03  drd
 * drd : oops - replot was broken
 *       oops - make command.o was broken
 *       fix the problems with the keyhole
 * Joerg Fischer : fix number of call args to 10 as documented
 *
 * Revision 1.286  1996/05/16  19:49:54  drd
 * drd: reset state of ytics did not match startup state
 *      print datafile name in error message if it failed to open
 *      default locale is "C", not ""  ("" gives error on linux ?)
 * Mark Diekhans : compile on FreeBSD
 * Jim Van Zandt : CGM tweaks
 *
 * Revision 1.285  1996/05/14  19:50:08  drd
 * drd: fix bug in linespoints / title ''  code
 *      show arrow output
 * Jim Van Zandt : cgm tweaks
 * Joji Maeda : trap division by zero in  view 90,90
 *
 * Revision 1.284  1996/05/14  18:59:18  drd
 * drd: second attempt to move open_printer out of win.trm
 *
 * Revision 1.283  1996/05/13  21:04:05  drd
 * drd: typo in gplt_x11.c
 *      incorporate latest makefile.all into Makefile.in
 *      try to clean up groff-ing of gnuplot.ms (not there yet)
 *
 * Revision 1.282  1996/05/13  19:27:31  drd
 * Bruce Bowler: VMS gplt_x11.c termination fix
 * drd: NT changes to makefile.all
 *      move open_printer() etc from win.trm to wprinter.c
 *      time.c changes to compile on NT
 *
 * Revision 1.281  1996/05/08  20:57:12  drd
 * Dick Crawford: latest docs
 * Alex Woo, William Kirby : latest gif driver (name changed)
 *
 * Revision 1.280  1996/05/08  19:24:16  drd
 * drd: another change to the shared segment of the makefiles
 *      (core.mak was a bad choice of name, since make clean kills it)
 *
 * Revision 1.279  1996/03/04  22:10:45  drd
 * drd: more tweaks to term.c API
 * Alex Woo / Sam Shen : new gdgif.trm [no help yet]
 *
 * Revision 1.278  1996/03/04  21:41:36  drd
 * drd: second attempt at getting core.mak into Makefile
 *
 * Revision 1.277  1996/03/04  20:50:33  drd
 * drd: add core.mak, and arrange for it to be appended to Makefile by
 *      ./configure
 *
 * Revision 1.276  1996/03/04  20:33:54  drd
 * drd: largish restructure of outfile access: rules are too complex to
 *      allow manipulation outside term.c, so provide a term_* API
 *      Separate out the replacement time routines into time.c, allowing
 *      use of either system routines or supplied routines
 *      Add TERM_BINARY flag to term structure
 *      gplt_x11.c/VMS - typo in client message code
 * Joerg: fix typo in setting of axes
 * Werner: increase fixed string buffers in tgif.trm
 *
 * Revision 1.275  1996/03/01  21:17:30  drd
 * drd: new file time.c - update lots of makefiles
 *      fix typo in VMS gplt_x11.c
 *
 * Revision 1.274  1996/02/27  20:50:47  drd
 * drd: fix multiplot for x11
 *      commit replot line at last possible moment, guarding against typos
 *
 * Revision 1.273  1996/02/26  23:12:36  drd
 * Roger Fearick : fix   pause -1 ; print 1
 * drd :  fix  plot sin(x) ; replot cos(x)
 *
 * Revision 1.272  1996/02/26  22:49:44  drd
 * William Kirby,Alex Woo,Roger Fearick: doc fixes
 * William Kirby: tweaks to djgpp/2 and bc/win32 makefiles
 * Roger Fearick: doc2ipf fixes
 * David Liu: emxvga fixes
 * drd: oops - wgnupl32.def was not commited to cvs
 *
 * Revision 1.271  1996/02/26  21:55:17  drd
 * drd: detect on_exit() in place of atexit() for sunos 4
 * Jim Van Zandt : cgm driver (portability tweaks by drd)
 * Lucas Hart, drd : better gnuplot_x11 error response in VMS
 * Rolf Niepraschk : compile gnuplot_x11 on VMS
 * John Turner: tweaks to compile cleanly
 * John Eaton: solid/dash for pslatex
 *
 * Revision 1.270  1996/02/24  19:10:24  drd
 * drd: accept lp as an abbreviation for linespoints
 *      accept using ::4  as  using 1:2:4
 * Joerg Fischer : new format letter %P for multiple of pi
 * drd: get rid of some of the obvious problems in gnuplot.tex output
 *      define DOS32 in emx and djgpp makefiles
 * Emmanual Bigler,drd: tweaks for djgpp vsn 2
 * drd: small tweaks to show output
 *      feed option on dumb terminal
 *      tweaks to new x11 terminal
 *
 * Revision 1.269  1996/02/23  01:32:04  drd
 * drd: make new x11 driver work for VMS
 *      handle WM_DELETE_WINDOW messages neatly (all x11 ports)
 * Dick Crawford, drd: docs
 *
 * Revision 1.268  1996/02/22  00:54:55  drd
 * Neil Mathews: set auto fix
 * drd: show arrows ; user-defined tics
 * Roger Fearick: os/2 patches
 * Emmanual Bigler: makefile for djgpp 2
 * Joerg Fischer: GPMIN/MAX
 * Anthony Heading: daycount bug
 *
 * Revision 1.267  1996/02/21  23:50:35  drd
 * Gary Holt, drd : multiple, persistent X windows
 * drd : tweaks to gdgif driver
 *
 * Revision 1.266  1996/02/17  18:28:00  drd
 * Dick Crawford - terminal-driver help text
 * Mark Charney - location of gdgif library
 * Thomas Koenig - trap floating point exception in set view 0,90
 * drd: TERM_CAN_MULTIPLOT in pm.trm
 *      rationalise code in x11.trm
 *      HAVE_LOCALE -> NO_LOCALE_H in configuration
 *      tweak makefiles for documentation
 *
 * Revision 1.265  1996/01/30  23:47:14  drd
 * drd: tweak malefile.djg
 * Stefan Bodewig: doc2info.c and xref.c
 * Dick Crawford: new gnuplot.doc
 *   - note that there is a problem with tables in doc2ipf !
 *
 * Revision 1.264  1996/01/28  16:31:44  drd
 * drd: take out defunct atari.trm from some makefiles
 *      (try to) choose real(sqrt(z)) >= 0
 * Emmanuel Bigler, drd: encoding support for hpgl
 * Werner Geppert: fixes to help text in post.trm and x11.trm
 * Stefan Bodewig: add asinh etc
 * Pieter Vosbeek: new pslatex.trm
 * Emmanual Bigler: djsvga.trm
 *
 * Revision 1.263  1996/01/25  19:47:34  drd
 * drd: all doc2* filters use termdoc.c
 *      trap seg fault in doc2rtf.c
 *      tweak grass.trm to work with doc2*
 *
 * Revision 1.262  1996/01/24  22:18:24  drd
 * drd: optional heap checking if compiled with CHECK_HEAP_USE
 *      remove term/emxvgaold.trm  term/bigfig.trm  term/pstex.trm
 *      tweaks to hpljii.trm and win.trm for making manual with *.trm
 *      docs/termdoc.c reads from allterm.h if -DALL_TERM
 *
 * Revision 1.261  1996/01/23  20:42:24  drd
 * Stephen Peters : %%Orientation in postscript output
 * Thomas Witelski,drd: space after mirror in save_xtics
 * Axel Kielhorn,drd: reset x2range
 * Joerg Fischer,drd: change to GPMIN/GPMAX
 *                    extend scientific symbols in gsprintf
 *                    restore reading of files on command line
 *
 * Revision 1.260  1996/01/23  19:25:56  drd
 * drd: tweaks to compile on NT (hope I got them all)
 *
 * Revision 1.259  1996/01/22  21:22:25  drd
 * drd: have a go at   splot 'file' matrix   - index not done yet
 *
 * Revision 1.258  1996/01/18  20:36:22  drd
 * Pedro Mendes, drd: try to tidy up sources/build for win16/win32
 *
 * Revision 1.257  1996/01/17  23:09:43  drd
 * drd: first attempt at clear for multiplot
 *      catch seg fault in xstrftime
 *      fix output in show/save xrange for xdata time
 *
 * Revision 1.256  1996/01/16  22:12:34  drd
 * drd: trap a div by zero with xdtics / xmtics
 *      try to support tm_wday field in ggmtime()
 *      first attempt at timecolumn() and tm_sec() family
 *
 * Revision 1.255  1996/01/11  22:45:12  drd
 * Volker Solinus, drd: avoid namespace clashes with zlib used by png term
 * Joerg Fischer: more help moved into terminal driver files
 *
 * Revision 1.254  1996/01/11  22:25:28  drd
 * jrv@vanzandt.mv.com : auto contours at "nice" levels
 * hkeller@gwdg.de : improvements to contouring code
 *
 * Revision 1.253  1996/01/11  21:58:22  drd
 * Lucas Hart: trap a division by zero
 * Volker Solinus: fix some weird problems caused by folding editor
 * Carsten Steger: many small tweaks to compile on HP-UX and Amiga
 *
 * Revision 1.252  1996/01/11  21:43:34  drd
 * s.mccluney@bre.com: make  wgnuplot filename  run until plot window is
 *                     explicitly closed, or enter interactive session if
 *                     /noend is specified
 *
 * Revision 1.251  1996/01/10  21:26:19  drd
 * drd: add file gnuplot.opt containing a list of the core files
 *      update makefile.vms to use this. Comments in some other makefiles
 *
 * Revision 1.250  1996/01/10  20:18:37  drd
 * drd: add doc2html.c into the cvs tree, and so into distribution
 *      trivial tweaks to win/ files to compile with VC++
 *
 * Revision 1.249  1996/01/10  20:07:49  drd
 * cmw: patches for win32s using borland c + various small tweaks
 *
 * Revision 1.248  1995/12/20  22:44:28  drd
 * Erik Luijten: missing static keyword in set.c
 *               fix a small layout problem in Makefile.in
 * Joerg Fischer: put back in the extra line in boxed key
 * drd: post eps bounding box
 *      make %T print as %d not %f hence no trailing .000000
 *
 * Revision 1.247  1995/12/20  21:46:41  drd
 * Stefan Bodewig: updated remaining terminals
 * Joerg Fischer: moved help text to drivers
 * Dick Crawford: docs/ps_guide.ps
 *
 * Revision 1.246  1995/12/18  22:41:13  drd
 * drd: accept %+... in gprintf()
 *      first attempt at accepting  set ytics [start,]step[,end]
 *
 * Revision 1.245  1995/12/14  21:08:59  drd
 * drd: oops - range calculation wrong in splines
 * Neil Mathews: catch a possible pointer mis-access
 * Joerg Fischer: tidy the set/show messages
 *
 * Revision 1.244  1995/12/12  22:15:24  drd
 * drd: make interpol.c aware of dual axes and log scales
 *      tighten up on plot using n  cf  plot using 0:n, including plot..every
 *
 * Revision 1.243  1995/12/11  23:14:09  drd
 * drd: dont seg fault if initial term not identified
 *      dont take logs twice in spline stuff
 *      both min and MIN are defined by some compilers - use GPMIN
 *      update spline demo
 * various: small tweaks for different compilers
 *
 * Revision 1.242  1995/12/10  18:40:57  drd
 * drd: back out change to bitmap.h [dont quite understand, actually !]
 *      second attempt at setlocale stuff
 *
 * Revision 1.241  1995/12/07  21:58:06  drd
 * drd: tweaks to configure.in, Makefile.in
 *      add set locale
 * Hans Olav Eggestad: timeseries patches
 *
 * Revision 1.240  1995/12/05  22:18:51  drd
 * drd: try to fix the postscript bounding-box and reencode problems
 * Joerg Fischer: cp437 and cp830 code pages for postscript
 * drd: bogus code in boundary() in graphics.c (ybot used before set)
 *
 * Revision 1.239  1995/12/02  22:04:44  drd
 * carsten steger: amiga fixes, plus upgrades for several terminals
 * drd: make post.trm variables static
 *
 * Revision 1.238  1995/12/02  21:16:50  drd
 * drd and many others: small tweaks to code to avoid compiler warnings
 *     and improve portability
 *
 * Revision 1.237  1995/11/29  19:01:22  drd
 * schooley@ee.gatech.edu: mif.trm
 * Ian MacPhedran: hpgl.trm
 *
 * Revision 1.236  1995/11/29  17:45:30  drd
 * drd: changes for VMS
 *
 * Revision 1.235  1995/11/29  14:32:26  drd
 * schuh@meteo.uni-koeln.de: fix splines, etc
 *                           plot 'file' smooth ...
 *
 * Revision 1.234  1995/11/27  12:57:30  drd
 * Ian_MacPhedran@engr.USask.CA : coloured points in fig output
 *
 * Revision 1.233  1995/11/27  11:28:48  drd
 * drd: add x2 label
 *      allow fonts for title / axis labels / time
 *      allow strftime format for timestamp
 *
 * Revision 1.232  1995/11/20  12:04:53  drd
 * Joerg Fischer: extra line in boxed key
 *
 * Revision 1.231  1995/11/20  11:28:45  drd
 * drd: a few portability concerns
 * beebe@math.utah.edu - trap use of void fn in an expression.
 *
 * Revision 1.230  1995/11/17  17:58:01  drd
 * drd: try to eliminate the MAXINT problem in datafile.c
 *
 * Revision 1.229  1995/11/03  11:23:54  drd
 * drd: accept x1y2 etc as well as first or second
 *      small change to tics for very small plots
 *
 * Revision 1.228  1995/11/02  16:55:29  drd
 * drd: dont core dump when plot is very small - disable key instead
 *      when autoscaling tics, take plot size and font size into account
 * Neil Mathews : dont forget to alloc room for string terminator
 *
 * Revision 1.227  1995/11/02  13:14:05  drd
 * drd: support boxed key with key under
 *
 * Revision 1.226  1995/11/01  19:05:06  drd
 * drd: splot fix for rs6000
 *      allow negative ticslevel
 *      geometric series tics for logscale
 *      enhanced sprintf for tic format
 *
 * Revision 1.225  1995/11/01  17:43:50  drd
 * drd: added png and updated several other drivers, particularly for help
 *      add the code to doc2gih and doc2hlp to merge help from driver files
 *
 * Revision 1.224  1995/08/25  07:39:46  drd
 * drd: show, save and reset 'origin'
 *      trap seg.fault in dumb.trm
 *      avoid garbage on exit from linux vga
 *      reset ordinate at double blank line
 * John Turner : tidy up key
 *
 * Revision 1.223  1995/08/24  17:06:47  drd
 * drd: detect tempnam in configure
 *
 * Revision 1.222  1995/08/24  13:06:32  drd
 * drd: run configure.in through autoconf 2.4
 *
 * Revision 1.221  1995/08/24  12:49:55  drd
 * drd: update dumb and latex terminals
 * Joerg Fischer : small change to pm driver
 *
 * Revision 1.220  1995/08/24  11:54:12  drd
 * drd: many small changes :   index a:b:c for splot, fixes for vms compile,
 *      preserve \ in strings saved then loaded
 *
 * Revision 1.219  1995/07/28  16:22:38  drd
 * drd: still trying to get it to compile on sun
 *
 * Revision 1.218  1995/07/28  10:19:03  drd
 * drd: more changes to make it compile on sun
 *
 * Revision 1.217  1995/07/27  16:23:05  drd
 * drd: changes to compile on archaic sun compiler
 *      - some ANSI prototypes without __P had sneaked in
 *      - sun compiler doesn't merge adjacent string constants ?
 *      - 0L caused error ?
 *      (still a problem with float.h ?)
 *
 * Revision 1.216  1995/07/21  10:45:45  drd
 * Ian McPhedron, drd - updated various tek drivers to new format
 *
 * Revision 1.215  1995/06/22  15:20:37  drd
 * drd: fix garbage output from print {1, 1.74268162741998e-08}
 *      atari terminals in makefile.unx
 *
 * Revision 1.214  1995/06/19  14:22:14  drd
 * werner@bilbo.mez.ruhr-uni-bochum.de : tgif.trm
 * Dick Crawford : new docs
 *
 * Revision 1.213  1995/06/19  13:10:25  drd
 * drd: fix missing ytics in splot
 *      splot x*y ; plot x  didn't (seem to) do the plot
 * tony@plaza.ds.adp.com : atof not prototyped in scanner.c
 * dirk@lstm.uni-erlangen.de : typos in term.h and ataries.trm
 *
 * Revision 1.212  1995/06/16  12:37:09  drd
 * drd : update regis terminal
 *
 * Revision 1.211  1995/06/16  12:02:41  drd
 * turner@lanl.gov : /usr/openwin -> $OPENWINHOME in makefile.unx
 *
 * Revision 1.210  1995/06/16  11:58:32  drd
 * schuh@meteo.Uni-Koeln.DE : add bezier, etc to saved output
 *
 * Revision 1.209  1995/06/16  11:45:50  drd
 * drd: make enhpost an 'enhanced' option on post.trm
 *
 * Revision 1.208  1995/06/16  10:07:51  drd
 * Thomas.Koenig : set encoding
 *
 * Revision 1.207  1995/06/16  09:06:49  drd
 * macphed@dvinci.usask.ca, drd: implement MINEXP
 *
 * Revision 1.206  1995/06/16  07:57:00  drd
 * drd: saved absolute key position incorrectly
 *
 * Revision 1.205  1995/06/15  15:51:21  drd
 * Richard Stanton : os2 makefile
 * prm@aber.ac.uk  : windows patch
 *
 * Revision 1.204  1995/06/15  15:35:59  drd
 * drd, papp@tpri6f.gsi.de : effort to store all co-ordinate numbers
 *                           in unsigned variables
 *
 * Revision 1.203  1995/06/15  10:51:22  drd
 * drd: show zeroax  was showing wrong linetype
 * pieter@wfw.wtb.tue.nl : update pslatex driver to output plain TeX
 *
 * Revision 1.202  1995/06/15  10:30:42  drd
 * drd: key wrong with   set cont; splot ... notitle
 * anonymous: makefile.tc fixes
 *
 * Revision 1.201  1995/06/14  13:13:03  drd
 * drd: h_tic and v_tic reversed in xtick_callback and ytick_callback
 *
 * Revision 1.200  1995/06/14  12:20:42  drd
 * dirk@lstm.uni-erlangen.de: atari patches
 *
 * Revision 1.199  1995/06/13  14:13:23  drd
 * drd: tidy up contour key entries.
 *      accept   set clabel ['format'] to control key printf format.
 *
 * Revision 1.198  1995/06/13  10:18:10  drd
 * drd: portrait and landscape were switched in enh/post.trm
 *
 * Revision 1.197  1995/06/12  18:09:30  drd
 * drd: fix ylabel x-coord when origin is displaced from 0,0
 *      fix output from 'show margin'
 *
 * Revision 1.196  1995/06/12  14:19:07  drd
 * drd: fix bug where empty data files not correctly trapped
 *
 * Revision 1.195  1995/06/12  12:05:04  drd
 * drd: fix bug in fortran D/Q detection
 *      put #ifdef LINUX around linux.trm in term.h
 *
 * Revision 1.194  1995/05/26  17:41:52  drd
 * anonymous ! (uploaded to /incoming)
 *
 * Revision 1.193  1995/05/26  17:25:47  drd
 * Roger Fearick: os/2 patches
 *
 * Revision 1.192  1995/05/25  16:44:27  drd
 * drd: multiplot for splot; suspend only when prompt is issued
 *
 * Revision 1.191  1995/05/25  14:24:08  drd
 * drd: change term_tbl[term] to term-> in preparation for change to
 *      linked lists of terminals
 *
 * Revision 1.190  1995/05/25  13:38:17  drd
 * drd: fix memory leak when plotting files without \n before EOF
 *
 * Revision 1.189  1995/05/25  10:47:21  drd
 * drd: error in parsing   set *range reverse writeback
 *
 * Revision 1.188  1995/05/12  12:26:35  drd
 * woo,drd: initial multiplot support
 *
 * Revision 1.187  1995/05/11  15:22:51  drd
 * Richard Stanton, Roger Fearick : OS/2 patches
 * Russel Lang, Pedro Mendes      : windows patches
 *
 * Revision 1.186  1995/05/11  12:05:09  drd
 * Martijn Schipper : os9 port
 *
 * Revision 1.185  1995/05/09  15:47:32  drd
 * drd: trivial bug in reporting of extension of ranges < zero
 *
 * Revision 1.184  1995/05/09  15:35:01  drd
 * drd: range reverse and writeback
 *
 * Revision 1.183  1995/05/09  12:27:15  drd
 * several beta testers : mismatch in declarations for {xy}fact across files
 *
 * Revision 1.182  1995/05/09  12:23:51  drd
 * drd: realloc(NULL,x) is not to be trusted
 *
 * Revision 1.181  1995/04/27  14:33:42  drd
 * dirk@lstm.uni-erlangen.de : fix 'set nozeroaxis' bug
 * drd: syntax error in gplt_x11.c
 *
 * Revision 1.180  1995/04/27  14:00:11  drd
 * Richard Standton: os/2 changes
 * mikulik@labs.polycnrs-gre.fr : djgpp changes
 * drd: two blank lines after table output, for index
 *      pointsize parsing in gplt_x11.c
 *
 * Revision 1.179  1995/04/27  12:06:05  drd
 * drd: more recent FAQ
 *      update some makefiles
 *      use graph_error() rather than int_error() while graphics active
 *      allow mix of co-ordinate systems _within_ arrow/label posn.
 *
 * Revision 1.178  1995/04/24  10:41:54  drd
 * Roger Fearick: more os/2 changes
 * drd: trivial portability fixes
 *
 * Revision 1.177  1995/04/22  14:22:07  drd
 * drd: accept set size x  since if there is no ,y  xsize already changed
 *      guard against division by zero when fit is almost perfect
 *
 * Revision 1.176  1995/04/21  15:02:03  drd
 * drd: update a few of the makefiles
 *      temporary fixes to the non-portable timeseries code
 *
 * Revision 1.175  1995/04/21  12:46:18  drd
 * drd: try to accept qQ in place of e for fortran quad numbers in datafile
 *      split graph3d.c into graph3d.c, util3d.c, hidden3d.c  [needs cleanup]
 *
 * Revision 1.174  1995/04/20  18:11:55  drd
 * olav@melvin.jordforsk.nlh.no : timeseries update
 *                                set missing
 * prm@aber.ac.uk : fix cd for win32
 *
 * Revision 1.173  1995/04/20  14:27:28  drd
 * drd: modify table3d output so that it can be read back in as a data splot
 *
 * Revision 1.172  1995/04/20  13:18:52  drd
 * prm@aber.ac.uk : win32 changes
 * David Liu: merge pc graphics routines
 *
 * Revision 1.171  1995/04/20  12:54:40  drd
 * drd: allow arbitrary number of columns and arbitrary length data lines
 *      start undefined fit variables at 1 rather than 1e-30
 * Werner Geppert: new-format texdraw.trm
 *
 * Revision 1.170  1995/04/19  15:57:04  drd
 * Roger Fearick: changes for os/2
 *
 * Revision 1.169  1995/04/19  14:01:42  drd
 * Alex Woo, drd: isnumber -> isanumber to avoid BSD name conflict
 *
 * Revision 1.168  1995/04/19  13:15:20  drd
 * drd: remove polar.dat and antenna.dat from makefiles.
 *
 * Revision 1.167  1995/04/19  10:59:04  drd
 * mccauley@ecn.purdue.edu: update grass terminal
 * drd: fix bug in inverted range in plot2d.c
 *
 * Revision 1.166  1995/04/13  17:06:42  drd
 * drd: last-minute changes to makefile.vms
 *      implement sin(x) as expected for set angles degrees
 *      fix bug : acos(cos({0,1})) was undefined
 *
 * Revision 1.165  1995/04/13  15:26:38  drd
 * drd: rewrite compiler detection in buildvms.com
 *      fix plot second,x
 *      add a set_pointsize function to the terminal driver fn
 *      mods to term.c to support old and new forms of driver
 *      initial implementation of tic mirror and axis for splot
 *      a few  set no*  commands for consistency
 *      make  set bar  continuous rather than on/off
 *
 * Revision 1.164  1995/04/07  18:03:59  drd
 * drd: more fixes from betatesters.
 *      finally fix contour bug on OpenVMS/AXP and splot crash on OSF/AXP
 *
 * Revision 1.163  1995/04/06  13:59:00  drd
 * drd: the various small fixes to makefiles since release of 162
 *      fixes for set zero   and  plot second,first,x
 *
 * Revision 1.162  1995/04/04  16:00:22  drd
 * drd: remove gnubin from makefiles
 *
 * Revision 1.161  1995/04/04  15:14:13  drd
 * drd: set size square. graph and screen coordinates for labels, etc
 *      via is a requried keyword for fit. more code cleanups (#if 0)
 *
 * Revision 1.160  1995/04/03  19:09:21  drd
 * drd: allow key to be positioned at screen/graph co-ordinates
 *
 * Revision 1.159  1995/04/03  18:23:39  drd
 * phy6tc@gps.leeds.ac.uk : updated README.mf and makefile.unx for solaris
 * roger@rsun1.ms.ornl.gov, drd: accept i as abbreviation for index
 *                               was not autoscaling for binary splot
 * drd: implement absolute labels and arrows
 *      small changes to makefile.tc
 *
 * Revision 1.158  1995/04/03  11:13:44  drd
 * rjl: 16-bit changes (ie make pointers GPHUGE)
 *
 * Revision 1.157  1995/04/03  10:36:10  drd
 * rjl: win32 patches
 *
 * Revision 1.156  1995/03/31  17:53:12  drd
 * drd: a few more corrections to command.c/plot2d.c/plot3d.c
 *
 * Revision 1.155  1995/03/31  16:19:03  drd
 * drd: extract some code from command.c into plot2d.c and plot3d.c
 *      update makefiles, but I've probably missed something.
 *      (what's Makefile.in - can it be used to autogenerate the makefiles ?)
 *
 * Revision 1.154  1995/03/31  13:24:06  drd
 * drd: more code cleanups, vms prototypes...
 *      split setshow.c into set.c and show.c
 *
 * Revision 1.153  1995/03/31  10:22:23  drd
 * AL:  no auto cast of 0 to 0.0 to gen_tics() with k&r compilers.
 * drd: ditto for cast of double to int in calls to dbl_raise()
 *      code cleanup based on output from gcc -pedantic -Wall
 *      cc -xansi for SGI  and  -DHAVE_UNISTD_H for linux
 *
 * Revision 1.152  1995/03/28  14:27:02  drd
 * drd: update 00test and CodeStyle
 * AL:  explicit promotion of int->double since k&r compilers cant prototype
 *
 * Revision 1.151  1995/03/27  18:04:44  drd
 * drd: New (temporary) file WhatsNew
 *      update some demos (not sure cvs has noticed..?)
 *      zeroaxis linetype for 3d
 *      draw 2d zeroaxes after grid to get the correct linetype
 *
 * Revision 1.150  1995/03/27  15:00:04  drd
 * drd: couple of problems with non-ansi compilers.
 *      fonts for labels weren't being saved properly
 *      allow linetype to be specied for grid and zeroaxes
 *      allow all four margins to be specified : xmargin -> [lbrt]margin
 *
 * Revision 1.149  1995/03/27  09:41:50  drd
 * drd: reorder linux alphabetically in term.c
 *      rename popen.c to amiga.c, add a CVS id line, update makefile.amg
 *      more tweaks to makefile.vms
 *
 * Revision 1.148  1995/03/26  16:59:13  drd
 * drd : code cleanup in graph3d.c, command.c
 * brouard@sauvy.ined.fr : contour levels fixed when z is logscale
 *
 * Revision 1.147  1995/03/26  15:02:08  drd
 * drd: many, many changes. Second axes. Rewrite of polar mode. every option
 *      on datafile. splot and fit use datafile module. grid at any/all of
 *      tics. Auto-set left margin. Clip data at edge of splot. Remove
 *      need for parametric mode for 3-column data file. border for splot.
 *      Tics on border. Tics optionally mirrored on opposite border. Allow
 *      plot ''  to reuse last name. Various small bugfixes.
 *
 * Revision 1.146  1995/03/26  13:26:31  drd
 * drd: trap realloc(NULL,size) bug in vaxcrtl
 *
 * Revision 1.145  1995/03/05  12:33:29  alex
 * AL: suid fixes for linux
 *
 * Revision 1.144  1994/12/10  12:06:18  alex
 * AL: updated configure.in for autoconf 2.x
 *
 * Revision 1.143  1994/10/06  14:33:39  alex
 * Roger Fearick: OS/2 patches for alpha 140
 *
 * Revision 1.142  1994/09/29  22:37:56  alex
 * David J. Liu: read wait string from /dev/console in linux.trm
 *
 * Revision 1.141  1994/09/18  16:45:59  alex
 * David J. Liu: new linux driver, new terminal function set_font (only
 * Postscript yet), new emx terminal
 *
 * Revision 1.140  1994/09/13  23:12:18  alex
 * AL: updated 00test
 *
 * Revision 1.139  1994/09/13  22:24:37  alex
 * David Denholm: unary +, duplicate symbols in contour.c, plot.h
 *   makefile.vms fixes, undefined dummy vars in plot command,
 *   some bugfixes / additions to key
 *
 * Revision 1.138  1994/09/13  19:21:06  alex
 * AL: added -DNO_GIH to vms and Windows makefiles
 *
 * Revision 1.137  1994/09/13  18:54:22  alex
 * Andrew McLean: avoid CFLAGS commandline overflow in makefile.tc
 *
 * Revision 1.136  1994/09/13  18:35:02  alex
 * Carsten Steger: pipe support for Amiga
 *
 * Revision 1.135  1994/09/13  18:20:35  alex
 * AL: avoid integer overflow in constant expressions for 16bit compilers.
 * Hope I caught all.
 *
 * Revision 1.134  1994/09/13  16:14:13  alex
 * AL: memory functions moved to alloc.c. removed default cases again.
 *
 * Revision 1.133  1994/09/03  12:18:05  alex
 * AL: EOF lockup fix in readline.c
 *
 * Revision 1.132  1994/09/03  12:05:00  alex
 * Carsten Steger: fix for missing tics in polar mode. (Some patch conflicts,
 * still not working completely)
 *
 * Revision 1.131  1994/09/03  10:41:56  alex
 * AL: strftime rewrite, removed strftime dummy from misc.c, still has to be
 *     added to buildvms.com and makefile.vms
 *
 * Revision 1.130  1994/08/28  15:34:26  alex
 * AL: changed defines MEMCPY and MEMSET to BCOPY and BZERO, changed all
 * occurences to bzero to memset.
 * added default case with warning to all switch statements
 * removed VFORK code and comments
 *
 * Revision 1.129  1994/08/28  12:58:45  alex
 * AL: reintroduced NOGAMMA flag, gamma in specfun in only used if NOGAMMA is
 * defined. (Maybe the flag needs to be added to some of the makefiles)
 *
 * Revision 1.128  1994/08/28  11:45:30  alex
 * AL: change check for valid data after strtod call
 *
 * Revision 1.127  1994/08/28  11:12:52  alex
 * David Denholm: using patch
 *
 * Revision 1.126  1994/08/27  18:01:05  alex
 * AL: took out blank line fix again
 *
 * Revision 1.125  1994/08/18  16:39:28  alex
 * AL: accept line with whitespace a separator in data files.
 *
 * Revision 1.124  1994/08/18  16:14:40  alex
 * AL: new mkdist script, autoconf fixes for NeXT, support for GNU readline
 *
 * Revision 1.123  1994/08/09  10:03:46  alex
 * Yehavi Bourvine: space around = in fit.c, matrix.c
 *
 * Revision 1.122  1994/08/05  08:57:48  alex
 * AL: fixes for non-ANSI compilers
 *
 * Revision 1.121  1994/07/30  16:37:39  alex
 * AL: added errorfix.sh
 *
 * Revision 1.120  1994/07/30  16:35:39  alex
 * AL: fixup script for #error directives on brain dead compilers, install.sh
 * from autoconf
 *
 * Revision 1.119  1994/07/27  16:45:38  alex
 * AL: generate options string that PSLATEX_init understands
 *
 * Revision 1.118  1994/07/27  15:34:54  alex
 * Pieter Vosbeek: new pstex driver
 *
 * Revision 1.117  1994/07/27  14:48:48  alex
 * Vivek Khera: update for pslatex driver
 *
 * Revision 1.116  1994/07/27  14:28:33  alex
 * David Denholm: pad struct coordinate to 32 bytes
 *
 * Revision 1.115  1994/07/24  15:54:17  alex
 * Martin P.J. Zinser: move VMS status definition inside main to avoid double
 * definition on APX
 *
 * Revision 1.114  1994/07/24  15:39:08  alex
 * Brian McKeever, David Denholm, AL: added comment about distinguishing
 * VAX and Alpha in buildvms.com
 *
 * Revision 1.113  1994/07/24  15:28:47  alex
 * Richard Mathar: chance makefile.unx comment about Convex 10.1, use -O1 for
 * convex_x11.
 *
 * Revision 1.112  1994/07/23  15:28:07  alex
 * David Denholm: recognise X11 flags is uppercase also
 *
 * Revision 1.111  1994/06/26  15:58:44  alex
 * David Denholm: check that pointsize is >0
 *
 * Revision 1.110  1994/06/26  15:50:20  alex
 * David Denholm: variable pointsize, some bug fixes
 * AL: add pointsize to next.trm+gnuplot.doc
 *
 * Revision 1.109  1994/06/25  13:23:27  alex
 * Carsten Steger: Amiga fixes
 *
 * Revision 1.108  1994/06/25  13:16:25  alex
 * Carsten Steger: HPUX fixes
 *
 * Revision 1.107  1994/06/25  12:52:36  alex
 * Matt Heffron: more point symbols for enhpost.trm
 *
 * Revision 1.106  1994/06/25  12:44:13  alex
 * AL: install lasergnu from sourcedir in Makefile.in
 * Alexander Woo,Hans Olav Eggestad: fixed for graphics.c
 * added timedat.dem to all.dem
 *
 * Revision 1.105  1994/06/25  11:44:51  alex
 * AL: configure created with new autoconf 1.11
 *
 * Revision 1.104  1994/05/02  22:26:02  alex
 * AL: added substitute function strerror, cast qsort function to right type
 * removed warning about return type in tek.trm
 *
 * Revision 1.103  1994/05/01  16:12:49  alex
 * Daniel S. Lewart: AIX makefile fixes
 *
 * Revision 1.102  1994/05/01  16:04:40  alex
 * David Denholm: vms fixes in buildvms and makefile.vms
 *
 * Revision 1.101  1994/05/01  15:36:50  alex
 * Alex Woo: fixes for timedat, Borland C, OSF
 *
 * Revision 1.100  1994/05/01  15:23:11  alex
 * Yehavi Bourvine: fixes for vms
 *
 * Revision 1.99  1994/04/30  16:09:21  alex
 * Olaf Flebbe: use doubles for reading data files, print error message when %f
 * is used
 *
 * Revision 1.98  1994/04/30  15:35:20  alex
 * AL: signal function type, added GPL to end of configure.in
 *
 * Revision 1.97  1994/04/29  10:43:31  alex
 * AL: cleaned up signal calls, removed obsolete next_31 makefile target,
 *     removed autoconf call from Makefile.in, check for signal type in
 *     configure
 *
 * Revision 1.96  1994/04/26  12:54:54  alex
 * Carsten Grammes: removed single character input and variable function
 * arguments from fit.c, moved fit demofiles to ./demo, updated version message
 * AL: removed obsolete test for variable arguments from configure, wasn't
 * working anyway
 *
 * Revision 1.95  1994/04/26  11:34:01  alex
 * Raymond Toy: Update for PSTricks terminal
 *
 * Revision 1.94  1994/04/26  11:14:03  alex
 * Jay I Choe: different line styles for tek40xx, typo in draw_clip_line in
 * graph3d.c
 *
 * Revision 1.93  1994/04/26  11:01:11  alex
 * AL: removed call to malloc_debug, configure with autoconf 1.9
 *
 * Revision 1.92  1994/04/06  08:52:20  alex
 * AL: moved cd and pwd commands out of command(), setdrive fix for MSC
 * removed extern declarations from term/*, updated 00test
 *
 * Revision 1.91  1994/04/05  17:32:14  alex
 * AL: moved stdio.h and setjmp.h to stdfn, env now declared in plot.h
 *
 * Revision 1.90  1994/04/05  16:19:10  alex
 * Matt Heffron: enhanced postscript driver, if and call command
 *
 * Revision 1.89  1994/04/01  15:46:52  alex
 * AL: changed variable argument lists to use either stdarg or varargs
 *     added checks to configure
 *
 * Revision 1.88  1994/04/01  00:12:38  alex
 * Carsten Steger: fix for draw_clip_line when more than two intersections are
 * found
 *
 * Revision 1.87  1994/04/01  00:07:27  alex
 * Carsten Steger: changes for hpux 9.0
 *
 * Revision 1.86  1994/03/31  23:09:54  alex
 * AL: renamed stringfn.h to stdfn.h, moved stdlib, sys/types, errno, time
 * to stdfn.h, added checks for headers to configure, removed declarations for
 * stdlib, time and math functions
 *
 * Revision 1.85  1994/03/30  16:19:41  alex
 * David Denholm: changes for openvms/axp
 * AL: moved some global variable declarations to plot.h
 *
 * Revision 1.84  1994/03/30  02:04:46  alex
 * Alexander Woo: makefile changes for solaris and osf, minor tics are off
 * by default
 *
 * Revision 1.83  1994/03/30  01:40:10  alex
 * Roger Fearick: os2 fixes for term/pm.trm
 *
 * Revision 1.82  1994/03/30  01:27:58  alex
 * Carsten Steger: amiga fixes, new reset command,
 *                 docs/*.c now includes "ansichek.h"
 * AL: removed -Idocs from all makefiles
 *
 * Revision 1.81  1994/03/30  00:57:23  alex
 * Olaf Flebbe: check for string.h and strchr in configure
 *
 * Revision 1.80  1994/03/30  00:35:50  alex
 * Olaf Flebbe: print up to 15 digits in print command
 *
 * Revision 1.79  1994/03/29  22:49:47  alex
 * AL: changed action tables to dynamic size, read_line fix
 *
 * Revision 1.78  1994/03/29  19:10:32  alex
 * AL: changed token table to dynamic size
 *
 * Revision 1.77  1994/03/29  16:33:15  alex
 * AL: removed useless replot_line_len and extend_input_line
 *
 * Revision 1.76  1994/03/29  16:26:17  alex
 * AL: changed replot_line to dynamic length
 *
 * Revision 1.75  1994/03/29  13:36:54  alex
 * AL: readline with variable line length
 *
 * Revision 1.74  1994/03/23  17:36:03  alex
 * Robert Cunningham: draw 3d impulses from 0 plane and not from base
 * Raymond Toy,AL: find X if includes and libs are in the standard directories
 *
 * Revision 1.73  1994/03/22  14:26:23  alex
 * Yehavi Bourvine: changes for gnufit with vms
 * AL: removed NOGAMMA define since it is no longer needed
 *
 * Revision 1.72  1994/03/22  13:40:16  alex
 * Olaf Flebbe: gnufit changes to Makefile.in, autoconf code to detect strnicmp
 * and strcasecmp
 * AL: added -DSTRNICMP to Borland and MS makefiles, working strnicmp function
 * autoconf code to detect termios and tcgetattr on NeXT
 *
 * Revision 1.71  1994/03/22  02:15:17  alex
 * AL: sgtty version of kbhit and getch in fit.c. TERMIOS_FIT now called TERMIOS
 * (as in readline), NeXT uses termios in readline also. new targets for old
 * (i.e. <=3.1) versions of NeXTstep
 *
 * Revision 1.70  1994/03/21  17:29:03  alex
 * AL: merged DOS version of read_line into generic one. vms still missing.
 *
 * Revision 1.69  1994/03/20  14:57:04  alex
 * AL: changed interactive and file input to dynamic line len. dos and vms not
 * yet.
 *
 * Revision 1.68  1994/03/19  14:28:48  alex
 * AL: added buffer size parameter to copy_str, quote_str and capture, removed
 * quotel_str
 *
 * Revision 1.67  1994/03/18  23:08:27  alex
 * AL: removed extern declarations without prototypes
 *
 * Revision 1.66  1994/03/18  22:42:01  alex
 * AL: removed most(?) extern function declarations without prototypes
 *
 * Revision 1.65  1994/03/18  21:06:03  alex
 * Carsten Gammes: fix for prototype for solve_tri_diag
 * David Denholm, AL: fix for 0.0**1.0 and arg(0)
 * Olaf Flebbe, AL: use signed int in dumb term, add nl after dumping plot
 * AL: removed printf warnings from hpgl.trm and regis.trm
 *
 * Revision 1.64  1994/03/18  16:23:24  alex
 * AL: removed ANSI prototypes in fit.c, kbhit and getch static
 *
 * Revision 1.63  1994/03/17  10:44:20  alex
 * AL: updated 00test
 *
 * Revision 1.62  1994/03/17  00:54:13  alex
 * AL: deANSIfied the gnufit functions, added readme.1st as README.fit
 *
 * Revision 1.61  1994/03/17  00:04:28  alex
 * Carsten Grammes: gnufit 1.2, new files fit.c, fit.h, matrix.c, matrix.h,
 * type.h, fitdemo directory
 *
 * Revision 1.60  1994/03/14  17:28:50  alex
 * AL: new file stringfn.h to include string.h or strings.h, removed all
 * external declarations for str??? functions
 *
 * Revision 1.59  1994/03/14  14:44:41  alex
 * AL: changed all functions with char arguments to ANSI style, changed more
 * index calls, removed warning about HUGE redefinition on NeXT
 *
 * Revision 1.58  1994/03/13  16:46:49  alex
 * Wolfram Gloger: include ansichek after system includes, don't redefine
 * __P if defined in linux
 *
 * Revision 1.57  1994/03/13  16:32:50  alex
 * Yehavi Bourvine: changes for vms
 *
 * Revision 1.56  1994/03/13  16:10:55  alex
 * Alex Woo: changes for SGI IRIX
 *
 * Revision 1.55  1994/03/13  16:01:52  alex
 * Ton van Overbeek: changes for PureC
 * AL: changed all uses of index and rindex to strchr and strrchr
 *
 * Revision 1.54  1994/03/13  15:32:57  alex
 * AL: some prototype fixes to compile on PCs
 *
 * Revision 1.53  1994/03/13  15:27:51  alex
 * John Interrante: better cleanup in Makefile.in, also draw horizontal grid
 * lines, don't print terminal number
 * AL: rename docs/makefile and docs/latextut/makefile in configure
 *
 * Revision 1.52  1994/03/13  14:57:57  alex
 * Carsten Steger: revised amiga makefile, some more prototypes, removed
 * unused variables, fix for ytick==0.0
 *
 * Revision 1.51  1994/03/13  14:28:14  alex
 * Olaf Flebbe: search for lgamma in libm
 *
 * Revision 1.50  1994/03/03  14:07:40  alex
 * AL: removed some ANSI-style functions, # always in first column
 *
 * Revision 1.49  1994/03/02  00:13:15  alex
 * AL: added 00test README file
 *
 * Revision 1.48  1994/02/25  16:38:09  alex
 * AL: added prototypes to files in docs. Added -Idocs to all makefiles that
 * don't (cd docs; make something)
 *
 * Revision 1.47  1994/02/25  15:14:05  alex
 * AL: added ANSI prototypes for all files . and term, new files protos.h,
 * fnproto.h, binary.h, ansichek.h
 *
 * Revision 1.46  1994/01/29  16:27:49  alex
 * HO Eggestad's timeseries mods
 *
 * Revision 1.45  1994/01/27  23:15:21  alex
 * AL: Atari ST MultiAES mods, Projects file
 *
 * Revision 1.44  1994/01/07  13:53:29  alex
 * AL: changed version numbers to 3.5
 *
 * Revision 1.43  1994/01/03  16:49:28  alex
 * Olaf Flebbe: use DBL_MAX for VERYLARGE on sgi, since HUGE==FLT_MAX
 *
 * Revision 1.42  1994/01/03  16:43:28  alex
 * AL,Paul Mitchell: change X11 settings for sgi, pass INSTALL to docs/makefile
 *
 * Revision 1.41  1994/01/03  16:14:09  alex
 * AL: don't put `point' into LaTeX options string
 *
 * Revision 1.40  1994/01/03  16:03:38  alex
 * AL: memory efficient checkin procedure. Boy, I hate DOS.
 *
 * Revision 1.39  1994/01/03  15:49:45  alex
 * AL: new script nuke, -a option for all
 *
 * Revision 1.38  1994/01/03  02:24:51  alex
 * AL: #if'ed out obsolete vfork shell function
 *
 * Revision 1.37  1994/01/02  14:54:34  alex
 * David Ciemiewicz: don't redefine VREPRINT
 *
 * Revision 1.36  1994/01/02  14:49:29  alex
 * Olaf Flebbe: give specific message when too many tokens
 *
 * Revision 1.35  1994/01/02  14:35:04  alex
 * Olaf Flebbe: add sony_news to #ifdef for sys/types.h
 *
 * Revision 1.34  1994/01/02  14:30:05  alex
 * Timothy L D Collins: mention solaris as possible machine type in makefile.unx
 *
 * Revision 1.33  1994/01/02  14:20:35  alex
 * AL: use read/write string for sscanf in gnuplot_x11
 *
 * Revision 1.32  1994/01/02  13:47:41  alex
 * Marc van Woerkom,AL: check for undefined DISPLAY env var
 *
 * Revision 1.31  1994/01/02  13:42:06  alex
 * Phil Garner: check OSF1 instead of __alpha for signal return type
 * AL: abort option in civers
 *
 * Revision 1.30  1994/01/02  13:28:56  alex
 * AL: merged bigfig in fig, new civers, all, files
 *
 * Revision 1.29  1993/12/30  21:58:52  alex
 * Alex Woo: data/command mix mod
 *
 * Revision 1.29  1993/12/30  21:58:52  alex
 * Alex Woo: data/command mix mod
 *
 * Revision 1.28  1993/10/15  00:11:29  alex
 * Radey Shouman: parametric notitle bug
 *
 * Revision 1.27  1993/10/14  20:10:53  alex
 * David Kotz: MailFTP via Dartmouth server in 0README
 *
 * Revision 1.26  1993/10/08  17:20:30  alex
 * Alexander Woo: user selected borders and tic marks, seems to conflict with
 * minor tic marks
 *
 * Revision 1.25  1993/10/08  17:05:55  alex
 * Akira Sawada: call XSetWindowBackgroundPixmap after drawing picture
 *
 * Revision 1.24  1993/10/08  16:40:49  alex
 * AL: new define CHARSET7BIT in readline.c. The readline function now accepts
 *     extended chars (e.g. latin1, MSDOS codepage etc.)
 *
 * Revision 1.23  1993/10/08  16:27:31  alex
 * Atsushi Mori: logarithmic impulses
 *
 * Revision 1.22  1993/10/08  14:46:49  alex
 * Henri Gavin: row/col fix for dgrid3d
 *
 * Revision 1.21  1993/10/08  14:43:31  alex
 * AL: set pipe signal to ignore
 *
 * Revision 1.20  1993/10/08  14:37:53  alex
 * Dave Shield, AL: fixed comments regarding HELPDEST in makefile.unx
 *
 * Revision 1.19  1993/10/08  13:58:51  alex
 * AL: missing {, tabs in makefile.unx
 *
 * Revision 1.18  1993/10/07  19:23:49  alex
 * Scott D. Heavner: Linux svga driver, new file term/linux.trm
 *
 * Revision 1.17  1993/10/07  19:10:58  alex
 * N.G. Brookins: ReGIS fix
 *
 * Revision 1.16  1993/10/07  17:19:49  alex
 * Vivek Khera: Color option for PSLaTeX driver
 *
 * Revision 1.15  1993/10/07  17:14:11  alex
 * Timothy L D Collins: new hpgl driver hp7550
 *
 * Revision 1.14  1993/10/07  16:57:37  alex
 * Wolfram Gloger: Sloppy mem management fix, LINUX_FLAGS in makefile.unx
 *
 * Revision 1.13  1993/10/07  16:53:03  alex
 * Carsten Steger: demo fixes
 *
 * Revision 1.12  1993/10/07  15:01:55  alex
 * Emmanuel Bigler: missing -I in HP_FLAGS
 *
 * Revision 1.11  1993/10/07  14:56:17  alex
 * Bruno PIGUET: missing ()s in term/hpgl.trm PCL_YMAX
 *
 * Revision 1.10  1993/10/07  13:24:42  alex
 * Roland B Roberts, Alex Woo: gcc on VMS
 *
 * Revision 1.9  1993/10/07  12:56:53  alex
 * AL: a couple of .cvsignore file, no for final releases
 *
 * Revision 1.8  1993/10/07  12:52:58  alex
 * Alex Woo: fix a logic error in polar grid plotting, labels every 30 degrees
 *
 * Revision 1.7  1993/10/07  11:09:33  alex
 * Nick Strobel: Documentation for new errorbar styles
 *
 * Revision 1.6  1993/10/07  10:21:21  alex
 * Lars Henke, Nick Strobel: minor ticmarks, new errorbar styles: yerrorbars,
 * xerrorbars, xyerrorbars, boxxyerrorbars, interpolation: splines, csplines,
 * sbezier, bezier, errorbars with and without ticks, Amiga cleanup
 * AL: added interpol.c to all makefiles
 *
 * Revision 1.5  1993/10/06  15:51:27  alex
 * AL: Added Log to version.c, commitvers
 *     cast in dxf.trm
 *
 */

#include "plot.h"

char version[] = "3.5 (pre 3.6)";
char patchlevel[] = "beta 347pl3";
char date[] = "Mon Jun 22 13:22:33 BST 1998"; 
char gnuplot_copyright[] = "Copyright(C) 1986 - 1993, 1998";

char faq_location[] = FAQ_LOCATION;
char bug_email[] = CONTACT;
char help_email[] = HELPMAIL;
