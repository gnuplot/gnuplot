# This requires pmview.exe on the PATH. You can change the GIF viewer
# for any other (explore.exe -q, for instance...)

! awk -f 4demo.awk Demo.gp	| \
  sed  "s/#BEGIN/set term gif/; \
	s/#BEFORE_PLOT/set out 'demo.gif'/; \
	s/#AFTER_PLOT/set out/; \
	s/#BEFORE_PAUSE/!pmview demo.gif/; \
	s/pause/#pause/" \
  > TMP.gp
 
load 'TMP.gp'

!del demo.gif
#!del TMP.gp
