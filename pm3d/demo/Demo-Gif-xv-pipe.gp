# gif with xv previewer with data piping into it (untested)

! awk -f 4demo.awk Demo.gp	| \
  sed  "s/#BEGIN/set term gif/; \
	s/#BEFORE_PLOT/set out '|xv -'/; \
	s/#AFTER_PLOT/set out;/; \
	s/pause/#pause/" \
  > TMP.gp

load 'TMP.gp'

!rm demo.gif
#!rm TMP.gp
