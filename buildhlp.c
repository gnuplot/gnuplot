/*
 * buildhlp.c  -- a program to build an MS-DOS help tree from a VMS .HLP file
 * Colin Kelley
 * January 1987
 */

#include <stdio.h>
#include <ctype.h>

#define TRUE 1
#define FALSE 0

#define HELPMAIN "HELP.HLM"
#define EXT ".HLP"

char cwd[80];
char buffer[10000];

int currlevel, nextlevel;
char currname[13], nextname[13];

int line = 1;

main(argc,argv)
int argc;
char *argv[];
{
register char *bptr;
register int c;
FILE *f;
int done = FALSE;

	if (argc != 2) {
		fprintf(stderr,"usage:  %s start-dir < helpfile\n\n",argv[0]);
		fprintf(stderr,"  where <start-dir> is the directory in which to start building the tree\n");
		fprintf(stderr,"  and <helpfile> is in VMS help file format\n");
		exit(1);
	}

	getcwd(cwd,sizeof(cwd));
	if (chdir(argv[1]) == -1) {
		perror("can't chdir() to starting directory");
		exit_err(argv[1]);
	}


	if (scanf("%d %8s",&currlevel,currname) != 2)
		exit_err("scanf() error");

	do {
		for (c = currlevel*2 - 1; --c;)
			putchar(' ');
		puts(currname);

		strcat(currname,EXT);

		while ((c = getchar()) != EOF && c != '\n')
			;
		if (c == '\n')
			line++;

		if ((c = getchar()) != EOF && c != ' ')
			ungetc(c,stdin);

		bptr = buffer;
		while ((c = getchar()) != EOF) {
			*bptr++ = c;
			if (c == '\n') {
				line++;
				if ((c = getchar()) != EOF) {
					if (c != ' ')
						ungetc(c,stdin);		/* push it back */
					if (isdigit(c))
						break;
				}
			}
		}
		*bptr = '\0';

		if (c == EOF) {
			done = TRUE;
			nextlevel = 1;
		}
		else {
			if (scanf("%d %8s",&nextlevel,nextname) != 2)
				exit_err("scanf() error");
		}

		if (!done && nextlevel > currlevel) {	/* have a subtopic */
			if (chdir(currname) == -1) {	/* does it already exist? */
				if (mkdir(currname) == -1) {
					perror("mkdir()");
					exit_err(currname);
				}
				if (chdir(currname) == -1) {
					perror("chdir()");
					exit_err(currname);
				}
			}
			f = fopen(HELPMAIN,"w");
		}
		else
			f = fopen(currname,"w");

		while (nextlevel <= --currlevel)
			if (chdir("..") == -1) {
				perror("chdir()");
				exit_err("..");
			}

		if (f == NULL) {
			perror("fopen()");
			exit_err(currname);
		}
		fputs(buffer,f);
		fclose(f);

		currlevel = nextlevel;
		strcpy(currname,nextname);

	} while (!done);

	chdir(cwd);
	exit(0);
}

exit_err(msg)
char msg[];
{
	fprintf(stderr,"%s (input line %d)\n",msg,line);
	chdir(cwd);
	exit(1);
}
