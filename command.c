/* GNUPLOT - command.c */
/*
 * Copyright (C) 1986, 1987, 1990, 1991   Thomas Williams, Colin Kelley
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted, 
 * provided that the above copyright notice appear in all copies and 
 * that both that copyright notice and this permission notice appear 
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the modified code.  Modifications are to be distributed 
 * as patches to released version.
 *  
 * This software is provided "as is" without express or implied warranty.
 * 
 *
 * AUTHORS
 * 
 *   Original Software:
 *     Thomas Williams,  Colin Kelley.
 * 
 *   Gnuplot 2.0 additions:
 *       Russell Lang, Dave Kotz, John Campbell.
 *
 *   Gnuplot 3.0 additions:
 *       Gershon Elber and many others.
 * 
 * Send your comments or suggestions to 
 *  pixar!info-gnuplot@sun.com.
 * This is a mailing list; to join it send a note to 
 *  pixar!info-gnuplot-request@sun.com.  
 * Send bug reports to
 *  pixar!bug-gnuplot@sun.com.
 */

#include <stdio.h>
#include <math.h>

#ifdef AMIGA_AC_5
#include <time.h>
void sleep();		/* defined later */
#endif

#ifdef MSDOS
#include <process.h>

#ifdef __ZTC__
#define P_WAIT 0
#include <time.h>	/* usleep() */
#else

#ifdef __TURBOC__
#include <dos.h>	/* sleep() */
#include <conio.h>
extern unsigned _stklen = 16394;	/* increase stack size */

#else	/* must be MSC */
#include <time.h>	/* kludge to provide sleep() */
void sleep();		/* defined later */
#endif /* TURBOC */
#endif /* ZTC */

#endif /* MSDOS */

#ifdef AMIGA_LC_5_1
#include <proto/dos.h>
void sleep();
#endif	/* AMIGA_LC_5_1 */

#include "plot.h"
#include "setshow.h"
#include "help.h"

#ifndef STDOUT
#define STDOUT 1
#endif

#ifndef HELPFILE
#ifdef AMIGA_LC_5_1
#define HELPFILE "S:gnuplot.gih"
#else
#define HELPFILE "docs/gnuplot.gih" /* changed by makefile */
#endif
#endif

#define inrange(z,min,max) ((min<max) ? ((z>=min)&&(z<=max)) : ((z>=max)&&(z<=min)) )

/*
 * instead of <strings.h>
 */

extern char *gets(),*getenv();
extern char *strcpy(),*strncpy(),*strcat();
extern int strlen(), strcmp();

/*
 * Only reference to contours library.
 */
extern struct gnuplot_contours *contour();

#if defined(unix) && !defined(hpux)
#ifdef GETCWD
extern char *getcwd();	/* some Unix's use getcwd */
#else
extern char *getwd();	/* most Unix's use getwd */
#endif
#else
extern char *getcwd();	/* Turbo C, MSC and VMS use getcwd */
#endif

#ifdef vms
int vms_vkid; /* Virtual keyboard id */
#endif

extern int chdir();

extern double magnitude(),angle(),real(),imag();
extern struct value *const_express(), *pop(), *complex();
extern struct at_type *temp_at(), *perm_at();
extern struct udft_entry *add_udf();
extern struct udvt_entry *add_udv();
extern void squash_spaces();
extern void lower_case();

/* local functions */
static enum coord_type adjustlog();

extern BOOLEAN interactive;	/* from plot.c */

/* input data, parsing variables */
struct lexical_unit token[MAX_TOKENS];
char input_line[MAX_LINE_LEN+1] = "";
int num_tokens, c_token;
int inline_num = 0;			/* input line number */

char c_dummy_var[MAX_NUM_VAR][MAX_ID_LEN+1];	/* current dummy vars */

/* the curves/surfaces of the plot */
struct curve_points *first_plot = NULL;
struct surface_points *first_3dplot = NULL;
static struct udft_entry plot_func;
struct udft_entry *dummy_func;

/* support for replot command */
char replot_line[MAX_LINE_LEN+1] = "";
static int plot_token;					/* start of 'plot' command */

/* If last plot was a 3d one. */
BOOLEAN is_3d_plot = FALSE;

com_line()
{
	read_line(PROMPT);

	/* So we can flag any new output: if false at time of error, */
	/* we reprint the command line before printing caret. */
	/* TRUE for interactive terminals, since the command line is typed. */
	/* FALSE for non-terminal stdin, so command line is printed anyway. */
	/* (DFK 11/89) */
	screen_ok = interactive; 

	do_line();
}


do_line()	  /* also used in load_file */
{
	if (is_system(input_line[0])) {
		do_system();
		(void) fputs("!\n",stderr);
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
    int i;
    char sv_file[MAX_LINE_LEN+1];
    /* string holding name of save or load file */

	for (i = 0; i < MAX_NUM_VAR; i++)
	    c_dummy_var[i][0] = '\0';		/* no dummy variables */

	if (is_definition(c_token))
		define();
	else if (almost_equals(c_token,"h$elp") || equals(c_token,"?")) {
	    c_token++;
	    do_help();
	}
	else if (almost_equals(c_token,"test")) {
	    c_token++;
		test_term();
	}
	else if (almost_equals(c_token,"pa$use")) {
		struct value a;
		int stime, text=0;
		char buf[MAX_LINE_LEN+1];

		c_token++;
		stime = (int )real(const_express(&a));
		if (!(END_OF_COMMAND)) {
			if (!isstring(c_token))
				int_error("expecting string",c_token);
			else {
				quotel_str(buf,c_token);
				(void) fprintf (stderr, "%s",buf);
				text = 1;
			}
		}
		if (stime < 0) (void) fgets (buf,MAX_LINE_LEN,stdin);  
						/* Hold until CR hit. */
#ifdef __ZTC__
		if (stime > 0) usleep((unsigned long) stime);
#else
		if (stime > 0) sleep((unsigned int) stime);
#endif
		if (text != 0 && stime >= 0) (void) fprintf (stderr,"\n");
		c_token++;
		screen_ok = FALSE;
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
	else if (almost_equals(c_token,"sp$lot")) {
		plot_token = c_token++;
		plot3drequest();
	}
	else if (almost_equals(c_token,"rep$lot")) {
		if (replot_line[0] == '\0') 
			int_error("no previous plot",c_token);
		c_token++;
		replotrequest();
	}
	else if (almost_equals(c_token,"se$t"))
		set_command();
	else if (almost_equals(c_token,"sh$ow"))
		show_command();
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
		else if (almost_equals(c_token,"s$et")) {
			if (!isstring(++c_token))
				int_error("expecting filename",c_token);
			else {
				quote_str(sv_file,c_token);
				save_set(fopen(sv_file,"w"));
			}
		}
		else if (isstring(c_token)) {
			quote_str(sv_file,c_token);
			save_all(fopen(sv_file,"w"));
		}
		else {
			int_error(
		"filename or keyword 'functions', 'variables', or 'set' expected",
					c_token);
		}
		c_token++;
	}
	else if (almost_equals(c_token,"l$oad")) {
		if (!isstring(++c_token))
			int_error("expecting filename",c_token);
		else {
			quote_str(sv_file,c_token);
			load_file(fopen(sv_file,"r"), sv_file);	
		/* input_line[] and token[] now destroyed! */
			c_token = num_tokens = 0;
		}
	}
	else if (almost_equals(c_token,"cd")) {
		if (!isstring(++c_token))
			int_error("expecting directory name",c_token);
		else {
			quotel_str(sv_file,c_token);
			if (chdir(sv_file)) {
			  int_error("Can't change to this directory",c_token);
			}
		c_token++;
		}
	}
	else if (almost_equals(c_token,"pwd")) {
#if defined(unix) && !defined(hpux)
#ifdef GETCWD
	  (void) getcwd(sv_file,MAX_ID_LEN); /* some Unix's use getcwd */
#else
	  (void) getwd(sv_file); /* most Unix's use getwd */
#endif
#else
/* Turbo C and VMS have getcwd() */
	  (void) getcwd(sv_file,MAX_ID_LEN);
#endif
	  fprintf(stderr,"%s\n", sv_file);
	  c_token++;
	}
	else if (almost_equals(c_token,"ex$it") ||
			almost_equals(c_token,"q$uit")) {
		done(IO_SUCCESS);
	}
	else if (!equals(c_token,";")) {		/* null statement */
		int_error("invalid command",c_token);
	}
}

replotrequest()
{
char str[MAX_LINE_LEN+1];
		if(equals(c_token,"["))
			int_error("cannot set range with replot",c_token);
		if (!END_OF_COMMAND) {
			capture(str,c_token,num_tokens-1);
			if ( (strlen(str) + strlen(replot_line)) <= MAX_LINE_LEN-1) {
				(void) strcat(replot_line,",");
				(void) strcat(replot_line,str); 
			} else {
				int_error("plot line too long with replot arguments",c_token);
			}
		}
		(void) strcpy(input_line,replot_line);
		screen_ok = FALSE;
		num_tokens = scanner(input_line);
		c_token = 1;					/* skip the 'plot' part */
		is_3d_plot ? plot3drequest() : plotrequest();
}


plotrequest()
/*
   In the parametric case we can say 
      plot [a= -4:4] [-2:2] [-1:1] sin(a),a**2
   while in the non-parametric case we would say only
      plot [b= -2:2] [-1:1] sin(b)
*/
{
    BOOLEAN changed;
    int dummy_token = -1;

    is_3d_plot = FALSE;

    if (parametric && strcmp(dummy_var[0], "u") == 0)
	strcpy (dummy_var[0], "t");

    autoscale_lt = autoscale_t;
    autoscale_lx = autoscale_x;
    autoscale_ly = autoscale_y;

	if (!term)					/* unknown */
		int_error("use 'set term' to set terminal type first",c_token);

	if (equals(c_token,"[")) {
		c_token++;
		if (isletter(c_token)) {
		    if (equals(c_token+1,"=")) {
			   dummy_token = c_token;
			   c_token += 2;
		    } else {
			   /* oops; probably an expression with a variable. */
			   /* Parse it as an xmin expression. */
			   /* used to be: int_error("'=' expected",c_token); */
		    }
		}
		changed = parametric ? load_range(&tmin,&tmax):load_range(&xmin,&xmax);
		if (!equals(c_token,"]"))
			int_error("']' expected",c_token);
		c_token++;
		if (changed) {
			if (parametric)
				autoscale_lt = FALSE;
			else
				autoscale_lx = FALSE;
		}
	}

	if (parametric && equals(c_token,"[")) { /* set optional x ranges */
		c_token++;
		changed = load_range(&xmin,&xmax);
		if (!equals(c_token,"]"))
			int_error("']' expected",c_token);
		c_token++;
		if (changed)
		  autoscale_lx = FALSE;
	}

	if (equals(c_token,"[")) { /* set optional y ranges */
		c_token++;
		changed = load_range(&ymin,&ymax);
		if (!equals(c_token,"]"))
			int_error("']' expected",c_token);
		c_token++;
		if (changed)
		  autoscale_ly = FALSE;
	}

     /* use the default dummy variable unless changed */
	if (dummy_token >= 0)
	  copy_str(c_dummy_var[0],dummy_token);
	else
	  (void) strcpy(c_dummy_var[0],dummy_var[0]);

	eval_plots();
}

plot3drequest()
/*
   in the parametric case we would say
      splot [u= -Pi:Pi] [v= 0:2*Pi] [-1:1] [-1:1] [-1:1] sin(v)*cos(u),sin(v)*cos(u),sin(u)
   in the non-parametric case we would say only
      splot [x= -2:2] [y= -5:5] sin(x)*cos(y)

*/
{
    BOOLEAN changed;
    int dummy_token0 = -1,
	dummy_token1 = -1;

    is_3d_plot = TRUE;

    if (parametric && strcmp(dummy_var[0], "t") == 0) {
    	strcpy (dummy_var[0], "u");
    	strcpy (dummy_var[1], "v");
    }

    autoscale_lx = autoscale_x;
    autoscale_ly = autoscale_y;
    autoscale_lz = autoscale_z;

	if (!term)					/* unknown */
		int_error("use 'set term' to set terminal type first",c_token);

	if (equals(c_token,"[")) {
		c_token++;
		if (isletter(c_token)) {
		    if (equals(c_token+1,"=")) {
			   dummy_token0 = c_token;
			   c_token += 2;
		    } else {
			   /* oops; probably an expression with a variable. */
			   /* Parse it as an xmin expression. */
			   /* used to be: int_error("'=' expected",c_token); */
		    }
		}
		changed = parametric ? load_range(&umin,&umax):load_range(&xmin,&xmax);
		if (!equals(c_token,"]"))
			int_error("']' expected",c_token);
		c_token++;
		if (changed && !parametric) {
			autoscale_lx = FALSE;
		}
	}

	if (equals(c_token,"[")) {
		c_token++;
		if (isletter(c_token)) {
		    if (equals(c_token+1,"=")) {
			   dummy_token1 = c_token;
			   c_token += 2;
		    } else {
			   /* oops; probably an expression with a variable. */
			   /* Parse it as an xmin expression. */
			   /* used to be: int_error("'=' expected",c_token); */
		    }
		}
		changed = parametric ? load_range(&vmin,&vmax):load_range(&ymin,&ymax);
		if (!equals(c_token,"]"))
			int_error("']' expected",c_token);
		c_token++;
		if (changed && !parametric) {
			autoscale_ly = FALSE;
		}
	}

	if (equals(c_token,"[")) { /* set optional x ranges */
		c_token++;
		changed = load_range(&xmin,&xmax);
		if (!equals(c_token,"]"))
			int_error("']' expected",c_token);
		c_token++;
		if (changed)
		  autoscale_lx = FALSE;
	}

	if (equals(c_token,"[")) { /* set optional y ranges */
		c_token++;
		changed = load_range(&ymin,&ymax);
		if (!equals(c_token,"]"))
			int_error("']' expected",c_token);
		c_token++;
		if (changed)
		  autoscale_ly = FALSE;
	}

	if (equals(c_token,"[")) { /* set optional z ranges */
		c_token++;
		changed = load_range(&zmin,&zmax);
		if (!equals(c_token,"]"))
			int_error("']' expected",c_token);
		c_token++;
		if (changed)
		  autoscale_lz = FALSE;
	}

     /* use the default dummy variable unless changed */
	if (dummy_token0 >= 0)
	  copy_str(c_dummy_var[0],dummy_token0);
	else
	  (void) strcpy(c_dummy_var[0],dummy_var[0]);

	if (dummy_token1 >= 0)
	  copy_str(c_dummy_var[1],dummy_token1);
	else
	  (void) strcpy(c_dummy_var[1],dummy_var[1]);

	eval_3dplots();
}


define()
{
register int start_token;  /* the 1st token in the function definition */
register struct udvt_entry *udv;
register struct udft_entry *udf;

	if (equals(c_token+1,"(")) {
		/* function ! */
		start_token = c_token;
		copy_str(c_dummy_var[0], c_token + 2);
		if(equals(c_token+3,",")) {
			copy_str(c_dummy_var[1], c_token + 4);
			c_token += 2;  /* skip the  , dummy2 */
		}
		c_token += 5; /* skip (, dummy, ) and = */
		if (END_OF_COMMAND)
			int_error("function definition expected",c_token);
		udf = dummy_func = add_udf(start_token);
		if (udf->at)				/* already a dynamic a.t. there */
			free((char *)udf->at);	/* so free it first */
		if ((udf->at = perm_at()) == (struct at_type *)NULL)
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
register int i, l_num, datum;
register FILE *fp;
float x, y;
float ylow, yhigh;			/* for error bars */
float temp;
BOOLEAN yfirst;
char format[MAX_LINE_LEN+1], data_file[MAX_LINE_LEN+1], line[MAX_LINE_LEN+1];
char *float_format = "%f", *float_skip = "%*f";
int xcol = 1, ycol = 2, yemin = 3, yemax = 4;
struct value colvalue;
#ifdef AMIGA_LC_5_1
int num_perc_ast;
char *start_search;
#endif /* AMIGA_LC_5_1 */

	quote_str(data_file, c_token);
	this_plot->plot_type = DATA;
	if ((fp = fopen(data_file, "r")) == (FILE *)NULL)
		os_error("can't open data file", c_token);

	format[0] = '\0';
	yfirst = FALSE;
	c_token++;	/* skip data file name */
	if (almost_equals(c_token,"u$sing")) {
		c_token++;  	/* skip "using" */
		if (!END_OF_COMMAND && !isstring(c_token)) {
			struct value a;
			ycol = (int)magnitude(const_express(&a));
			if (equals(c_token,":")) {
				c_token++;      /* skip ":" */
				xcol = ycol;
				ycol = (int)magnitude(const_express(&a));
				if (equals(c_token,":")) {
					c_token++;      /* skip ":" */
					yemin = (int)magnitude(const_express(&a));
					if (equals(c_token,":")) {
						c_token++;      /* skip ":" */
						yemax = (int)magnitude(const_express(&a));
					}
					else
					        yemax = -1;
				}
				else {
				        yemin = -1;
				        yemax = -1;
				}
			}
			else {
			        yemin = -1;
			        yemax = -1;
			}
			if (xcol > ycol) yfirst = TRUE;
		}
		if (!END_OF_COMMAND && isstring(c_token)) {
			quotel_str(format, c_token);
			c_token++;	/* skip format */
		}
	}
	if (strlen(format) == 0) {
		for(i = 1; i <= ((xcol > ycol) ? xcol : ycol); i++)
			if ((i == xcol) || (i == ycol))
				(void) strcat(format,float_format);
			else
				(void) strcat(format,float_skip);

		if (yemin > 0) {
			/* We have error bars - handle them. */
			yemin -= (xcol > ycol) ? xcol : ycol;
			yemax -= (xcol > ycol) ? xcol : ycol;
			if (yemin > yemax && yemax > 0) {
				i = yemin;
				yemin = yemax;
				yemax = i;
			}

			if (yemin == yemax)
				int_error("Two error bar columns are the same",
					  NO_CARET);

			if (yemin <= 0)
				int_error("Error bar columns must follow data columns",
					  NO_CARET);

			for (i = 1; i < yemin; i++)
				(void) strcat(format,float_skip);
			(void) strcat(format,float_format);

			if (yemax > 0) {
				for (i = 1; i < yemax - yemin; i++)
					(void) strcat(format,float_skip);
				(void) strcat(format,float_format);
			}
		}
	}

	l_num = 0;
     datum = 0;
	i = 0;
#ifdef AMIGA_LC_5_1
	num_perc_ast = 0;
	start_search = format;
	while (*start_search != '\0') {
		if (start_search[0] == '%')
			if (start_search[1] == '*') num_perc_ast++;
		start_search++;
	}
#endif /* AMIGA_LC_5_1 */
	while ( fgets(line, MAX_LINE_LEN, fp) != (char *)NULL ) {
		l_num++;
		if (is_comment(line[0]))
			continue;		/* ignore comments */
		if (i >= this_plot->p_max) {
		    /* overflow about to occur. Extend size of points[]
			* array. We either double the size, or add 1000 points,
			* whichever is a smaller increment. Note i=p_max.
			*/
		    cp_extend(this_plot, i + (i < 1000 ? i : 1000));
		}
		if (!line[1]) { /* is it blank line ? */
			/* break in data, make next point undefined */
			this_plot->points[i].type = UNDEFINED;
			i++;
			continue;
		}

#ifdef AMIGA_LC_5_1
		switch (sscanf(line, format, &x, &y, &ylow, &yhigh) -
			 num_perc_ast) {
#else /* AMIGA_LC_5_1 */
		switch (sscanf(line, format, &x, &y, &ylow, &yhigh)) {
#endif /* AMIGA_LC_5_1 */
		    case 1: {		/* only one number on the line */
			   y = x;		/* assign that number to y */
			   x = datum;	/* and make the index into x */
			   /* no break; !!! */
		    }
		    case 2: {
			   if (yfirst) { /* exchange x and y */
				  temp = y;
				  y = x;
				  x = temp;
			   }
			   datum++;

			   /* ylow and yhigh are same as y */
			   store2d_point(this_plot, i++, x, y, y, y);
			   break;
		    }
		    case 3: {		/* x, y, ydelta */
			   if (yfirst) { /* exchange x and y */
				  temp = y;
				  y = x;
				  x = temp;
			   }
			   datum++;

			   /* ydelta is in the ylow variable */
			   store2d_point(this_plot, i++, x, y, y-ylow, y+ylow);
			   break;
		    }
		    case 4: {		/* x, y, ylow, yhigh */
			   if (yfirst) { /* exchange x and y */
				  temp = y;
				  y = x;
				  x = temp;
			   }
			   datum++;

			   store2d_point(this_plot, i++, x, y, ylow, yhigh);
			   break;
		    }
		    default: {
			   (void) sprintf(line, "bad data on line %d", l_num);
			   int_error(line,c_token);
		    }
		}
	}
	this_plot->p_count = i;
	cp_extend(this_plot, i);	/* shrink to fit */
	(void) fclose(fp);
}

/* called by get_data for each point */
store2d_point(this_plot, i, x, y, ylow, yhigh)
struct curve_points *this_plot;
int i;					/* point number */
#ifdef AMIGA_LC_5_1
double x, y;
double ylow, yhigh;
#else
float x, y;
float ylow, yhigh;
#endif
{
    struct coordinate *cp = &(this_plot->points[i]);

    /* the easy part: */
    cp->type = INRANGE;
    cp->x = x;
    cp->y = y;
    cp->ylow = ylow;
    cp->yhigh = yhigh;
    
    /* Adjust for log scale. */
    if (log_x)
	 cp->type = adjustlog(cp->type, &(cp->x));
    if (log_y) {
	   cp->type = adjustlog(cp->type, &(cp->y));
	   /* Note ylow,yhigh can't affect cp->type. */
	   (void)     adjustlog(cp->type, &(cp->ylow));
	   (void)     adjustlog(cp->type, &(cp->yhigh));
    }

    /* Now adjust the xrange, or declare the point out of range */
    /* The yrange is handled later, once we know whether to 
	* include ylow, yhigh in the calculation. See adjust_yrange()
	*/
    if (cp->type == INRANGE)
	 if (autoscale_lx || inrange(x,xmin,xmax)) {
		if (autoscale_lx) {
		    if (x < xmin) xmin = x;
		    if (x > xmax) xmax = x;
		}
	 } else {
		cp->type = OUTRANGE;
	 }
}

/* Adjust for log scale:
 * take the log of the second parameter, in place, if possible. 
 * If not possible, return new type for point; if possible, then 
 * return old type for point.
 */
static enum coord_type
adjustlog(type, val)
	enum coord_type type;
	coordval *val;
{
    if (*val < 0.0) {
	   return(UNDEFINED);
    } else if (*val == 0.0) {
	   *val = -VERYLARGE;
	   return(OUTRANGE);
    } else {
	   *val = log10(*val);
	   return(type);
    }
}


/* now adjust the yrange, or declare the point out of range */
/* this does all points in a curve */
adjust_yrange(curve)
	struct curve_points *curve;
{
    BOOLEAN ebars = (curve->plot_style == ERRORBARS);
    int npoints = curve->p_count; /* number of points */
    coordval y, ylow, yhigh;	/* one point value */
    struct coordinate *cp;	/* one coordinate */
    int i;				/* index into points */

    for (i = 0; i < npoints; i++) {
	   cp = &(curve->points[i]);
	   if (cp->type == INRANGE) {
		  y = cp->y;
		  if ((autoscale_ly ||
		       inrange(log_y ? pow(10.0, y) : y, ymin, ymax) ||
		       polar)) {
			 if (autoscale_ly) {
				if (y < ymin) ymin = y;
				if (y > ymax) ymax = y;
				if (ebars) {
				    ylow = cp->ylow;
				    yhigh = cp->yhigh;
				    if (ylow < ymin) ymin = ylow;
				    if (ylow > ymax) ymax = ylow;
				    if (yhigh < ymin) ymin = yhigh;
				    if (yhigh > ymax) ymax = yhigh;
				}
			 }
		  } else {
			 cp->type = OUTRANGE;
		  }
	   }
    }
}


get_3ddata(this_plot)
struct surface_points *this_plot;
{
register int i, j, l_num, xdatum, ydatum;
register FILE *fp;
float x, y, z;
BOOLEAN only_z = FALSE;
char format[MAX_LINE_LEN+1], data_file[MAX_LINE_LEN+1], line[MAX_LINE_LEN+1];
char *float_format = "%f", *float_skip = "%*f";
int xcol = 1, ycol = 2, zcol = 3, pt_in_iso_crv = 0, maxcol;
enum XYZ_order_type {XYZ, YXZ, ZXY, XZY, ZYX, YZX, XY, YX} xyz_order;
struct iso_curve *this_iso;
#ifdef AMIGA_LC_5_1
int num_perc_ast;
char *start_search;
#endif /* AMIGA_LC_5_1 */
	    
	quote_str(data_file, c_token);
	this_plot->plot_type = DATA3D;
	this_plot->has_grid_topology = TRUE;
	if ((fp = fopen(data_file, "r")) == (FILE *)NULL)
		os_error("can't open data file", c_token);

	format[0] = '\0';
	c_token++;	/* skip data file name */
	if (almost_equals(c_token,"u$sing")) {
		c_token++;  	/* skip "using" */
		if (!END_OF_COMMAND && !isstring(c_token)) {
			struct value a;
			zcol = (int)magnitude(const_express(&a));
			only_z = TRUE;
			if (equals(c_token,":")) {
				c_token++;	/* skip ":" */
				only_z = FALSE;
				ycol = zcol;
				zcol = (int)magnitude(const_express(&a));
				if (equals(c_token,":")) {
					c_token++;	/* skip ":" */
					xcol = ycol;
					ycol = zcol;
					zcol = (int)magnitude(const_express(&a));
				}
				else {
					if (mapping3d == MAP3D_CARTESIAN)
						int_error("Must specify 1 or 3 columns",c_token);
					xcol = ycol;
					ycol = zcol;
				}
			}
			if (!only_z)
				if ( (xcol == ycol) || (ycol == zcol) || (xcol == zcol))
					int_error("Columns must be distinct",c_token);
		}
		if (!END_OF_COMMAND && isstring(c_token)) {
			quotel_str(format, c_token);
			c_token++;	/* skip format */
		}
	}

	switch (mapping3d) {
	    case MAP3D_CARTESIAN:
		maxcol = (xcol > ycol) ? xcol : ycol;
		maxcol = (maxcol > zcol) ? maxcol : zcol;
		if (!only_z) {	/* Determine ordering of input columns */
 			if (zcol == maxcol) {
 				if (xcol < ycol)
 					xyz_order = XYZ;  /* scanf(x,y,z) */
 				else
 					xyz_order = YXZ;  /* scanf(y,x,z) */
 			}
 			else if (ycol == maxcol) {
 				if (xcol < zcol)
 					xyz_order = XZY;  /* scanf(x,z,y) */
 				else
 					xyz_order = ZXY;  /* scanf(z,x,y) */
 			}
 			else {
 				if (ycol < zcol)
 					xyz_order = YZX;  /* scanf(y,z,x) */
 				else
 					xyz_order = ZYX;  /* scanf(z,y,x) */
 			}
		}
		if (strlen(format) == 0) {
			if (only_z) {
				for(i = 1; i <= zcol; i++)
					if (i == zcol)
						(void) strcat(format,float_format);
					else
						(void) strcat(format,float_skip);
			}
			else {
				for(i = 1; i <= maxcol; i++)
					if ((i == xcol) || (i == ycol) || (i == zcol))
						(void) strcat(format,float_format);
					else
						(void) strcat(format,float_skip);
			}
		}
	        break;
	    case MAP3D_SPHERICAL:
	    case MAP3D_CYLINDRICAL:
		if (only_z)
			int_error("Two columns for spherical/cylindrical coords.",c_token);
		maxcol = (xcol > ycol) ? xcol : ycol;
		xyz_order = (xcol < ycol) ? XY : YX;
		for(i = 1; i <= maxcol; i++)
			if ((i == xcol) || (i == ycol))
				(void) strcat(format,float_format);
			else
				(void) strcat(format,float_skip);
	}
#ifdef AMIGA_LC_5_1
	num_perc_ast = 0;
	start_search = format;
	while (*start_search != '\0') {
		if (start_search[0] == '%')
			if (start_search[1] == '*') num_perc_ast++;
		start_search++;
	}
#endif /* AMIGA_LC_5_1 */

	l_num = 0;
	xdatum = 0;
	ydatum = 0;
	this_plot->num_iso_read = 0;
	this_plot->has_grid_topology = TRUE;
	if ( this_plot->iso_crvs != NULL ) {
	    struct iso_curve *icrv, *icrvs = this_plot->iso_crvs;

	    while ( icrvs ) {
		icrv = icrvs;
		icrvs = icrvs->next;
		iso_free( icrv );
	    }
	    this_plot->iso_crvs = NULL;
	}
	this_iso = iso_alloc( samples );

	while ( fgets(line, MAX_LINE_LEN, fp) != (char *)NULL ) {
		l_num++;
		if (is_comment(line[0]))
			continue;		/* ignore comments */
		if (!line[1]) {			/* is it blank line ? */
		        if (pt_in_iso_crv == 0) {
			        if (xdatum == 0)
					continue;
			        pt_in_iso_crv = xdatum;
			}

			if (xdatum > 0) {
				this_iso->p_count = xdatum;
				this_iso->next = this_plot->iso_crvs;
				this_plot->iso_crvs = this_iso;
				this_plot->num_iso_read++;

				if (xdatum != pt_in_iso_crv)
					this_plot->has_grid_topology = FALSE;

				this_iso = iso_alloc(pt_in_iso_crv);
				xdatum = 0;
				ydatum++;
			}
			continue;
		}

		if (xdatum >= this_iso->p_max)
		{
		    /* overflow about to occur. Extend size of points[]
			* array. We either double the size, or add 1000 points,
			* whichever is a smaller increment. Note i=p_max.
			*/
		    iso_extend(this_iso,
			       xdatum + (xdatum < 1000 ? xdatum : 1000));
		}

#ifdef AMIGA_LC_5_1
		switch (sscanf(line, format, &x, &y, &z) - num_perc_ast) {
#else
		switch (sscanf(line, format, &x, &y, &z)) {
#endif
		    case 3: 		/* All parameter are specified. */
			   if (!only_z) {
				switch (xyz_order) {
				   case XYZ:	/* scanf(x,y,z) */
					this_iso->points[xdatum].x = x;
					this_iso->points[xdatum].y = y;
					this_iso->points[xdatum].z = z;
					break;
				   case XZY:	/* scanf(x,z,y) */
					this_iso->points[xdatum].x = x;
					this_iso->points[xdatum].y = z;
					this_iso->points[xdatum].z = y;
					break;
				   case YXZ: 	/* scanf(y,x,z) */
					this_iso->points[xdatum].x = y;
					this_iso->points[xdatum].y = x;
					this_iso->points[xdatum].z = z;
					break;
				   case ZXY:	/* scanf(z,x,y) */
					this_iso->points[xdatum].x = y;
					this_iso->points[xdatum].y = z;
					this_iso->points[xdatum].z = x;
					break;
				   case YZX:	/* scanf(y,z,x) */
					this_iso->points[xdatum].x = z;
					this_iso->points[xdatum].y = x;
					this_iso->points[xdatum].z = y;
					break;
				   case ZYX:	/* scanf(z,y,x) */
					this_iso->points[xdatum].x = z;
					this_iso->points[xdatum].y = y;
					this_iso->points[xdatum].z = x;
					break;
				}
				if (xyz_order != XYZ) {
					x = this_iso->points[xdatum].x;
					y = this_iso->points[xdatum].y;
					z = this_iso->points[xdatum].z;
				}
				if (!parametric)
					int_error("Must be in parametric mode.",
						  NO_CARET);
			   	break;
			   }
		    case 1: 		/* only one number on the line */
			   if (!only_z)
				int_error("3 columns expected, only 1 found", c_token);
			   /* assign that number to z */
			   this_iso->points[xdatum].z = x;
			   z = x;
			   this_iso->points[xdatum].x = xdatum;
			   x = this_iso->points[xdatum].x;
			   this_iso->points[xdatum].y = ydatum;
			   y = this_iso->points[xdatum].y;
			   if (parametric)
				int_error("Must be in non parametric mode.",
					  NO_CARET);
			   break;
		    case 2:
			   switch (xyz_order) {
			       case YX:
				   z = x;	/* Use z as temp */
				   x = y;
				   y = z;
				   break;
			       default:
				   break;
			   }
			   switch (mapping3d) {
			       case MAP3D_CARTESIAN:
			           int_error("2 columns found, 3 expected",
					     c_token);
				   break;
			       case MAP3D_SPHERICAL:
				   if (angles_format == ANGLES_DEGREES) {
				       x *= DEG2RAD; /* Convert to radians. */
				       y *= DEG2RAD;
				   }
				   this_iso->points[xdatum].x = cos(x) * cos(y);
				   this_iso->points[xdatum].y = sin(x) * cos(y);
				   this_iso->points[xdatum].z = sin(y);
				   break;
			       case MAP3D_CYLINDRICAL:
				   if (angles_format == ANGLES_DEGREES)
				       x *= DEG2RAD; /* Convert to radians. */
				   this_iso->points[xdatum].x = cos(x);
				   this_iso->points[xdatum].y = sin(x);
				   this_iso->points[xdatum].z = y;
				   break;
			   }
			   x = this_iso->points[xdatum].x;
			   y = this_iso->points[xdatum].y;
			   z = this_iso->points[xdatum].z;
			   break;
		    default:
			   (void) sprintf(line, "bad data on line %d", l_num);
			   int_error(line,c_token);
		}

		if (log_x) {
		    if (x < 0.0)
			int_error("X value must be above 0 for log scale!",
				  NO_CARET);
		    else
			this_iso->points[xdatum].x =
			    log10(this_iso->points[xdatum].x);
		}
		if (log_y) {
		    if (y < 0.0)
			int_error("Y value must be above 0 for log scale!",
				  NO_CARET);
		    else
			this_iso->points[xdatum].y =
			    log10(this_iso->points[xdatum].y);
		}
		if (log_z) {
		    if (z < 0.0)
			int_error("Z value must be above 0 for log scale!",
				  NO_CARET);
		    else
			this_iso->points[xdatum].z =
			    log10(this_iso->points[xdatum].z);
		}

		if (autoscale_lx) {
			if (x < xmin) xmin = x;
			if (x > xmax) xmax = x;
		}
		if (autoscale_ly) {
			if (y < ymin) ymin = y;
			if (y > ymax) ymax = y;
		}
		if (autoscale_lz) {
			if (z < zmin) zmin = z;
			if (z > zmax) zmax = z;
		}

		xdatum++;
	}

	if (xdatum > 0) {
		this_plot->num_iso_read++; /* Update last iso. */
		this_iso->p_count = xdatum;

		this_iso->next = this_plot->iso_crvs; 
		this_plot->iso_crvs = this_iso;

		if (xdatum != pt_in_iso_crv)
			this_plot->has_grid_topology = FALSE;

	}
	else {
		iso_free(this_iso);/* Free last allocation. */
	}

	(void) fclose(fp);

	if (this_plot->has_grid_topology) {
	        struct iso_curve *new_icrvs = NULL;
		int num_new_iso = this_plot->iso_crvs->p_count,
		    len_new_iso = this_plot->num_iso_read;

		/* Now we need to set the other direction (pseudo) isolines. */
	        for (i = 0; i < num_new_iso; i++) {
		    struct iso_curve *new_icrv = iso_alloc(len_new_iso);

		    new_icrv->p_count = len_new_iso;

		    for (j = 0, this_iso = this_plot->iso_crvs;
		         this_iso != NULL;
			 j++, this_iso = this_iso->next) {
			new_icrv->points[j].x = this_iso->points[i].x;
		     	new_icrv->points[j].y = this_iso->points[i].y;
		     	new_icrv->points[j].z = this_iso->points[i].z;
		    }

		    new_icrv->next = new_icrvs;
		    new_icrvs = new_icrv;
		}

		/* Append the new iso curves after the read ones. */
		for (this_iso = this_plot->iso_crvs;
		     this_iso->next != NULL;
		     this_iso = this_iso->next);
		this_iso->next = new_icrvs;
	}
}

/* print_points:
 * a debugging routine to print out the points of a curve,
 * and the curve structure. If curve<0, then we print the 
 * list of curves.
 */
static char *plot_type_names[4] = {
    "Function", "Data", "3D Function", "3d data"
};
static char *plot_style_names[6] = {
    "Lines", "Points", "Impulses", "LinesPoints", "Dots", "Errorbars"
};

print_points(curve)
	int curve;			/* which curve to print */
{
    register struct curve_points *this_plot;
    int i;

    if (curve < 0) {
	   for (this_plot = first_plot, i=0; 
		   this_plot != NULL; 
		   i++, this_plot = this_plot->next_cp) {
		  printf("Curve %d:\n", i);
		  if ((int)this_plot->plot_type >= 0 && (int)(this_plot->plot_type) < 4)
		    printf("Plot type %d: %s\n", (int)(this_plot->plot_type),
				 plot_type_names[(int)(this_plot->plot_type)]);
		  else
		    printf("Plot type %d: BAD\n", (int)(this_plot->plot_type));
		  if ((int)this_plot->plot_style >= 0 && (int)(this_plot->plot_style) < 6)
		    printf("Plot style %d: %s\n", (int)(this_plot->plot_style),
				 plot_style_names[(int)(this_plot->plot_style)]);
		  else
		    printf("Plot style %d: BAD\n", (int)(this_plot->plot_style));
		  printf("Plot title: '%s'\n", this_plot->title);
		  printf("Line type %d\n", this_plot->line_type);
		  printf("Point type %d\n", this_plot->point_type);
		  printf("max points %d\n", this_plot->p_max);
		  printf("current points %d\n", this_plot->p_count);
		  printf("\n");
	   }
    } else {
	   for (this_plot = first_plot, i = 0; 
		   i < curve && this_plot != NULL; 
		   i++, this_plot = this_plot->next_cp)
		;
	   if (this_plot == NULL)
		printf("Curve %d does not exist; list has %d curves\n", curve, i);
	   else {
		  printf ("Curve %d, %d points\n", curve, this_plot->p_count);
		  for (i = 0; i < this_plot->p_count; i++) {
			 printf("%c x=%g y=%g z=%g ylow=%g yhigh=%g\n", 
				   this_plot->points[i].type == INRANGE ? 'i'
				   : this_plot->points[i].type == OUTRANGE ? 'o'
				   : 'u',
				   this_plot->points[i].x,
				   this_plot->points[i].y,
				   this_plot->points[i].z,
				   this_plot->points[i].ylow,
				   this_plot->points[i].yhigh);
		  }
		  printf("\n");
	   }
    }    
}


/* This parses the plot command after any range specifications. 
 * To support autoscaling on the x axis, we want any data files to 
 * define the x range, then to plot any functions using that range. 
 * We thus parse the input twice, once to pick up the data files, 
 * and again to pick up the functions. Definitions are processed 
 * twice, but that won't hurt.
 */
eval_plots()
{
register int i;
register struct curve_points *this_plot, **tp_ptr;
register int start_token, end_token;
register int begin_token;
double x_min, x_max, y_min, y_max;
register double x, xdiff, temp;
static struct value a;
BOOLEAN ltmp, some_data_files = FALSE;
int plot_num, line_num, point_num, xparam=0;
char *xtitle;
void parametric_fixup();

	if (autoscale_ly) {
		ymin = VERYLARGE;
		ymax = -VERYLARGE;
	} else if (log_y && (ymin <= 0.0 || ymax <= 0.0))
			int_error("y range must be above 0 for log scale!",
				NO_CARET);

	tp_ptr = &(first_plot);
	plot_num = 0;
	line_num = 0; 	/* default line type */
	point_num = 0;	/* default point type */

	xtitle = NULL;

	begin_token = c_token;

/*** First Pass: Read through data files ***/
/* This pass serves to set the xrange and to parse the command, as well 
 * as filling in every thing except the function data. That is done after
 * the xrange is defined.
 */
	while (TRUE) {
		if (END_OF_COMMAND)
			int_error("function to plot expected",c_token);

		start_token = c_token;

		if (is_definition(c_token)) {
			define();
		} else {
			plot_num++;

			if (isstring(c_token)) {			/* data file to plot */
				if (parametric && xparam) 
					int_error("previous parametric function not fully specified",
																	c_token);

				if (!some_data_files && autoscale_lx) {
				    xmin = VERYLARGE;
				    xmax = -VERYLARGE;
				}
				some_data_files = TRUE;

				if (*tp_ptr)
				  this_plot = *tp_ptr;
				else {		/* no memory malloc()'d there yet */
				    this_plot = cp_alloc(MIN_CRV_POINTS);
				    *tp_ptr = this_plot;
				}
				this_plot->plot_type = DATA;
				this_plot->plot_style = data_style;
				end_token = c_token;
				get_data(this_plot); /* this also parses the using option */
			} 
			else {							/* function to plot */
				if (parametric)			/* working on x parametric function */
					xparam = 1 - xparam;
				if (*tp_ptr) {
				    this_plot = *tp_ptr;
				    cp_extend(this_plot, samples+1);
				} else {		/* no memory malloc()'d there yet */
				    this_plot = cp_alloc(samples+1);
				    *tp_ptr = this_plot;
				}
				this_plot->plot_type = FUNC;
				this_plot->plot_style = func_style;
				dummy_func = &plot_func;
				plot_func.at = temp_at();
				/* ignore it for now */
				end_token = c_token-1;
			}

			if (almost_equals(c_token,"t$itle")) {
				if (parametric) {
					if (xparam) 
						int_error(
		"\"title\" allowed only after parametric function fully specified",
																	c_token);
					else if (xtitle != NULL)
						xtitle[0] = '\0';  /* Remove default title .*/
				}
				c_token++;
				if ( isstring( c_token ) ) {
					m_quote_capture(&(this_plot->title),c_token,c_token);
				}
				else {
					int_error("expecting \"title\" for plot",c_token);
				}
				c_token++;
			}
  			else {
  				m_capture(&(this_plot->title),start_token,end_token);
 				if (xparam) xtitle = this_plot->title;
  			}
  
  			this_plot->line_type = line_num;
			this_plot->point_type = point_num;

			if (almost_equals(c_token,"w$ith")) {
				if (parametric && xparam) 
					int_error("\"with\" allowed only after parametric function fully specified",
									c_token);
			    this_plot->plot_style = get_style();
			}

			if ( !equals(c_token,",") && !END_OF_COMMAND ) {
				struct value t;
				this_plot->line_type = (int)real(const_express(&t))-1;
			}
			if ( !equals(c_token,",") && !END_OF_COMMAND ) {
				struct value t;
				this_plot->point_type = (int)real(const_express(&t))-1;
			}
			if ( (this_plot->plot_style == POINTS) ||
				 (this_plot->plot_style == LINESPOINTS) ||
				 (this_plot->plot_style == ERRORBARS) )
					if (!xparam) point_num++;
			if (!xparam) line_num++;

			if (this_plot->plot_type == DATA) 
			  /* now that we know the plot style, adjust the yrange */
			  adjust_yrange(this_plot);

			tp_ptr = &(this_plot->next_cp);
		}

		if (equals(c_token,",")) 
			c_token++;
		else  
			break;
	}

	if (parametric && xparam) 
		int_error("parametric function not fully specified", NO_CARET);

	if (parametric) {
	/* Swap t and x ranges for duration of these eval_plot computations. */
		ltmp = autoscale_lx; autoscale_lx = autoscale_lt; autoscale_lt = ltmp;
		temp = xmin; xmin = tmin; tmin = temp;
		temp = xmax; xmax = tmax; tmax = temp;
	}

/*** Second Pass: Evaluate the functions ***/
/* Everything is defined now, except the function data. We expect
 * no syntax errors, etc, since the above parsed it all. This makes 
 * the code below simpler. If autoscale_ly, the yrange may still change.
 */
     if (fabs(xmax-xmin) < zero)
	  if (autoscale_lx) {
		 fprintf(stderr, "Warning: empty %c range [%g:%g], ", 
			parametric ? 't' : 'x', xmin,xmax);
		 if (fabs(xmin) < zero) {
			/* completely arbitary */
			xmin = -1.;
			xmax = 1.;
		 } else {
			/* expand range by 10% in either direction */
			xmin = xmin * 0.9;
			xmax = xmax * 1.1;
		 }
		 fprintf(stderr, "adjusting to [%g:%g]\n", xmin,xmax);
	  } else {
		 int_error("x range is less than `zero`", c_token);
	  }

	/* give error if xrange badly set from missing datafile error */
	if (xmin == VERYLARGE || xmax == -VERYLARGE) {
		int_error("x range is invalid", c_token);
	}

    if (log_x) {
	   if (xmin <= 0.0 || xmax <= 0.0)
		int_error("x range must be greater than 0 for log scale!",NO_CARET);
	   x_min = log10(xmin);
	   x_max = log10(xmax);
    } else {
	   x_min = xmin;
	   x_max = xmax;
    }

    xdiff = (x_max - x_min) / (samples - 1);

    tp_ptr = &(first_plot);
    plot_num = 0;
    this_plot = first_plot;
    c_token = begin_token;	/* start over */

    /* Read through functions */
	while (TRUE) {
		if (is_definition(c_token)) {
			define();
		} else {
			plot_num++;
			if (isstring(c_token)) {			/* data file to plot */
			    /* ignore this now */
				c_token++;
				if (almost_equals(c_token,"u$sing")) {
					c_token++;  	/* skip "using" */
						if (!isstring(c_token)) {
						struct value a;
						(void)magnitude(const_express(&a)); /* skip xcol */
						if (equals(c_token,":")) {
							c_token++;      /* skip ":" */
							(void)magnitude(const_express(&a)); /* skip ycol */
						}
						if (equals(c_token,":")) {
							c_token++;      /* skip ":" */
							(void)magnitude(const_express(&a)); /* skip yemin */
						}
						if (equals(c_token,":")) {
							c_token++;      /* skip ":" */
							(void)magnitude(const_express(&a)); /* skip yemax */
						}
					}
					if (isstring(c_token))
						c_token++;      /* skip format string */
				}
			}
			else {					/* function to plot */
				if (parametric)			/* working on x parametric function */
					xparam = 1 - xparam;
				dummy_func = &plot_func;
				plot_func.at = temp_at(); /* reparse function */

				for (i = 0; i < samples; i++) {
				    x = x_min + i*xdiff;
				    /* if (log_x) PEM fix logscale x axis */
				    /* x = pow(10.0,x); 26-Sep-89 */
				    (void) complex(&plot_func.dummy_values[0],
							    log_x ? pow(10.0,x) : x,
							    0.0);

				    evaluate_at(plot_func.at,&a);

				    if (undefined || (fabs(imag(&a)) > zero)) {
					   this_plot->points[i].type = UNDEFINED;
					   continue;
				    }

				    temp = real(&a);

				    if (log_y && temp < 0.0) {
					   this_plot->points[i].type = UNDEFINED;
					   continue;
				    }

				    this_plot->points[i].x = x;
				    if (log_y) {
					   if (temp == 0.0) {
						  this_plot->points[i].type = OUTRANGE;
						  this_plot->points[i].y = -VERYLARGE;
						  continue;
					   } else {
						  this_plot->points[i].y = log10(temp);
					   }
				    } else
					 this_plot->points[i].y = temp;

				    if (autoscale_ly || polar
					   || inrange(temp, ymin, ymax)) {
					   this_plot->points[i].type = INRANGE;
					/* When xparam is 1 we are not really computing y's! */
						if (!xparam && autoscale_ly) {
					   	if (temp < ymin) ymin = temp;
					   	if (temp > ymax) ymax = temp;
						}
				    } else
					 this_plot->points[i].type = OUTRANGE;
				}
				this_plot->p_count = i; /* samples */
			 }

			/* title was handled above */
			if (almost_equals(c_token,"t$itle")) {
				c_token++;
				c_token++;
			}

			/* style was handled above */
			if (almost_equals(c_token,"w$ith")) {
			    c_token++;
			    c_token++;
			}

			/* line and point types were handled above */
			if ( !equals(c_token,",") && !END_OF_COMMAND ) {
				struct value t;
				(void)real(const_express(&t));
			}
			if ( !equals(c_token,",") && !END_OF_COMMAND ) {
				struct value t;
				(void)real(const_express(&t));
			}

 			tp_ptr = &(this_plot->next_cp); /* used below */
			this_plot = this_plot->next_cp;
		 }
		
		if (equals(c_token,",")) 
		  c_token++;
		else  
		  break;
	 }

    /* throw out all curve_points at end of list, that we don't need  */
    cp_free(*tp_ptr);
    *tp_ptr = NULL;

    if (fabs(ymax - ymin) < zero)
	 /* if autoscale, widen range */
	 if (autoscale_ly) {
		fprintf(stderr, "Warning: empty y range [%g:%g], ", ymin, ymax);
		if (fabs(ymin) < zero) {
		    ymin = -1.;
		    ymax = 1.;
		} else {
		    /* expand range by 10% in either direction */
		    ymin = ymin * 0.9;
		    ymax = ymax * 1.1;
		}
		fprintf(stderr, "adjusting to [%g:%g]\n", ymin, ymax);
	 } else {
		int_error("y range is less than `zero`", c_token);
	 }

/* Now we finally know the real ymin and ymax */
	if (log_y) {
		y_min = log10(ymin);
		y_max = log10(ymax);
	} else {
		y_min = ymin;
		y_max = ymax;
	}
	capture(replot_line,plot_token,c_token);

	if (parametric) {
	/* Now put t and x ranges back before we actually plot anything. */
		ltmp = autoscale_lx; autoscale_lx = autoscale_lt; autoscale_lt = ltmp;
		temp = xmin; xmin = tmin; tmin = temp;
		temp = xmax; xmax = tmax; tmax = temp;
		if (some_data_files && autoscale_lx) {
		/* 
			Stop any further autoscaling in this case (may be a mistake, have
  			to consider what is really wanted some day in the future--jdc). 
		*/
		    autoscale_lx = 0;
		}
	/* Now actually fix the plot pairs to be single plots. */
		parametric_fixup (first_plot, &plot_num, &x_min, &x_max);
	}

	do_plot(first_plot,plot_num,x_min,x_max,y_min,y_max);
	cp_free(first_plot);
	first_plot = NULL;
}

/* This parses the splot command after any range specifications. 
 * To support autoscaling on the x/z axis, we want any data files to 
 * define the x/y range, then to plot any functions using that range. 
 * We thus parse the input twice, once to pick up the data files, 
 * and again to pick up the functions. Definitions are processed 
 * twice, but that won't hurt.
 */
eval_3dplots()
{
register int i,j,k;
register struct surface_points *this_plot, **tp_3d_ptr;
register int start_token, end_token;
register int begin_token;
double x_min, x_max, y_min, y_max, z_min, z_max;
register double x, xdiff, xisodiff, y, ydiff, yisodiff, temp;
static struct value a;
BOOLEAN ltmp, some_data_files = FALSE;
int plot_num, line_num, point_num,
    crnt_param = 0; /* 0=x, 1=y, 2=z */
char *xtitle;
char *ytitle;
void parametric_3dfixup();

	if (autoscale_lz) {
		zmin = VERYLARGE;
		zmax = -VERYLARGE;
	} else if (log_z && (zmin <= 0.0 || zmax <= 0.0))
			int_error("z range must be above 0 for log scale!",
				NO_CARET);

	tp_3d_ptr = &(first_3dplot);
	plot_num = 0;
	line_num = 0; 	/* default line type */
	point_num = 0;	/* default point type */

	xtitle = NULL;
	ytitle = NULL;

	begin_token = c_token;

/*** First Pass: Read through data files ***/
/* This pass serves to set the x/yranges and to parse the command, as well 
 * as filling in every thing except the function data. That is done after
 * the x/yrange is defined.
 */
	while (TRUE) {
		if (END_OF_COMMAND)
			int_error("function to plt3d expected",c_token);

		start_token = c_token;

		if (is_definition(c_token)) {
			define();
		} else {
			plot_num++;

			if (isstring(c_token)) {			/* data file to plot */
				if (parametric && crnt_param != 0)
					int_error("previous parametric function not fully specified",
																	c_token);

				if (!some_data_files) {
					if (autoscale_lx) {
						xmin = VERYLARGE;
						xmax = -VERYLARGE;
					}
					if (autoscale_ly) {
						ymin = VERYLARGE;
						ymax = -VERYLARGE;
					}
				}

				some_data_files = TRUE;

				if (*tp_3d_ptr)
				    this_plot = *tp_3d_ptr;
				else {	/* no memory malloc()'d there yet */
				    /* Allocate samples * iso_samples twice for */
				    /* Each of the isoparametric direction. */
				    this_plot = sp_alloc(0,0);
				    *tp_3d_ptr = this_plot;
				}

				this_plot->plot_type = DATA3D;
				this_plot->plot_style = data_style;
				end_token = c_token;
				get_3ddata(this_plot); /* this also parses the using option */
			} 
			else {						/* function to plot */
				if (parametric) /* Rotate between x/y/z axes */
					crnt_param = (crnt_param+1) % 3;
				if (*tp_3d_ptr) {
				    this_plot = *tp_3d_ptr;
				    sp_replace(this_plot,samples,2*iso_samples);
				}
				else {	/* no memory malloc()'d there yet */
				    /* Allocate samples * iso_samples twice for */
				    /* Each of the isoparametric direction. */
				    this_plot = sp_alloc(samples,2*iso_samples);
				    *tp_3d_ptr = this_plot;
				}

				this_plot->plot_type = FUNC3D;
				this_plot->has_grid_topology = TRUE;
				this_plot->plot_style = func_style;
				dummy_func = &plot_func;
				plot_func.at = temp_at();
				/* ignore it for now */
				end_token = c_token-1;
			}

			if (almost_equals(c_token,"t$itle")) {
				if (parametric) {
				        if (crnt_param)
						int_error(
		"\"title\" allowed only after parametric function fully specified",
																	c_token);
					else {
						/* Remove default title */
						if (xtitle != NULL)
							xtitle[0] = '\0';
						if (ytitle != NULL)
							ytitle[0] = '\0';
					}
			        }
				c_token++;
				if ( isstring( c_token ) ) {
					m_quote_capture(&(this_plot->title),c_token,c_token);
				}
				else {
					int_error("expecting \"title\" for plot",c_token);
				}
				c_token++;
			}
  			else {
  				m_capture(&(this_plot->title),start_token,end_token);
				if (crnt_param == 1) xtitle = this_plot->title;
				if (crnt_param == 2) ytitle = this_plot->title;
  			}
  
  			this_plot->line_type = line_num;
			this_plot->point_type = point_num;

			if (almost_equals(c_token,"w$ith")) {
			    this_plot->plot_style = get_style();
			}

			if ( !equals(c_token,",") && !END_OF_COMMAND ) {
				struct value t;
				this_plot->line_type = (int)real(const_express(&t))-1;
			}
			if ( !equals(c_token,",") && !END_OF_COMMAND ) {
				struct value t;
				this_plot->point_type = (int)real(const_express(&t))-1;
			}
			if ( (this_plot->plot_style == POINTS) ||
				 (this_plot->plot_style == LINESPOINTS) ||
				 (this_plot->plot_style == ERRORBARS) )
					if (crnt_param == 0)
						point_num +=
						    1 + (draw_contour != 0);
			if (crnt_param == 0)
			    line_num += 1 + (draw_contour != 0);

			tp_3d_ptr = &(this_plot->next_sp);
		}

		if (equals(c_token,",")) 
			c_token++;
		else  
			break;
	}

	if (parametric && crnt_param != 0)
		int_error("parametric function not fully specified", NO_CARET);

	if (parametric) {
	/* Swap u/v and x/y ranges for duration of these eval_plot computations. */
		ltmp = autoscale_lx; autoscale_lx = autoscale_lu; autoscale_lu = ltmp;
		ltmp = autoscale_ly; autoscale_ly = autoscale_lv; autoscale_lv = ltmp;
		temp = xmin; xmin = umin; umin = temp;
		temp = xmax; xmax = umax; umax = temp;
		temp = ymin; ymin = vmin; vmin = temp;
		temp = ymax; ymax = vmax; vmax = temp;
	}
/*** Second Pass: Evaluate the functions ***/
/* Everything is defined now, except the function data. We expect
 * no syntax errors, etc, since the above parsed it all. This makes 
 * the code below simpler. If autoscale_ly, the yrange may still change.
 */
     if (xmin == xmax)
	  if (autoscale_lx) {
		 fprintf(stderr, "Warning: empty x range [%g:%g], ", 
			 xmin,xmax);
		 if (xmin == 0.0) {
			/* completely arbitary */
			xmin = -1.;
			xmax = 1.;
		 } else {
			/* expand range by 10% in either direction */
			xmin = xmin * 0.9;
			xmax = xmax * 1.1;
		 }
		 fprintf(stderr, "adjusting to [%g:%g]\n", xmin,xmax);
	  } else {
		 int_error("x range is empty", c_token);
	  }

     if (ymin == ymax)
	  if (autoscale_ly) {
		 fprintf(stderr, "Warning: empty y range [%g:%g], ", 
			ymin,ymax);
		 if (ymin == 0.0) {
			/* completely arbitary */
			ymin = -1.;
			ymax = 1.;
		 } else {
			/* expand range by 10% in either direction */
			ymin = ymin * 0.9;
			ymax = ymax * 1.1;
		 }
		 fprintf(stderr, "adjusting to [%g:%g]\n", ymin,ymax);
	  } else {
		 int_error("y range is empty", c_token);
	  }

    /* give error if xrange badly set from missing datafile error */
    if (xmin == VERYLARGE || xmax == -VERYLARGE) {
	int_error("x range is invalid", c_token);
    }

    if (log_x) {
	   if (xmin <= 0.0 || xmax <= 0.0)
		int_error("x range must be greater than 0 for log scale!",NO_CARET);
	   x_min = log10(xmin);
	   x_max = log10(xmax);
    } else {
	   x_min = xmin;
	   x_max = xmax;
    }

    if (log_y) {
	   if (ymin <= 0.0 || ymax <= 0.0)
		int_error("y range must be greater than 0 for log scale!",NO_CARET);
	   y_min = log10(ymin);
	   y_max = log10(ymax);
    } else {
	   y_min = ymin;
	   y_max = ymax;
    }

    if (samples < 2 || iso_samples < 2)
	int_error("samples or iso_samples < 2. Must be at least 2.\n");

    xdiff = (x_max - x_min) / (samples - 1);
    ydiff = (y_max - y_min) / (samples - 1);
    xisodiff = (x_max - x_min) / (iso_samples - 1);
    yisodiff = (y_max - y_min) / (iso_samples - 1);

    plot_num = 0;
    this_plot = first_3dplot;
    c_token = begin_token;	/* start over */

    /* Read through functions */
	while (TRUE) {
		if (is_definition(c_token)) {
			define();
		} else {
			plot_num++;
			if (isstring(c_token)) {			/* data file to plot */
			    /* ignore this now */
				c_token++;
				if (almost_equals(c_token,"u$sing")) {
					c_token++;  	/* skip "using" */
                    			if (!isstring(c_token)) {
						struct value a;
						(void)magnitude(const_express(&a));	/* skip xcol */
						if (equals(c_token,":")) {
							c_token++;	/* skip ":" */
							(void)magnitude(const_express(&a));	/* skip ycol */
							if (equals(c_token,":")) {
								c_token++;	/* skip ":" */
								(void)magnitude(const_express(&a));	/* skip zcol */
							}
						}
					}
                    			if (isstring(c_token))
						c_token++;	/* skip format string */
				}
			}
			else {					/* function to plot */
				struct iso_curve *this_iso = this_plot->iso_crvs;
				struct coordinate *points = this_iso->points;
				
				if (parametric)
					crnt_param = (crnt_param+1) % 3;
				dummy_func = &plot_func;
				plot_func.at = temp_at(); /* reparse function */

				for (j = 0; j < iso_samples; j++) {
				    y = y_min + j*yisodiff;
				    /* if (log_y) PEM fix logscale y axis */
				    /* y = pow(10.0,y); 26-Sep-89 */
				    (void) complex(&plot_func.dummy_values[1],
							    log_y ? pow(10.0,y) : y,
							    0.0);

				    for (i = 0; i < samples; i++) {
				        x = x_min + i*xdiff;
				        /* if (log_x) PEM fix logscale x axis */
				        /* x = pow(10.0,x); 26-Sep-89 */
				        (void) complex(&plot_func.dummy_values[0],
							    log_x ? pow(10.0,x) : x,
							    0.0);

				        points[i].x = x;
				        points[i].y = y;

				        evaluate_at(plot_func.at,&a);

				        if (undefined || (fabs(imag(&a)) > zero)) {
					   points[i].type = UNDEFINED;
					   continue;
				        }

				        temp = real(&a);

				        if (log_z && temp < 0.0) {
					   points[i].type = UNDEFINED;
					   continue;
				        }

				        if (log_z) {
					   if (temp == 0.0) {
						  points[i].type = OUTRANGE;
						  points[i].z = -VERYLARGE;
						  continue;
					   } else {
						  points[i].z = log10(temp);
					   }
				        } else
					   points[i].z = temp;

				        if (autoscale_lz
					   || inrange(temp, zmin, zmax)) {
					   points[i].type = INRANGE;
					   if (autoscale_lz) {
					      if (temp < zmin) zmin = temp;
					      if (temp > zmax) zmax = temp;
					   }
				        } else
					   points[i].type = OUTRANGE;
				    }
				    this_iso->p_count = samples;
				    this_iso = this_iso->next;
				    points = this_iso->points;
				}

				for (i = 0; i < iso_samples; i++) {
				    x = x_min + i*xisodiff;
				    /* if (log_x) PEM fix logscale x axis */
				    /* x = pow(10.0,x); 26-Sep-89 */
				    (void) complex(&plot_func.dummy_values[0],
							    log_x ? pow(10.0,x) : x,
							    0.0);

				    for (j = 0; j < samples; j++) {
				        y = y_min + j*ydiff;
				        /* if (log_y) PEM fix logscale y axis */
				        /* y = pow(10.0,y); 26-Sep-89 */
				        (void) complex(&plot_func.dummy_values[1],
							    log_y ? pow(10.0,y) : y,
							    0.0);

				        points[j].x = x;
				        points[j].y = y;

				        evaluate_at(plot_func.at,&a);

				        if (undefined || (fabs(imag(&a)) > zero)) {
					   points[j].type = UNDEFINED;
					   continue;
				        }

				        temp = real(&a);

				        if (log_z && temp < 0.0) {
					   points[j].type = UNDEFINED;
					   continue;
				        }

				        if (log_z) {
					   if (temp == 0.0) {
						  points[j].type = OUTRANGE;
						  points[j].z = -VERYLARGE;
						  continue;
					   } else {
						  points[j].z = log10(temp);
					   }
				        } else
					   points[j].z = temp;

				        if (autoscale_lz
					   || inrange(temp, zmin, zmax)) {
					   points[j].type = INRANGE;
					   if (autoscale_lz) {
					      if (temp < zmin) zmin = temp;
					      if (temp > zmax) zmax = temp;
					   }
				        } else
					   points[j].type = OUTRANGE;
				    }
    				    this_iso->p_count = samples;
				    this_iso = this_iso->next;
				    points = this_iso ? this_iso->points : NULL;
				}
			 }

			/* title was handled above */
			if (almost_equals(c_token,"t$itle")) {
				c_token++;
				c_token++;
			}

			/* style was handled above */
			if (almost_equals(c_token,"w$ith")) {
			    c_token++;
			    c_token++;
			}

			/* line and point types were handled above */
			if ( !equals(c_token,",") && !END_OF_COMMAND ) {
				struct value t;
				(void)real(const_express(&t));
			}
			if ( !equals(c_token,",") && !END_OF_COMMAND ) {
				struct value t;
				(void)real(const_express(&t));
			}

			this_plot = this_plot->next_sp;
		 }

		if (equals(c_token,","))
		  c_token++;
		else
		  break;
	 }

    if (fabs(zmax - zmin) < zero)
	 /* if autoscale, widen range */
	 if (autoscale_lz) {
		fprintf(stderr, "Warning: empty z range [%g:%g], ", zmin, zmax);
		if (fabs(zmin) < zero ) {
		    zmin = -1.;
		    zmax = 1.;
		} else {
		    /* expand range by 10% in either direction */
		    zmin = zmin * 0.9;
		    zmax = zmax * 1.1;
		}
		fprintf(stderr, "adjusting to [%g:%g]\n", zmin, zmax);
	 } else {
		int_error("z range is less than `zero`", c_token);
	 }

/* Now we finally know the real zmin and zmax */
	if (log_z) {
		if (zmin <= 0.0 || zmax <= 0.0)
			int_error("z range must be greater than 0 for log scale!",NO_CARET);
		z_min = log10(zmin);
		z_max = log10(zmax);
	} else {
		z_min = zmin;
		z_max = zmax;
	}
	capture(replot_line,plot_token,c_token);

	if (parametric) {
	/* Now put u/v and x/y ranges back before we actually plot anything. */
		ltmp = autoscale_lx; autoscale_lx = autoscale_lu; autoscale_lu = ltmp;
		ltmp = autoscale_ly; autoscale_ly = autoscale_lv; autoscale_lv = ltmp;
		temp = xmin; xmin = umin; umin = temp;
		temp = xmax; xmax = umax; umax = temp;
		temp = ymin; ymin = vmin; vmin = temp;
		temp = ymax; ymax = vmax; vmax = temp;

	/* Now actually fix the plot triplets to be single plots. */
		parametric_3dfixup(first_3dplot, &plot_num,
				   &x_min, &x_max, &y_min, &y_max,
				   &z_min, &z_max);
		if (log_x) {
			if (x_min <= 0.0 || x_max <= 0.0)
				int_error("x range must be greater than 0 for log scale!",NO_CARET);
			x_min = log10(x_min);
			x_max = log10(x_max);
		}

		if (log_y) {
			if (y_min <= 0.0 || y_max <= 0.0)
				int_error("y range must be greater than 0 for log scale!",NO_CARET);
			y_min = log10(y_min);
			y_max = log10(y_max);
		}

		if (log_z) {
			if (z_min <= 0.0 || z_max <= 0.0)
				int_error("z range must be greater than 0 for log scale!",NO_CARET);
			z_min = log10(z_min);
			z_max = log10(z_max);
		}
	}


	/* Creates contours if contours are to be plotted as well. */
	if (draw_contour) {
		for (this_plot=first_3dplot, i=0;
		     i < plot_num;
		     this_plot=this_plot->next_sp, i++) {
			if (this_plot->contours) {
				struct gnuplot_contours *cntr, *cntrs = this_plot->contours;

				while (cntrs) {
					cntr = cntrs;
					cntrs = cntrs->next;
					free(cntr->coords);
					free(cntr);
				}
			}
			/* Make sure this one can be contoured. */
			if (!this_plot->has_grid_topology) {
			    this_plot->contours = NULL;
			    int_error("Can not contour non grid data!",NO_CARET);
			}
			if (this_plot->plot_type == DATA3D)
			    this_plot->contours = contour(
					       this_plot->num_iso_read,
					       this_plot->iso_crvs,
					       contour_levels, contour_pts,
					       contour_kind, contour_order);
			else
			    this_plot->contours = contour(iso_samples,
					       this_plot->iso_crvs,
					       contour_levels, contour_pts,
					       contour_kind, contour_order);
		}
	}

	do_3dplot(first_3dplot,plot_num,x_min,x_max,y_min,y_max,z_min,z_max);
	sp_free(first_3dplot);
	first_3dplot = NULL;
}

done(status)
int status;
{
	if (term && term_init)
		(*term_tbl[term].reset)();
#ifdef vms
	vms_reset();
#endif
	exit(status);
}

void parametric_fixup (start_plot, plot_num, x_min, x_max)
struct curve_points *start_plot;
int *plot_num;
double *x_min, *x_max;
/*
	The hardest part of this routine is collapsing the FUNC plot types
   in the list (which are gauranteed to occur in (x,y) pairs while 
	preserving the non-FUNC type plots intact.  This means we have to
	work our way through various lists.  Examples (hand checked):
		start_plot:F1->F2->NULL ==> F2->NULL
		start_plot:F1->F2->F3->F4->F5->F6->NULL ==> F2->F4->F6->NULL
		start_plot:F1->F2->D1->D2->F3->F4->D3->NULL ==> F2->D1->D2->F4->D3->NULL
	
	Of course, the more interesting work is to move the y values of
	the x function to become the x values of the y function (checking
	the mins and maxs as we go along).
*/
{
	struct curve_points *xp, *new_list, *yp = start_plot, *tmp, 
			*free_list, *free_head=NULL;
	int i, tlen, curve; 
	char *new_title;
	double lxmin, lxmax, temp;

	if (autoscale_lx) {
		lxmin = VERYLARGE;
		lxmax = -VERYLARGE;
	} else {
		lxmin = xmin;
		lxmax = xmax;
	}

/* 
	Ok, go through all the plots and move FUNC types together.  Note: this
	originally was written to look for a NULL next pointer, but gnuplot 
	wants to be sticky in grabbing memory and the right number of items
	in the plot list is controlled by the plot_num variable.

	Since gnuplot wants to do this sticky business, a free_list of 
	curve_points is kept and then tagged onto the end of the plot list as
	this seems more in the spirit of the original memory behavior than
	simply freeing the memory.  I'm personally not convinced this sort
	of concern is worth it since the time spent computing points seems
	to dominate any garbage collecting that might be saved here...
*/
	new_list = xp = start_plot; 
	yp = xp->next_cp;
   curve = 0;
	for (; curve < *plot_num; xp = xp->next_cp,yp = yp->next_cp,curve++) {
		if (xp->plot_type != FUNC) {
			continue;
		}
	/* Here's a FUNC parametric function defined as two parts. */
		--(*plot_num);
	/* 
		Go through all the points assigning the y's from xp to be the
		x's for yp.  Check max's and min's as you go.
	*/
		for (i = 0; i < yp->p_count; ++i) {
		/* 
			Throw away excess xp points, mark excess yp points as OUTRANGE.
		*/
			if (i > xp->p_count) {
				yp->points[i].type = OUTRANGE;
				continue;
			}
		/* 
			Just as we had to do when we computed y values--now check that
			x's (computed parametrically) are in the permitted ranges as well.
		*/
			temp = xp->points[i].y;   /* New x value for yp function. */
			yp->points[i].x = temp;
		/* Handle undefined values differently from normal ranges. */
			if (xp->points[i].type == UNDEFINED)
				yp->points[i].type = xp->points[i].type;  
			if (autoscale_lx || polar
					   || inrange(temp, lxmin, lxmax)) {
			   if (autoscale_lx && temp < lxmin) lxmin = temp;
				if (autoscale_lx && temp > lxmax) lxmax = temp;
			} else
			yp->points[i].type = OUTRANGE;  /* Due to x value. */
		}
   /* Ok, fix up the title to include both the xp and yp plots. */
		if (xp->title && xp->title[0] != '\0') {
			tlen = strlen (yp->title) + strlen (xp->title) + 3;
      	new_title = alloc ((unsigned int) tlen, "string");
			strcpy (new_title, xp->title);  
			strcat (new_title, ", ");       /* + 2 */
			strcat (new_title, yp->title);  /* + 1 = + 3 */
			free (yp->title);
			yp->title = new_title;
		}
	/* Eliminate the first curve (xparam) and just use the second. */
		if (xp == start_plot) {
		/* Simply nip off the first element of the list. */
			new_list = first_plot = yp;
			xp = xp->next_cp;
			if (yp->next_cp != NULL)
				yp = yp->next_cp;
		/* Add start_plot to the free_list. */
			if (free_head == NULL) {
				free_list = free_head = start_plot;
				free_head->next_cp = NULL;
			} else {
				free_list->next_cp = start_plot;
				start_plot->next_cp = NULL;
				free_list = start_plot;
			}
		}
		else {
		/* Here, remove the xp node and replace it with the yp node. */
			tmp = xp;
		/* Pass over any data files that might have been in place. */
			while (new_list->next_cp && new_list->next_cp != xp) 
				new_list = new_list->next_cp;
			new_list->next_cp = yp;
			new_list = new_list->next_cp;
			xp = xp->next_cp;
			if (yp->next_cp != NULL)
				yp = yp->next_cp;
		/* Add tmp to the free_list. */
			tmp->next_cp = NULL;
			if (free_head == NULL) {
				free_list = free_head = tmp;
			} else {
				free_list->next_cp = tmp;
				free_list = tmp;
			}
		}
	}
/* Ok, stick the free list at the end of the curve_points plot list. */
	while (new_list->next_cp != NULL)
		new_list = new_list->next_cp;
	new_list->next_cp = free_head;

/* Report the overall graph mins and maxs. */
	*x_min = lxmin;
	*x_max = lxmax;
}

void parametric_3dfixup(start_plot, plot_num, x_min, x_max, y_min, y_max,
							    z_min, z_max)
struct surface_points *start_plot;
int *plot_num;
double *x_min, *x_max, *y_min, *y_max, *z_min, *z_max;
/*
	The hardest part of this routine is collapsing the FUNC plot types
   in the list (which are gauranteed to occur in (x,y,z) triplets while 
	preserving the non-FUNC type plots intact.  This means we have to
	work our way through various lists.  Examples (hand checked):
		start_plot:F1->F2->F3->NULL ==> F3->NULL
		start_plot:F1->F2->F3->F4->F5->F6->NULL ==> F3->F6->NULL
		start_plot:F1->F2->F3->D1->D2->F4->F5->F6->D3->NULL ==>
						F3->D1->D2->F6->D3->NULL
*/
{
	struct surface_points *xp, *yp, *zp, *new_list, *tmp, 
			*free_list, *free_head=NULL;
	struct iso_curve *icrvs, *xicrvs, *yicrvs, *zicrvs;
	int i, tlen, surface;
	char *new_title;
	double lxmin, lxmax, lymin, lymax, lzmin, lzmax, temp;

	if (autoscale_lx) {
		lxmin = VERYLARGE;
		lxmax = -VERYLARGE;
	} else {
		lxmin = xmin;
		lxmax = xmax;
	}

	if (autoscale_ly) {
		lymin = VERYLARGE;
		lymax = -VERYLARGE;
	} else {
		lymin = ymin;
		lymax = ymax;
	}

	if (autoscale_lz) {
		lzmin = VERYLARGE;
		lzmax = -VERYLARGE;
	} else {
		lzmin = zmin;
		lzmax = zmax;
	}

/* 
	Ok, go through all the plots and move FUNC3D types together.  Note:
	this originally was written to look for a NULL next pointer, but
	gnuplot wants to be sticky in grabbing memory and the right number
	of items in the plot list is controlled by the plot_num variable.

	Since gnuplot wants to do this sticky business, a free_list of 
	surface_points is kept and then tagged onto the end of the plot list as
	this seems more in the spirit of the original memory behavior than
	simply freeing the memory.  I'm personally not convinced this sort
	of concern is worth it since the time spent computing points seems
	to dominate any garbage collecting that might be saved here...
*/
	new_list = xp = start_plot; 
	for (surface = 0; surface < *plot_num; surface++) {
		if (xp->plot_type != FUNC3D) {
			icrvs = xp->iso_crvs;

			while ( icrvs ) {
				struct coordinate *points = icrvs->points;

				for (i = 0; i < icrvs->p_count; ++i) {
					if (lxmin > points[i].x)
						lxmin = points[i].x;
					if (lxmax < points[i].x)
						lxmax = points[i].x;
					if (lymin > points[i].y)
						lymin = points[i].y;
					if (lymax < points[i].y)
						lymax = points[i].y;
					if (lzmin > points[i].z)
						lzmin = points[i].z;
					if (lzmax < points[i].z)
						lzmax = points[i].z;
				}

				icrvs = icrvs->next;
			}
			xp = xp->next_sp;
			continue;
		}

		yp = xp->next_sp;
		zp = yp->next_sp;

	/* Here's a FUNC3D parametric function defined as three parts. */
		(*plot_num) -= 2;
	/* 
		Go through all the points and assign the x's and y's from xp
		and yp to zp.  Check max's and min's as you go.
	*/
		xicrvs = xp->iso_crvs;
		yicrvs = yp->iso_crvs;
		zicrvs = zp->iso_crvs;
		while ( zicrvs ) {
			struct coordinate *xpoints = xicrvs->points,
				     *ypoints = yicrvs->points,
				     *zpoints = zicrvs->points;
			for (i = 0; i < zicrvs->p_count; ++i) {
				zpoints[i].x = xpoints[i].z;
				zpoints[i].y = ypoints[i].z;

				if (lxmin > zpoints[i].x) lxmin = zpoints[i].x;
				if (lxmax < zpoints[i].x) lxmax = zpoints[i].x;
				if (lymin > zpoints[i].y) lymin = zpoints[i].y;
				if (lymax < zpoints[i].y) lymax = zpoints[i].y;
				if (lzmin > zpoints[i].z) lzmin = zpoints[i].z;
				if (lzmax < zpoints[i].z) lzmax = zpoints[i].z;
			}
			xicrvs = xicrvs->next;
			yicrvs = yicrvs->next;
			zicrvs = zicrvs->next;
		}

	/* Ok, fix up the title to include xp and yp plots. */
		if ((xp->title && xp->title[0] != '\0') ||
		    (yp->title && yp->title[0] != '\0')) {
			tlen = (xp->title ? strlen(xp->title) : 0) +
			       (yp->title ? strlen(yp->title) : 0) +
			       (zp->title ? strlen(zp->title) : 0) + 5;
			new_title = alloc ((unsigned int) tlen, "string");
			new_title[0] = 0;
			if (xp->title) {
				strcat(new_title, xp->title);
				strcat(new_title, ", ");       /* + 2 */
			}
			if (yp->title) {
				strcat(new_title, yp->title);
				strcat(new_title, ", ");       /* + 2 */
			}
			if (zp->title) {
				strcat(new_title, zp->title);
			}
			free (zp->title);
			zp->title = new_title;
		}

	/* Eliminate the first two surfaces (xp and yp) and just use the third. */
		if (xp == start_plot) {
		/* Simply nip off the first two elements of the list. */
			new_list = first_3dplot = zp;
			xp = zp->next_sp;
		/* Add xp and yp to the free_list. */
			if (free_head == NULL) {
				free_head = start_plot;
			} else {
				free_list->next_sp = start_plot;
			}
			free_list = start_plot->next_sp;
			free_list->next_sp = NULL;
		}
		else {
		/* Here, remove the xp,yp nodes and replace them with the zp node. */
			tmp = xp;
		/* Pass over any data files that might have been in place. */
			while (new_list->next_sp && new_list->next_sp != xp)
				new_list = new_list->next_sp;
			new_list->next_sp = zp;
			new_list = zp;
			xp = zp->next_sp;
		/* Add tmp to the free_list. */
			if (free_head == NULL) {
				free_head = tmp;
			} else {
				free_list->next_sp = tmp;
			}
			free_list = tmp->next_sp;
			free_list->next_sp = NULL;
		}
	}
/* Ok, stick the free list at the end of the surface_points plot list. */
	while (new_list->next_sp != NULL)
		new_list = new_list->next_sp;
	new_list->next_sp = free_head;

/* Report the overall graph mins and maxs. */
	if (autoscale_lx) {
		*x_min = (log_x ? pow(10.0, lxmin) : lxmin);
		*x_max = (log_x ? pow(10.0, lxmax) : lxmax);
	}
	else {
		*x_min = xmin;
		*x_max = xmax;
	}
	if (autoscale_ly) {
		*y_min = (log_y ? pow(10.0, lymin) : lymin);
		*y_max = (log_y ? pow(10.0, lymax) : lymax);
	}
	else {
		*y_min = ymin;
		*y_max = ymax;
	}
	if (autoscale_lz) {
		*z_min = (log_z ? pow(10.0, lzmin) : lzmin);
		*z_max = (log_z ? pow(10.0, lzmax) : lzmax);
	}
	else {
		*z_min = zmin;
		*z_max = zmax;
	}
}

#ifdef AMIGA_LC_5_1
void sleep(delay)
unsigned int delay;
{
  Delay(50 * delay);
}
#endif

#ifdef AMIGA_AC_5
void sleep(delay)
unsigned int delay;
{
unsigned long time_is_up;
	time_is_up = time(NULL) + (unsigned long) delay; 
	while (time(NULL)<time_is_up)
		/* wait */ ;
}
#endif

#ifdef MSDOS
#ifndef __TURBOC__	/* Turbo C already has sleep() */
#ifndef __ZTC__ 	/* ZTC already has usleep() */
/* kludge to provide sleep() for msc 5.1 */
void sleep(delay)
unsigned int delay;
{
unsigned long time_is_up;
	time_is_up = time(NULL) + (unsigned long) delay; 
	while (time(NULL)<time_is_up)
		/* wait */ ;
}
#endif /* not ZTC */
#endif /* not TURBOC */
#endif /* MSDOS */


/* Support for input, shell, and help for various systems */

#ifdef vms

#include <descrip.h>
#include <rmsdef.h>
#include <errno.h>
#include <smgdef.h>
#include <smgmsg.h>

extern lib$get_input(), lib$put_output();
extern smg$read_composed_line();

int vms_len;

unsigned int status[2] = {1, 0};

static char help[MAX_LINE_LEN+1] = "gnuplot";

$DESCRIPTOR(prompt_desc,PROMPT);
$DESCRIPTOR(line_desc,input_line);

$DESCRIPTOR(help_desc,help);
$DESCRIPTOR(helpfile_desc,"GNUPLOT$HELP");


read_line(prompt)
char *prompt;
{
    int more, start=0;
    char expand_prompt[40];

    prompt_desc.dsc$w_length = strlen (prompt);
    prompt_desc.dsc$a_pointer = prompt;
    (void) strcpy (expand_prompt, "_");
    (void) strncat (expand_prompt, prompt, 38);
    do {
        line_desc.dsc$w_length = MAX_LINE_LEN - start;
        line_desc.dsc$a_pointer = &input_line[start];
        switch(status[1] = smg$read_composed_line(&vms_vkid,0,&line_desc, &prompt_desc, &vms_len)){
		  case SMG$_EOF:
		  done(IO_SUCCESS);	/* ^Z isn't really an error */
		  break;
		  case RMS$_TNS:	/* didn't press return in time *
						   /
						   vms_len--; /* skip the last character */
		  break;			/* and parse anyway */
		  case RMS$_BES:	/* Bad Escape Sequence */
		  case RMS$_PES:	/* Partial Escape Sequence */
		  sys$putmsg(status);
		  vms_len = 0;		/* ignore the line */
		  break;
		  case SS$_NORMAL:
		  break;			/* everything's fine */
		  default:
		  done(status[1]);	/* give the error message */
        }
        start += vms_len;
        input_line[start] = '\0';
	   inline_num++;
        if (input_line[start-1] == '\\') {
		  /* Allow for a continuation line. */
		  prompt_desc.dsc$w_length = strlen (expand_prompt);
		  prompt_desc.dsc$a_pointer = expand_prompt;
		  more = 1;
		  --start;
        }
        else {
		  line_desc.dsc$w_length = strlen(input_line);
		  line_desc.dsc$a_pointer = input_line;
		  more = 0;
        }
    } while (more);
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

/* do_help: (not VMS, although it would work)
 * Give help to the user. 
 * It parses the command line into helpbuf and supplies help for that 
 * string. Then, if there are subtopics available for that key,
 * it prompts the user with this string. If more input is
 * given, do_help is called recursively, with the argument the index of 
 * null character in the string. Thus a more specific help can be 
 * supplied. This can be done repeatedly. 
 * If null input is given, the function returns, effecting a
 * backward climb up the tree.
 * David Kotz (David.Kotz@Dartmouth.edu) 10/89
 */
do_help()
{
    static char *helpbuf = NULL;
    static char *prompt = NULL;
    int base;				/* index of first char AFTER help string */
    int len;				/* length of current help string */
    BOOLEAN more_help;
    BOOLEAN only;			/* TRUE if only printing subtopics */
    int subtopics;			/* 0 if no subtopics for this topic */
    int start;				/* starting token of help string */
	char *help_ptr;			/* name of help file */

	if ( (help_ptr = getenv("GNUHELP")) == (char *)NULL )
		/* if can't find environment variable then just use HELPFILE */
		help_ptr = HELPFILE;

    /* Since MSDOS DGROUP segment is being overflowed we can not allow such  */
    /* huge static variables (1k each). Instead we dynamically allocate them */
    /* on the first call to this function...				     */
    if (helpbuf == NULL) {
	helpbuf = alloc(MAX_LINE_LEN, "help buffer");
	prompt = alloc(MAX_LINE_LEN, "help prompt");
	helpbuf[0] = prompt[0] = 0;
    }

    len = base = strlen(helpbuf);

    /* find the end of the help command */
    for (start = c_token; !(END_OF_COMMAND); c_token++)
	 ;
    /* copy new help input into helpbuf */
    if (len > 0)
	 helpbuf[len++] = ' ';	/* add a space */
    capture(helpbuf+len, start, c_token-1);
    squash_spaces(helpbuf+base); /* only bother with new stuff */
    lower_case(helpbuf+base); /* only bother with new stuff */
    len = strlen(helpbuf);

    /* now, a lone ? will print subtopics only */
    if (strcmp(helpbuf + (base ? base+1 : 0), "?") == 0) {
	   /* subtopics only */
	   subtopics = 1;
	   only = TRUE;
	   helpbuf[base] = '\0';	/* cut off question mark */
    } else {
	   /* normal help request */
	   subtopics = 0;
	   only = FALSE;
    }

    switch (help(helpbuf, help_ptr, &subtopics)) {
	   case H_FOUND: {
		  /* already printed the help info */
		  /* subtopics now is true if there were any subtopics */
		  screen_ok = FALSE;
    
		  do {
			 if (subtopics && !only) {
				/* prompt for subtopic with current help string */
				if (len > 0)
				  (void) sprintf(prompt, "Subtopic of %s: ", helpbuf);
				else
				  (void) strcpy(prompt, "Help topic: ");
				read_line(prompt);
				num_tokens = scanner(input_line);
				c_token = 0;
				more_help = !(END_OF_COMMAND);
				if (more_help)
				  /* base for next level is all of current helpbuf */
				  do_help();
			 } else 
			   more_help = FALSE;
		  } while(more_help);
    
		  break;
	   }
	   case H_NOTFOUND: {
		  printf("Sorry, no help for '%s'\n", helpbuf);
		  break;
	   }
	   case H_ERROR: {
		  perror(help_ptr);
		  break;
	   }
	   default: {		/* defensive programming */
		  int_error("Impossible case in switch\n", NO_CARET);
		  /* NOTREACHED */
	   }
    }
    
    helpbuf[base] = '\0';	/* cut it off where we started */
}

#ifdef AMIGA_AC_5
char strg0[256];
#endif

do_system()
{
#ifdef AMIGA_AC_5
   char *parms[80];
   void getparms();

   getparms(input_line+1,parms);
   if(fexecv(parms[0],parms) < 0)
#else
   if (system(input_line + 1))
#endif
      os_error("system() failed",NO_CARET);
}

#ifdef AMIGA_AC_5

/******************************************************************************/
/*                                                                            */
/*  Parses the command string (for fexecv use) and  converts the first token  */
/*     to lower case                                                          */
/*                                                                            */
/******************************************************************************/

void getparms(command,parms)
   char *command;
   char **parms;
   {
   register int i = 0;                         /* A bunch of indices          */
   register int j = 0;
   register int k = 0;

   while(*(command+j) != '\0')                 /* Loop on string characters   */
      {
      parms[k++] = strg0+i;
      while(*(command+j) == ' ') ++j;
      while(*(command+j) != ' ' && *(command+j) != '\0')
         {
         if(*(command+j) == '"')               /* Get quoted string           */
            for(*(strg0+(i++)) = *(command+(j++));
                *(command+j)  != '"';
                *(strg0+(i++)) = *(command+(j++)));
         *(strg0+(i++)) = *(command+(j++));
         }
      *(strg0+(i++)) = '\0';                   /* NUL terminate every token   */
      }
   parms[k] = '\0';

   for(k=strlen(strg0)-1; k>=0; --k)           /* Convert to lower case       */
      *(strg0+k)>='A' && *(strg0+k)<='Z'? *(strg0+k)|=32: *(strg0+k);
   }

#endif /* AMIGA_AC_5 */

#ifdef READLINE
char *
rlgets(s, n, prompt)
char *s;
int n;
char *prompt;
{
      char *readline();
      static char *line = (char *)NULL;

      /* If we already have a line, first free it */
      if(line != (char *)NULL) 
              free(line);

      line = readline((interactive)?prompt:"");

      /* If it's not an EOF */
      if(line) {
	  if (*line)
              add_history(line);
	  strncpy(s, line, n);
	  return s;
      }

      return line;
}
#endif /* READLINE */

#ifdef MSDOS

#ifdef __TURBOC__
/* cgets implemented using dos functions */
/* Maurice Castro 22/5/91 */
char *doscgets(s)
char *s;
{
   long datseg;

   /* protect and preserve segments - call dos to do the dirty work */
   datseg = _DS;

   _DX = FP_OFF(s);
   _DS = FP_SEG(s);
   _AH = 0x0A;
   geninterrupt(33);
   _DS = datseg;

   /* check for a carriage return and then clobber it with a null */
   if (s[s[1]+2] == '\r') 
      s[s[1]+2] = 0;

   /* return the input string */
   return(&(s[2]));
   }
#endif /* __TURBOC__ */


read_line(prompt)
	char *prompt;
{
    register int i;
    int start = 0, ilen = 0;
    BOOLEAN more;
    int last;
    char *p, *crnt_prompt = prompt;
    
#ifndef __ZTC__
	if (interactive) { /* if interactive use console IO so CED will work */
#ifndef READLINE
		cputs(prompt);
#endif /* READLINE */
		do {
		   ilen = MAX_LINE_LEN-start-1;
		   input_line[start] = ilen > 126 ? 126 : ilen;
#ifdef READLINE
		   input_line[start+2] = 0;
		   (void) rlgets(&(input_line[start+2]), ilen, crnt_prompt );
		   if (p = strchr(&(input_line[start+2]), '\r')) *p = 0;
		   if (p = strchr(&(input_line[start+2]), '\n')) *p = 0;
		   input_line[start+1] = strlen(&(input_line[start+2]));
#else /* READLINE */
#ifdef __TURBOC__
		   (void) doscgets(&(input_line[start]));
#else /* __TURBOC__ */
		   (void) cgets(&(input_line[start]));
#endif /* __TURBOC__ */
		   (void) putc('\n',stderr);
#endif /* READLINE */
		   if (input_line[start+2] == 26) {
			  /* end-of-file */
			  (void) putc('\n',stderr);
			  input_line[start] = '\0';
			  inline_num++;
			  if (start > 0)	/* don't quit yet - process what we have */
				more = FALSE;
			  else {
				 (void) putc('\n',stderr);
				 done(IO_SUCCESS);
				 /* NOTREACHED */
			  }
		   } else {
			  /* normal line input */
			  register i = start;
			  while ( (input_line[i] = input_line[i+2]) != (char)NULL )
				i++;		/* yuck!  move everything down two characters */

			  inline_num++;
			  last = strlen(input_line) - 1;
			  if (last + 1 >= MAX_LINE_LEN)
				int_error("Input line too long",NO_CARET);
					 
			  if (input_line[last] == '\\') { /* line continuation */
				 start = last;
				 more = TRUE;
			  } else
				more = FALSE;
		   }
#ifndef READLINE
		   if (more)
			cputs("> ");
#else
		   crnt_prompt = "> ";
#endif /* READLINE */
		} while(more);
	}
	else { /* not interactive */
#endif /* not ZTC */
		if (interactive)
		 fputs(prompt,stderr);
		do {
		   /* grab some input */
		   if ( fgets(&(input_line[start]), MAX_LINE_LEN - start, stdin) 
					== (char *)NULL ) {
			  /* end-of-file */
			  if (interactive)
				(void) putc('\n',stderr);
			  input_line[start] = '\0';
			  inline_num++;
			  if (start > 0)	/* don't quit yet - process what we have */
				more = FALSE;
			  else
				done(IO_SUCCESS); /* no return */
		   } else {
			  /* normal line input */
			  last = strlen(input_line) - 1;
			  if (input_line[last] == '\n') { /* remove any newline */
				 input_line[last] = '\0';
				 /* Watch out that we don't backup beyond 0 (1-1-1) */
				 if (last > 0) --last;
				 inline_num++;
			  } else if (last+1 >= MAX_LINE_LEN)
				int_error("Input line too long",NO_CARET);
					 
			  if (input_line[last] == '\\') { /* line continuation */
				 start = last;
				 more = TRUE;
			  } else
				more = FALSE;
		   }
			if (more && interactive)
			fputs("> ", stderr);
		} while(more);
#ifndef __ZTC
	}
#endif
}


do_shell()
{
register char *comspec;
	if ((comspec = getenv("COMSPEC")) == (char *)NULL)
		comspec = "\command.com";
	if (spawnl(P_WAIT,comspec,NULL) == -1)
		os_error("unable to spawn shell",NO_CARET);
}

#else /* MSDOS */
		/* plain old Unix */

read_line(prompt)
	char *prompt;
{
    int start = 0;
    BOOLEAN more = FALSE;
    int last = 0;

#ifndef READLINE
    if (interactive)
	 fputs(prompt,stderr);
#endif /* READLINE */
    do {
	   /* grab some input */
#ifdef READLINE
	 if (((interactive)
		 ?rlgets(&(input_line[start]), MAX_LINE_LEN - start,
				((more)?"> ":prompt))
		 :fgets(&(input_line[start]), MAX_LINE_LEN - start, stdin))
                              == (char *)NULL ) {
#else
	   if ( fgets(&(input_line[start]), MAX_LINE_LEN - start, stdin) 
				== (char *)NULL ) {
#endif /* READLINE */
		  /* end-of-file */
		  if (interactive)
		    (void) putc('\n',stderr);
		  input_line[start] = '\0';
		  inline_num++;
		  if (start > 0)	/* don't quit yet - process what we have */
		    more = FALSE;
		  else
		    done(IO_SUCCESS); /* no return */
	   } else {
		  /* normal line input */
		  last = strlen(input_line) - 1;
		  if (input_line[last] == '\n') { /* remove any newline */
			 input_line[last] = '\0';
                /* Watch out that we don't backup beyond 0 (1-1-1) */
			 if (last > 0) --last;
			 inline_num++;
		  } else if (last+1 >= MAX_LINE_LEN)
		    int_error("Input line too long",NO_CARET);
				 
		  if (input_line[last] == '\\') { /* line continuation */
			 start = last;
			 more = TRUE;
		  } else
		    more = FALSE;
	   }
#ifndef READLINE
        if (more && interactive)
		fputs("> ", stderr);
#endif
    } while(more);
}

#ifdef VFORK

do_shell()
{
register char *shell;
register int p;
static int execstat;
	if (!(shell = getenv("SHELL")))
		shell = SHELL;
#ifdef AMIGA_AC_5
	execstat = fexecl(shell,shell,NULL);
#else
	if ((p = vfork()) == 0) {
		execstat = execl(shell,shell,NULL);
		_exit(1);
	} else if (p == -1)
		os_error("vfork failed",c_token);
	else
		while (wait(NULL) != p)
#endif
			;
	if (execstat == -1)
		os_error("shell exec failed",c_token);
	(void) putc('\n',stderr);
}
#else /* VFORK */

#ifdef AMIGA_LC_5_1
do_shell()
{
register char *shell;
	if (!(shell = getenv("SHELL")))
		shell = SHELL;

	if (system(shell))
		os_error("system() failed",NO_CARET);

	(void) putc('\n',stderr);
}
#else /* AMIGA_LC_5_1 */

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
#endif /* AMIGA_LC_5_1 */
#endif /* VFORK */
#endif /* MSDOS */
#endif /* vms */
