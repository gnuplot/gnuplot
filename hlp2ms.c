/*
 * hlp2ms.c  -- program to convert VMS .HLP format to *roff -ms document
 * Thomas Williams 
 *
 * usage:  hlp2ms < file.hlp > file.ms
 *
 *   where file.hlp is a VMS .HLP file, and file.ms will be a [nt]roff
 *     document suitable for printing with nroff -ms or troff -ms
 *
 * typical usage for GNUPLOT:
 *
 *   vmshelp.csh /usr/help/gnuplot/* | hlp2ms | troff -ms
 */

#include <stdio.h>
#include <ctype.h>

#define MAX_NAME_LEN	256
#define MAX_LINE_LEN	256
#define LINE_SKIP		3


main()
{
	init(stdout);
	convert(stdin,stdout);
	finish(stdout);
	exit(0);
}


init(b)
FILE *b;
{
			/* in nroff, increase line length by 8 and don't adjust lines */
	(void) fputs(".if n \\{.nr LL +8m\n.na \\}\n",b);
    (void) fputs(".nr PO +0.3i\n",b);
	(void) fputs(".so titlepage\n",b);
	(void) fputs(".pn 1\n",b);
    (void) fputs(".ds CH GNUPLOT\n",b);
	(void) fputs(".ds RH Page %\n",b);
	(void) fputs(".bp\n",b);
    (void) fputs(".nr PS 12\n",b);
    (void) fputs(".nr VS 13\n",b);
	(void) fputs(".ta 1.5i 3.0i 4.5i 6.0i 7.5i\n",b);
	(void) fputs("\\&\n.sp 3\n.PP\n",b);
	(void) fputs(".so intro\n",b);
}


convert(a,b)
FILE *a,*b;
{
static char string[MAX_LINE_LEN],line[MAX_LINE_LEN];
int old = 1;
int sh_i, i;

	while (fgets(line,MAX_LINE_LEN,a)) {

		if (isdigit(line[0])) {
			(void) sscanf(line,"%d %[^\n]s",&sh_i,string);

			(void) fprintf(b,".sp %d\n",(sh_i == 1) ? LINE_SKIP : LINE_SKIP-1);

			if (sh_i > old) {
				do
					(void) fputs(".RS\n.IP\n",b);
				while (++old < sh_i);
			}
			else if (sh_i < old) {
				do
					(void) fputs(".RE\n.br\n",b);
				while (--old > sh_i);
			}

			(void) fprintf(b,".NH %d\n%s\n.sp 1\n.LP\n",sh_i,string);
			old = sh_i;

			(void) fputs(".XS\n",b);
			(void) fputs(string,b);
			(void) fputs("\n.XE\n",b);

		} else {
			switch (line[1]) {
				case ' ' : fputs(".br\n",b); fputs(line+1,b); fputs(".br\n",b);
 						   break;
				case '\'': fputs("\\&",b);
				default  : (void) fputs(line+1,b); 
			}
		}

	}
}


finish(b)		/* spit out table of contents */
FILE *b;
{
	(void) fputs(".pn 1\n",b);
	(void) fputs(".ds RH %\n",b);
	(void) fputs(".af % i\n",b);
	(void) fputs(".bp\n.PX\n",b);
}
