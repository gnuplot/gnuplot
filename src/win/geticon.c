#ifndef lint
static char *RCSid = "$Id: geticon.c,v 1.2 1993/09/27 17:24:27 alex Exp $";
#endif

/* geticon.c */
/* extract Borland ascii format icons from resource script */
/* and write as Microsoft binary format .ICO files */
/* Russell Lang 1992-12-20 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* HBB 980809: naming a variable 'inline' is a bad idea, these days. Too
 * many compilers use it as a keyword... Changed to 'inputline' */
#define MAXLINE 255
FILE *rcfile;
char inputline[MAXLINE+1];
char *tok1, *tok2, *tok3;
char *p;
char iconname[MAXLINE+1];
FILE *iconfile;
int line;

int htoi(char ch)
{
    ch = toupper(ch);
    if (ch < '0')
        return(0);
    else if (ch <= '9')
        return((int)(ch - '0'));
    else if (ch < 'A')
        return(0);
    else if (ch <= 'F')
        return((int)(ch - 'A' + 10));
    return(0);
}

void
geticon(void)
{
char ch;
    fgets(inputline,MAXLINE,rcfile);
    line++;
    if (strncmp(inputline,"BEGIN",5)) {
    	fprintf(stderr,"Expecting BEGIN at line %d\n",line);
    	exit(3);
    }
    if ( (iconfile = fopen(iconname,"wb")) == (FILE *)NULL) {
        fprintf(stderr,"Can't open ICON file %s\n",iconname);
        exit(4);
    }
    fgets(inputline,MAXLINE,rcfile);
    line++;
    while (strncmp(inputline,"END",3) && !feof(rcfile)) {
        for (p = inputline; *p && (*p==' ' || *p == '\t' || *p=='\''); p++);
        while (isxdigit(*p)) {
            ch = htoi(*p++)<<4;
            ch += htoi(*p++);
            fputc(ch, iconfile);
            p++;
        }
        fgets(inputline,MAXLINE,rcfile);
        line++;
    }
    fclose(iconfile);
}

int
main(int argc, char *argv[])
{
    if ((argc < 2) || (argc > 3)) {
	fprintf(stderr,"Usage:  geticon  resource_file [icon_directory]\n");
	return(1);
    }
    if ( (rcfile = fopen(argv[1],"r")) == (FILE *)NULL) {
	fprintf(stderr,"Can't open RC file\n");
	return(2);
    }
    line = 0;
    while (fgets(inputline,MAXLINE,rcfile)) {
        line++;
	tok1 = strtok(inputline," \t\r\n");
	tok2 = strtok(NULL," \t\r\n");
	tok3 = strtok(NULL," \t\r\n");
	if (tok2 && !strcmp(tok2,"ICON") && (tok3 == (char *)NULL)) {
            iconname[0] = '\0';
	    if (argc == 3) {
                strcpy(iconname,argv[2]);
                strcat(iconname,"\\");
            }
            strcat(iconname,tok1);
            strcat(iconname,".ico");
	    fprintf(stdout,"%s\n",iconname);
	    geticon();
	}
    }
    return (0);
}
