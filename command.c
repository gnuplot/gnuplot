/*
 *
 *    gnutex/gnuplot translator  --  command.c
 *
 * By David Kotz, 1990.
 * Department of Computer Science, Duke University, Durham, NC 27706.
 * Mail to dfk@cs.duke.edu.
 */

#include <stdio.h>
#include <math.h>
#include "plot.h"

#ifndef STDOUT
#define STDOUT 1
#endif

extern char *malloc();

/*
 * global variables to hold status of 'set' options
 *
 */
BOOLEAN			autoscale_x = TRUE;
BOOLEAN			autoscale_y = TRUE;
BOOLEAN			autoscale_lx = TRUE; /* local to this plot */
BOOLEAN			autoscale_ly = TRUE; /* local to this plot */
enum PLOT_STYLE data_style	= POINTS,
				func_style	= LINES;
BOOLEAN			log_x		= FALSE,
				log_y		= FALSE;
FILE*			infile;
FILE*			outfile;
char			outstr[MAX_ID_LEN+1] = "STDOUT";
int				samples		= SAMPLES;
int				term		= 0;				/* unknown term is 0 */
double			xmin		= -10,
				xmax		= 10,
				ymin		= -10,
				ymax		= 10;
double			zero = ZERO;			/* zero threshold, not 0! */

BOOLEAN xtics = TRUE;
BOOLEAN ytics = TRUE;

BOOLEAN clipping = TRUE;

BOOLEAN undefined;

char title_string[MAX_LINE_LEN] = "";
char xlabel_string[MAX_LINE_LEN] = "";
char ylabel_string[MAX_LINE_LEN] = "";
char xformat[MAX_ID_LEN] = "";
char yformat[MAX_ID_LEN] = "";
int y_skip_factor = 0;
double plot_width = 4;		/* width  in inches, for latex */
double plot_height = 3;		/* height in inches, for latex */

/*
 * instead of <strings.h>
 */

char *gets(),*getenv();
char *strcpy(),*strcat();

double magnitude(),angle(),real(),imag();
struct value *const_express(), *pop(), *complex();


extern struct vt_entry vt[];
extern struct st_entry st[];
extern struct udft_entry udft[];

extern int inline;			/* line number of current input line */
char input_line[MAX_LINE_LEN],dummy_var[MAX_ID_LEN + 1];
struct at_type *curr_at;
int c_token;
int next_value = (int)NEXT_VALUE,next_function = 0,c_function = 0;
int num_tokens;

struct curve_points plot[MAX_PLOTS];
int plot_count = 0;
char plot_ranges[MAX_LINE_LEN+1];
BOOLEAN pending_plot = FALSE;	/* TRUE if a plot command is pending output */
int plot_line;				/* input line number of pending plot */
BOOLEAN key_on = TRUE;		/* will a key be displayed */
BOOLEAN plot_key = FALSE;	/* did we want a key for this plot? */
BOOLEAN label_on = FALSE;	/* will labels be displayed */
BOOLEAN arrow_on = FALSE;	/* will arrows be displayed */
BOOLEAN plot_label = FALSE;	/* do we want labels displayed? */
BOOLEAN plot_arrow = FALSE;	/* do we want arrows displayed? */
BOOLEAN plot_format = FALSE;	/* have we seen a set format? */

BOOLEAN was_output;			/* was there any output from command? */

int inline = 0;			/* input line number */

com_line()
{
	read_line();

#ifdef vms
    if (input_line[0] == '!' || input_line[0] == '$')
	 fprintf(outfile, "%s\n", input_line);
	else
	  do_line();
#else /* vms */
    if (input_line[0] == '!')
	 fprintf(outfile, "%s\n", input_line);
	else
	  do_line();
#endif

}

/* Read a line of input; much simplified by the fact that infile is 
 * never a terminal. Handles line continuation and EOF.
 */
read_line()
{
    int len, left;
    int start = 0;
    BOOLEAN more, stop;
    int last;

    /* read one command */
    left = MAX_LINE_LEN;
    start = 0;
    more = TRUE;
    stop = FALSE;

    while (more) {
	   if (fgets(&(input_line[start]), left, infile) == NULL) {
		  stop = TRUE;		/* EOF in file */
		  input_line[start] = '\0';
		  more = FALSE;	
	   } else {
		  inline++;
		  len = strlen(input_line) - 1;
		  if (input_line[len] == '\n') { /* remove any newline */
			 input_line[len] = '\0';
			 len--;
		  } else if (len+1 >= left)
		    int_error("Input line too long",NO_CARET);
				 
		  if (input_line[len] == '\\') { /* line continuation */
			 start = len;
			 left -= len;
		  } else
		    more = FALSE;
	   }
    }

    if (stop && input_line[0] == '\0')
	 done(0);
}

do_line()
{
    extern int comment_pos;	/* from scanner */
    BOOLEAN nonempty;		/* TRUE if output was nonempty */

    num_tokens = scanner(input_line);
    c_token = 0;
    nonempty = FALSE;
    while(c_token < num_tokens) {
	   was_output = FALSE;

	   command();			/* parse the next command */

	   if (c_token < num_tokens) /* something after command */
		if (equals(c_token,";")) {
		    c_token++;
		    if (was_output)
			 fprintf(outfile, "; ");
		} else
		  int_error("';' expected",c_token);
	   nonempty = was_output || nonempty;
    }

    /* write out any comment */
    if (comment_pos >= 0) {
	   if (nonempty)
		putc(' ', outfile);
	   fputs(input_line+comment_pos, outfile);
	   nonempty = TRUE;
    }
    if (nonempty || num_tokens == 0)
	 putc('\n', outfile);
}


command()
{
    int start_token = c_token;

    if (is_definition(c_token)) {
	   define();
	   copy_command(start_token, c_token-1);
    }
    else if (almost_equals(c_token,"h$elp") || equals(c_token,"?")) {
	   c_token++;
	   while (!(END_OF_COMMAND))
		c_token++;
	   err_msg("help command ignored (removed)");
    }
    else if (almost_equals(c_token,"pr$int")) {
	   struct value a;
	   c_token++;
	   (void) const_express(&a);
	   copy_command(start_token, c_token-1);
    }
    else if (almost_equals(c_token,"p$lot")) {
	   if (pending_plot)
		write_plot();
	   c_token++;
	   plotrequest();
	   pending_plot = TRUE;
	   plot_line = inline;
	   /* we defer output until eof or next plot command */
    }
    else if (almost_equals(c_token,"la$bel")) {
	   c_token++;
	   labelrequest();
    }
    else if (almost_equals(c_token,"k$ey")) {
	   c_token++;
	   keyrequest();
    }
    else if (almost_equals(c_token,"se$t")) {
	   c_token++;
	   if (almost_equals(c_token,"t$erminal")) {
		  c_token++;
		  if (!END_OF_COMMAND)
		    c_token++;
		  copy_command(start_token, c_token-1);
	   }
	   else if (almost_equals(c_token,"sa$mples")) {
		  struct value a;
		  c_token++;
		  (void)magnitude(const_express(&a));
		  copy_command(start_token, c_token-1);
	   }
	   else if (almost_equals(c_token,"o$utput")) {
		  c_token++;
		  if (!END_OF_COMMAND) { /* no file specified */
			 if (!isstring(c_token)) 
			   int_error("expecting filename",c_token);
			 else
			   c_token++;
		  }
		  copy_command(start_token, c_token-1);
	   }
	   else if (almost_equals(c_token,"a$utoscale")) {
		  c_token++;
		  if (END_OF_COMMAND) {
		  } else if (equals(c_token, "xy") || equals(c_token, "yx")) {
			 c_token++;
		  } else if (equals(c_token, "x")) {
			 c_token++;
		  } else if (equals(c_token, "y")) {
			 c_token++;
		  } else
		    int_error("expecting axes specification", c_token);
		  copy_command(start_token, c_token-1);
	   } 
	   else if (almost_equals(c_token,"noa$utoscale")) {
		  c_token++;
		  if (END_OF_COMMAND) {
		  } else if (equals(c_token, "xy") || equals(c_token, "yx")) {
			 c_token++;
		  } else if (equals(c_token, "x")) {
			 c_token++;
		  } else if (equals(c_token, "y")) {
			 c_token++;
		  } else
		    int_error("expecting axes specification", c_token);
		  copy_command(start_token, c_token-1);
	   } 
	   else if (almost_equals(c_token,"l$ogscale")) {
		  c_token++;
		  if (equals(c_token,"x")) {
			 c_token++;
		  } else if (equals(c_token,"y")) {
			 c_token++;
		  } else if (equals(c_token,"xy") || equals(c_token,"yx")) {
			 c_token++;
		  } else
		    int_error("expecting 'x', 'y', or 'xy'", c_token);
		  copy_command(start_token, c_token-1);
	   }
	   else if (almost_equals(c_token,"nol$ogscale")) {
		  copy_command(start_token, c_token++);
	   }
	   else if (almost_equals(c_token,"z$ero")) {
		  struct value a;
		  c_token++;
		  (void) magnitude(const_express(&a)); 
		  copy_command(start_token, c_token-1);
	   }
	   else if (almost_equals(c_token,"x$range")) {
		  c_token++;
		  if (!equals(c_token,"["))
		    int_error("expecting '['",c_token);
		  c_token++;
		  load_range();
		  if (!equals(c_token,"]"))
		    int_error("expecting ']'",c_token);
		  c_token++;
		  copy_command(start_token, c_token-1);
	   }
	   else if (almost_equals(c_token,"y$range")) {
		  c_token++;
		  if (!equals(c_token,"["))
		    int_error("expecting '['",c_token);
		  c_token++;
		  load_range();
		  if (!equals(c_token,"]"))
		    int_error("expecting ']'",c_token);
		  c_token++;
		  copy_command(start_token, c_token-1);
	   }
	   else if (almost_equals(c_token,"d$ata")) {
		  c_token++;
		  if (!almost_equals(c_token,"s$tyle")) 
		    int_error("expecting keyword 'style'",c_token);
		  c_token++;
		  if (END_OF_COMMAND)
		    int_error("expecting style name", c_token);
		  c_token++;
		  copy_command(start_token, c_token-1);
	   }
	   else if (almost_equals(c_token,"f$unction")) {
		  c_token++;
		  if (!almost_equals(c_token,"s$tyle")) 
		    int_error("expecting keyword 'style'",c_token);
		  c_token++;
		  if (END_OF_COMMAND)
		    int_error("expecting style name", c_token);
		  c_token++;
		  copy_command(start_token, c_token-1);
	   }
	   else if (almost_equals(c_token,"st$yle")) {
		  c_token++;
		  add_style();
		  err_msg("'set style' command obsolete (removed)");
	   }
	   else if (almost_equals(c_token,"xl$abel")) {
		  c_token++;
		  if (!isstring(c_token)) 
		    int_error("expecting x label string",c_token);
		  c_token++;
		  copy_command(start_token, c_token-1);
	   }
	   else if (almost_equals(c_token,"xt$ics")) {
		  copy_command(start_token, c_token++);
	   } 
	   else if (almost_equals(c_token,"noxt$ics")) {
		  copy_command(start_token, c_token++);
	   } 
	   else if (almost_equals(c_token,"yl$abel")) {
		  c_token++;
		  if (!isstring(c_token)) {
			 int_error("expecting y label string",c_token);
		  } else {
			 c_token++;
			 if (!END_OF_COMMAND) { /* skip factor specified */
				err_msg("ignoring skip factor on ylabel");
				copy_command(start_token, c_token-1);
				c_token++;
			 } else
			   copy_command(start_token, c_token-1);
		  }
	   }
	   else if (almost_equals(c_token,"yt$ics")) {
		  copy_command(start_token, c_token++);
	   } 
	   else if (almost_equals(c_token,"noyt$ics")) {
		  copy_command(start_token, c_token++);
	   } 
	   else if (almost_equals(c_token,"fo$rmat")) {
		  c_token++;
		  if (equals(c_token,"x")) {
			 c_token++;
		  } else if (equals(c_token,"y")) {
			 c_token++;
		  } else if (equals(c_token,"xy") || equals(c_token,"yx")) {
			 c_token++;
		  } else if (isstring(c_token)) {
		  } else
		    int_error("expecting 'x', 'y', or 'xy'",c_token);
		  if (!isstring(c_token))
		    int_error("expecting format string",c_token);
		  c_token++;
		  copy_command(start_token, c_token-1);
		  plot_format = TRUE;
	   }
	   else if (almost_equals(c_token,"ti$tle")) {
		  c_token++;
		  if (!isstring(c_token)) 
		    int_error("expecting title string",c_token);
		  c_token++;
		  copy_command(start_token, c_token-1);
	   }
	   else if (almost_equals(c_token,"si$ze")) {
		  struct value a;
		  c_token++;
		  plot_width = magnitude(const_express(&a));
		  if (!equals(c_token, ","))
		    int_error("expecting comma between width and height", c_token);
		  c_token++;		/* comma */
		  plot_height = magnitude(const_express(&a));
		  err_msg("'set size' command translated");
		  fprintf(outfile, "set size %.3g, %.3g", 
				(plot_width + 1.096)/5., /* adjust for margins too */
				(plot_height + 0.616)/3.);
		  was_output = TRUE;
	   }
	   else if (almost_equals(c_token,"cl$ip")) {
		  c_token++;
		  fprintf(outfile, "set clip points");
		  was_output = TRUE;
	   } 
	   else if (almost_equals(c_token,"nocl$ip")) {
		  c_token++;
		  fprintf(outfile, "set noclip points");
		  was_output = TRUE;
	   } 
	   else
		int_error("unknown set option (try 'help set')",c_token);
    }
    else if (almost_equals(c_token,"sh$ow")) {
	   if (almost_equals(++c_token,"f$unctions")) {
		  c_token++;
		  if (almost_equals(c_token,"s$tyle")) 
		    c_token++;
		  copy_command(start_token, c_token-1);
	   }
	   else if (almost_equals(c_token,"v$ariables")) {
		  copy_command(start_token, c_token++);
	   }
	   else if (almost_equals(c_token,"ac$tion_table") ||
			  equals(c_token,"at") ) {
		  c_token++;
		  show_at();
		  copy_command(start_token, c_token-1);
	   } 
	   else if (almost_equals(c_token,"d$ata")) {
		  c_token++;
		  if (!almost_equals(c_token,"s$tyle")) 
		    int_error("expecting keyword 'style'",c_token);
		  c_token++;
		  copy_command(start_token, c_token-1);
	   }
	   else if (almost_equals(c_token,"x$range")) {
		  copy_command(start_token, c_token++);
	   } 
	   else if (almost_equals(c_token,"y$range")) {
		  copy_command(start_token, c_token++);
	   } 
	   else if (almost_equals(c_token,"z$ero")) {
		  copy_command(start_token, c_token++);
	   }
	   else if (almost_equals(c_token,"sa$mples")) {
		  copy_command(start_token, c_token++);
	   }
	   else if (almost_equals(c_token,"o$utput")) {
		  copy_command(start_token, c_token++);
	   }
	   else if (almost_equals(c_token,"t$erminal")) {
		  copy_command(start_token, c_token++);
	   }
	   else if (almost_equals(c_token,"au$toscale")) {
		  copy_command(start_token, c_token++);
	   }
	   else if (almost_equals(c_token,"ve$rsion")) {
		  copy_command(start_token, c_token++);
	   } 
	   else if (almost_equals(c_token,"l$ogscale")) {
		  copy_command(start_token, c_token++);
	   }
	   else if (almost_equals(c_token,"cl$ipping")) {
		  copy_command(start_token, c_token++);
	   }
	   else if (almost_equals(c_token,"xl$abel")) {
		  copy_command(start_token, c_token++);
	   }
	   else if (almost_equals(c_token,"yl$abel")) {
		  copy_command(start_token, c_token++);
	   }
	   else if (almost_equals(c_token,"fo$rmat")) {
		  copy_command(start_token, c_token++);
	   }
	   else if (almost_equals(c_token,"ti$tle")) {
		  copy_command(start_token, c_token++);
	   }
	   else if (almost_equals(c_token,"si$ze")) {
		  copy_command(start_token, c_token++);
	   }
	   else if (almost_equals(c_token,"st$yle")) {
		  c_token++;
		  add_style();
		  err_msg("'show style' command ignored (removed)");
	   }
	   else if (almost_equals(c_token,"a$ll")) {
		  copy_command(start_token, c_token++);
	   }
	   else
		int_error("unknown show option (try 'help show')",c_token);
    }
    else if (almost_equals(c_token,"cl$ear")) {
	   copy_command(start_token, c_token++);
    }
    else if (almost_equals(c_token,"she$ll")) {
	   copy_command(start_token, c_token++);
    }
    else if (almost_equals(c_token,"sa$ve")) {
	   c_token++;
	   if (almost_equals(c_token,"f$unctions")
		  || almost_equals(c_token,"v$ariables")) {
		  c_token++;
		  if (!isstring(c_token))
		    int_error("expecting filename",c_token);
	   } else if (!isstring(c_token)) {
		  int_error("filename or keyword 'functions' or 'variables' expected",c_token);
	   }
	   c_token++;
	   copy_command(start_token, c_token-1);
    }
    else if (almost_equals(c_token,"lo$ad")) {
	   if (!isstring(++c_token))
		int_error("expecting filename",c_token);
	   c_token++;
	   copy_command(start_token, c_token-1);
    }
    else if (almost_equals(c_token,"ex$it") ||
		   almost_equals(c_token,"q$uit")) {
	   copy_command(start_token, c_token++);
	   /* done(IO_SUCCESS); */
    }
    else if (equals(c_token,";")) { /* null statement */
	   copy_command(start_token, c_token++);
    } else {
	   int_error("invalid command",c_token);
    }
}

load_range()
{
struct value t;
double a,b;

	if (equals(c_token,"]"))
		return;
	if (END_OF_COMMAND) {
	    int_error("starting range value or 'to' expected",c_token);
	} else if (!equals(c_token,"to") && !equals(c_token,":"))  {
		a = real(const_express(&t));
	}	
	if (!equals(c_token,"to") && !equals(c_token,":")) 
		int_error("Keyword 'to' or ':' expected",c_token);
	c_token++; 
	if (!equals(c_token,"]"))
		b = real(const_express(&t));
}


plotrequest()
{
    int start_token = c_token;

    if (equals(c_token,"[")) {
	   c_token++;
	   if (isletter(c_token)) {
		  c_token++;
		  if (equals(c_token,"="))
		    c_token++;
		  else
		    int_error("'=' expected",c_token);
	   }
	   load_range();
	   if (!equals(c_token,"]"))
		int_error("']' expected",c_token);
	   c_token++;
    }

    if (equals(c_token,"[")) { /* set optional y ranges */
	   c_token++;
	   load_range();
	   if (!equals(c_token,"]"))
		int_error("']' expected",c_token);
	   c_token++;
    }

    capture(plot_ranges, start_token, c_token-1);

    eval_plots();
}

/* Parse the definition of a style and add to table */
add_style()
{
    register int i;
    int style = -1;				/* the new style number */
    struct value a;
    register struct st_entry *stp;	/* quick access to new entry */
    int null_definition = TRUE; /* watch out for missing definitions */

	/* check if it's already in the table... */

    style = -1;
	for (i = 0; i < next_value; i++) {
		if (equals(c_token,st[i].st_name))
			style = i;
	}
    /* Not found - assign a new one */
    if (style < 0)
	 style = next_style;
    /* Found - redefine it */
    if (style <= FIXED_STYLES)
	 int_error("cannot redefine this style", c_token);
    if (style == MAX_STYLES)
	 int_error("user defined style space full",NO_CARET);
    else
	 next_style++;

    stp = &st[style];

    /* Copy in the name of the style */
    copy_str(stp->st_name,c_token);
    stp->st_undef = TRUE;
    c_token++;

    /* Point type */
    if(!isstring(c_token)) {
	   *stp->st_point = '\0'; /* null point definition */
    } else {
	   quote_str(stp->st_point, c_token);
	   c_token++;
	   null_definition = FALSE;
    }

     /* Spacing */
    if(END_OF_COMMAND) {
	   stp->st_spacing = 0;
	   stp->st_length = 0;
    } else {
	   /* read dot spacing */
	   if (!isnumber(c_token)) {
		  next_value--;
		  int_error("expecting spacing (in points) for style", c_token);
	   }
	   convert(&a, c_token);
	   stp->st_spacing = magnitude(&a);
	   if (stp->st_spacing < 0.1) {
		  next_value--;
		  int_error("unreasonable spacing value", c_token);
	   }
	   c_token++;

	   /* build dot sequence */
	   stp->st_length = 0;
	   
	   while(!(END_OF_COMMAND)) {
		  if (!isstring(c_token))
		    int_error("expecting a string defining a sequence style", c_token);
		  quote_str(stp->st_seq[stp->st_length++], c_token);
		  c_token++;
		  if (stp->st_length >= MAX_STYLE_SEQ_LENGTH)
		    int_error("style sequence too long", c_token);
	   }
	   null_definition = FALSE;

	   if (stp->st_length == 0)
		int_error("expecting dot sequence", c_token);
    }

    if (null_definition)
	 int_error("expecting definition of style", c_token);

    c_token++;
}


labelrequest()
{
    struct value a;
    double x,y;			/* the point */
    char pos[MAX_ID_LEN+1];	/* optional argument */
    char text[MAX_ID_LEN+1];	/* text of the label */
    double length = 0;		/* length of arrow */
    int dx = 0, dy = 0;		/* slope of arrow */

    /* x coordinate */
    const_express(&a);
    x = real(&a);
    if (log_x)		/* handle coords on logscale plot. PEM 01/17/89 */
	x = log10(x);

    if (!equals(c_token, ","))
	 int_error("comma expected", c_token);
    c_token++;

    /* y coordinate */
    const_express(&a);
    y = real(&a);
    if (log_y)		/* handle coords on logscale plot. PEM 01/17/89 */
	y = log10(y);

    /* text */
    if (isstring(c_token))
	 quote_str(text, c_token++);
    else
	 int_error("expecting text of the label", c_token);

    /* optional pos */
    pos[0] = '\0';
    if (!(END_OF_COMMAND)) {
	   copy_str(pos, c_token++);
	   
	   /* optional length for optional arrow  */
	   if (!(END_OF_COMMAND)) {
		  const_express(&a);
		  length = real(&a);

		  if (!(END_OF_COMMAND)) {
			 if (!equals(c_token, ","))
			   int_error("comma expected", c_token);
			 c_token++;
		  
			 const_express(&a);
			 dx = (int) real(&a);

			 if (!equals(c_token, ","))
			   int_error("comma expected", c_token);
			 c_token++;

			 const_express(&a);
			 dy = (int) real(&a);
		  }
	   }
    }

    save_label(x,y, text, pos, length, dx, dy);
}

/* translate the label  */
save_label(x,y, text, pos, length, dx, dy)
    double x,y;			/* the point */
    char text[MAX_ID_LEN+1];	/* text of the label */
    char pos[MAX_ID_LEN+1];	/* optional argument */
    double length;			/* length of arrow */
    int dx, dy;			/* slope of arrow */
{
    char *where = NULL;
    BOOLEAN adjust = FALSE;
    double ex, ey;

    if (instring(pos, 'l'))
	 where = "left";
    else if (instring(pos, 'r'))
	 where = "right";
    else
	 where = "center";

    if (instring(pos, 't') || instring(pos, 'b'))
	 adjust = TRUE;

    fprintf(outfile, "set label \"%s\" at %g,%g %s\n", text, x, y, where);

    if (length != 0) {
	   /* arrow option. Compute destination */
	   if (dx == 0 && dy == 0) {
		  if (instring(pos, 'l'))
		    dx = -1;
		  else if (instring(pos, 'r')) 
		    dx = 1;
		  else
		    dx = 0;
		  if (instring(pos, 't'))
		    dy = 1;
		  else if (instring(pos, 'b')) 
		    dy = -1;
		  else
		    dy = 0;
	   }
	   ex = x + (dx ? length : 0);
	   ey = y + (dx ? ((float)dy/dx * length) : length);

	   fprintf(outfile, "set arrow from %g,%g to %g,%g\n", x, y, ex, ey);
	   arrow_on = TRUE;
	   plot_arrow = TRUE;
    }

    label_on = TRUE;
    plot_label = TRUE;
    was_output = TRUE;
}

keyrequest()
{
    struct value a;
    double x,y;			/* the point */
    int styles[MAX_KEYS+1];	/* the style for each plot */
    char *text[MAX_KEYS+1];	/* text of each key entry */
    int style;
    int curve = 0;			/* index of the entry */

    /* x coordinate */
    const_express(&a);
    x = real(&a);

    if (!equals(c_token, ","))
	 int_error("comma expected", c_token);
    c_token++;

    /* y coordinate */
    const_express(&a);
    y = real(&a);

    do {					/* scan the entries in the key */
	   /* text */
	   if (isstring(c_token)) {
		  text[curve] = (char *) malloc(MAX_ID_LEN);
		  quote_str(text[curve], c_token++);
	   } else
		int_error("expecting text of the key entry", c_token);
	   
	   if (almost_equals(c_token, "w$ith"))
		c_token++;
	   else
		int_error("expecting 'with' style for key entry", c_token);

	   for (style = 0; style < next_style; style++)
		if (equals(c_token, st[style].st_name))
		  break;
	   if (style == next_style)
		int_error("unknown plot style; type 'show style'", 
				c_token);
	   else
		styles[curve] = style;
	   c_token++;

	   if (!END_OF_COMMAND)
		if (!equals(c_token, ","))
		  int_error("expecting ',' between key entries", c_token);
		else
		  c_token++;

	   curve++;
	   if (curve > MAX_KEYS)
		int_error("too many lines in the key", c_token);
    } while (!END_OF_COMMAND);

    text[curve] = NULL;		/* acts as terminator */

    save_key (x,y, styles, text);

    for (curve--; curve >= 0; curve--)
	 free(text[curve]);
}

save_key (x,y, styles, text)
	double x,y;			/* the position of the key */
	int styles[MAX_KEYS+1];	/* the style for each plot */
	char *text[MAX_KEYS+1];	/* text of each key entry */
{
    int curve;				/* index of the entry */

    for (curve = 0; text[curve] != NULL; curve++) {
	   strcpy(plot[curve].title, text[curve]);
    }
    fprintf(outfile, "set key %g,%g", x,y);
    was_output = TRUE;
    key_on = TRUE;
    plot_key = TRUE;

    err_msg("'key' command translated; position needs adjusting");
}

define()
{
register int value,start_token;  /* start_token is the 1st token in the	*/
								/* function definition.			*/

	if (equals(c_token+1,"(")) {
		/* function ! */
		start_token = c_token;
		copy_str(dummy_var, c_token + 2);
		c_token += 5; /* skip (, dummy, ) and = */
		value = c_function = user_defined(start_token);
		build_at(&(udft[value].at));
				/* define User Defined Function (parse.c)*/
		capture(udft[value].definition,start_token,c_token-1);
	}
	else {
		/* variable ! */
		c_token +=2;
		(void) const_express(&vt[value = add_value(c_token - 2) ].vt_value);
		vt[value].vt_undef = FALSE;
	}
}

/* This parses the plot command after any range specifications. 
 * We revert to one-pass parsing here. That's all we need.
 */
eval_plots()
{
register int plot_num, start_token;
struct value a;
int style;

	c_function = MAX_UDFS;		/* last udft[] entry used for plots */

	plot_num = 0;

	while (TRUE) {
		if (END_OF_COMMAND)
			int_error("function to plot expected",c_token);
		if (plot_num == MAX_PLOTS)
			int_error("maximum number of plots exceeded",NO_CARET);

		start_token = c_token;

		if (is_definition(c_token)) {
			define();
		} else {
			if (isstring(c_token)) {			/* data file to plot */
				plot[plot_num].plot_type = DATA;
				style = (int)data_style;
				c_token++; /* skip file name */
			} 
			else {							/* function to plot */
				plot[plot_num].plot_type = FUNC;
				style = (int)func_style;
				build_at(&udft[MAX_UDFS].at);
				/* that's all */
			}
			capture(plot[plot_num].def,start_token,c_token-1);
			plot[plot_num].title[0] = '\0';

			if (almost_equals(c_token,"w$ith")) {
				c_token++;
				for (style = 0; style < next_style; style++)
				  if (equals(c_token, st[style].st_name))
				    break;
				if (style == next_style)
				  int_error("unknown plot style; type 'show style'", 
						  c_token);
				else
				  c_token++;
			}
			plot[plot_num].plot_style = equiv_style(style);
			plot_num++;
		}

		if (equals(c_token,",")) 
			c_token++;
		else  
			break;
	}

    plot_count = plot_num;
}

/* return a standard style close to the given style */
int
equiv_style(style)
	int style;			/* may be a user-def style */
{
    BOOLEAN line, point;
    char message[MAX_LINE_LEN+1];

    if (style <= FIXED_STYLES)
	 return(style);

    point = (st[style].st_point[0] != '\0');
    line = (st[style].st_length > 0);

    sprintf(message,
		  "user-defined style %s replaced by similar standard style",
		  st[style].st_name);
    err_msg(message);

    if (point && line)
	 return(LINESPOINTS);
    if (point)
	 return(POINTS);
    if (line)
	 return(LINES);

    /* should not happen */
    err_msg("no standard style corresponds to this style");
    return(LINES);
}


/* write out the plot command now that we know titles */
write_plot()
{
    int plot_num;

    if (!plot_key && key_on) {
	   fprintf(outfile, "set nokey\n");
	   key_on = FALSE;
    }

    if (!plot_label && label_on) {
	   fprintf(outfile, "set nolabel\n");
	   label_on = FALSE;
    }

    if (!plot_arrow && arrow_on) {
	   fprintf(outfile, "set noarrow\n");
	   arrow_on = FALSE;
    }

    if (!plot_format) {
	   fprintf(outfile, "set format xy \"$%%g$\"\n");
	   plot_format = TRUE;
    }

    fprintf(outfile, "plot %s ", plot_ranges);

    for (plot_num = 0; plot_num < plot_count; plot_num++) {
	   if (plot_num > 0)
		fputs(", ", outfile);
	   fprintf(outfile, "%s", plot[plot_num].def);
	   if (plot[plot_num].title[0] != '\0')
		fprintf(outfile, " title \"%s\"", plot[plot_num].title);

	   fprintf(outfile, " with %s", st[plot[plot_num].plot_style].st_name);
    }

    fprintf(outfile, "\n");
    /* we don't set was_output since we have the newline already */

    fprintf(stderr, 
		  "\n***%d: plot command moved, watch out for use of variables\n", 
		  plot_line);

    pending_plot = FALSE;
    plot_key = FALSE;
    plot_label = FALSE;
    plot_arrow = FALSE;
}

/* copy a command from input to output */
copy_command(start, end)
	int start, end;		/* inclusive token numbers */
{
    char command_line[MAX_LINE_LEN+1];

    capture(command_line, start, end);
    fputs(command_line, outfile);
    was_output = TRUE;
}

done(status)
int status;
{
    if (pending_plot)
	 write_plot();

	exit(status);
}
