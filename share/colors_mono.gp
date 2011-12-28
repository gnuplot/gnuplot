#
# Provide a consistent set of four distinguishable
# black line types.
# NB: This does not work with "set term post mono"
#
set termoption dashed
unset for [i=1:4] linetype i
set linetype 4 lt 1 lw 2.5 lc rgb "black"
set linetype 3 lt 3 lw 1 lc rgb "black"
set linetype 2 lt 2 lw 1 lc rgb "black"
set linetype 1 lt -1 lw 1 lc rgb "black"
set linetype cycle 4
#
set palette gray
