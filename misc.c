/*
 *
 *    G N U P L O T  --  misc.c
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
#include "plot.h"
#ifdef __TURBOC__
#include <graphics.h>
#endif

extern BOOLEAN autoscale;
extern BOOLEAN polar;
extern BOOLEAN log_x, log_y;
extern FILE* outfile;
extern char outstr[];
extern int samples;
extern int term;
extern double zero;
extern double roff, loff, toff, boff;

extern BOOLEAN screen_ok;

extern int c_token;
extern struct at_type at;
extern struct ft_entry ft[];
extern struct udft_entry *first_udf;
extern struct udvt_entry *first_udv;
extern struct termentry term_tbl[];

char *malloc();

struct at_type *temp_at();


/*
 * cp_free() releases any memory which was previously malloc()'d to hold
 *   curve points.
 */
cp_free(cp)
struct curve_points *cp;
{
	if (cp) {
		cp_free(cp->next_cp);
		if (cp->title)
			free((char *)cp->title);
		free((char *)cp);
	}
}



save_functions(fp)
FILE *fp;
{
register struct udft_entry *udf = first_udf;
	
	if (fp) {
		while (udf) {
			if (udf->definition)
				fprintf(fp,"%s\n",udf->definition);
			udf = udf->next_udf;
		}
		(void) fclose(fp);
	} else
		os_error("Cannot open save file",c_token);			
}


save_variables(fp)
FILE *fp;
{
register struct udvt_entry *udv = first_udv->next_udv;	/* skip pi */

	if (fp) {
		while (udv) {
			if (!udv->udv_undef) {
				fprintf(fp,"%s = ",udv->udv_name);
				disp_value(fp,&(udv->udv_value));
				(void) putc('\n',fp);
			}
			udv = udv->next_udv;
		}
		(void) fclose(fp);
	} else
		os_error("Cannot open save file",c_token);			
}


save_all(fp)
FILE *fp;
{
register struct udft_entry *udf = first_udf;
register struct udvt_entry *udv = first_udv->next_udv;	/* skip pi */

	if (fp) {
		while (udf) {
			if (udf->definition)
				fprintf(fp,"%s\n",udf->definition);
			udf = udf->next_udf;
		}
		while (udv) {
			if (!udv->udv_undef) {
				fprintf(fp,"%s = ",udv->udv_name);
				disp_value(fp,&(udv->udv_value));
				(void) putc('\n',fp);
			}
			udv = udv->next_udv;
		}
		(void) fclose(fp);
	} else
		os_error("Cannot open save file",c_token);			
}


load_file(fp)
FILE *fp;
{
register int len;
extern char input_line[];

	if (fp) {
		while (fgets(input_line,MAX_LINE_LEN,fp)) {
			len = strlen(input_line) - 1;
			if (input_line[len] == '\n')
				input_line[len] = '\0';

			screen_ok = FALSE;	/* make sure command line is
					   echoed on error */
			do_line();
		}
		(void) fclose(fp);
	} else
		os_error("Cannot open load file",c_token);
}


show_style(name,style)
char name[];
enum PLOT_STYLE style;
{
	fprintf(stderr,"\t%s are plotted with ",name);
	switch (style) {
		case LINES: fprintf(stderr,"lines\n"); break;
		case POINTS: fprintf(stderr,"points\n"); break;
		case IMPULSES: fprintf(stderr,"impulses\n"); break;
	}
}

show_range(name,min,max)
char name;
double min,max;
{
	fprintf(stderr,"\t%crange is [%g : %g]\n",name,min,max);
}

show_zero()
{
	fprintf(stderr,"\tzero is %g\n",zero);
}

show_offsets()
{
	fprintf(stderr,"\toffsets are %g, %g, %g, %g\n",roff,loff,toff,boff);
}

show_samples()
{
	fprintf(stderr,"\tsampling rate is %d\n",samples);
}

show_output()
{
	fprintf(stderr,"\toutput is sent to %s\n",outstr);
}

show_term()
{
	fprintf(stderr,"\tterminal type is %s\n",term_tbl[term].name);
}

show_polar()
{
	if (polar)
		fprintf(stderr,"\tPolar coordinates are in effect\n");
	else
		fprintf(stderr,"\tRectangular coordinates are in effect\n");
}

show_autoscale()
{
	fprintf(stderr,"\tautoscaling is %s\n",(autoscale)? "ON" : "OFF");
}

show_logscale()
{
	if (log_x && log_y)
		fprintf(stderr,"\tlogscaling both x and y axes\n");
	else if (log_x)
		fprintf(stderr,"\tlogscaling x axis\n");
	else if (log_y)
		fprintf(stderr,"\tlogscaling y axis\n");
	else
		fprintf(stderr,"\tno logscaling\n");
}

show_variables()
{
register struct udvt_entry *udv = first_udv;

	fprintf(stderr,"\n\tVariables:\n");
	while (udv) {
		fprintf(stderr,"\t%-*s ",MAX_ID_LEN,udv->udv_name);
		if (udv->udv_undef)
			fputs("is undefined\n",stderr);
		else {
			fputs("= ",stderr);
			disp_value(stderr,&(udv->udv_value));
			(void) putc('\n',stderr);
		}
		udv = udv->next_udv;
	}
}


show_functions()
{
register struct udft_entry *udf = first_udf;

	fprintf(stderr,"\n\tUser-Defined Functions:\n");

	while (udf) {
		if (udf->definition)
			fprintf(stderr,"\t%s\n",udf->definition);
		else
			fprintf(stderr,"\t%s is undefined\n",udf->udf_name);
		udf = udf->next_udf;
	}
}


show_at()
{
	(void) putc('\n',stderr);
	disp_at(temp_at(),0);
}


disp_at(curr_at, level)
struct at_type *curr_at;
int level;
{
register int i, j;
register union argument *arg;

	for (i = 0; i < curr_at->a_count; i++) {
		(void) putc('\t',stderr);
		for (j = 0; j < level; j++)
			(void) putc(' ',stderr);	/* indent */

			/* print name of instruction */

		fputs(ft[(int)(curr_at->actions[i].index)].f_name,stderr);
		arg = &(curr_at->actions[i].arg);

			/* now print optional argument */

		switch(curr_at->actions[i].index) {
		  case PUSH:	fprintf(stderr," %s\n", arg->udv_arg->udv_name);
					break;
		  case PUSHC:	(void) putc(' ',stderr);
					disp_value(stderr,&(arg->v_arg));
					(void) putc('\n',stderr);
					break;
		  case PUSHD:	fprintf(stderr," %s dummy\n",
					  arg->udf_arg->udf_name);
					break;
		  case CALL:	fprintf(stderr," %s", arg->udf_arg->udf_name);
					if (arg->udf_arg->at) {
						(void) putc('\n',stderr);
						disp_at(arg->udf_arg->at,level+2); /* recurse! */
					} else
						fputs(" (undefined)\n",stderr);
					break;
		  case JUMP:
		  case JUMPZ:
		  case JUMPNZ:
		  case JTERN:
					fprintf(stderr," +%d\n",arg->j_arg);
					break;
		  default:
					(void) putc('\n',stderr);
		}
	}
}


show_version()
{
extern char version[];
extern char date[];
static char *authors[] = {"Thomas Williams","Colin Kelley"};
int x;
long time();

	x = time((long *)NULL) & 1;
	fprintf(stderr,"\n\t%s\n\t%sversion %s\n\tlast modified %s\n",
		PROGRAM, OS, version, date);
	fprintf(stderr,"\tCopyright (C) 1986, 1987  %s, %s\n\n",
		authors[x],authors[1-x]);
}
