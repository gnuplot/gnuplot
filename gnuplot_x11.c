#ifndef lint
static char *RCSid = "$Id: gnuplot_x11.c,v 3.26 92/03/24 22:35:52 woo Exp Locker: woo $";
#endif

/*-----------------------------------------------------------------------------
 *   gnuplot_x11 - X11 outboard terminal driver for gnuplot 3
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
 *      O'Reilly & Associates: X Window System - Volumes 1 & 2
 *
 *   This code is provided as is and with no warranties of any kind.
 *       
 *   Ed Kubaitis (ejk@uiuc.edu)
 *   Computing & Communications Services Office 
 *   University of Illinois, Urbana
 *---------------------------------------------------------------------------*/

#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
 
#include <stdio.h>
#include <signal.h>

#ifndef FD_SET
#ifndef OLD_SELECT
#include <sys/select.h>
#else   /* OLD_SELECT */
#define FD_SET(n, p)    ((p)->fds_bits[0] |= (1 << ((n) % 32)))
#define FD_CLR(n, p)    ((p)->fds_bits[0] &= ~(1 << ((n) % 32)))
#define FD_ISSET(n, p)  ((p)->fds_bits[0] & (1 << ((n) % 32)))
#define FD_ZERO(p)      bzero((char *)(p), sizeof(*(p)))
#endif  /* OLD_SELECT */
#endif  /* FD_SET */

#include <errno.h>
extern int errno;

#define FallbackFont "fixed"
#define Ncolors 13
unsigned long colors[Ncolors];

char dashes[10][5] = { {0}, {1,6,0}, 
   {0}, {4,2,0}, {1,3,0}, {4,4,0}, {1,5,0}, {4,4,4,1,0}, {4,2,0}, {1,3,0}
   };

Display *dpy; int scr; Window win, root;
Visual *vis; GC gc = (GC)0; Pixmap pixmap; XFontStruct *font;
unsigned int W = 640, H = 450; int D, gX = 100, gY = 100;

Bool Mono = 0, Gray = 0, Rv = 0, Clear = 0;
char Name[64] = "gnuplot";
char Class[64] = "Gnuplot";

int cx=0, cy=0, vchar, nc = 0, ncalloc = 0;
double xscale, yscale;
#define X(x) (int) (x * xscale)
#define Y(y) (int) ((4095-y) * yscale)
enum JUSTIFY { LEFT, CENTRE, RIGHT } jmode;

#define Nbuf 1024
char buf[Nbuf], **commands = (char **)0;

FILE *X11_ipc = stdin;
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
 *   mainloop - process X events and input from gnuplot
 *
 *   On systems with a fully implemented select(), select is used (without
 *   timeout) to sense both input from the X server network connection and
 *   pipe input from gnuplot. On platforms with an incomplete or faulty 
 *   select(), select (with timeout) is used for the server, and a temporary 
 *   file rather than a pipe is used for gnuplot input.
 *---------------------------------------------------------------------------*/

mainloop() {
   int nf, nfds, cn = ConnectionNumber(dpy), in = fileno(X11_ipc);
   struct timeval timeout, *timer = (struct timeval *)0;
   fd_set rset, tset;
   unsigned long all = 0xffffffff;
   XEvent xe;

   FD_ZERO(&rset);
   FD_SET(cn, &rset);

#ifndef CRIPPLED_SELECT
   FD_SET(in, &rset);
   nfds = (cn > in) ? cn + 1 : in + 1;
#else  /* CRIPPLED_SELECT */
   timeout.tv_sec = 1;
   timeout.tv_usec = 0;
   timer = &timeout;
   sprintf(X11_ipcpath, "/tmp/Gnuplot_%d", getppid());
   nfds = cn + 1;
#endif /* CRIPPLED_SELECT */

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
#ifndef CRIPPLED_SELECT
      FD_ISSET(in, &tset) && accept();
#else  /* CRIPPLED_SELECT */
      if ((X11_ipc = fopen(X11_ipcpath, "r"))) {
	 unlink(X11_ipcpath);
	 accept();
	 fclose(X11_ipc);
	 }
#endif /* CRIPPLED_SELECT */
      }
   }

/*-----------------------------------------------------------------------------
 *   accept - accept & record new plot from gnuplot inboard X11 driver
 *---------------------------------------------------------------------------*/

accept() {

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

/*-----------------------------------------------------------------------------
 *   display - display last plot from gnuplot inboard X11 driver
 *---------------------------------------------------------------------------*/

display() {
   int n, x, y, sw, sl, lt, width, type;
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
	 width = (lt == 0) ? 2 : 0;
	 if (!Mono) {
	    if (lt != 1) 
	       type = LineSolid;
	    else {
	       type = LineOnOffDash;
	       XSetDashes(dpy, gc, 0, dashes[lt], strlen(dashes[lt]));
	       }
	    XSetForeground(dpy, gc, colors[lt+3]);
	    }
	 else {
	    type  = (lt == 0 || lt == 2) ? LineSolid : LineOnOffDash;
	    if (dashes[lt][0])
	       XSetDashes(dpy, gc, 0, dashes[lt], strlen(dashes[lt]));
	    }
	 XSetLineAttributes( dpy,gc, width, type, CapButt, JoinBevel);
	 }
      }

   /* trigger exposure of background pixmap */
   XClearWindow(dpy,win);
   XFlush(dpy);
   }

/*-----------------------------------------------------------------------------
 *   resize - rescale last plot if window resized
 *---------------------------------------------------------------------------*/

Bool init = True;

resize(xce) XConfigureEvent *xce; {
   if (!init || xce->width != W || xce->height != H) {
      W = xce->width; H = xce->height;
      display();
      init = True;
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

#define Nopt 25
static XrmOptionDescRec options[] = {
   {"-mono",             ".mono",             XrmoptionNoArg,   "on" },
   {"-gray",             ".gray",             XrmoptionNoArg,   "on" },
   {"-clear",            ".clear",            XrmoptionNoArg,   "on" },
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

   char *display = getenv("DISPLAY"),  *home = getenv("HOME");
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

   sprintf(buf, "%s/%s", AppDefDir, "Gnuplot");
   dbApp = XrmGetFileDatabase(buf);
   XrmMergeDatabases(dbApp, &db);

   /*---get server or ~/.Xdefaults-------------------------------------------*/

   if (server_defaults)
      dbDef = XrmGetStringDatabase(server_defaults);
   else {
      sprintf(buf, "%s/.Xdefaults", home);
      dbDef = XrmGetFileDatabase(buf);
      }
   XrmMergeDatabases(dbDef, &db);

   /*---get XENVIRONMENT or  ~/.Xdefaults-hostname---------------------------*/

   if (env = getenv("XENVIRONMENT")) 
      dbEnv = XrmGetFileDatabase(env);
   else {
      char *p, host[MAXHOSTNAMELEN];
      if (gethostname(host, MAXHOSTNAMELEN) < 0) {
	 fprintf(stderr, "gnuplot: gethostname failed. X11 aborted.\n");
	 exit(1);
	 }
      if (p = index(host, '.')) *p = '\0';
      sprintf(buf, "%s/.Xdefaults-%s", home, host);
      dbEnv = XrmGetFileDatabase(buf);
      }
   XrmMergeDatabases(dbEnv, &db);

   /*---merge command line options-------------------------------------------*/

   XrmMergeDatabases(dbCmd, &db);

   /*---determine geometry, font and colors----------------------------------*/

   pr_geometry();
   pr_font();
   pr_color();

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
   char option[20], *v, *type = (Gray) ? "Gray" : "Color";
   XColor used, exact;
   Colormap cmap;
   int n;

   pr_GetR(db, ".mono")         && On(value.addr) && Mono++;
   pr_GetR(db, ".gray")         && On(value.addr) && Gray++;
   pr_GetR(db, ".reverseVideo") && On(value.addr) && Rv++;

   if (!Gray && (vis->class == GrayScale || vis->class == StaticGray)) Mono++;

   if (!Mono) {
      cmap = DefaultColormap(dpy, scr);
      for (n=0; n<Ncolors; n++) {
	 strcpy(option, ".");
	 strcat(option, color_keys[n]);
	 (n > 1) && strcat(option, type);
	 v = pr_GetR(db, option) 
	     ? value.addr
	     : ((Gray) ? gray_values[n] : color_values[n]);
	 if (XAllocNamedColor(dpy, cmap, v, &used, &exact))
	    colors[n] = used.pixel;
	 else {
	    fprintf(stderr, "\ngnuplot: can't allocate %s:%s\n", option, v);
	    fprintf(stderr, "gnuplot: reverting to monochrome\n");
	    Mono++; break;
	    }
	 }
      }
   if (Mono) {
      colors[0] = (Rv) ? black : white ;
      for (n=1; n<Ncolors; n++)  colors[n] = (Rv) ? white : black;
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

      if (flags & WidthValue)  W = w;
      if (flags & HeightValue) H = h;
      if (flags & XValue) {
	 if (flags & XNegative) x += DisplayWidth(dpy,scr);
	 gX = x;
	 }
      if (flags & YValue) {
	 if (flags & YNegative) y += DisplayHeight(dpy,scr);
	 gY = y;
	 }
      }
   }

/*-----------------------------------------------------------------------------
 *   pr_window - create window 
 *---------------------------------------------------------------------------*/

pr_window() {
   char *title =  pr_GetR(db, ".title");
   XSizeHints hints;

   win = XCreateSimpleWindow(dpy, root, gX, gY, W, H, 2, colors[1], colors[0]);

   pr_GetR(db, ".clear") && On(value.addr) && Clear++;

   hints.flags = PPosition;
   hints.x = gX; hints.y = gY;
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
