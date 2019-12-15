/*
 * This section is added to the user manual in place of the 
 * terminal descriptions for latex emtex eepic and tpic if these
 * terminals are disabled, as they are by default in version 5.4
 */
#ifdef TERM_HELP
START_HELP(latex)
"1 latex",
"?set terminal latex",
"?set term latex",
"?terminal latex",
"?term latex",
"?latex",
" Gnuplot provides a variety of terminals for use with TeX/LaTeX.",
"",
" (1) TeX/LaTeX compatible terminals based on use of PostScript",
" See  `epslatex`, `pslatex`, `mp` (metapost),  and `pstricks`.",
"",
" (2) TeX/LaTeX compatible terminals based on cairo graphics",
" See `cairolatex`.",
"",
" (3) The `tikz` terminal uses an external lua script (see `lua`)",
" to produce files for the PGF and TikZ packages.",
" Use the command `set term tikz help` to print terminal options.",
"",
" (4) The `pict2e` terminal (new in version 5.4) replaces a set of legacy",
" terminals `latex`, `emtex`, `eepic`, and `tpic` present in older versions",
" of gnuplot. See `pict2e`.",
"",
" (5) Others, see `context`, `mf` (metafont).",
""
END_HELP(latex)
#endif
