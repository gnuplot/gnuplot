# make one postscript file with all demo pictures. Use ghostview for previewing

! awk -f 4demo.awk Demo.gp | \
  sed  "s/#BEGIN/set term postscript enhanced color; set out 'demo.ps'/; \
	s/pause/#pause/" \
  > TMP.gp
 
load 'TMP.gp'

print "Use ghostview for previewing file 'demo.ps'"
