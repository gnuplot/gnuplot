!         
! GNUPLOT Makefile for VMS, Vers. 2.0, 1996/07/09
!
! "$ MMS" or "$ MMK" makes GNUPLOT.(E,A)XE, gnuplot_X11.(E,A)XE, GNUPLOT.HLB
! "$ MMS ALL" makes in addition GNUPLOT.HTML, GNUPLOT.TEX
! "$ MMS/MACRO=VAXC" or "$ MMS/MACRO=GNUC" for the other compilers.
!
! (Rolf Niepraschk, niepraschk@ptb.de)
!

.IFDEF SCNDCALL  !------------------- second call ------------

.IFDEF AXP
O=ABJ
X=AXE
.ELSE
O=OBJ
X=EXE
.ENDIF

T = [.TERM]
D = [.DOCS]
L = [.DOCS.LATEXTUT]
M = [.DEMO]

DEFAULT : gnuplot.$(X) gnuplot_X11.$(X) $(M)binary1 gnuplot.hlb
	@ !

ALL : DEFAULT gnuplot.html $(D)gnuplot.tex
	@ !

.IFDEF GNUC
CC = GCC
CFLAGS = /NOOP/define=(ANSI_C,NOGAMMA,NO_GIH,NO_LOCALE_H,X11,PIPES,VAXCRTL)
CRTL_SHARE = ,GNU_CC:[000000]GCCLIB.OLB/lib,$(CWD)linkopt.vms/opt
.ENDIF

.IFDEF VAXC
CFLAGS = /STAND=VAXC/NOOP/define=(NOGAMMA,NO_GIH,NO_LOCALE_H,X11,PIPES,VAXCRTL)
CRTL_SHARE = ,linkopt.vms/opt
.ENDIF

.IFDEF DECC
CFLAGS = /NOOP/define=(ANSI_C,NOGAMMA,NO_GIH,NO_LOCALE_H,X11,PIPES,DECCRTL)/prefix=all
CRTL_SHARE =
.ENDIF	
	
.SUFFIXES :                   ! clear the suffix list first
.SUFFIXES : .$(X) .$(O) .C 
.C.$(O) :
	$(CC) /OBJ=$@ $(CFLAGS) $< 
                  
.$(O).$(X) :         
	LINK /EXE=$@ $(CRTL_SHARE) $+ 
          	            
X11_LIB = SYS$SHARE:DECW$XLIBSHR/SHARE
X11OPT_FILE = x11vms.opt
OPT_FILE = gnuplot.opt

CREATE_OPT = @genopt.com
PURGE = purge /nolog
CD = SET DEFAULT
CWD = SYS$DISK:[]
SAY = WRITE SYS$OUTPUT
 
TERMFLAGS = /INCL=($(T),$(CWD))   
         
.INCLUDE MAKEFILE.ALL 

OBJS = $(COREOBJS) version.$(O) vms.$(O)

.FIRST
	@ MAKEDIR = F$ENVIRONMENT("DEFAULT")
     
.LAST
!	@ IF F$SEARCH("$(OPT_FILE)") .NES. "" THEN DELETE /NOLOG $(OPT_FILE);*
	@ IF F$SEARCH("*.$(O)",).NES."" THEN $(PURGE) *.$(O)
	@ IF F$SEARCH("*.$(X)",).NES."" THEN $(PURGE) *.$(X)
	@ IF F$SEARCH("*.HLP",).NES."" THEN $(PURGE) *.HLP
	@ IF F$SEARCH("*.HLB",).NES."" THEN $(PURGE) *.HLB
	@ IF F$SEARCH("*.HTML",).NES."" THEN $(PURGE) *.HTML
	@ IF F$SEARCH("*.DVI",).NES."" THEN $(PURGE) *.DVI
	
$(OPT_FILE) : $(OBJS)
              @ LIST := $+
              @ $(CREATE_OPT) $@/write LIST
              
gnuplot.$(X) : $(OBJS) $(OPT_FILE) 
	link /exe=$@ $(OPT_FILE)/opt $(CRTL_SHARE)  
	@ $(SAY) ""
	@ $(SAY) "Your gnuplot executable is $@"
	@ $(SAY) ""

gnuplot_X11.$(X) : gplt_x11.$(O) $(X11OPT_FILE) 
	LINK /EXE=$@ GPLT_X11.$(O), $(X11OPT_FILE)/opt $(CRTL_SHARE)
	@ $(SAY) ""
	@ $(SAY) "Your gnuplot_x11 executable is $@"
	@ $(SAY) ""
			                
term.$(O) : term.c term.h $(CORETERM) 
	$(CC) /OBJ=$@ $(CFLAGS) $(TERMFLAGS) $*.c 
         
$(X11OPT_FILE) : 
           @ OPEN/WRITE OUT_FILE $(X11OPT_FILE)
           @ WRITE OUT_FILE "$(X11_LIB)"
           @ CLOSE OUT_FILE
           
gnuplot.hlb : gnuplot.hlp 
	@ IF "''F$SEARCH("$@")'" .EQS. "" THEN LIBRARY/CREATE/HELP $@
	LIBRARY $@ $<    
	
gnuplot.hlp : doc2hlp.$(X) $(D)gnuplot.doc 
        CREATE_DOC := $ $(CWD)$<
        CREATE_DOC $(D)gnuplot.doc $@

gnuplot.html : doc2html.$(X) $(D)gnuplot.doc 
        CREATE_DOC := $ $(CWD)$<
        CREATE_DOC $(D)gnuplot.doc $@

$(D)gnuplot.tex : doc2tex.$(X) $(D)gnuplot.doc 
        CREATE_DOC := $ $(CWD)$<
        CREATE_DOC $(D)gnuplot.doc $@  

gnuplot.dvi : $(D)gnuplot.tex $(D)titlepag.tex $(D)toc_entr.sty
	$(CD) $(D)
	LATEX $*
	LATEX $*
	RENAME $@ 'MAKEDIR'$@
	$(CD) 'MAKEDIR'
        
doc2hlp.$(X) : doc2hlp.$(O)    	
doc2html.$(X) : doc2html.$(O)          
doc2tex.$(X) : doc2tex.$(O)  

doc2hlp.$(O) doc2html.$(O) doc2tex.$(O) : $(D)termdoc.c $(D)allterm.h
	$(CC) /OBJ=$@ $(CFLAGS) $(TERMFLAGS) $(D)$*.c
		  		
$(D)allterm.h : $(CORETERM)
!       COPY /CONCATENATE $+ $@ !!!
        COPY /CONCATENATE $(T)*.trm $@

$(M)binary1 $(M)binary2 $(M)binary3 : bf_test.$(X)
	$(CD) $(M)
	RUN 'MAKEDIR'$<
	@ $(CD) 'MAKEDIR'

bf_test.$(X) : bf_test.$(O) binary.$(O) alloc.$(O)             

CLEAN :
	IF F$SEARCH("*.$(O)",).NES."" THEN DEL *.$(O);*
	IF F$SEARCH("$(M)bf_test.$(X)",).NES."" THEN DEL $(M)bf_test.$(X);*
	IF F$SEARCH("doc2tex.$(X)",).NES."" THEN DEL doc2tex.$(X);*
	IF F$SEARCH("doc2html.$(X)",).NES."" THEN DEL doc2html.$(X);*
	IF F$SEARCH("$(D)allterm.h",).NES."" THEN DEL $(D)allterm.h;*
	IF F$SEARCH("gnuplot.hlp",).NES."" THEN DEL gnuplot.hlp;*

VERYCLEAN : CLEAN
	IF F$SEARCH("gnuplot.$(X)",).NES."" THEN DEL gnuplot.$(X);*
	IF F$SEARCH("gnuplot.dvi",).NES."" THEN DEL gnuplot.dvi;*
	IF F$SEARCH("gnuplot.html",).NES."" THEN DEL gnuplot.html;*
	IF F$SEARCH("gnuplot.hlb",).NES."" THEN DEL gnuplot.hlb;*
	IF F$SEARCH("$(D)gnuplot.tex",).NES."" THEN DEL $(D)gnuplot.tex;*
	IF F$SEARCH("$(M)binary1.",).NES."" THEN DEL $(M)binary1.;*
	IF F$SEARCH("$(M)binary2.",).NES."" THEN DEL $(M)binary2.;*
	IF F$SEARCH("$(M)binary3.",).NES."" THEN DEL $(M)binary3.;*
	
.ELSE	!------------------- first call ------------

SAY = WRITE SYS$OUTPUT

? $(MMSTARGETS) : DEFAULT
	@ !

DEFAULT :

.IFDEF GNUC
	@ CCOMP = "GNUC=1"
	@ $(SAY) "Making Gnuplot with GNUC..."
.ELSE
.IFDEF VAXC
	@ CCOMP = "VAXC=1"
	@ $(SAY) "Making Gnuplot with VAXC..."	
.ELSE
	@ CCOMP = "DECC=1"	
	@ $(SAY) "Making Gnuplot with DECC..."	
.ENDIF
.ENDIF	
	@ $(SAY) ""		
	@ PARAM = "/MACRO=(SCNDCALL=1,''CCOMP')"
	@ IF F$GETSYI("ARCH_TYPE") .NE. 1 THEN \
	  PARAM = "/MACRO=(SCNDCALL=1,''CCOMP',AXP=1)"
	@ $(MMS)/IGNORE=WARNING 'PARAM' $(MMSTARGETS)	! second call
		
.ENDIF	! SCNDCALL  
