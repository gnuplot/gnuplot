# makefile for 32-bit version of GnuplotPM driver
# compiled using emx 0.8e
# -- .c files have // style comments, so require some trickery in
# -- order to compile
# --    -x c++         treat as c++ ...
# --    -U__cpluplus   but don't use some defines in .h files
# --    -E             preprocess to strip comments ...
# --    -o temp.i      into temp file
# --    $(C..temp.i    then compile preprocessed file (whew)
#

CFLAGS= -c -Zomf -Zsys
CC=gcc
INCL=/emx/include
OBJ=gnupmdrv.obj gclient.obj print.obj dialogs.obj

.c.obj:
        $(CC) -U__cplusplus -x c++ -I $(INCL) -Ic:/toolkt20/c/os2h -E -o temp.i $*.c
	$(CC) $(CFLAGS) -o $*.obj temp.i 
        del temp.i

gnupmdrv.exe: gnupmdrv.hlp gnupmdrv.res $(OBJ) gnupmdrv.def
     gcc -o gnupmdrv.exe -Zomf -Zsys $(OBJ) gnupmdrv.def -lsys -los2 
     rc gnupmdrv.res gnupmdrv.exe

help: gnupmdrv.hlp

gnupmdrv.res : gnupmdrv.rc  gnuplot.ico dialogs.h
     rc -r gnupmdrv

gnupmdrv.hlp: gnupmdrv.itl
    ipfcprep gnupmdrv.itl gnupmdrv.ii -D GENHELP
    gcc -E -o gnupmdrv.i -x c -DGENHELP gnupmdrv.ii
    ipfc gnupmdrv.i
    del gnupmdrv.ii
    del gnupmdrv.i

gnuplot.ico: gnuicon.uue
    uudecode gnuicon.uue
