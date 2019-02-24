/* Convert output from plotstyles.gnu (winhelp) to OS/2 bitmap format.
   Requires GBM 1.76 tools (available on hobbes.nmsu.edu) 
*/

dir = "windows\"
"@echo off"
"rxqueue /clear"
"dir /b" dir || "*.png |rxqueue"
do while queued() > 0
  parse pull name
  parse var name fname "." ext
  nname = fname || ".bmp"
  "gbmconv" dir || name nname
end
