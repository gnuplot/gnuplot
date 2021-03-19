/*
 * This section is added to the user manual in place of separate sections
 * for a whole bunch of legacy HP plotter/printer terminal drivers.
 */
#ifdef TERM_HELP
START_HELP(hpgeneric)
"1 HP terminals",
" Gnuplot provides two generic terminals for old Hewlett-Packard pen plotters",
" and printers. The HPGL printer control language was introduced in 1974 and is",
" recognized by many plotters and printers from that era. See `set term hpgl`.",
" The PCL5 printer control language was introduced in 1990 and became standard",
" for many devices by HP and others.  See `set term pcl5`.",
" Both of these terminals are included in gnuplot by default.",
"",
" There are also legacy terminals to support specific plotters (hp26 hp2648),",
" inkjet printers (hp500c), and early laserjet printers (hplj hpljii).",
" These are not included unless you modify the source code file `term.h`.",
""
END_HELP(hpgeneric)
#endif
