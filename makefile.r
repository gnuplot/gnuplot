
# $Id: makefile.r%v 3.50.1.17 1993/08/27 05:21:33 woo Exp woo $
#
# $Log: makefile.r%v $
# Revision 3.50.1.17  1993/08/27  05:21:33  woo
# V. Khera's fig patch
#
# Revision 3.50.1.16  1993/08/27  05:04:42  woo
# Added <errno.h> to help.c and added support routines to binary.c
#
# Revision 3.50.1.15  1993/08/21  15:23:42  woo
# Rewrote print_3dtable
#
# Revision 3.50.1.14  1993/08/19  04:10:23  woo
# V. Broman sun_mgr driver
#
# Revision 3.50.1.13  1993/08/19  03:21:26  woo
# R. Lang fix to MS-Windows print driver
#
# Revision 3.50.1.11  1993/08/10  03:55:03  woo
# R. Lang mod to change location of plot in hppj.trm
#
# Revision 3.50.1.10  1993/08/07  16:06:34  woo
# Fix using n bug, configure 1.3
#
# Revision 3.50.1.9  1993/08/05  05:38:59  woo
# Fix timedate location for splot and plot + CONFIGURE ROUTINE
#
# Revision 3.50.1.8  1993/07/27  05:37:15  woo
# More mods for SOLARIS 2.2 using gcc
#
# Revision 3.50.1.7  1993/07/27  04:57:58  woo
# Fix impulse plot style if logscale y
#
# Revision 3.50.1.6  1993/07/27  03:35:12  woo
# Fix epsviewe.m 
#
# Revision 3.50.1.4  1993/07/26  06:17:24  woo
# Solaris mods to sun_x11 and gplt_x11.c
#
# Revision 3.50.1.3  1993/07/26  05:38:19  woo
# DEC ALPHA OSF1 mods
#
# Revision 3.50.1.2  1993/07/26  05:12:42  woo
# fix of replot of replot bug
#
# Revision 3.50.1.1  1993/07/26  04:59:54  woo
# fix of splot notitle bug
#
# Revision 3.50  1993/07/09  05:35:24  woo
# Baseline version 3.5 version
#
# Revision 3.38.2.153  1993/07/08  03:43:32  woo
# Added dgrid3d save to misc.c and updated titlepages
#
# Revision 3.38.2.153  1993/07/08  03:43:32  woo
# Added dgrid3d save to misc.c and updated titlepages
#
# Revision 3.38.2.152  1993/07/06  15:03:56  woo
# A. Lehmann cleanup of 3_5b1
#
# Revision 3.38.2.151  1993/07/06  14:33:51  woo
# New 0FAQ and AXPVMS fix to gplt_x11.c
#
# Revision 3.38.2.150  1993/07/02  16:42:43  woo
# A. Reeh fix for monochrome X11 under VMS
#
# Revision 3.38.2.149  1993/07/01  20:31:55  woo
# New version of GRASS makefile
#
# Revision 3.38.2.148  1993/07/01  13:54:25  woo
# T. Collins changes to Latex line types
#
# Revision 3.38.2.147  1993/07/01  13:41:44  woo
# A. Lehmann fix for hpux internal.c
#
# Revision 3.38.2.146  1993/06/30  21:24:08  woo
# Fix to flash graphics, rosenbrock function in contours.dem
#
# Revision 3.38.2.145  1993/06/30  20:16:22  woo
# Yehavi fixes for AXPVMS
#
# Revision 3.38.2.144  1993/06/23  00:34:39  woo
# Reordered latex line types -- lighter default line
#
# Revision 3.38.2.143  1993/06/23  00:10:28  woo
# R. Shouman fix of blank title induced linetype bug
#
# Revision 3.38.2.142  1993/06/21  14:43:21  woo
# P. Egghart check on PLOSS for matherr and pass MY_FLAGS to subdirectories
#
# Revision 3.38.2.141  1993/06/19  13:00:21  woo
# Okidata driver and time_t for sun386 mod
#
# Revision 3.38.2.140  1993/06/19  12:29:34  woo
# J. Abbey fixes for Alliant
#
# Revision 3.38.2.139  1993/06/19  01:03:49  woo
# Fix for 3d clipping bug
#
# Revision 3.38.2.138  1993/06/16  00:38:14  woo
# O. Franksson MIF mod for vertical text and better char size
#
# Revision 3.38.2.137  1993/06/15  23:57:02  woo
# Linux fixes
#
# Revision 3.38.2.136  1993/06/15  23:33:29  woo
# V. Khera non-interactive stderr fix
#
# Revision 3.38.2.135  1993/06/15  22:02:26  woo
# splot fix for x & y tics and labels
#
# Revision 3.38.2.134  1993/06/04  01:03:20  woo
# Linux additions to makefile.unx
#
# Revision 3.38.2.133  1993/06/04  00:34:23  woo
# D. Lewart cleanup of documentation and AIX flags
#
# Revision 3.38.2.132  1993/06/04  00:01:25  woo
# Fixed range reverse tic bug for splot, fixed reported bug for pcl landscape mode
#
# Revision 3.38.2.131  1993/06/03  23:54:25  woo
# Fixed author list and version number in titlepages
#
# Revision 3.38.2.130  1993/05/26  00:54:46  woo
# Information on comp.graphics.gnuplot whenever mailing list mentioned
#
# Revision 3.38.2.129  1993/05/18  23:36:20  woo
# D. Lewart cleanup of 3_3b14
#
# Revision 3.38.2.128  1993/05/18  23:21:07  woo
# C. Steger's mod to animate.dem
#
# Revision 3.38.2.127  1993/05/06  00:06:02  woo
# More R. Lang memory realignments for MS-DOS and MS Windows
#
# Revision 3.38.2.126  1993/05/05  22:39:48  woo
# New mif driver
#
# Revision 3.38.2.125  1993/05/05  00:02:28  woo
# R. Lang addition of GPFAR pointers
#
# Revision 3.38.2.124  1993/05/04  22:53:47  woo
# C. Steger cleanup
#
# Revision 3.38.2.123  1993/04/30  00:56:28  woo
# M Levine mods to hpgl.trm to allow printing under VMS (add carriage returns)
#
# Revision 3.38.2.122  1993/04/30  00:31:45  woo
# M Levine mods to add 16 colors for REGIS devices
#
# Revision 3.38.2.121  1993/04/30  00:02:32  woo
# Mods for OpenVMS on DEC AXP
#
# Revision 3.38.2.120  1993/04/26  00:25:11  woo
# Aztec C patches for the Amiga
#
# Revision 3.38.2.119  1993/04/25  23:55:53  woo
# C. Steger cleanup of 3_3b13
#
# Revision 3.38.2.118  1993/04/19  23:22:02  woo
# Add 0BUGS,0FAQ and MacCauley solaris mods
#
# Revision 3.38.2.117  1993/04/15  02:30:21  woo
# R. Davis cleanup of NeXT terminal driver
#
# Revision 3.38.2.116  1993/04/15  02:10:38  woo
# R. Lang mods for SVGAKIT to EMX gcc
#
# Revision 3.38.2.115  1993/04/15  01:58:16  woo
# Cleaning up mailing list addresses in trm's
#
# Revision 3.38.2.114  1993/04/15  01:48:40  woo
# D. Lewart cleanup of 3_3b12
#
# Revision 3.38.2.113  1993/04/15  01:38:07  woo
# C. Steger clean gpic driver
#
# Revision 3.38.2.112  1993/04/03  12:04:22  woo
# C. Steger cleanup of 3_3b12
#
# Revision 3.38.2.111  1993/04/03  11:58:46  woo
# T. Richardson fix of real contour levels bug
#
# Revision 3.38.2.110  1993/04/03  11:49:19  woo
# T. Broekart parametric data file fix
#
# Revision 3.38.2.109  1993/04/03  00:00:57  woo
# S. Rosen 180 dpi Epson driver
#
# Revision 3.38.2.108  1993/03/29  13:14:23  woo
# K. Yee fixes to texdraw.trm
#
# Revision 3.38.2.107  1993/03/26  22:52:14  woo
# R. Lang fix to doc2rtf.c and gnuplot.doc
#
# Revision 3.38.2.106  1993/03/26  22:48:05  woo
# Added X11 term entry for backward compatibility
#
# Revision 3.38.2.105  1993/03/23  03:21:55  woo
# D. Lewart cleanup of 3_3b11
#
# Revision 3.38.2.103  1993/03/23  03:01:56  woo
# V. Khera Fig 2.1 graphics editor driver
#
# Revision 3.38.3.102  1993/03/23  02:49:02  woo
# R. Toy's fixes for gamma function
#
# Revision 3.38.2.101  1993/03/23  02:22:10  woo
# D. Johnson LINUX addition to makefile.unx
#
# Revision 3.38.2.100  1993/03/22  03:23:10  woo
# 3B2 and KSR support
#
# Revision 3.38.2.99  1993/03/22  02:28:47  woo
# R. Lang's fix of 3_3b11 post.trm
#
# Revision 3.38.2.98  1993/03/22  02:22:16  woo
# R. Shouman postscript mods
#
# Revision 3.38.2.97  1993/03/16  15:07:24  woo
# C. Steger's cleanup of 3_3b10
#
# Revision 3.38.2.96  1993/03/15  21:39:29  woo
# More R. Fearick 930312 os2 mods
#
# Revision 3.38.2.95  1993/03/15  21:32:02  woo
# R. Fearick 930312 os/2 mods
#
# Revision 3.38.2.94  1993/03/15  20:28:00  woo
# Fixed logscale y label bug
#
# Revision 3.38.2.93  1993/03/15  19:26:12  woo
# T. Collins cleanup for Sequent Dynix
#
# Revision 3.38.2.92  1993/03/12  20:06:08  woo
# A. Lehman fix of GPFAR bug in binary.c, more is_log_func fixes
#
# Revision 3.38.2.91  1993/03/08  01:40:06  woo
# R. Lang fix of MS Windows Menu error (show instead of set)
#
# Revision 3.38.2.90  1993/03/08  01:12:01  woo
# R. Lang fix of contour bugs with farmalloc
#
# Revision 3.38.2.89  1993/03/06  01:37:13  woo
# T. Broekaert fix of parametric log_func
#
# Revision 3.38.2.88  1993/03/04  01:47:16  woo
# Added polar grids
#
# Revision 3.38.2.87  1993/03/02  19:31:38  woo
# GRASS (Geographics Information Systems) mods
#
# Revision 3.38.2.86  1993/03/01  01:50:57  woo
# R. de Oliveira Apollo mods for 3_3b9
#
# Revision 3.38.2.85  1993/03/01  01:45:52  woo
# New Klein.dat data file
#
# Revision 3.38.2.84  1993/03/01  01:36:45  woo
# Lewart's cleanup of 3_3b9
#
# Revision 3.38.2.83  1993/03/01  01:06:14  woo
# More DEC_OSF mods
#
# Revision 3.38.2.82  1993/03/01  00:28:27  woo
# Fixed zrange parse error for parametric splot
#
# Revision 3.38.2.81  1993/02/24  02:29:34  woo
# C. Liu's Zortech mods to 3_3b9
#
# Revision 3.38.2.80  1993/02/20  18:34:10  woo
# Fixes to some demo files and LATEX font options by J. Holloway
#
# Revision 3.38.2.79  1993/02/20  13:44:21  woo
# C. Liu preparatory Zortech extended DOS mods
#
# Revision 3.38.2.78  1993/02/20  02:59:43  woo
# Changed complex() -> Gcomplex(), integer() -> Ginteger() and DEC OSF1 mods
#
# Revision 3.38.2.77  1993/02/19  02:18:03  woo
# R. Shouman hidden+nosurface draw_clip_line fix
#
# Revision 3.38.2.76  1993/02/19  01:33:00  woo
# K. Laprade switch quote_str to quotel_str in command.c
#
# Revision 3.38.2.75  1993/02/19  01:25:45  woo
# V. Snyder check for view 90,0
#
# Revision 3.38.2.74  1993/02/19  00:05:24  woo
# R. Lang Windows NT preparatory mods
#
# Revision 3.38.2.73  1993/02/18  03:24:36  woo
# V. Snyder animation mods
#
# Revision 3.38.2.72  1993/02/09  03:02:15  woo
# Sigfrid Lundberg siglun@lotke.teorekkol.lu.se GPIC (troff) driver
#
# Revision 3.38.2.71  1993/02/08  02:41:39  woo
# R. Lang fix to ANSI C __UNIX__ define instead of UNIX
#
# Revision 3.38.2.70  1993/02/08  02:13:53  woo
# A. Lehman general cleanup and Atari mods
#
# Revision 3.38.2.69  1993/02/07  00:52:34  woo
# C. Steger cleanup of isosamples
#
# Revision 3.38.2.68  1993/02/07  00:40:17  woo
# R. Lang's MIF driver fixes
#
# Revision 3.38.2.67  1993/02/06  23:35:49  woo
# T. Richardson's gnugraph driver
#
# Revision 3.38.2.66  1993/02/06  03:04:28  woo
# D. Tabor's fix to align 3d grids with tic marks in parametric mode
#
# Revision 3.38.2.65  1993/02/06  02:26:55  woo
# D. Lewart's cleanup of docs directory
#
# Revision 3.38.2.64  1993/02/05  03:41:53  woo
# A. Woo fixes to mif.trm
#
# Revision 3.38.2.63  1993/02/05  03:38:49  woo
# R. Shouman notitle extensions
#
# Revision 3.38.2.62  1993/02/05  03:32:09  woo
# R. Lang 930128 MS Windows patch
#
# Revision 3.38.2.61  1993/01/27  02:35:31  woo
# C. Steger's isosamples mod
#
# Revision 3.38.2.60  1993/01/27  01:46:55  woo
# C. Steger addition of klein bottle to singulr.dem and new metafont driver
#
# Revision 3.38.2.59  1993/01/26  01:08:58  woo
# R. Fearick 930125 OS/2 mods
#
# Revision 3.38.2.58  1993/01/26  00:26:54  woo
# P. Johnson MSC 7.0 mods to beta 7 release
#
# Revision 3.38.2.57  1993/01/23  00:56:18  woo
# R. Lang's EMX for MS-DOS mods
#
# Revision 3.38.2.56  1993/01/22  02:38:10  woo
# C. Steger metafont driver mods and demo cleanup
#
# Revision 3.38.2.55  1993/01/22  01:50:01  woo
# C. Steger's SAS/C 6.1 for AMIGA mods
#
# Revision 3.38.2.54  1993/01/21  01:31:48  woo
# R. Lang's datafile fix
#
# Revision 3.38.2.53  1993/01/21  01:11:07  woo
# D. Lewart cleanup to BETA 7 release
#
# Revision 3.38.2.52  1993/01/17  03:43:20  woo
# Deskjet 500C  Color Driver
#
# Revision 3.38.2.51  1993/01/16  14:54:28  woo
# Remove faralloc for DJGPP
#
# Revision 3.38.2.50  1993/01/16  14:54:28  woo
# D. Lewart's RS6000 and cleanup fix
#
# Revision 3.38.2.49  1993/01/15  01:02:52  woo
# Modified color postscript driver to retain dotted and dashed lines
#
# Revision 3.38.2.48  1993/01/14  04:21:27  woo
# R. Lang cleanup fix of 930112
#
# Revision 3.38.2.47  1993/01/14  04:05:38  woo
# E. Youngdale fix for scattered data
#
# Revision 3.38.2.46  1993/01/11  04:41:47  woo
# D. Tabor segmented memory fix for X286
#
# Revision 3.38.2.45  1993/01/11  04:36:01  woo
# Various fixes to demo files, demo -> dem, suggested by Interrante
#
# Revision 3.38.2.44  1993/01/11  03:57:44  woo
# J. Interrante fixes to Perl script
#
# Revision 3.38  1993/01/11  03:55:32  woo
# MS-DOS makefile for RCS checkin
#
# Revision 3.38.2.43  1993/01/10  21:58:46  woo
# Update Copyright and Date
#
# Revision 3.38.2.42  1993/01/07  17:07:24  woo
# R. Lang + P. Johnson unsigned char compare fix
#
# Revision 3.38.2.41  1993/01/07  16:58:04  woo
# R. Lang 12-23 windows + P. Johnson MS-C mods
#
# Revision 3.38.2.40  1993/01/07  16:49:42  woo
# R. Davis save fixes
#
# Revision 3.38.2.39  1992/12/15  17:09:35  woo
# VMS fixes + C. Steger documentation for MF driver
#
# Revision 3.38.2.38  1992/12/12  07:11:02  woo
# R. Lang 12/10 MS Windows mods
#
# Revision 3.38.2.37  1992/12/12  06:52:38  woo
# R. Cunningham fix for binary stdin
#
# Revision 3.38.2.36  1992/12/12  06:40:15  woo
# Airfoil Demo & C. Steger's metafont driver fixes
#
# Revision 3.38.2.35  1992/12/12  06:16:45  woo
# R. Cunningham internal reset error fix & sys/types in misc.c
#
# Revision 3.38.2.34  1992/12/12  05:56:21  woo
# D. Lewart latex and punctuation mod -- large
#
# Revision 3.38.2.33  1992/12/04  19:49:20  woo
# E. van der Maarel documentation fixes
#
# Revision 3.38.2.32  1992/12/04  18:32:51  woo
# C. Steger's AMIGA SC 6.1 mod + singular.dem
#
# Revision 3.38.2.31  1992/12/04  17:59:01  woo
# R. Lang's 1204 windows ICON patch.
#
# Revision 3.38.2.30  1992/12/03  18:20:15  woo
# R. Lang's 1203 windows patch.
#
# Revision 3.38.2.29  1992/12/01  22:21:45  woo
# A. Lehmann's prototype fixes
#
# Revision 3.38.2.28  1992/12/01  20:40:45  woo
# J. Campbell's modifications to the README
#
# Revision 3.38.2.26  1992/12/01  03:47:53  woo
# Changed index to strchr in rgip.trm and imagen.trm
#
# Revision 3.38.2.25  1992/12/01  03:42:02  woo
# More P. Johnson MSC fixes
#
# Revision 3.38.2.24  1992/11/26  04:25:08  woo
# G. Phillips pslatex driver
#
# Revision 3.38.2.23  1992/11/26  03:01:17  woo
# E. Youngdale fix of imagen.trm
#
# Revision 3.38.2.22  1992/11/26  02:58:19  woo
# R. Lang fix of gnuplot.def
#
# Revision 3.38.2.21  1992/11/24  16:22:20  woo
# P. Johnson fixes for MS-C 7.0
#
# Revision 3.38.2.20  1992/11/24  16:00:56  woo
# R. Lang 1123d fix to Print! (ascii now)
#
# Revision 3.38.2.19  1992/11/24  16:00:56  woo
# Changed arc.nasa.gov to dartmouth.edu in all files
#
# Revision 3.38.2.18  1992/11/23  21:09:28  woo
# R. Lang 1123c 16 char directory bugfix.
#
# Revision 3.38.2.17  1992/11/23  21:02:52  woo
# R. Lang 1123b drag-drop fix for windows.
#
# Revision 3.38.2.16  1992/11/23  20:20:12  woo
# R. Lang 1123a fixes for README.win, data size in windows.
#
# Revision 3.38.2.15  1992/11/23  19:59:26  woo
# E. Youngdale y-axis hidden line fix.
#
# Revision 3.38.2.14  1992/11/21  02:05:09  woo
# R. Lang's 11/20 Windows patch.
#
# Revision 3.38.2.12  1992/11/20  18:14:40  woo
# New prob.dem with steps.
#
# Revision 3.38.2.11  1992/11/20  01:35:24  woo
# Gershon multiple mesh mods.
#
# Revision 3.38.2.10  1992/11/16  17:52:15  woo
# R. Fearick fixes, gnuplot.ico to gnupmdrv.c
#
# Revision 3.38.2.9  1992/11/16  17:38:05  woo
# R. Lang new win (MS-Windows) subdirectory
#
# Revision 3.38.2.8  1992/11/16  16:52:54  woo
# A. Lehmann no BGI mods
#
# Revision 3.38.2.7  1992/11/14  03:49:17  woo
# R. Lang multiple patches
#
# Revision 3.38.2.6  1992/11/14  02:22:20  woo
# H. Eggestad rgip and imagen drivers fixes, hpgl fix
#
# Revision 3.38.2.5  1992/11/10  02:16:55  woo
# Gershon Elber scatter to grid mods
#
# Revision 3.38.2.2  1992/11/06  06:29:44  woo
# Roger Fearick OS/2 mods
#
# Revision 3.38.1.53  1992/11/06  05:20:26  woo
# Jos fixes to specfunc documentation-- fixes to gpcard
#
# Revision 3.38.1.52  1992/11/05  00:39:01  woo
# R. Shouman notitle Option
#
# Revision 3.38.1.51  1992/11/04  23:58:39  woo
# Ed Kubaitis X11 fixes
#
# Revision 3.38.1.50  1992/11/04  21:37:10  woo
# Misc. Bug Fixes, new get_data, many improvements from D. Lewart
#
# Revision 3.38.1.49  1992/10/31  07:03:11  woo
# H Olav Eggestad RGIP Uniplex driver -- requires POSIX
#
# Revision 3.38.1.48  1992/10/31  06:05:57  woo
# J Grosh steps mod
#
# Revision 3.38.1.47  1992/10/31  03:26:29  woo
# R. Lang ylabel rotate fix
#
# Revision 3.38.1.46  1992/10/29  00:00:30  woo
# Ed Kubaitis X11 mods, surface1 demo fix
#
# Revision 3.38.1.45  1992/10/28  22:07:26  woo
# More bugfixes, some demo changes for CRAY
#
# Revision 3.38.1.44  1992/10/27  18:48:40  woo
# More bugfixes, new get_data.c from RJL, iris trm fix
#
# Revision 3.38.1.43  1992/10/23  20:08:14  woo
# Misc. bugfixes, Corel.trm, DEFAULTTERM, VMS
#
# Revision 3.38.1.42  1992/10/23  04:14:41  woo
# A. Lehmann version of Koechling pureC Atari version
#
# Revision 3.38.1.41  1992/10/22  21:16:07  woo
# R. Lang's documentation corrections
#
# Revision 3.38.1.40  1992/10/22  04:23:58  woo
# E. Youngdale LITE mods which remove hidden line removal
#
# Revision 3.38.1.39  1992/10/22  03:25:57  woo
# R. Cunningham's contours, zero, mixed data fixes
#
# Revision 3.38.1.38  1992/10/22  02:05:13  woo
# R. Toy's PSTRICKS driver
#
# Revision 3.38.1.37  1992/10/19  05:24:40  woo
# New R. Lang MS-Windows Mods
#
# Revision 3.38.1.36  1992/10/09  17:38:48  woo
# MIF terminal description added to gnuplot.doc
#
# Revision 3.38.1.35  1992/10/08  21:47:33  woo
# A. Lehmman PIPES for gcc on Atari and fixes
#
# Revision 3.38.1.34  1992/10/08  21:04:21  woo
# J. Richardson Apollo mods
#
# Revision 3.38.1.33  1992/10/08  20:30:43  woo
# Y. Arrouye fix to post.trm and empty title fix
#
# Revision 3.38.1.32  1992/10/08  20:03:51  woo
# C. Sophocleous fix to readline.c
#
# Revision 3.38.1.31  1992/10/08  19:43:14  woo
# Gershon fix of tpw@ama.caltech.edu bug
#
# Revision 3.38.1.30  1992/10/08  19:04:07  woo
# Neal Holtz tgif driver
#
# Revision 3.38.1.29  1992/10/08  17:44:21  woo
# Removed ANSI C prototypes from tpic.trm
#
# Revision 3.38.1.28  1992/10/08  17:27:08  woo
# Eric Youngdale Imagen driver mods
#
# Revision 3.38.1.27  1992/10/08  17:17:22  woo
# Rob Cunningham addition of datafiles to help plot
#
# Revision 3.38.1.26  1992/10/08  16:25:42  woo
# More D. Tabor contour mods
#
# Revision 3.38.1.25  1992/09/28  05:59:16  woo
# Changed Default Font to Helvetica in PostScript
#
# Revision 3.38.1.24  1992/09/28  04:56:48  woo
# L. Crowl mods for logs to any base
#
# Revision 3.38.1.23  1992/09/28  02:06:30  woo
# Honoo Suzuki TPIC driver
#
# Revision 3.38.1.22  1992/09/27  19:49:08  woo
# Chris Parks CorelDraw! driver
#
# Revision 3.38.1.21  1992/09/27  19:29:35  woo
# Tom Swiler's new HPGL2 driver
#
# Revision 3.38.1.20  1992/09/27  19:11:57  woo
# More R. Eckardt's ISC-2.2 fixes
#
# Revision 3.38.1.19  1992/09/27  19:06:10  woo
# More R. Lang's DJGPP mods
#
# Revision 3.38.1.18  1992/08/27  23:36:16  woo
# R. Lang's DJGPP mods and djsvga terminal driver
#
# Revision 3.38.1.17  1992/08/26  04:59:50  woo
# R. Eckardt's ISC-2.2 fixes
#
# Revision 3.38.1.16  1992/08/26  03:34:04  woo
# Gershon auto splot fix
#
# Revision 3.38.1.15  1992/08/22  04:52:47  woo
# R Lang's mods for Borland C++ 3.1 Phar Lap Dos 286Extender
#
# Revision 3.38.1.14  1992/08/22  04:43:38  woo
# P. Klosowski's Talaris EXCL driver
#
# Revision 3.38.1.13  1992/08/22  03:44:11  woo
# C. Steger's Amiga Mods
#
# Revision 3.38.1.12  1992/08/08  21:29:55  woo
# A. Lehmann's Atari Mods
#
# Revision 3.38.1.11  1992/08/08  19:33:09  woo
# Frame Maker MIF 3.0 format driver
#
# Revision 3.38.1.10  1992/08/08  06:15:06  woo
# E Youngdale's memory & speed fix for hiddenline removal
#
# Revision 3.38.1.9  1992/08/08  05:23:07  woo
# R Lang's fix to sorting for hiddenline removal + Cray fix
#
# Revision 3.38.1.8  1992/07/12  22:51:13  woo
# Yehavi Bourvine mods, new BUILDVMS, command.c fix
#
# Revision 3.38.1.7  1992/07/12  05:45:39  woo
# Luecken mods, hp26, post
#
# Revision 3.38.1.6  1992/07/11  04:56:15  woo
# Additions to gnuplot.doc for binary files, day & month tics
#
# Revision 3.38.1.5  1992/07/10  04:55:01  woo
# July 3rd MS Windows mod
#
# Revision 3.38.1.4  1992/07/09  14:52:27  woo
# Fix sscanf in command.c, cdecl in graph3d.c & new using.dem
#
# Revision 3.38.1.3  1992/07/09  05:13:30  woo
# New Latex Driver
#
# Revision 3.38.1.2  1992/07/08  05:23:20  woo
# DOS Memory fix, GPHUGE pointers
#
# Revision 3.38.1.1  1992/07/08  05:06:53  woo
# ShowSubTopics fix
#
# Revision 3.38  1992/06/17  03:55:15  woo
# MS Windows patches, day_of_week mod, gnubin fix, texdraw, new hidden mods
#
# Revision 3.37  1992/05/28  03:31:19  woo
# bug fix of runaway gnuplot_x11.c, SUN util.c, xlib driver & new hidden line removal routines
#
# Revision 3.36  1992/04/18  05:47:28  woo
# bug fix of thru mod, new stat.inc and prob.dem
#
# Revision 3.35  1992/03/31  04:49:54  woo
# BETA 1 of version 3.3 - bug fixes to binary routines
#
# Revision 3.34  1992/03/27  06:08:57  woo
# mods for VMS X11 to gnuplot_x11.c and x11.trm
#
# Revision 3.33  1992/03/27  05:14:14  woo
# add binary files, does not work under MSDOS BCC 2.0
#
# Revision 3.32  1992/03/20  04:15:09  woo
# add atari, metafont, hp2623 mods, more nec, pcl5 options
#
# Revision 3.31  1992/03/17  17:03:29  woo
# add regis, ln03p, pc getenv, paintjet, iso8869, hpgl, pcl, doc2info.pl
#
# Revision 3.30  1992/03/17  03:43:08  woo
# add sgtty readline mods and thru mods, detected CRAY bug
#
# Revision 3.29  1992/03/16  00:10:19  woo
# add discrete contour levels
#
# Revision 3.28  1992/03/15  16:06:22  woo
# add calln and probability density mods
#
# Revision 3.25  1992/03/14  21:41:36  woo
# gnuplot3.2, beta 5
#
# Revision 3.24  1992/02/29  16:23:41  woo
# gnuplot3.2, beta 4
#
# Revision 3.23  1992/02/21  20:18:16  woo
# gnuplot3.2, beta 3
#
#
############################################################
#
RCSVER = 3.50.1.17
RCSCOM = "V. Khera's fig patch"
# List of source files
# Used for making shar files, lint, and some dependencies.
DIRS = term demo docs docs/latextut

CSOURCE1 = bf_test.c binary.c command.c setshow.c
CSOURCE2 = help.c gnubin.c graphics.c graph3d.c internal.c
CSOURCE3 = misc.c eval.c parse.c plot.c readline.c scanner.c standard.c
CSOURCE4 = bitmap.c term.c util.c version.c
CSOURCE5 = term/ai.trm term/amiga.trm term/aed.trm term/atari.trm \
	term/bigfig.trm term/cgi.trm term/corel.trm \
	term/djsvga.trm term/dumb.trm \
	term/dxf.trm term/dxy.trm term/debug.trm \
	term/emxvga.trm term/eepic.trm term/epson.trm term/excl.trm \
	term/fig.trm term/grass.trm term/hp26.trm term/hp2648.trm term/hpgl.trm \
	term/hp500c.trm term/hpljii.trm term/metafont.trm term/mgr.trm\
	term/apollo.trm term/gpr.trm term/hppj.trm term/compact.c
CSOURCE6 = term/impcodes.h term/imagen.trm term/next.trm term/object.h \
	term/iris4d.trm term/kyo.trm term/latex.trm term/mif.trm \
	term/pbm.trm term/pslatex.trm term/gpic.trm term/gnugraph.trm
CSOURCE7 = term/post.trm term/pstricks.trm term/qms.trm term/regis.trm \
	term/rgip.trm term/sun.trm \
	term/t410x.trm term/tek.trm term/texdraw.trm term/tgif.h \
	term/tgif.trm term/tpic.trm \
	term/unixpc.trm term/unixplot.trm \
	term/v384.trm term/vws.trm term/x11.trm term/xlib.trm
CSOURCE8 = contour.c specfun.c gplt_x11.c
NEXTSRC  = epsviewe.m epsviewe.h
# not C code, but still needed

DEMOS = demo/1.dat demo/2.dat demo/3.dat demo/contours.dem \
	demo/controls.dem demo/electron.dem demo/glass.dat demo/param.dem \
	demo/polar.dem demo/simple.dem demo/surface1.dem \
	demo/surface2.dem demo/using.dat demo/using.dem demo/world.cor \
	demo/world.dat demo/world.dem \
	demo/err.dat demo/poldat.dem demo/polar.dat demo/errorbar.dem \
	demo/antenna.dat demo/all.dem demo/animate.dem demo/bivariat.dem \
	demo/prob.dem demo/stat.inc demo/prob2.dem demo/random.dem \
	demo/discrete.dem demo/hidden.dem demo/airfoil.dem demo/gnuplot.rot\
	demo/binary.dem demo/spline.dem demo/steps.dem demo/steps.dat \
	demo/multimsh.dem demo/whale.dat demo/hemisphr.dat \
	demo/scatter.dem demo/scatter2.dat demo/singulr.dem demo/klein.dat 

CONFIGURE = configure configure.in Makefile.in docs/Makefile.in\
	docs/latextut/Makefile.in 0CONFIG

ETC = Copyright 0README README.gnu README.ami makefile.unx makefile.vms \
	linkopt.amg makefile.amg makefile.ami linkopt.vms buildvms.com \
	lasergnu makefile.r makefile.nt makefile.g 0FAQ 0BUGS\
	term/README History gnuplot.el intergra.x11 0INSTALL README.3p1\
	README.3p2 README.3p3 README.3p4 README.pro README.nex README.x11 \
	README.3d README.mf README.win README.iso README.3p5 README.pic \
	README.xli $(CONFIGURE)

#BETA files (not standard distribution files)
BETA = 
# PC-specific files
PC = corgraph.asm corplot.c header.mac hrcgraph.asm lineproc.mac \
	linkopt.msc makefile.msc makefile.tc makefile.st makefile.djg \
	pcgraph.asm gnuplot.def makefile.286 makefile.emx \
	makefile.ztc linkopt.ztc term/fg.trm term/pc.trm 
WINDOWS = makefile.win makefile.msw README.win win/wcommon.h \
	win/wgnuplib.c win/wgnuplib.def win/wgnuplib.h win/wgnuplib.rc \
	win/wgnuplot.def win/wgnuplot.hpj win/wgnuplot.mnu win/wgnuplot.rc \
	win/wgraph.c win/winmain.c win/wmenu.c win/wpause.c \
	win/wprinter.c win/wresourc.h win/wtext.c win/wtext.h \
	win/geticon.c docs/doc2rtf.c term/win.trm
OS2 = makefile.os2 os2/makefile os2/dialogs.c os2/dialogs.h os2/gclient.c \
	os2/gnuicon.uue os2/gnupmdrv.c os2/gnupmdrv.def os2/gnupmdrv.h \
	os2/gnupmdrv.itl os2/gnupmdrv.rc os2/print.c docs/doc2ipf.c \
	README.os2 term/pm.trm

# Documentation and help files
DOCS1 = docs/makefile docs/README docs/checkdoc.c docs/doc2gih.c \
	docs/doc2hlp.c docs/doc2hlp.com docs/doc2ms.c docs/doc2tex.c \
	docs/gnuplot.1 docs/lasergnu.1 docs/toc_entr.sty docs/doc2info.pl \
	docs/titlepag.ms docs/titlepag.tex docs/makefile.ami \
	docs/doc2rtf.c 
DOCS2 = docs/gnuplot.doc docs/gpcard.tex
DOCS3 = docs/latextut/makefile docs/latextut/eg1.plt \
	docs/latextut/eg2.plt docs/latextut/eg3.dat docs/latextut/eg3.plt \
	docs/latextut/eg4.plt docs/latextut/eg5.plt docs/latextut/eg6.plt \
	docs/latextut/header.tex docs/latextut/tutorial.tex \
	docs/latextut/linepoin.plt 

#########################################################################
################################################################
# Miscellaneous targets

SOURCES=plot.h help.h setshow.h bitmap.h term.h $(CSOURCE1) $(CSOURCE2) \
	$(CSOURCE3) $(CSOURCE4) $(CSOURCE5) $(CSOURCE6) $(CSOURCE7)\
	$(CSOURCE8) $(NEXTSRC) $(WINDOWS) $(OS2)

DOCS  = $(DOCS1) $(DOCS2) $(DOCS3)

lint:
	lint -hx $(SOURCES)

clean:
	rm -f *.o *.orig *.rej *~ *.bak term/*~ term/*.orig term/*.bak
	( cd docs; $(MAKE) $(MFLAGS) clean )
	( cd docs/latextut; $(MAKE) $(MFLAGS) clean )

spotless:
	rm -f *.o *~ *.orig *.rej *.bak term/*~ term/*.orig term/*.bak \
	TAGS gnuplot gnuplot_x11 \
	bf_test demo/binary[1-3]
	( cd docs; $(MAKE) $(MFLAGS) clean )
	( cd docs/latextut; $(MAKE) $(MFLAGS) spotless )

################################################################
# Making shar files for mailing gnuplot

shar: gnuplot.sh00 gnuplot.sh01 gnuplot.sh02 gnuplot.sh03 gnuplot.sh04 \
	gnuplot.sh05 gnuplot.sh06 gnuplot.sh07 gnuplot.sh08 \
	gnuplot.sh09 gnuplot.sh10 gnuplot.sh11 gnuplot.sh12 \
	gnuplot.sh13 gnuplot.sh14 gnuplot.sh15 gnuplot.sh16

gnuplot.sh00:
	echo '#!/bin/sh' > gnuplot.sh00
	echo '# This is a shell file to make directories' >> gnuplot.sh00
	echo mkdir $(DIRS) >> gnuplot.sh00

gnuplot.sh01: $(ETC)
	shar $(ETC) > gnuplot.sh01

gnuplot.sh02: $(DOCS1)
	shar $(DOCS1) > gnuplot.sh02

gnuplot.sh03: $(DOCS2)
	shar $(DOCS2) > gnuplot.sh03

gnuplot.sh04: $(DOCS3)
	shar $(DOCS3) > gnuplot.sh04

gnuplot.sh05: $(CSOURCE1)
	shar $(CSOURCE1) > gnuplot.sh05

gnuplot.sh06: $(CSOURCE2)
	shar $(CSOURCE2) > gnuplot.sh06

gnuplot.sh07: $(CSOURCE3)
	shar $(CSOURCE3) > gnuplot.sh07

gnuplot.sh08: $(CSOURCE4)
	shar $(CSOURCE4) > gnuplot.sh08

gnuplot.sh09: $(CSOURCE5)
	shar $(CSOURCE5) > gnuplot.sh09

gnuplot.sh10: $(CSOURCE6)
	shar $(CSOURCE6) > gnuplot.sh10

gnuplot.sh11: $(CSOURCE7)
	shar $(CSOURCE7) > gnuplot.sh11

gnuplot.sh12: $(PC)
	shar $(PC) > gnuplot.sh12

gnuplot.sh13: $(CSOURCE8)
	shar $(CSOURCE8) > gnuplot.sh13

gnuplot.sh14: $(DEMOS)
	shar $(DEMOS) > gnuplot.sh14

gnuplot.sh15: $(WINDOWS)
	shar $(WINDOWS) > gnuplot.sh15

gnuplot.sh16: $(BETA)
	shar $(BETA) > gnuplot.sh16

tar: $(ETC) $(SOURCES) $(PC) $(DEMOS) $(BETA) $(DOCS)
	$(TAR) cvf /tmp/gnuplot.tar $(ETC) $(SOURCES) $(PC)\
	     $(DEMOS) $(BETA) $(DOCS)
#
# the following uses Rick Saltz's makekit shar generation tools
#

kit: $(ETC) $(SOURCES) $(PC) $(DEMOS) $(BETA) $(DOCS)
	makekit -s135k -k30 $(ETC) $(SOURCES) $(PC)\
	     $(DEMOS) $(BETA) $(DOCS) MANIFEST

branch: rcs rcsdoc rcsdemo

rcs:
	rcs -b$(RCSVER) $(ETC) $(SOURCES) $(PC)

rcsdoc:
	rcs -b$(RCSVER) $(DOCS)

rcsdemo:
	rcs -b$(RCSVER) $(DEMOS)

ciall: ci cidocs cidemo

ci:
	ci -l$(RCSVER) -m$(RCSCOM) -t-$(RCSCOM) $(SOURCES) $(PC) $(ETC)

cidocs:
	ci -l$(RCSVER) -m$(RCSCOM) -t-$(RCSCOM) $(DOCS)

cidemo:
	ci -l$(RCSVER) -m$(RCSCOM) -t-$(RCSCOM) $(DEMOS)

ciforce:
	ci -f$(RCSVER) -m$(RCSCOM) -t-$(RCSCOM) $(SOURCES) $(ETC) $(DOCS) $(DEMOS) $(PC)

coall: co codocs codemo

co:
	co -l -r$(RCSVER) $(ETC) $(SOURCES) $(PC)

codocs:
	co -l -r$(RCSVER) $(DOCS)

codemo:
	co -l -r$(RCSVER) $(DEMOS)

