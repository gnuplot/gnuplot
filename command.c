/*
 *
 *    G N U P L O T  --  command.c
 *
 *  Copyright (C) 1986, 1987  Thomas Williams, Colin Kelley
 *
 *  You may use this code as you wish if credit is given and this message
 *  is retained.
 *
 *  Please e-mail any useful additions to vu-vlsi!plot so they may be
 *  included in later releases.
 *
 *  This file should be edited with 4-column tabs!  (:set ts=4 sw=4 in vi)
 */

#include <stdio.h>
#include <math.h>

#ifdef MSDOS
#include <process.h>
#endif

#include "plot.h"

#ifndef STDOUT
#define STDOUT 1
#endif

/*
 * global variables to hold status of 'set' options
 *
 */
BOOLEAN			autoscale	= TRUE;
char			dummy_var[MAX_ID_LEN+1] = "x";
enum PLOT_STYLE data_style	= POINTS,
				func_style	= LINES;
BOOLEAN			log_x		= FALSE,
				log_y		= FALSE;
FILE*			outfile;
char			outstr[MAX_ID_LEN+1] = "STDOUT";
int				samples		= SAMPLES;
int				term		= 0;				/* unknown term is 0 */
double			xmin		= -10.0,
				xmax		= 10.0,
				ymin		= -10.0,
				ymax		= 10.0;
double			zero = ZERO;			/* zero threshold, not 0! */


BOOLEAN screen_ok;
BOOLEAN term_init;
BOOLEAN undefined;

/*
 * instead of <strings.h>
 */

char *gets(),*getenv();
char *strcpy(),*strncpy(),*strcat();

char *malloc();

double magnitude(),angle(),real(),imag();
struct value *const_express(), *pop(), *complex();
struct at_type *temp_at(), *perm_at();
struct udft_entry *add_udf();
struct udvt_entry *add_udv();

extern struct termentry term_tbl[];

struct lexical_unit token[MAX_TOKENS];
char input_line[MAX_LINE_LEN+1] = "set term ";
char c_dummy_var[MAX_ID_LEN+1]; 		/* current dummy var */
int num_tokens, c_token;

struct curve_points *first_plot = NULL;
struct udft_entry plot_func, *dummy_func;

static char replot_line[MAX_LINE_LEN+1];
static int plot_token;					/* start of 'plot' command */
static char help[MAX_LINE_LEN] = HELP;

com_line()
{
	read_line();

	screen_ok = TRUE; /* so we can flag any new output */

	do_line();
}


do_line()	  /* also used in load_file */
{
	if (is_comment(input_line[0]))
		return;
	if (is_system(input_line[0])) {
		do_system();
		fputs("!\n",stderr);
		return;
	}
	num_tokens = scanner(input_line);
	c_token = 0;
	while(c_token < num_tokens) {
		command();
		if (c_token < num_tokens)	/* something after command */
			if (equals(c_token,";"))
				c_token++;
			else
					int_error("';' expected",c_token);
	}
}



command()
{
static char sv_file[MAX_LINE_LEN+1];
			/* string holding name of save or load file */

	c_dummy_var[0] = '\0';		/* no dummy variable */

	if (is_definition(c_token))
		define();
	else if (equals(c_token,"help") || equals(c_token,"?")) {
		register int len;
		register char *help_ptr;

		c_token++;
		if ((help_ptr = getenv("GNUHELP")))	/* initial command */
			(void) strncpy(help,help_ptr,sizeof(help) - 1);
		else
			(void) strncpy(help,HELP,sizeof(help) - 1);

		while (!(END_OF_COMMAND)) {
			len = strlen(help);
			help[len] = ' ';   /* put blank between help segments */
			copy_str(help+len+1,c_token++);
		}
		do_help();
		screen_ok = FALSE;
		c_token++;
	}
	else if (almost_equals(c_token,"pr$int")) {
		struct value a;

		c_token++;
		(void) const_express(&a);
		(void) putc('\t',stderr);
		disp_value(stderr,&a);
		(void) putc('\n',stderr);
		screen_ok = FALSE;
	}
	else if (almost_equals(c_token,"p$lot")) {
		plot_token = c_token++;
		plotrequest();
	}
	else if (almost_equals(c_token,"rep$lot")) {
		if (replot_line[0] == '\0') 
			int_error("no previous plot",c_token);
		(void) strcpy(input_line,replot_line);
		screen_ok = FALSE;
		num_tokens = scanner(input_line);
		c_token = 1;					/* skip the 'plot' part */
		plotrequest();
	}
	else if (almost_equals(c_token,"se$t"))
		set_stuff();
	else if (almost_equals(c_token,"sh$ow"))
		show_stuff();
	else if (almost_equals(c_token,"cl$ear")) {
		if (!term_init) {
			(*term_tbl[term].init)();
			term_init = TRUE;
		}
		(*term_tbl[term].graphics)();
		(*term_tbl[term].text)();
		(void) fflush(outfile);
		screen_ok = FALSE;
		c_token++;
	}
	else if (almost_equals(c_token,"she$ll")) {
		do_shell();
		screen_ok = FALSE;
		c_token++;
	}
	else if (almost_equals(c_token,"sa$ve")) {
		if (almost_equals(++c_token,"f$unctions")) {
			if (!isstring(++c_token))
				int_error("expecting filename",c_token);
			else {
				quote_str(sv_file,c_token);
				save_functions(fopen(sv_file,"w"));
			}
		}
		else if (almost_equals(c_token,"v$ariables")) {
			if (!isstring(++c_token))
				int_error("expecting filename",c_token);
			else {
				quote_str(sv_file,c_token);
				save_variables(fopen(sv_file,"w"));
			}
		}
		else if (isstring(c_token)) {
			quote_str(sv_file,c_token);
			save_all(fopen(sv_file,"w"));
		}
		else {
			int_error(
		"filename or keyword 'functions' or 'variables' expected",c_token);
		}
		c_token++;
	}
	else if (almost_equals(c_token,"l$oad")) {
		if (!isstring(++c_token))
			int_error("expecting filename",c_token);
		else {
			quote_str(sv_file,c_token);
			load_file(fopen(sv_file,"r"));	
		/* input_line[] and token[] now destroyed! */
			c_token = num_tokens = 0;
		}
	}
	else if (almost_equals(c_token,"ex$it") ||
			almost_equals(c_token,"q$uit")) {
		done(IO_SUCCESS);
	}
	else if (!equals(c_token,";")) {		/* null statement */
		int_error("invalid command",c_token);
	}
}


enum PLOT_STYLE
get_style()
{
register enum PLOT_STYLE ps;

	c_token++;
	if (almost_equals(c_token,"l$ines"))
		ps = LINES;
	else if (almost_equals(c_token,"i$mpulses"))
		ps = IMPULSES;
	else if (almost_equals(c_token,"p$oints"))
		ps = POINTS;
	else
		int_error("expecting 'lines', 'points', or 'impulses'",c_token);
	c_token++;
	return(ps);
}


set_stuff()
{
static char testfile[MAX_LINE_LEN+1];

	if (almost_equals(++c_token,"a$utoscale")) {
		autoscale = TRUE;
		c_token++;
	}
	else if (almost_equals(c_token,"noa$utoscale")) {
		autoscale = FALSE;
		c_token++;
	}
	else if (almost_equals(c_token,"d$ata")) {
		c_token++;
		if (!almost_equals(c_token,"s$tyle"))
			int_error("expecting keyword 'style'",c_token);
		data_style = get_style();
	}
	else if (almost_equals(c_token,"d$ummy")) {
		c_token++;
		copy_str(dummy_var,c_token++);
	}
	else if (almost_equals(c_token,"f$unction")) {
		c_token++;
		if (!almost_equals(c_token,"s$tyle"))
			int_error("expecting keyword 'style'",c_token);
		func_style = get_style();
	}
	else if (almost_equals(c_token,"l$ogscale")) {
		c_token++;
		if (equals(c_token,"x")) {
			log_y = FALSE;
			log_x = TRUE;
			c_token++;
		}
		else if (equals(c_token,"y")) {
			log_x = FALSE;
			log_y = TRUE;
			c_token++;
		}
		else if (equals(c_token,"xy") || equals(c_token,"yx")) {
			log_x = log_y = TRUE;
			c_token++;
		}
		else
			int_error("expecting 'x', 'y', or 'xy'",c_token);
	}
	else if (almost_equals(c_token,"nol$ogscale")) {
		log_x = log_y = FALSE;
		c_token++;
	}
	else if (almost_equals(c_token,"o$utput")) {
		register FILE *f;

		c_token++;
		if (END_OF_COMMAND) {	/* no file specified */
			(void) fclose(outfile);
			outfile = fdopen(dup(STDOUT), "w");
			term_init = FALSE;
			(void) strcpy(outstr,"STDOUT");
		} else if (!isstring(c_token))
			int_error("expecting filename",c_token);
		else {
			quote_str(testfile,c_token);
			if (!(f = fopen(testfile,"w"))) {
			  os_error("cannot open file; output not changed",c_token);
			}
			(void) fclose(outfile);
			outfile = f;
			term_init = FALSE;
			outstr[0] = '\'';
			(void) strcat(strcpy(outstr+1,testfile),"'");
		}
		c_token++;
	}
	else if (almost_equals(c_token,"sa$mples")) {
		register int tsamp;
		struct value a;

		c_token++;
		tsamp = (int)magnitude(const_express(&a));
		if (tsamp < 1)
			int_error("sampling rate must be > 0; sampling unchanged",
				c_token);
		else {
			register struct curve_points *f_p = first_plot;

			first_plot = NULL;
			cp_free(f_p);
			samples = tsamp;
		}
	}
	else if (almost_equals(c_token,"t$erminal")) {
		c_token++;
		if (END_OF_COMMAND) {
			list_terms();
			screen_ok = FALSE;
		}
		else
			term = set_term(c_token);
		c_token++;
	}
	else if (almost_equals(c_token,"x$range")) {
		c_token++;
		if (!equals(c_token,"["))
			int_error("expecting '['",c_token);
		c_token++;
		load_range(&xmin,&xmax);
		if (!equals(c_token,"]"))
			int_error("expecting ']'",c_token);
		c_token++;
	}
	else if (almost_equals(c_token,"y$range")) {
		c_token++;
		if (!equals(c_token,"["))
			int_error("expecting '['",c_token);
		c_token++;
		load_range(&ymin,&ymax);
		autoscale = FALSE;
		if (!equals(c_token,"]"))
			int_error("expecting ']'",c_token);
		c_token++;
	}
	else if (almost_equals(c_token,"z$ero")) {
		struct value a;
		c_token++;
		zero = magnitude(const_express(&a));
	}
	else
		int_error(
	"valid set options:  '[no]autoscale', 'data', 'dummy', 'function',\n\
'[no]logscale', 'output', 'samples', 'terminal', 'xrange', 'yrange', 'zero'",
	c_token);
}


show_stuff()
{
	if (almost_equals(++c_token,"ac$tion_table") ||
			 equals(c_token,"at") ) {
		c_token++;
		show_at();
		c_token++;
	}
	else if (almost_equals(c_token,"au$toscale")) {
		(void) putc('\n',stderr);
		show_autoscale();
		c_token++;
	}
	else if (almost_equals(c_token,"d$ata")) {
		c_token++;
		if (!almost_equals(c_token,"s$tyle"))
			int_error("expecting keyword 'style'",c_token);
		(void) putc('\n',stderr);
		show_style("data",data_style);
		c_token++;
	}
	else if (almost_equals(c_token,"d$ummy")) {
		fprintf(stderr,"\n\tdummy variable is %s\n",dummy_var);
		c_token++;
	}
	else if (almost_equals(c_token,"f$unctions")) {
		c_token++;
		if (almost_equals(c_token,"s$tyle"))  {
			(void) putc('\n',stderr);
			show_style("functions",func_style);
			c_token++;
		}
		else
			show_functions();
	}
	else if (almost_equals(c_token,"l$ogscale")) {
		(void) putc('\n',stderr);
		show_logscale();
		c_token++;
	}
	else if (almost_equals(c_token,"o$utput")) {
		(void) putc('\n',stderr);
		show_output();
		c_token++;
	}
	else if (almost_equals(c_token,"sa$mples")) {
		(void) putc('\n',stderr);
		show_samples();
		c_token++;
	}
	else if (almost_equals(c_token,"t$erminal")) {
		(void) putc('\n',stderr);
		show_term();
		c_token++;
	}
	else if (almost_equals(c_token,"v$ariables")) {
		show_variables();
		c_token++;
	}
	else if (almost_equals(c_token,"ve$rsion")) {
		show_version();
		c_token++;
	}
	else if (almost_equals(c_token,"x$range")) {
		(void) putc('\n',stderr);
		show_range('x',xmin,xmax);
		c_token++;
	}
	else if (almost_equals(c_token,"y$range")) {
		(void) putc('\n',stderr);
		show_range('y',ymin,ymax);
		c_token++;
	}
	else if (almost_equals(c_token,"z$ero")) {
		(void) putc('\n',stderr);
		show_zero();
		c_token++;
	}
	else if (almost_equals(c_token,"a$ll")) {
		c_token++;
		show_version();
		fprintf(stderr,"\tdummy variable is %s\n",dummy_var);
		show_style("data",data_style);
		show_style("functions",func_style);
		show_output();
		show_term();
		show_samples();
		show_logscale();
		show_autoscale();
		show_zero();
		show_range('x',xmin,xmax);
		show_range('y',ymin,ymax);
		show_variables();
		show_functions();
		c_token++;
	}
	else
		int_error(
	"valid show options:  'action_table', 'all', 'autoscale', 'data',\n\
'dummy', 'function', 'logscale', 'output', 'samples', 'terminal',\n\
'variables', 'version', 'xrange', 'yrange', 'zero'", c_token);
	screen_ok = FALSE;
	(void) putc('\n',stderr);
}


load_range(a,b)
double *a,*b;
{
struct value t;

	if (equals(c_token,"]"))
		return;
	if (END_OF_COMMAND) {
	    int_error("starting range value or ':' expected",c_token);
	} else if (!equals(c_token,"to") && !equals(c_token,":"))  {
		*a = real(const_express(&t));
	}	
	if (!equals(c_token,"to") && !equals(c_token,":"))
		int_error("':' expected",c_token);
	c_token++;
	if (!equals(c_token,"]"))
		*b = real(const_express(&t));
}


plotrequest()
{
	
	if (!term)					/* unknown */
		int_error("use 'set term' to set terminal type first",c_token);

	if (equals(c_token,"[")) {
		c_token++;
		if (isletter(c_token)) {
			copy_str(c_dummy_var,c_token++);
			if (equals(c_token,"="))
				c_token++;
			else
				int_error("'=' expected",c_token);
		}
		load_range(&xmin,&xmax);
		if (!equals(c_token,"]"))
			int_error("']' expected",c_token);
		c_token++;
	}

	if (equals(c_token,"[")) { /* set optional y ranges */
		c_token++;
		load_range(&ymin,&ymax);
		autoscale = FALSE;
		if (!equals(c_token,"]"))
			int_error("']' expected",c_token);
		c_token++;
	}

	eval_plots();
}


define()
{
register int start_token;  /* the 1st token in the function definition */
register struct udvt_entry *udv;
register struct udft_entry *udf;

	if (equals(c_token+1,"(")) {
		/* function ! */
		start_token = c_token;
		copy_str(c_dummy_var, c_token + 2);
		c_token += 5; /* skip (, dummy, ) and = */
		if (END_OF_COMMAND)
			int_error("function definition expected",c_token);
		udf = dummy_func = add_udf(start_token);
		if (udf->at)				/* already a dynamic a.t. there */
			free((char *)udf->at);	/* so free it first */
		if (!(udf->at = perm_at()))
			int_error("not enough memory for function",start_token);
		m_capture(&(udf->definition),start_token,c_token-1);
	}
	else {
		/* variable ! */
		start_token = c_token;
		c_token +=2;
		udv = add_udv(start_token);
		(void) const_express(&(udv->udv_value));
		udv->udv_undef = FALSE;
	}
}


get_data(this_plot)
struct curve_points *this_plot;
{
static char data_file[MAX_LINE_LEN+1], line[MAX_LINE_LEN+1];
register int i, overflow, l_num;
register FILE *fp;
float x, y;

	quote_str(data_file, c_token);
	this_plot->plot_type = DATA;
	if (!(fp = fopen(data_file, "r")))
		os_error("can't open data file", c_token);

	l_num = 0;

	overflow = i = 0;

	while (fgets(line, MAX_LINE_LEN, fp)) {
		l_num++;
		if (is_comment(line[0]) || ! line[1])	/* line[0] will be '\n' */
			continue;		/* ignore comments  and blank lines */

		if (i == samples+1) {
			overflow = i;	/* keep track for error message later */
			i--;			/* so we don't fall off end of points[i] */
		}
		switch (sscanf(line, "%f %f", &x, &y)) {
			case 1:			/* only one number on the line */
				y = x;		/* so use it as the y value, */
				x = i;		/* and use the index as the x */
			/* no break; !!! */
			case 2:
				this_plot->points[i].undefined = TRUE;
				if (x >= xmin && x <= xmax && (autoscale ||
					(y >= ymin && y <= ymax))) {
					if (log_x) {
						if (x <= 0.0)
							break;
						this_plot->points[i].x = log10(x);
					} else
						this_plot->points[i].x = x;
					if (log_y) {
						if (y <= 0.0)
							break;
						this_plot->points[i].y = log10(y);
					} else
						this_plot->points[i].y = y;
					if (autoscale) {
						if (y < ymin) ymin = y;
						if (y > ymax) ymax = y;
					}

					this_plot->points[i].undefined = FALSE;
				}
				if (overflow)
					overflow++;
				else
					i++;
				break;

			default:
				(void) sprintf(line, "bad data on line %d", l_num);
				int_error(line,c_token);
		}
	}
	if (overflow) {
		(void) sprintf(line,
	"%d data points found--samples must be set at least this high",overflow);
				/* actually, samples can be one less! */
		int_error(line,c_token);
	}
	this_plot->p_count = i;
	(void) fclose(fp);
}


eval_plots()
{
register int i;
register struct curve_points *this_plot, **tp_ptr;
register int start_token, mysamples;
register double x_min, x_max, y_min, y_max, x;
register double xdiff, temp;
register int plot_num;
static struct value a;

	/* don't sample higher than output device can handle! */
	mysamples = (samples <= term_tbl[term].xmax) ?samples :term_tbl[term].xmax;

	if (log_x) {
		if (xmin < 0.0 || xmax < 0.0)
			int_error("x range must be above 0 for log scale!",NO_CARET);
		x_min = log10(xmin);
		x_max = log10(xmax);
	} else {
		x_min = xmin;
		x_max = xmax;
	}

	if (autoscale) {
		ymin = HUGE;
		ymax = -HUGE;
	} else if (log_y && (ymin <= 0.0 || ymax <= 0.0))
			int_error("y range must be above 0 for log scale!",
				NO_CARET);

	xdiff = (x_max - x_min) / mysamples;

	tp_ptr = &(first_plot);
	plot_num = 0;

	while (TRUE) {
		if (END_OF_COMMAND)
			int_error("function to plot expected",c_token);

		start_token = c_token;

		if (is_definition(c_token)) {
			define();
		} else {
			plot_num++;
			if (*tp_ptr)
				this_plot = *tp_ptr;
			else {		/* no memory malloc()'d there yet */
				this_plot = (struct curve_points *)
					malloc((unsigned int) (sizeof(struct curve_points) -
					(MAX_POINTS - (samples+1))*sizeof(struct coordinate)));
				if (!this_plot)
					int_error("out of memory",c_token);
				this_plot->next_cp = NULL;
				this_plot->title = NULL;
				*tp_ptr = this_plot;
			}

			if (isstring(c_token)) {			/* data file to plot */
				this_plot->plot_type = DATA;
				this_plot->plot_style = data_style;
				get_data(this_plot);
				c_token++;
			}
			else {							/* function to plot */
				this_plot->plot_type = FUNC;
				this_plot->plot_style = func_style;

				(void) strcpy(c_dummy_var,dummy_var);
				dummy_func = &plot_func;

				plot_func.at = temp_at();

				for (i = 0; i <= mysamples; i++) {
					if (i == samples+1)
						int_error("number of points exceeded samples",
							NO_CARET);
					x = x_min + i*xdiff;
					if (log_x)
						x = pow(10.0,x);
					(void) complex(&plot_func.dummy_value, x, 0.0);
					
					evaluate_at(plot_func.at,&a);

					if (this_plot->points[i].undefined =
						undefined || (fabs(imag(&a)) > zero))
							continue;

					temp = real(&a);

					if (log_y && temp <= 0.0) {
							this_plot->points[i].undefined = TRUE;
							continue;
					}
					if (autoscale) {
						if (temp < ymin) ymin = temp;
						if (temp > ymax) ymax = temp;
					} else if (temp < ymin || temp > ymax) {
						this_plot->points[i].undefined = TRUE;
						continue;
					}

					this_plot->points[i].y = log_y ? log10(temp) : temp;
				}
				this_plot->p_count = i; /* mysamples + 1 */
			}
			m_capture(&(this_plot->title),start_token,c_token-1);
			if (almost_equals(c_token,"w$ith"))
				this_plot->plot_style = get_style();
			tp_ptr = &(this_plot->next_cp);
		}

		if (equals(c_token,","))
			c_token++;
		else
			break;
	}

	if (autoscale && (ymin == ymax))
		ymax += 1.0;	/* kludge to avoid divide-by-zero in do_plot */

	if (log_y) {
		y_min = log10(ymin);
		y_max = log10(ymax);
	} else {
		y_min = ymin;
		y_max = ymax;
	}
	capture(replot_line,plot_token,c_token);	
	do_plot(first_plot,plot_num,x_min,x_max,y_min,y_max);
}



done(status)
int status;
{
	if (term)
		(*term_tbl[term].reset)();
	exit(status);
}


#ifdef vms

#include <descrip.h>
#include <rmsdef.h>
#include <errno.h>

extern lib$get_input(), lib$put_output();

int vms_len;

unsigned int status[2] = {1, 0};

$DESCRIPTOR(prompt_desc,PROMPT);
$DESCRIPTOR(line_desc,input_line);

$DESCRIPTOR(help_desc,help);
$DESCRIPTOR(helpfile_desc,"GNUPLOT$HELP");


read_line()
{
	switch(status[1] = lib$get_input(&line_desc, &prompt_desc, &vms_len)){
		case RMS$_EOF:
			done(IO_SUCCESS);	/* ^Z isn't really an error */
			break;
		case RMS$_TNS:			/* didn't press return in time */
			vms_len--;		/* skip the last character */
			break;			/* and parse anyway */
		case RMS$_BES:			/* Bad Escape Sequence */
		case RMS$_PES:			/* Partial Escape Sequence */
			sys$putmsg(status);
			vms_len = 0;		/* ignore the line */
			break;
		case SS$_NORMAL:
			break;			/* everything's fine */
		default:
			done(status[1]);	/* give the error message */
	}
	input_line[vms_len] = '\0';
}


do_help()
{
	help_desc.dsc$w_length = strlen(help);
	if ((vaxc$errno = lbr$output_help(lib$put_output,0,&help_desc,
		&helpfile_desc,0,lib$get_input)) != SS$_NORMAL)
			os_error("can't open GNUPLOT$HELP");
}


do_shell()
{
	if ((vaxc$errno = lib$spawn()) != SS$_NORMAL) {
		os_error("spawn error",NO_CARET);
	}
}


do_system()
{
	input_line[0] = ' ';	/* an embarrassment, but... */

	if ((vaxc$errno = lib$spawn(&line_desc)) != SS$_NORMAL)
		os_error("spawn error",NO_CARET);

	(void) putc('\n',stderr);
}

#else /* vms */

do_help()
{
	if (system(help))
		os_error("can't spawn help",c_token);
}


do_system()
{
	if (system(input_line + 1))
		os_error("system() failed",NO_CARET);
}

#ifdef MSDOS

read_line()
{
register int i;

	input_line[0] = MAX_LINE_LEN - 1;
	cputs(PROMPT);
	cgets(input_line);			/* console input so CED will work */
	(void) putc('\n',stderr);
	if (input_line[2] == 26) {
		(void) putc('\n',stderr);		/* end-of-file */
		done(IO_SUCCESS);
	}

	i = 0;
	while (input_line[i] = input_line[i+2])
		i++;		/* yuck!  move everything down two characters */
}


do_shell()
{
register char *comspec;
	if (!(comspec = getenv("COMSPEC")))
		comspec = "\command.com";
	if (spawnl(P_WAIT,comspec,NULL) == -1)
		os_error("unable to spawn shell",NO_CARET);
}

#else /* MSDOS */
		/* plain old Unix */

read_line()
{
	fputs(PROMPT,stderr);
	if (!gets(input_line)) {
		(void) putc('\n',stderr);		/* end-of-file */
		done(IO_SUCCESS);
	}
}

#ifdef VFORK

do_shell()
{
register char *shell;
register int p;
static int execstat;
	if (!(shell = getenv("SHELL")))
		shell = SHELL;
	if ((p = vfork()) == 0) {
		execstat = execl(shell,shell,NULL);
		_exit(1);
	} else if (p == -1)
		os_error("vfork failed",c_token);
	else
		while (wait(NULL) != p)
			;
	if (execstat == -1)
		os_error("shell exec failed",c_token);
	(void) putc('\n',stderr);
}
#else /* VFORK */

#define EXEC "exec "
do_shell()
{
static char exec[100] = EXEC;
register char *shell;
	if (!(shell = getenv("SHELL")))
		shell = SHELL;

	if (system(strncpy(&exec[sizeof(EXEC)-1],shell,
		sizeof(exec)-sizeof(EXEC)-1)))
		os_error("system() failed",NO_CARET);

	(void) putc('\n',stderr);
}
#endif /* VFORK */
#endif /* MSDOS */
#endif /* vms */
