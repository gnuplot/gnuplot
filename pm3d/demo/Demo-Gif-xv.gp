# gif with xv previewer

! awk -f 4demo.awk Demo.gp	| \
  sed  "s/#BEGIN/set term gif/; \
	s/#BEFORE_PLOT/set out 'demo.gif'/; \
	s/#AFTER_PLOT/set out/; \
	s/#BEFORE_PAUSE/!xv demo.gif/; \
	s/pause/#pause/" \
  > TMP.gp

load 'TMP.gp'

!rm demo.gif
#!rm TMP.gp
