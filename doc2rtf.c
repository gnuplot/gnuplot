/*
 * doc2rtf.c  -- program to convert Gnuplot .DOC format to MS windows
 * help (.rtf) format.
 *
 * This involves stripping all lines with a leading digit or
 * a leading @, #, or %.
 * Modified by Maurice Castro from doc2gih.c by Thomas Williams 
 *
 * usage:  doc2rtf file.doc file.rtf [-d]
 *
 */

/* note that tables must begin in at least the second column to */
/* be formatted correctly and tabs are forbidden */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#define MAX_LINE_LEN	1024
#define TRUE 1
#define FALSE 0

struct LIST
{
	int level;
	int line;
	char *string;
	struct LIST *next;
	};

struct LIST *list = NULL;
struct LIST *head = NULL;

struct LIST *keylist = NULL;
struct LIST *keyhead = NULL;

int debug = FALSE;

void footnote();
void parse();
void refs();
void convert();
void process_line();
int lookup();

main(argc,argv)
int argc;
char **argv;
{
FILE * infile;
FILE * outfile;
	if (argc==4 && argv[3][0]=='-' && argv[3][1]=='d')
		debug = TRUE;

	if (argc != 3 && !debug) {
		fprintf(stderr,"Usage: %s infile outfile\n", argv[0]);
		return(1);
	}
	if ( (infile = fopen(argv[1],"r")) == (FILE *)NULL) {
		fprintf(stderr,"%s: Can't open %s for reading\n",
			argv[0], argv[1]);
		return(1);
	}
	if ( (outfile = fopen(argv[2],"w")) == (FILE *)NULL) {
		fprintf(stderr,"%s: Can't open %s for writing\n",
			argv[0], argv[2]);
	}
	parse(infile);
	convert(infile,outfile);
	return(0);
}

/* scan the file and build a list of line numbers where particular levels are */
void parse(a)
FILE *a;
{
    static char line[MAX_LINE_LEN];
	char *c;
	int lineno=0;
	int lastline=0;

    while (fgets(line,MAX_LINE_LEN,a)) 
    {
	lineno++;
	if (isdigit(line[0]))
	{
		if (list == NULL)	
			head = (list = (struct LIST *) malloc(sizeof(struct LIST)));
		else
			list = (list->next = (struct LIST *) malloc(sizeof(struct LIST)));
		list->line = lastline = lineno;
		list->level = line[0] - '0';
		list->string = (char *) malloc (strlen(line)+1);
		c = strtok(&(line[1]),"\n");
		strcpy(list->string, c);
		list->next = NULL;
	}
	if (line[0]=='?')
	{
		if (keylist == NULL)	
			keyhead = (keylist = (struct LIST *) malloc(sizeof(struct LIST)));
		else
			keylist = (keylist->next = (struct LIST *) malloc(sizeof(struct LIST)));
		keylist->line = lastline;
		keylist->level = line[0] - '0';
		keylist->string = (char *) malloc (strlen(line)+1);
		c = strtok(&(line[1]),"\n");
		strcpy(keylist->string, c);
		keylist->next = NULL;
	}
	}
	rewind(a);
    }

/* look up an in text reference */
int
lookup(s)
char *s;
{
	char *c;
	char tokstr[MAX_LINE_LEN];
	char *match; 
	int l;

	strcpy(tokstr, s);

	/* first try the ? keyword entries */
	keylist = keyhead;
	while (keylist != NULL)
	{
		c = keylist->string;
		while (isspace(*c)) c++;
		if (!strcmp(s, c)) return(keylist->line);
		keylist = keylist->next;
		}

	/* then try titles */
	match = strtok(tokstr, " \n\t");
	l = 0; /* level */
	
	list = head;
	while (list != NULL)
	{
		c = list->string;
		while (isspace(*c)) c++;
		if (!strcmp(match, c)) 
		{
			l = list->level;
			match = strtok(NULL, "\n\t ");
			if (match == NULL)
			{
				return(list->line);
				}
			}
		if (l > list->level)
			break;
		list = list->next;
		}
	return(-1);
	}

/* search through the list to find any references */
void
refs(l, f)
int l;
FILE *f;
{
	int curlevel;
	char str[MAX_LINE_LEN];
	char *c;

	/* find current line */
	list = head;
	while (list->line != l)
		list = list->next;
	curlevel = list->level;
	list = list->next;		/* look at next element before going on */
	while (list != NULL)
	{
		/* we are onto the next topic so stop */
		if (list->level == curlevel)
			break;
		/* these are the next topics down the list */
		if (list->level == curlevel+1)
		{
			c = list->string;
			while (isspace(*c)) c++;
			fprintf(f,"\\par{\\uldb %s}",c);
			fprintf(f,"{\\v loc%d}\n",list->line);
			}
		list = list->next;
		}
	}

/* generate an RTF footnote with reference char c and text s */
void footnote(c, s, b)
char c;
char *s;
FILE *b;
{
	fprintf(b,"%c{\\footnote %c %s}\n",c,c,s);
	}

void
convert(a,b)
	FILE *a,*b;
{
    static char line[MAX_LINE_LEN];
	
	/* generate rtf header */
	fprintf(b,"{\\rtf1\\ansi ");		/* vers 1 rtf, ansi char set */
	fprintf(b,"\\deff0");				/* default font font 0 */
	/* font table: font 0 proportional, font 1 fixed */
	fprintf(b,"{\\fonttbl{\\f0\\fswiss Helv;}{\\f1\\fmodern Courier;}{\\f2\\fmodern Pica;}}\n");

	/* process each line of the file */
    while (fgets(line,MAX_LINE_LEN,a)) {
	   process_line(line, b);
	   }

	/* close final page and generate trailer */
	fprintf(b,"}{\\plain \\page}\n");
	fprintf(b,"}\n");
}

void
process_line(line, b)
	char *line;
	FILE *b;
{
    static int line_count = 0;
    static char line2[MAX_LINE_LEN];
    static int last_line;
	int i;
	int j;
	static int startpage = 1;
	char str[MAX_LINE_LEN];
	char topic[MAX_LINE_LEN];
	int k, l;
	static int tabl=0;
	static int para=0;
	static int llpara=0;
	static int inquote = FALSE;
	static int inref = FALSE;

	line_count++;

	i = 0;
	j = 0;
	while (line[i] != '\0')
	{
		switch(line[i])
		{
			case '\\':
			case '{':
			case '}':
				line2[j] = '\\';
				j++;
				line2[j] = line[i];
				break;
			case '\r':
			case '\n':
				break;
			case '`':	/* backquotes mean boldface or link */
				if (line[1]==' ')	/* tabular line */
					line2[j] = line[i];
				else if ((!inref) && (!inquote))
				{
					k=i+1;	/* index into current string */
					l=0;	/* index into topic string */
					while ((line[k] != '`') && (line[k] != '\0'))
					{
						topic[l] = line[k];
						k++;
						l++;
						}
					topic[l] = '\0';
					k = lookup(topic);
					if ((k > 0) && (k != last_line))
					{
						line2[j++] = '{';
						line2[j++] = '\\';
						line2[j++] = 'u';
						line2[j++] = 'l';
						line2[j++] = 'd';
						line2[j++] = 'b';
						line2[j] = ' ';
						inref = k;
						}
					else
					{
						if (debug)
							fprintf(stderr,"Can't make link for \042%s\042 on line %d\n",topic,line_count);
						line2[j++] = '{';
						line2[j++] = '\\';
						line2[j++] = 'b';
						line2[j] = ' ';
						inquote = TRUE;
						}
					}
				else
				{
					if (inquote && inref)
						fprintf(stderr, "Warning: Reference Quote conflict line %d\n", line_count);
					if (inquote)
					{
						line2[j] = '}';
						inquote = FALSE;
						}
					if (inref)
					{
						/* must be inref */
						sprintf(topic,"%d",inref);
						line2[j++] = '}';
						line2[j++] = '{';
						line2[j++] = '\\';
						line2[j++] = 'v';
						line2[j++] = ' ';
						line2[j++] = 'l';
						line2[j++] = 'o';
						line2[j++] = 'c';
						k = 0;
						while (topic[k] != '\0')
						{
							line2[j++] = topic[k];
							k++;
							}
						line2[j] = '}';
						inref = 0;
						}
				}
				break;
			default:
				line2[j] = line[i];
			}
		i++;
		j++;
		line2[j] = '\0';
		}

	i = 1;

    switch(line[0]) {		/* control character */
	   case '?': {			/* interactive help entry */
		if ((line2[1] != '\0') && (line2[1] != ' '))
			footnote('K',&(line2[1]),b);
		  break;
	   }
	   case '@': {			/* start/end table */
		  break;			/* ignore */
	   }
	   case '#': {			/* latex table entry */
		  break;			/* ignore */
	   }
	   case '%': {			/* troff table entry */
		  break;			/* ignore */
	   }
	   case '\n':			/* empty text line */
			fprintf(b,"\\par\n");
			llpara = para;
			para = 0;
			tabl = 0;
			break;
	   case ' ': {			/* normal text line */
		  if ((line2[1] == '\0') || (line2[1] == '\n'))
		  {
				fprintf(b,"\\par\n"); 
				llpara = para;
				para = 0;
				tabl = 0;
				}
		  if (line2[1] == ' ') 
		  {
				if (!tabl)
				{
					fprintf(b,"\\par\n"); 
					}
				fprintf(b,"{\\pard \\plain \\f1\\fs20 ");
			  	fprintf(b,"%s",&line2[1]); 
				fprintf(b,"}\\par\n");
				llpara = 0;
				para = 0;
				tabl = 1;
				}
		  else
		  {
				if (!para)
				{
					if (llpara)
						fprintf(b,"\\par\n"); /* blank line between paragraphs */
					llpara = 0;
					para = 1;		/* not in para so start one */
					tabl = 0;
					fprintf(b,"\\pard \\plain \\qj \\fs20 \\f0 ");
					}
			  	fprintf(b,"%s \n",&line2[1]); 
				}
		  break;
	   }
	   default: {
		  if (isdigit(line[0])) { /* start of section */
			if (!startpage)
			{
				refs(last_line,b);
				fprintf(b,"}{\\plain \\page}\n");
				}
			para = 0;					/* not in a paragraph */
			tabl = 0;
			last_line = line_count;
			startpage = 0;
			fprintf(b,"{\n");
			sprintf(str,"browse:%05d", line_count);
			footnote('+',str,b);
			footnote('$',&(line2[1]),b); /* title */
			fprintf(b,"{\\b \\fs24 %s}\\plain\\par\\par\n",&(line2[1]));
			/* output unique ID */
			sprintf(str,"loc%d", line_count);
			footnote('#',str,b);
		  } else
		    fprintf(stderr, "unknown control code '%c' in column 1, line %d\n",
			    line[0], line_count);
		  break;
	   }
    }
}

