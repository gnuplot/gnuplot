#ifndef lint
static char *RCSid = "$Id: gplt_x11.c%v 3.50.1.9 1993/08/05 05:38:59 woo Exp $";
#endif


/*-----------------------------------------------------------------------------
 *   gnuplot_x11 - X11 outboard terminal driver for gnuplot 3.3
 *
 *   Requires installation of companion inboard x11 driver in gnuplot/term.c
 *
 *   Acknowledgements: 
 *      Chris Peterson (MIT)
 *      Dana Chee (Bellcore) 
 *      Arthur Smith (Cornell)
 *      Hendri Hondorp (University of Twente, The Netherlands)
 *      Bill Kucharski (Solbourne)
 *      Charlie Kline (University of Illinois)
 *      Yehavi Bourvine (Hebrew University of Jerusalem, Israel)
 *      Russell Lang (Monash University, Australia)
 *      O'Reilly & Associates: X Window System - Volumes 1 & 2
 *
 *   This code is provided as is and with no warranties of any kind.
 *       
 * There is a mailing list for gnuplot users. Note, however, that the
 * newsgroup 
 *	comp.graphics.gnuplot 
 * is identical to the mailing list (they
 * both carry the same set of messages). We prefer that you read the
 * messages through that newsgroup, to subscribing to the mailing list.
 * (If you can read that newsgroup, and are already on the mailing list,
 * please send a message info-gnuplot-request@dartmouth.edu, asking to be
 * removed from the mailing list.)
 *
 * The address for mailing to list members is
 *	   info-gnuplot@dartmouth.edu
 * and for mailing administrative requests is 
 *	   info-gnuplot-request@dartmouth.edu
 * The mailing list for bug reports is 
 *	   bug-gnuplot@dartmouth.edu
 * The list of those interested in beta-test versions is
 *	   info-gnuplot-beta@dartmouth.edu
 *---------------------------------------------------------------------------*/

#define DEFAULT_X11
#if defined(VMS) || defined(CRIPPLED_SELECT)
#undef DEFAULT_X11
#endif
#if defined(VMS) && defined(CRIPPLED_SELECT)
Error. Incompatible options.
#endif

#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
 
#include <stdio.h>
#include <signal.h>

#ifdef BSD_TYPES
#include <sys/bsdtypes.h>
#endif /* BSD_TYPES */

#if !defined(VMS) && !defined(FD_SET) && !defined(OLD_SELECT)
#include <sys/select.h>
#endif /* !VMS && !FD_SET && !OLD_SELECT */
 
#ifndef FD_SET

#define FD_SET(n, p)    ((p)->fds_bits[0] |= (1 << ((n) % 32)))
#define FD_CLR(n, p)    ((p)->fds_bits[0] &= ~(1 << ((n) % 32)))
#define FD_ISSET(n, p)  ((p)->fds_bits[0] & (1 << ((n) % 32)))
#define FD_ZERO(p)      bzero((char *)(p), sizeof(*(p)))

#endif /* !FD_SET */

#ifdef SOLARIS
#include <sys/systeminfo.h>
#endif /* SOLARIS */


#include <errno.h>
extern int errno;

#define FallbackFont "fixed"

#define Ncolors 13
unsigned long colors[Ncolors];

#define Nwidths 10
unsigned int widths[Nwidths] = { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

#define Ndashes 10
char dashes[Ndashes][5];

Display *dpy; int scr; Window win, root;
Visual *vis; GC gc = (GC)0; Pixmap pixmap; XFontStruct *font;
unsigned int W = 640, H = 450; int D, gX = 100, gY = 100;
unsigned int BorderWidth = 2;
unsigned User_Size = 0, User_Position = 0; /* User specified? */


Bool Mono = 0, Gray = 0, Rv = 0, Clear = 0;
char Name[64] = "gnuplot";
char Class[64] = "Gnuplot";

int cx=0, cy=0, vchar, nc = 0, ncalloc = 0;
double xscale, yscale, pointsize;
#define X(x) (int) (x * xscale)
#define Y(y) (int) ((4095-y) * yscale)
enum JUSTIFY { LEFT, CENTRE, RIGHT } jmode;

#define Nbuf 1024
char buf[Nbuf], **commands = (char **)0;

FILE *X11_ipc;
char X11_ipcpath[32];


/*-----------------------------------------------------------------------------
 *   main program 
 *---------------------------------------------------------------------------*/

main(argc, argv) int argc; char *argv[]; {

   preset(argc, argv);
   mainloop();
   exit(0);

   }

/*-----------------------------------------------------------------------------
 *   mainloop processing - process X events and input from gnuplot
 *
 *   Three different versions of main loop processing are provided to support
 *   three different platforms.
 * 
 *   DEFAULT_X11:     use select() for both X events and input on stdin 
 *                    from gnuplot inboard driver
 *
 *   CRIPPLED_SELECT: use select() to service X events and check during 
 *                    select timeout for temporary plot file created
 *                    by inboard driver
 *
 *   VMS:             use XNextEvent to service X events and AST to
 *                    service input from gnuplot inboard driver on stdin 
 *---------------------------------------------------------------------------*/


#ifdef DEFAULT_X11
/*-----------------------------------------------------------------------------
 *    DEFAULT_X11 mainloop
 *---------------------------------------------------------------------------*/

mainloop() {
   int nf, nfds, cn = ConnectionNumber(dpy), in = fileno(stdin);
   struct timeval timeout, *timer = (struct timeval *)0;
   fd_set rset, tset;
   unsigned long all = 0xffffffff;
   XEvent xe;

   X11_ipc = stdin;

   FD_ZERO(&rset);
   FD_SET(cn, &rset);

   FD_SET(in, &rset);
   nfds = (cn > in) ? cn + 1 : in + 1;

#ifdef ISC22
/* Added by Robert Eckardt, RobertE@beta.TP2.Ruhr-Uni-Bochum.de */
   timeout.tv_sec  = 0;		/* select() in ISC2.2 needs timeout */
   timeout.tv_usec = 300000;	/* otherwise input from gnuplot is */
   timer = &timeout;		/* suspended til next X event. */
#endif /* ISC22 	 (0.3s are short enough not to be noticed */

   while(1) {
      tset = rset;
      nf = select(nfds, &tset, (fd_set *)0, (fd_set *)0, timer);
      if (nf < 0) {
	 if (errno == EINTR) continue;
	 fprintf(stderr, "gnuplot: select failed. errno:%d\n", errno);
	 exit(1);
	 }
      nf > 0 && XNoOp(dpy);
      if (FD_ISSET(cn, &tset)) {
	 while (XCheckMaskEvent(dpy, all, &xe)) {
	    (xe.type == ConfigureNotify)  && resize(&xe); 
	    }
	 }
      FD_ISSET(in, &tset) && record();
      }
   }

#endif


#ifdef CRIPPLED_SELECT
/*-----------------------------------------------------------------------------
 *    CRIPPLED_SELECT mainloop
 *---------------------------------------------------------------------------*/

mainloop() {
   int nf, nfds, cn = ConnectionNumber(dpy);
   struct timeval timeout, *timer;
   fd_set rset, tset;
   unsigned long all = 0xffffffff;
   XEvent xe;

   FD_ZERO(&rset);
   FD_SET(cn, &rset);

   timeout.tv_sec = 1;
   timeout.tv_usec = 0;
   timer = &timeout;
   sprintf(X11_ipcpath, "/tmp/Gnuplot_%d", getppid());
   nfds = cn + 1;

   while(1) {
      tset = rset;
      nf = select(nfds, &tset, (fd_set *)0, (fd_set *)0, timer);
      if (nf < 0) {
	 if (errno == EINTR) continue;
	 fprintf(stderr, "gnuplot: select failed. errno:%d\n", errno);
	 exit(1);
	 }
      nf > 0 && XNoOp(dpy);
      if (FD_ISSET(cn, &tset)) {
	 while (XCheckMaskEvent(dpy, all, &xe)) {
	    (xe.type == ConfigureNotify)  && resize(&xe); 
	    }
	 }
      if ((X11_ipc = fopen(X11_ipcpath, "r"))) {
	 unlink(X11_ipcpath);
	 record();
	 fclose(X11_ipc);
	 }
      }
   }
#endif /* CRIPPLED_SELECT */


#ifdef VMS
/*-----------------------------------------------------------------------------
 *    VMS mainloop - Yehavi Bourvine - YEHAVI@VMS.HUJI.AC.IL
 *---------------------------------------------------------------------------*/

/*  In VMS there is no decent Select(). hence, we have to loop inside
 *  XGetNextEvent for getting the next X window event. In order to get input
 *  from the master we assign a channel to SYS$INPUT and use AST's in order to
 *  receive data. */

#include <iodef.h>
char    STDIIN[] = "SYS$INPUT:";
short   STDIINchannel, STDIINiosb[4];
struct  { short size, type; char  *address; } STDIINdesc;
char    STDIINbuffer[64];
int     status;


mainloop() {
   XEvent xe;

   STDIINdesc.size = strlen(STDIIN); 
   STDIINdesc.type = 0;
   STDIINdesc.address = STDIIN;
   status = sys$assign(&STDIINdesc, &STDIINchannel, 0, 0, 0);
   if((status & 0x1) == 0)  exit(status); 
   ast();

   for(;;) {
      XNextEvent(dpy, &xe);
      (xe.type == ConfigureNotify)  && resize(&xe); 
      }
   }

ast() {
   int record();
   int status = sys$qio(0, STDIINchannel, IO$_READVBLK, STDIINiosb, record,
		        0, STDIINbuffer, sizeof(STDIINbuffer) -1,0, 0, 0, 0);
   if((status & 0x1) == 0) exit(status);
   }
#endif /* VMS */


#ifndef VMS
/*-----------------------------------------------------------------------------
 *   record - record new plot from gnuplot inboard X11 driver (Unix)
 *---------------------------------------------------------------------------*/

record() {

   while (fgets(buf, Nbuf, X11_ipc)) {
     if (*buf == 'G') {                           /* enter graphics mode */
	 if (commands) {
	    int n; for (n=0; n<nc; n++) free(commands[n]);
	    free(commands);
	    }
	 commands = (char **)0; nc = ncalloc = 0;
         }
      else if (*buf == 'E') { display(); break; } /* leave graphics mode */
      else if (*buf == 'R') { exit(0); }          /* leave X11/x11 mode  */
      else {                                      /* record command      */
	 char *p;
	 if (nc >= ncalloc) {
	    ncalloc = ncalloc*2 + 1;
	    commands = (commands)
	       ? (char **)realloc(commands, ncalloc * sizeof(char *))
	       : (char **)malloc(sizeof(char *));
	    }
	 p = (char *)malloc((unsigned)strlen(buf)+1);
	 if (!commands || !p) {
	    fprintf(stderr, "gnuplot: can't get memory. X11 aborted.\n");
	    exit(1);
	    }
	 commands[nc++] = strcpy(p, buf);
	 }
      }
   if (feof(X11_ipc) || ferror(X11_ipc)) exit(1);
   }

#else    /* VMS */
/*-----------------------------------------------------------------------------
 *   record - record new plot from gnuplot inboard X11 driver (VMS)
 *---------------------------------------------------------------------------*/

record() {
   int	status;

   if((STDIINiosb[0] & 0x1) == 0) exit(STDIINiosb[0]);
   STDIINbuffer[STDIINiosb[1]] = '\0';
   strcpy(buf, STDIINbuffer);

   if (*buf == 'G') {                           /* enter graphics mode */
      if (commands) {
         int n; for (n=0; n<nc; n++) free(commands[n]);
         free(commands);
         }
       commands = (char **)0; nc = ncalloc = 0;
       }
   else if (*buf == 'E') {                      /* leave graphics mode */
      display(); 
      }
   else if (*buf == 'R') {                      /* leave x11/X11 mode  */
       sys$cancel(STDIINchannel);
       XCloseDisplay(dpy);
       sys$delprc(0,0);	  /* Somehow it doesn't drop itself... */
       exit(1); 
       }
   else {                                       /* record command      */
      char *p;
      if (nc >= ncalloc) {
	 ncalloc = ncalloc*2 + 1;
	 commands = (commands)
	    ? (char **)realloc(commands, ncalloc * sizeof(char *))
	    : (char **)malloc(sizeof(char *));
	 }
      p = (char *)malloc((unsigned)strlen(buf)+1);
      if (!commands || !p) {
	 fprintf(stderr, "gnuplot: can't get memory. X11 aborted.\n");
	 exit(1);
	 }
      commands[nc++] = strcpy(p, buf);
      }
   ast();
   }
#endif /* VMS */

/*-----------------------------------------------------------------------------
 *   display - display last plot from gnuplot inboard X11 driver
 *---------------------------------------------------------------------------*/

display() {
   int n, x, y, sw, sl, lt, width, type, point, px, py;
   char *buf, *str;

   if (!nc) return;

   /* set scaling factor between internal driver & window geometry */
   xscale = (double)W / 4096.;  yscale = (double)H / 4096.;  

   /* create new pixmap & GC */
   if (gc) { XFreeGC(dpy, gc); XFreePixmap(dpy, pixmap); }
   pixmap = XCreatePixmap(dpy, root, W, H, D);
   gc = XCreateGC(dpy, pixmap, 0, (XGCValues *)0);
   XSetFont(dpy, gc, font->fid);

   /* set pixmap background */
   XSetForeground(dpy, gc, colors[0]);
   XFillRectangle(dpy, pixmap, gc, 0, 0, W, H);
   XSetBackground(dpy, gc, colors[0]);

   /* set new pixmap as window background */
   XSetWindowBackgroundPixmap(dpy, win, pixmap);

   /* top the window but don't put keyboard or mouse focus into it. */
   XMapRaised(dpy, win);

   /* momentarily clear the window first if requested */
   if (Clear) {
      XClearWindow(dpy, win);
      XFlush(dpy);
      }

   /* loop over accumulated commands from inboard driver */
   for (n=0; n<nc; n++) {
      buf = commands[n];

      /*   X11_vector(x,y) - draw vector  */
      if (*buf == 'V') { 
	 sscanf(buf, "V%4d%4d", &x, &y);  
	 XDrawLine(dpy, pixmap, gc, X(cx), Y(cy), X(x), Y(y));
	 cx = x; cy = y;
	 }

      /*   X11_move(x,y) - move  */
      else if (*buf == 'M') 
	 sscanf(buf, "M%4d%4d", &cx, &cy);  

      /*   X11_put_text(x,y,str) - draw text   */
      else if (*buf == 'T') { 
	 sscanf(buf, "T%4d%4d", &x, &y);  
	 str = buf + 9; sl = strlen(str) - 1;
	 sw = XTextWidth(font, str, sl);
	 switch(jmode) {
	    case LEFT:   sw = 0;     break;
	    case CENTRE: sw = -sw/2; break;
	    case RIGHT:  sw = -sw;   break;
	    }
	 XSetForeground(dpy, gc, colors[2]);
	 XDrawString(dpy, pixmap, gc, X(x)+sw, Y(y)+vchar/3, str, sl);
	 XSetForeground(dpy, gc, colors[lt+3]);
	 }

      /*   X11_justify_text(mode) - set text justification mode  */
      else if (*buf == 'J') 
	 sscanf(buf, "J%4d", &jmode);

      /*   X11_linetype(type) - set line type  */
      else if (*buf == 'L') { 
	 sscanf(buf, "L%4d", &lt);
	 lt = (lt%8)+2;
	 width = widths[lt];
	 if (dashes[lt][0]) {
	    type = LineOnOffDash;
	    XSetDashes(dpy, gc, 0, dashes[lt], strlen(dashes[lt]));
	    }
	 else {
	    type = LineSolid;
	    }
	 XSetForeground(dpy, gc, colors[lt+3]);
	 XSetLineAttributes( dpy,gc, width, type, CapButt, JoinBevel);
  	 }

      /*   X11_point(number) - draw a point */
      else if (*buf == 'P') { 
 	 sscanf(buf, "P%1d%4d%4d", &point, &x, &y);  
 	 if (point==7) {
 	    /* set point size */
 	    px = (int) (x * xscale * pointsize);
 	    py = (int) (y * yscale * pointsize);
 	    }
 	 else {
	    if (type != LineSolid || width != 0) {  /* select solid line */
	       XSetLineAttributes(dpy, gc, 0, LineSolid, CapButt, JoinBevel);
	       }
	    switch(point) {
	       case 0: /* dot */
		   XDrawLine(dpy,pixmap,gc, X(x), Y(y), X(x), Y(y));
		   break;
	       case 1: /* do diamond */ 
		   XDrawLine(dpy,pixmap,gc, X(x)-px, Y(y), X(x), Y(y)-py);
		   XDrawLine(dpy,pixmap,gc, X(x), Y(y)-py, X(x)+px, Y(y));
		   XDrawLine(dpy,pixmap,gc, X(x)+px, Y(y), X(x), Y(y)+py);
		   XDrawLine(dpy,pixmap,gc, X(x), Y(y)+py, X(x)-px, Y(y));
		   XDrawLine(dpy,pixmap,gc, X(x), Y(y), X(x), Y(y));
		   break;
	       case 2: /* do plus */ 
		   XDrawLine(dpy,pixmap,gc, X(x)-px, Y(y), X(x)+px, Y(y));
		   XDrawLine(dpy,pixmap,gc, X(x), Y(y)-py, X(x), Y(y)+py);
		   break;
	       case 3: /* do box */ 
		   XDrawLine(dpy,pixmap,gc, X(x)-px, Y(y)-py, X(x)+px, Y(y)-py);
		   XDrawLine(dpy,pixmap,gc, X(x)+px, Y(y)-py, X(x)+px, Y(y)+py);
		   XDrawLine(dpy,pixmap,gc, X(x)+px, Y(y)+py, X(x)-px, Y(y)+py);
		   XDrawLine(dpy,pixmap,gc, X(x)-px, Y(y)+py, X(x)-px, Y(y)-py);
		   XDrawLine(dpy,pixmap,gc, X(x), Y(y), X(x), Y(y));
		   break;
	       case 4: /* do X */ 
		   XDrawLine(dpy,pixmap,gc, X(x)-px, Y(y)-py, X(x)+px, Y(y)+py);
		   XDrawLine(dpy,pixmap,gc, X(x)-px, Y(y)+py, X(x)+px, Y(y)-py);
		   break;
	       case 5: /* do triangle */ 
		   XDrawLine(dpy,pixmap,gc, X(x), Y(y)-(4*px/3), 
			     X(x)-(4*px/3), Y(y)+(2*py/3));
		   XDrawLine(dpy,pixmap,gc, X(x), Y(y)-(4*px/3), 
			     X(x)+(4*px/3), Y(y)+(2*py/3));
		   XDrawLine(dpy,pixmap,gc, X(x)-(4*px/3), Y(y)+(2*py/3), 
			     X(x)+(4*px/3), Y(y)+(2*py/3));
		   XDrawLine(dpy,pixmap,gc, X(x), Y(y), X(x), Y(y));
		   break;
	       case 6: /* do star */ 
		   XDrawLine(dpy,pixmap,gc, X(x)-px, Y(y), X(x)+px, Y(y));
		   XDrawLine(dpy,pixmap,gc, X(x), Y(y)-py, X(x), Y(y)+py);
		   XDrawLine(dpy,pixmap,gc, X(x)-px, Y(y)-py, X(x)+px, Y(y)+py);
		   XDrawLine(dpy,pixmap,gc, X(x)-px, Y(y)+py, X(x)+px, Y(y)-py);
		   break;
	       }
	    if (type != LineSolid || width != 0) {  /* select solid line */
	       XSetLineAttributes(dpy, gc, width, type, CapButt, JoinBevel);
	       }
	    }
	 }
      }

   /* trigger exposure of background pixmap */
   XClearWindow(dpy,win);
   XFlush(dpy);
   }

/*-----------------------------------------------------------------------------
 *   resize - rescale last plot if window resized
 *---------------------------------------------------------------------------*/

resize(xce) XConfigureEvent *xce; {
   int w = xce->width, h = xce->height;
   
   if (w>1 && h>1 && (w != W || h != H)) {
      W = w; H = h;
      display();
      }
   }


/*-----------------------------------------------------------------------------
 *   preset - determine options, open display, create window
 *---------------------------------------------------------------------------*/

#define On(v) ( !strcmp(v,"on") || !strcmp(v,"true") || \
		!strcmp(v,"On") || !strcmp(v,"True") )

#define AppDefDir "/usr/lib/X11/app-defaults"
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

static XrmDatabase dbCmd, dbApp, dbDef, dbEnv, db = (XrmDatabase)0;

char *pr_GetR(), *getenv(), *type[20];
XrmValue value;

#define Nopt 27
static XrmOptionDescRec options[] = {
   {"-mono",             ".mono",             XrmoptionNoArg,   "on" },
   {"-gray",             ".gray",             XrmoptionNoArg,   "on" },
   {"-clear",            ".clear",            XrmoptionNoArg,   "on" },
   {"-tvtwm",            ".tvtwm",            XrmoptionNoArg,   "on" },
   {"-pointsize",        ".pointsize",        XrmoptionSepArg,  NULL },
   {"-display",          ".display",          XrmoptionSepArg,  NULL },
   {"-name",             ".name",             XrmoptionSepArg,  NULL },
   {"-geometry",         "*geometry",         XrmoptionSepArg,  NULL },
   {"-background",       "*background",       XrmoptionSepArg,  NULL },
   {"-bg",               "*background",       XrmoptionSepArg,  NULL },
   {"-foreground",       "*foreground",       XrmoptionSepArg,  NULL },
   {"-fg",               "*foreground",       XrmoptionSepArg,  NULL },
   {"-bordercolor",      "*bordercolor",      XrmoptionSepArg,  NULL },
   {"-bd",               "*bordercolor",      XrmoptionSepArg,  NULL },
   {"-borderwidth",      ".borderwidth",      XrmoptionSepArg,  NULL },
   {"-bw",               ".borderwidth",      XrmoptionSepArg,  NULL },
   {"-font",             "*font",             XrmoptionSepArg,  NULL },
   {"-fn",               "*font",             XrmoptionSepArg,  NULL },
   {"-reverse",          "*reverseVideo",     XrmoptionNoArg,   "on" },
   {"-rv",               "*reverseVideo",     XrmoptionNoArg,   "on" },
   {"+rv",               "*reverseVideo",     XrmoptionNoArg,   "off"},
   {"-iconic",           "*iconic",           XrmoptionNoArg,   "on" },
   {"-synchronous",      "*synchronous",      XrmoptionNoArg,   "on" },
   {"-xnllanguage",      "*xnllanguage",      XrmoptionSepArg,  NULL },
   {"-selectionTimeout", "*selectionTimeout", XrmoptionSepArg,  NULL },
   {"-title",            ".title",            XrmoptionSepArg,  NULL },
   {"-xrm",              NULL,                XrmoptionResArg,  NULL },
   };

preset(argc, argv) int argc; char *argv[]; {
   int Argc = argc; char **Argv = argv;

#ifdef VMS
   char *display = (char *) 0;
#else
   char *display = getenv("DISPLAY");
#endif
   char *home = getenv("HOME");
   char *server_defaults, *env, buf[256];

   /*---set to ignore ^C and ^Z----------------------------------------------*/

   signal(SIGINT, SIG_IGN);
#ifdef SIGTSTP
   signal(SIGTSTP, SIG_IGN);
#endif

   /*---prescan arguments for "-name"----------------------------------------*/

   while(++Argv, --Argc > 0) {
      if (!strcmp(*Argv, "-name") && Argc > 1) {
	 strncpy(Name, Argv[1], 64);
	 strncpy(Class, Argv[1], 64);
	 if (Class[0] >= 'a' && Class[0] <= 'z') Class[0] -= 0x20;
	 }
      }
   Argc = argc; Argv = argv;

   /*---parse command line---------------------------------------------------*/

   XrmInitialize();
   XrmParseCommand(&dbCmd, options, Nopt, Name, &Argc, Argv);
   if (Argc > 1) {
      fprintf(stderr, "\ngnuplot: bad option: %s\n", Argv[1]);
      fprintf(stderr, "gnuplot: X11 aborted.\n");
      exit(1);
      }
   if (pr_GetR(dbCmd, ".display")) display = value.addr;

   /*---open display---------------------------------------------------------*/

   dpy = XOpenDisplay(display); 
   if (!dpy) {
      fprintf(stderr, "\ngnuplot: unable to open display '%s'\n", display);
      fprintf(stderr, "gnuplot: X11 aborted.\n");
      exit(1);
      }
   scr = DefaultScreen(dpy);
   vis = DefaultVisual(dpy,scr);
   D = DefaultDepth(dpy,scr);
   root = DefaultRootWindow(dpy);
   server_defaults = XResourceManagerString(dpy);

   /*---get application defaults--(subset of Xt processing)------------------*/

#ifdef VMS
   strcpy (buf, "DECW$USER_DEFAULTS:GNUPLOT_X11.INI");
#else
   sprintf(buf, "%s/%s", AppDefDir, "Gnuplot");
#endif
   dbApp = XrmGetFileDatabase(buf);
   XrmMergeDatabases(dbApp, &db);

   /*---get server or ~/.Xdefaults-------------------------------------------*/

   if (server_defaults)
      dbDef = XrmGetStringDatabase(server_defaults);
   else {
#ifdef VMS
      strcpy(buf,"DECW$USER_DEFAULTS:DECW$XDEFAULTS.DAT");
#else
      sprintf(buf, "%s/.Xdefaults", home);
#endif
      dbDef = XrmGetFileDatabase(buf);
      }
   XrmMergeDatabases(dbDef, &db);

   /*---get XENVIRONMENT or  ~/.Xdefaults-hostname---------------------------*/

#ifndef VMS
   if (env = getenv("XENVIRONMENT")) 
      dbEnv = XrmGetFileDatabase(env);
   else {
      char *p, host[MAXHOSTNAMELEN];
#ifdef SOLARIS
      if (sysinfo(SI_HOSTNAME, host, MAXHOSTNAMELEN) < 0) {
         fprintf(stderr, "gnuplot: sysinfo failed. X11 aborted.\n");
#else
      if (gethostname(host, MAXHOSTNAMELEN) < 0) {
         fprintf(stderr, "gnuplot: gethostname failed. X11 aborted.\n");
#endif /* SOLARIS */
	 exit(1);
	 }
      if (p = index(host, '.')) *p = '\0';
      sprintf(buf, "%s/.Xdefaults-%s", home, host);
      dbEnv = XrmGetFileDatabase(buf);
      }
   XrmMergeDatabases(dbEnv, &db);
#endif   /* not VMS */

   /*---merge command line options-------------------------------------------*/

   XrmMergeDatabases(dbCmd, &db);

   /*---set geometry, font, colors, line widths, dash styles, point size-----*/

   pr_geometry();
   pr_font();
   pr_color();
   pr_width();
   pr_dashes();
   pr_pointsize();

   /*---create window--------------------------------------------------------*/

   pr_window();

   } 

/*-----------------------------------------------------------------------------
 *   pr_GetR - get resource from database using "-name" option (if any)
 *---------------------------------------------------------------------------*/

char *
pr_GetR(db, resource) XrmDatabase db; char *resource; {
   char name[128], class[128], *rc;

   strcpy(name, Name); strcat(name, resource);
   strcpy(class, Class); strcat(class, resource);
   rc = XrmGetResource(db, name, class, type, &value)
      ? (char *)value.addr 
      : (char *)0;
   return(rc);
   }

/*-----------------------------------------------------------------------------
 *   pr_color - determine color values
 *---------------------------------------------------------------------------*/

char color_keys[Ncolors][30] =   { 
   "background", "bordercolor", "text", "border", "axis", 
   "line1", "line2", "line3",  "line4", 
   "line5", "line6", "line7",  "line8" 
   };
char color_values[Ncolors][30] = { 
   "white", "black",  "black",  "black",  "black", 
   "red",   "green",  "blue",   "magenta", 
   "cyan",  "sienna", "orange", "coral" 
   };
char gray_values[Ncolors][30] = { 
   "black",   "white",  "white",  "gray50", "gray50",
   "gray100", "gray60", "gray80", "gray40", 
   "gray90",  "gray50", "gray70", "gray30" 
   };

pr_color() {
   unsigned long black = BlackPixel(dpy, scr), white = WhitePixel(dpy,scr);
   char option[20], color[30], *v, *type; 
   XColor xcolor;
   Colormap cmap;
   double intensity = -1;
   int n;

   pr_GetR(db, ".mono")         && On(value.addr) && Mono++;
   pr_GetR(db, ".gray")         && On(value.addr) && Gray++;
   pr_GetR(db, ".reverseVideo") && On(value.addr) && Rv++;

   if (!Gray && (vis->class == GrayScale || vis->class == StaticGray)) Mono++;

   if (!Mono) {
      cmap = DefaultColormap(dpy, scr);
      type = (Gray) ? "Gray" : "Color";

      for (n=0; n<Ncolors; n++) {
	 strcpy(option, ".");
	 strcat(option, color_keys[n]);
	 (n > 1) && strcat(option, type);
	 v = pr_GetR(db, option) 
	     ? value.addr
	     : ((Gray) ? gray_values[n] : color_values[n]);

	 if (sscanf(v,"%30[^,],%lf", color, &intensity) == 2) {
	    if (intensity < 0 || intensity > 1) {
	       fprintf(stderr, "\ngnuplot: invalid color intensity in '%s'\n",
                       color);
	       intensity = 1;
	       }
	    }
	 else { 
	    strcpy(color, v);
	    intensity = 1;
	    }

	 if (!XParseColor(dpy, cmap, color, &xcolor)) {
	    fprintf(stderr, "\ngnuplot: unable to parse '%s'. Using black.\n",
                    color);
	    colors[n] = black;
	    }
	 else {
	    xcolor.red *= intensity;
	    xcolor.green *= intensity;
	    xcolor.blue *= intensity;
	    if (XAllocColor(dpy, cmap, &xcolor)) {
	       colors[n] = xcolor.pixel;
	       }
	    else {
	       fprintf(stderr, "\ngnuplot: can't allocate '%s'. Using black.\n",
                        v);
	       colors[n] = black;
	       }
	    }
	 }
      }
   else {
      colors[0] = (Rv) ? black : white ;
      for (n=1; n<Ncolors; n++)  colors[n] = (Rv) ? white : black;
      }
   }

/*-----------------------------------------------------------------------------
 *   pr_dashes - determine line dash styles 
 *---------------------------------------------------------------------------*/

char dash_keys[Ndashes][10] =   { 
   "border", "axis",
   "line1", "line2", "line3",  "line4", "line5", "line6", "line7",  "line8" 
   };

char dash_mono[Ndashes][10] =   { 
   "0", "16",
   "0", "42", "13",  "44", "15", "4441", "42",  "13" 
   };

char dash_color[Ndashes][10] =   { 
   "0", "16",
   "0", "0", "0", "0", "0", "0", "0", "0" 
   };

pr_dashes() {
   int n, j, l, ok;
   char option[20], *v; 
   for (n=0; n<Ndashes; n++) {
      strcpy(option, ".");
      strcat(option, dash_keys[n]);
      strcat(option, "Dashes");
      v = pr_GetR(db, option) 
	  ? value.addr
	  : ((Mono) ? dash_mono[n] : dash_color[n]);
      l = strlen(v);
      if (l == 1 && *v == '0') {
	 dashes[n][0] = (unsigned char)0;
	 continue;
	 }
      for (ok=0, j=0; j<l; j++) { v[j] >= '1' && v[j] <= '9' && ok++; }
      if (ok != l || (ok != 2 && ok != 4)) {
	 fprintf(stderr, "gnuplot: illegal dashes value %s:%s\n", option, v);
	 dashes[n][0] = (unsigned char)0;
	 continue;
	 }
      for(j=0; j<l; j++) {
	 dashes[n][j] = (unsigned char) (v[j] - '0');
	 }
      dashes[n][l] = (unsigned char)0;
      }
   }

/*-----------------------------------------------------------------------------
 *   pr_font - determine font          
 *---------------------------------------------------------------------------*/

pr_font() {
   char *fontname = pr_GetR(db, ".font");

   if (!fontname) fontname = FallbackFont;
   font = XLoadQueryFont(dpy, fontname);
   if (!font) {
      fprintf(stderr, "\ngnuplot: can't load font '%s'\n", fontname);
      fprintf(stderr, "gnuplot: using font '%s' instead.\n", FallbackFont);
      font = XLoadQueryFont(dpy, FallbackFont);
      if (!font) {
	 fprintf(stderr, "gnuplot: can't load font '%s'\n", FallbackFont);
	 fprintf(stderr, "gnuplot: no useable font - X11 aborted.\n");
         exit(1);
	 }
      }
   vchar = font->ascent + font->descent;
   }

/*-----------------------------------------------------------------------------
 *   pr_geometry - determine window geometry      
 *---------------------------------------------------------------------------*/

pr_geometry() {
   char *geometry = pr_GetR(db, ".geometry");
   int x, y, flags;
   unsigned int w, h; 

   if (geometry) {
      flags = XParseGeometry(geometry, &x, &y, &w, &h);
      if (flags & WidthValue)  User_Size = 1, W = w;
      if (flags & HeightValue) User_Size = 1, H = h;
      if (flags & XValue) {
         if (flags & XNegative)
            x += DisplayWidth(dpy,scr) - W - BorderWidth*2;
         User_Position = 1, gX = x;
         }
      if (flags & YValue) {
         if (flags & YNegative)
            y += DisplayHeight(dpy,scr) - H - BorderWidth*2;
         User_Position = 1, gY = y;
         }
      }
   }

/*-----------------------------------------------------------------------------
 *   pr_pointsize - determine size of points for 'points' plotting style
 *---------------------------------------------------------------------------*/

pr_pointsize() {
   char *p = pr_GetR(db, ".pointsize") ? value.addr : "1.0" ;

   if (sscanf(p,"%lf", &pointsize) == 1) {
      if (pointsize <= 0 || pointsize > 10) {
	 fprintf(stderr, "\ngnuplot: invalid pointsize '%s'\n", p);
	 pointsize = 1;
	 }
      }
   else { 
      fprintf(stderr, "\ngnuplot: invalid pointsize '%s'\n", p);
      pointsize = 1;
      }
   }

/*-----------------------------------------------------------------------------
 *   pr_width - determine line width values
 *---------------------------------------------------------------------------*/

char width_keys[Nwidths][30] =   { 
   "border", "axis",
   "line1", "line2", "line3",  "line4", "line5", "line6", "line7",  "line8" 
   };

pr_width() {
   int n;
   char option[20], *v; 
   for (n=0; n<Nwidths; n++) {
      strcpy(option, ".");
      strcat(option, width_keys[n]);
      strcat(option, "Width");
      if (v = pr_GetR(db, option)) {
	 if ( *v < '0' || *v > '4' || strlen(v) > 1)
	    fprintf(stderr, "gnuplot: illegal width value %s:%s\n", option, v);
	 else 
	    widths[n] = (unsigned int)atoi(v);
	 }
      }
   }

/*-----------------------------------------------------------------------------
 *   pr_window - create window 
 *---------------------------------------------------------------------------*/

pr_window() {
   char *title =  pr_GetR(db, ".title");
   static XSizeHints hints;
   int Tvtwm = 0;

   win = XCreateSimpleWindow(dpy, root, gX, gY, W, H, BorderWidth,
                             colors[1], colors[0]);

   pr_GetR(db, ".clear") && On(value.addr) && Clear++;
   pr_GetR(db, ".tvtwm") && On(value.addr) && Tvtwm++;

   if (!Tvtwm) {
      hints.flags = (User_Position ? USPosition : PPosition);
      hints.flags |= (User_Size ? USSize : PSize);
      }
   else {
      hints.flags = PPosition;
      }
   hints.x = gX; hints.y = gY;        
   hints.width = W; hints.height = H;

   XSetNormalHints(dpy, win, &hints);

   if (pr_GetR(db, ".iconic") && On(value.addr)) {
      XWMHints wmh;

      wmh.flags = StateHint ;
      wmh.initial_state = IconicState;
      XSetWMHints(dpy, win, &wmh);
      } 

   XStoreName(dpy, win, ((title) ? title : Class));

   XSelectInput(dpy, win, StructureNotifyMask);
   XMapWindow(dpy, win);
   
   }
