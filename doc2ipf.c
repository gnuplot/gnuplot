#ifndef lint
static char *RCSid = "$Id: doc2ipf.c%v 3.50 1993/07/09 05:35:24 woo Exp $";
#endif

/*
 * doc2ipf.c  -- program to convert Gnuplot .DOC format to OS/2
 * ipfc  (.inf/.hlp) format.
 *
 * Modified by Roger Fearick from doc2rtf by M Castro 
 *
 * usage:  doc2ipf gnuplot.doc gnuplot.itl
 *
 */

/* note that tables must begin in at least the second column to */
/* be formatted correctly and tabs are forbidden */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

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

struct TABENTRY { /* may have 3 column tables */
        struct TABENTRY *next ;
        char col[3][256] ;
        } ;

struct TABENTRY table = {NULL} ;
struct TABENTRY *tableins = &table ;
int tablecols = 0 ;
int tablewidth[3] = {0,0,0} ;        
int tablelines = 0 ;

int debug = FALSE;

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
        c = strtok(&(line[1]),"\n");
        if( c == NULL || *c == '\0' ) c = list->string ;
        keylist->string = (char *) malloc (strlen(c)+1);
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
    list = list->next;        /* look at next element before going on */
    while (list != NULL)
    {
        /* we are onto the next topic so stop */
        if (list->level == curlevel)
            break;
        /* these are the next topics down the list */
        if (list->level == curlevel+1)
        {
            c = list->string;
            }
        list = list->next;
        }
    }

void
convert(a,b)
    FILE *a,*b;
{
    static char line[MAX_LINE_LEN];
    
    /* generate rtf header */
    fprintf(b,":userdoc.\n:prolog.\n");
    fprintf(b,":title.GNUPLOT\n");
    fprintf(b,":docprof toc=1234.\n:eprolog.\n");

    /* process each line of the file */
        while (fgets(line,MAX_LINE_LEN,a)) {
       process_line(line, b);
       }

    /* close final page and generate trailer */
    fprintf(b,"\n:euserdoc.\n");
}

void
process_line(line, b)
    char *line;
    FILE *b;
{
    static int line_count = 0;
    static char line2[MAX_LINE_LEN];
    static int last_line;
    char hyplink1[64] ;
    char *pt, *tablerow ;
    int i;
    int j;
    static int startpage = 1;
    char str[MAX_LINE_LEN];
    char topic[MAX_LINE_LEN];
    int k, l;
    static int tabl=0;
    static int para=0;
    static int inquote = FALSE;
    static int inref = FALSE;
    static int intable = FALSE ; 
    static int intablebut = FALSE ;
    static FILE *bo= NULL, *bt = NULL ;

    line_count++;

    if( bo == NULL ) bo = b ;
    i = 0;
    j = 0;
    while (line[i] != NULL)
    {
        switch(line[i])
        {
            case '$':
                if( intable && line[0] == '%' ) {                                    
                   ++i ;
                   if( line[i+1]=='$'|| line[i]=='x' || line[i]=='|'){
                      while ( line[i] != '$' ) line2[j++]=line[i++] ;
                      --j;
                      }
                   else {
                       while( line[i] != '$' ) i++ ;
                       if( line[i+1]==',' ) i++ ;
                       if( line[i+1]==' ' ) i++ ;
                       line2[j]=line[++i] ;
                       }
                   }
                break ;
            case ':':
                strcpy( &line2[j], "&colon." ) ;
                j += strlen( "&colon." ) - 1 ;
                break ;

            case '&':
                strcpy( &line2[j], "&amp." ) ;
                j += strlen( "&amp." ) - 1 ;
                break ;
                                
            case '\r':
            case '\n':
                break;
            case '`':    /* backquotes mean boldface or link */
                if ((!inref) && (!inquote))
                {
                    k=i+1;    /* index into current string */
                    l=0;    /* index into topic string */
                    while ((line[k] != '`') && (line[k] != NULL))
                    {
                        topic[l] = line[k];
                        k++;
                        l++;
                        }
                    topic[l] = NULL;
                    k = lookup(topic);
                    if (k > 0)
                    {
                        sprintf( hyplink1, ":link reftype=hd res=%d.", k ) ;
                        strcpy( line2+j, hyplink1 ) ;
                        j += strlen( hyplink1 )-1 ;
                        
                        inref = k;
                        }
                    else
                    {
                        if (debug)
                            fprintf(stderr,"Can't make link for \042%s\042 on line %d\n",topic,line_count);
                        line2[j++] = ':';
                        line2[j++] = 'h';
                        line2[j++] = 'p';
                        line2[j++] = '2';
                        line2[j] = '.';
                        inquote = TRUE;
                        }
                    }
                else
                {
                    if (inquote && inref)
                        fprintf(stderr, "Warning: Reference Quote conflict line %d\n", line_count);
                    if (inquote)
                    {
                        line2[j++] = ':';
                        line2[j++] = 'e';
                        line2[j++] = 'h';
                        line2[j++] = 'p';
                        line2[j++] = '2';
                        line2[j] = '.';
                        inquote = FALSE;
                        }
                    if (inref)
                    {
                        /* must be inref */
                        line2[j++] = ':';
                        line2[j++] = 'e';
                        line2[j++] = 'l';
                        line2[j++] = 'i';
                        line2[j++] = 'n';
                        line2[j++] = 'k';
                        line2[j] = '.';
                        inref = 0;
                        }
                }
                break;
            default:
                line2[j] = line[i];
            }
        i++;
        j++;
        line2[j] = NULL;
        }

    i = 1;

    switch(line[0]) {        /* control character */
       case '?': {            /* interactive help entry */
                if( intable ) intablebut = TRUE ;
               break;
       }
       case '@': {            /* start/end table */
                  intable = !intable ;  
                  if( intable ) {
                    tablelines = 0;
                    tablecols = 0 ;
                    tableins = &table ;
                    for(j=0;j<3;j++)tablewidth[j]=0 ;
                    }
                  else { /* dump table */
                    intablebut = FALSE ;
                    tableins = table.next ;
                    fprintf(b,":table cols=\'") ;
                    for( j=0;j<3;j++)
                        if(tablewidth[j]>0) fprintf(b," %d",tablewidth[j]);
                    fprintf(b,"\'.\n") ;
                    tableins=tableins->next ;     
                    while( tableins != NULL ) {
                        if( tableins->col[0][0] != '_' ) {
                            fprintf(b,":row.\n" ) ;
                            for( j=0;j<tablecols;j++) 
                                fprintf(b,":c.%s\n", tableins->col[j] ) ;
                            }
                        tableins = tableins->next ;
                        }
                    fprintf(b,":etable.\n") ;
                    if( bt != NULL ) {
                        rewind( bt ) ;
                        while( fgets(str, MAX_LINE_LEN, bt) )
                            fputs( str, b ) ; 
                        fclose( bt ) ;
                        remove( "doc2ipf.tmp" ) ;
                        bt = NULL ;
                        bo = b ;
                        }
                    }
          break;            /* ignore */
       }
       case '#': {            /* latex table entry */
          break;            /* ignore */
       }
       case '%': {            /* troff table entry */
              if( intable ) {
                    tablerow = line2 ;
                    tableins->next = malloc( sizeof( struct TABENTRY ) ) ;
                    tableins = tableins->next ;
                    tableins->next = NULL ;
                    j=0 ;
                while((pt=strtok( tablerow, "%@\n" ))!=NULL) {
                    if( *pt != '\0' ) { /* ignore null columns */
                        strcpy( tableins->col[j], pt ) ;
                        k=strlen( pt ) ;
                        if( k > tablewidth[j] ) tablewidth[j]=k ;
                        ++j ;
                        tablerow = NULL ;
                        if( j > tablecols ) tablecols = j ;
                        }
                    }
                for( j; j<3; j++ ) tableins->col[j][0]='\0' ;        
                }
          break;            /* ignore */
       }
       case '\n':            /* empty text line */
            para = 0;
            tabl = 0;
            fprintf(bo,":p.");
            break;
       case ' ': {            /* normal text line */
                  if( intable && ! intablebut ) break ;
                  if( intablebut ) { /* indexed items in  table, copy
                                      to file after table by saving in
                                      a temp file meantime */
                                if( bt == NULL ) {
                                    fflush(bo) ;
                                    bt = fopen( "doc2ipf.tmp", "w+" ) ;
                                    if( bt==NULL ) fprintf(stderr, "cant open temp\n" ) ;
                                    else bo = bt ; 
                                    }
                        }
                  if( intablebut && (bt==NULL )) break ;
          if ((line2[1] == NULL) || (line2[1] == '\n'))
          {
                fprintf(bo,":p."); 
                para = 0;
                }
          if (line2[1] == ' ') 
          {
                if (!tabl)
                {
                    fprintf(bo,":p."); 
                    }
                  fprintf(bo,"%s",&line2[1]); 
                fprintf(bo,"\n.br\n");
                tabl = 1;
                para = 0;
                }
          else
          {
                if (!para)
                {
                    para = 1;        /* not in para so start one */
                    tabl = 0;
                    }
                  fprintf(bo,"%s \n",&line2[1]); 
                }
                  fflush(bo) ;
          break;
       }
       default: {
          if (isdigit(line[0])) { /* start of section */
                  if( intable ) {
                      intablebut = TRUE ;
                                if( bt == NULL ) {
                                    fflush(bo) ;
                                    bt = fopen( "doc2ipf.tmp", "w+" ) ;
                                    if( bt==NULL ) fprintf(stderr, "cant open temp\n" ) ;
                                    else bo = bt ; 
                                    }
                                }
            if (!startpage)
            {
                refs(last_line,bo);
                }
            para = 0;                    /* not in a paragraph */
            tabl = 0;
            last_line = line_count;
            startpage = 0;
fprintf( stderr, "%d: %s\n", line_count, &line2[1] ) ;
            k=lookup(&line2[2]) ;
/*            if( k<0 ) fprintf(bo,":h%c.", line[0]=='1'?line[0]:line[0]-1);
            else*/ fprintf(bo,":h%c res=%d.", line[0]=='1'?line[0]:line[0]-1,line_count);
            fprintf(bo,&(line2[1])); /* title */
            fprintf(bo,"\n:p." ) ;
          } else
            fprintf(stderr, "unknown control code '%c' in column 1, line %d\n",
                line[0], line_count);
          break;
       }
    }
}

