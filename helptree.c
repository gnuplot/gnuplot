#include <stdio.h>
#include <ctype.h>
#ifndef __TURBOC__
#include <sys/file.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>

extern errno, sys_nerr;
extern char *sys_errlist[];

#define ErrOut(s1,s2) {\
fprintf(stderr, "\n%s:: ", sys_errlist[errno]);\
fprintf(stderr, s1, s2);\
fprintf(stderr, "\n");\
exit(1);}

main(argc,argv)
int argc;
char *argv[];
/*
   Routine to build a flat file out of a help tree or build a help tree
   out of a flat file.  This routine requires a switch (-f or -t) to
   determine which mode it will run in and a path name that represents
   the root of a help tree.  A flat help file is useful for moving a
   help tree from one system to another and, in addition, VMS can use
   it directly as a help file.

   -f assume we are creating a flat file, path is the root of an existing
      help tree and the flat help file will be written to stdout.

   -t assume we are creating a help tree, path is the root of a help tree
      that is to be created.

   usage: helptree -f path       or      helptree -t path

*/
{

/* Parse options. */
   if (argc != 3 || argv[1][0] != '-') usage();
   if (argv[1][1] == 'f')
      build_file (argv[2], 0);
   else if (argv[1][1] == 't')
      build_tree (argv[2]);

   exit (0);
/*NOTREACHED*/
}


build_file (path,level)
char *path;
int level;
{
   FILE *fp, *fopen();
   char buf[BUFSIZ], name[BUFSIZ];
   char *names, *src, *dst, *str_append(), *next_name();

   ++level;
   if (chdir(path) < 0)
      ErrOut("Can't cd to %s", path);

/* Copy any TEXT file out to stdout. */
   if ((fp = fopen ("TEXT", "r")) != NULL) {
      while (fgets (buf, BUFSIZ, fp) != NULL) {
	 putc (' ', stdout);  /* Put leading space on output lines */
         fputs (buf, stdout);
      }
      fclose (fp);
   }
   names = NULL;

/* Read any DIR file and build a list of names. */
   if ((fp = fopen ("DIR", "r")) != NULL) {
      while (fgets (buf, BUFSIZ, fp) != NULL) {
         src = buf;
         dst = name;
         while (*src && *src != '*') *dst++ = *src++;
         if (*src == '*') {
         /* Append this name to the end of the name list. */
            *dst = '\0';
            names = str_append (names, name);
         }
      }
      fclose (fp);
   }
/* Cycle through the names and do each one found. */
   src = names;
   while ((src = next_name (src, name)) != NULL) {
      if (isdirectory(name)) {
         printf ("%d %s\n", level, name);
         build_file (name, level);  /* Recurse. */
         if (chdir ("..") < 0)
            ErrOut ("Error with cd ..", NULL);
      }
   }
}


static int lncnt=0, done=0;  /* Set in put_text */
static char buf[BUFSIZ];

build_tree (start_path)
char *start_path;
/*
   This routine assumes that a properly formatted .hlp file is being read
   in from standard input.  Properly formatted means that the only lines
   which begin in column 1 are numeric indices of a tree (directory) 
   level.
*/
{
   int level, curlevel=0;
   char cmd[256], *ptr, path[BUFSIZ];
   
   strcpy (path, start_path);
   while (!done) {
      sprintf (cmd,"mkdir %s", path);
      system (cmd); 
      if (curlevel > 0)
         append_DIR (path);
      if (chdir(path) < 0)
         ErrOut("Can't cd to %s", path);
      ++curlevel;

      put_text ("TEXT");
   /* Buf now contains new level item or we reached EOF */
      if (done) break;

      level = atoi (buf);
      ptr = buf;
      while (isspace(*ptr) || isdigit(*ptr)) ++ptr;
      ptr[strlen(ptr)-1] = '\0';  /* Remove \n */
      strcpy (path, ptr);

      while (curlevel > level) {
         if (chdir("..") < 0)
            ErrOut("Can't cd to ..", path);
         if (--curlevel < 1) {
            fprintf (stderr, "Error with input near line %d\n",lncnt);
            exit(1);
         }
      }
   }
}

append_DIR (path)
char *path;
/*
   Append ``path'' to the DIR file found at this directory level.
*/
{
   FILE *fdir, *fopen();

   if ((fdir = fopen ("DIR", "a")) == NULL) {
      fprintf (stderr, "Couldn't build DIRS near line %d\n", lncnt);
      ErrOut ("Open failure for DIRS in %s", path);
   }
   fprintf (fdir, "%s*%s\n", path, path);
   fclose (fdir);
}


put_text (file)
char *file;
{
   FILE *ftext = NULL, *fopen();
   char *bufptr;

   while ((bufptr = fgets (buf, BUFSIZ, stdin)) != NULL) {
      ++lncnt;
      if (!isspace(buf[0])) {
         break;
      }
   /* Open the file the first time we have to write to it. */
      if (ftext == NULL) {
         if ((ftext = fopen (file, "w")) == NULL) 
            ErrOut ("Can't open TEXT file near line %d in input", lncnt);
      }
      fputs (buf+1, ftext);  /* Don't write first blank */
   }
   if (ftext != NULL)
      fclose(ftext);
   if (bufptr == NULL) {
      done = 1;
   }
}


int isdirectory(name)
char *name;
{
   struct stat sbuf;
   int isdir=1;
   
   if (stat(name,&sbuf) < 0) {
      isdir = 0;
      fprintf (stderr, "Can't stat %s\n", name);
   }

   if (!(sbuf.st_mode & S_IFDIR)) {
      isdir = 0;
      fprintf (stderr, "%s is not a directory!\n", name);
   }
   return isdir;
}


char *next_name (src, name)
char *src, *name;
/*
   Routine to store next name in src into and then return pointer to next
   name.  src is a list of names separated by a ' '.
*/
{
   char *dst=name;

   if (!*src) return NULL;

   while (*src && *src != ' ') *dst++ = *src++;

   *dst = '\0';

   if (dst == name) return NULL;

   while (*src == ' ') ++src;

   return src;
}


char *str_append (names, name)
char *names, *name;
/*
   Routine to stick name into the names list.
*/
{
   char *realloc(), *malloc();
   int l1=0, l2;

   if (names != NULL)
      l1 = strlen(names);
   else
      l1 = -1;

   l2 = strlen(name);

   if (names == NULL)
      names = malloc ((unsigned )l1+l2+2);
   else
      names = realloc (names, (unsigned )l1+l2+2);

   if (l1 > 0)
      names[l1] = ' ';
   
   strcpy (&names[l1+1], name);

   return names;
}


usage()
{
   fprintf (stderr, "usage: helptree -f path\tor\thelptree -t path\n\n");
   fprintf (stderr,
   "-f assume we are creating a flat file, path is the root of an existing\n");
   fprintf (stderr,
   "   help tree and the flat help file will be written to stdout.\n\n");

   fprintf (stderr,
   "-t assume we are creating a help tree, path is the root of a help tree\n");
   fprintf (stderr,"   that is to be created.\n");

   exit(1);
}
