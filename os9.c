/*
 * os9.c
 *
 * Some Unix like functions that gnuplot uses.
 * Original sources from the blars lib.
 *
 */

#include <stdio.h>
#include <modes.h>
#include <direct.h>
#include <sgstat.h>

#ifdef PIPES

/* Original version by Robert A. Larson */
/* Adapted by M.N. Schipper */

#include <string.h>
#include <module.h>

extern char *_environ;
extern int os9fork();
extern mh_com *modlink();
extern mh_com *modloadp();

static int proc[_NFILE];
static mh_com *loadmods[_NFILE];

FILE *popen(command, mode)
char *command;
char *mode;
{
    int temp, fd;
    FILE *pipe;
    char *argv[4];
    register char *cp;
    mh_com *mod;
    int linked = 0;

    if(mode[1]!='\0' || (*mode!='r' && *mode!='w')) return (FILE *)NULL;
    fd = (*mode=='r');
    if((temp = dup(fd)) <= 0) return (FILE *)NULL;
    if((pipe = fopen("/pipe", "r+")) == NULL) {
    	close(temp);
    	return (FILE *)NULL;
    }
    close(fd);
    dup(fileno(pipe));

    if (strrchr (command, '/') == NULL)
		mod = modlink (command, 0);
	else
		mod = (mh_com *) -1;
	if (mod == (mh_com *) -1)
		loadmods[fileno(pipe)] = mod = modloadp (command, 0, NULL);
	else
	{
		linked = 1;
		loadmods[fileno(pipe)] = (mh_com *) -1;
	}
		
    argv[0] = "shell";
    if (mod != (mh_com *) -1)
    {
	    argv[1] = "ex";
	    argv[2] = command;
    	argv[3] = (char *)NULL;
    }
    else
    {
    	argv[1] = command;
	    argv[2] = (char *)NULL;
	}
    if((proc[fileno(pipe)] = os9exec(os9fork, argv[0], argv, _environ, 0, 0)) < 0)
    {
		fclose(pipe);
		pipe = NULL;
	}
    close(fd);
    dup(temp);
    close(temp);
	if (linked && mod != (mh_com *) -1)
		munlink (mod);
    return pipe;
}

int pclose(pipe)
FILE *pipe;
{
    int p, stat, w;

    if((p = proc[fileno(pipe)]) <= 0) return -1;
    proc[fileno(pipe)] = 0;
    fflush(pipe);
	if (loadmods[fileno(pipe)] != (mh_com *) -1)
		munlink (loadmods[fileno(pipe)]);
    fclose(pipe);
    while((w=wait(&stat)) != -1 && w!=p) {}
    return w==-1 ? -1 : stat;
}

#endif	/* PIPES */


int isatty(f)
int f;
{
    struct sgbuf sgbuf;

    if(_gs_opt(f, &sgbuf) < 0) return -1;
    return sgbuf.sg_class == 0;
}


char *getwd(p)
char *p;
{
    register char *cp;
    register struct dirent *dp;
    register int l, olddot = 0, i, d, dot, dotdot;
    struct dirent db[8];
    char buf[1024];

    cp = &buf[1024-1];
    *cp = '\0';
    for(;;) {
	if((d = open(".", S_IREAD | S_IFDIR)) < 0) {
	    if(*cp) chdir(cp+1);
	    return NULL;
	}
	if((i = read(d, (char *)db, sizeof(db))) == 0) {
	    if(*cp) chdir(cp+1);
	    close(d);
	    return NULL;
	}
	dotdot = db[0].dir_addr;
	dot = db[1].dir_addr;
	if(olddot) {
	    i -= 2 * sizeof(struct dirent);
	    dp = &db[2];
	    for(;;) {
	        if(i <= 0) {
		    if((i = read(d, (char *)db, sizeof(db))) == 0) {
			if(*cp) chdir(cp+1);
			close(d);
			return NULL;
		    }
		    dp = &db[0];
		}
		if(olddot == dp->dir_addr) {
		    l = strlen(dp->dir_name);
		    /* last character has parity bit set... */
		    *--cp = dp->dir_name[--l] & 0x7f;
		    while(l) *--cp = dp->dir_name[--l];
		    *--cp = '/';
		    break;
		}
		i -= sizeof(struct dirent);
		dp++;
	    }
	}
	if(dot==dotdot) {
	    if(*cp) chdir(cp+1);
	    *p = '/';
	    if(_gs_devn(d, p+1) < 0) {
	        close(d);
		return NULL;
	    }
	    close(d);
	    strcat(p, cp);
	    return p;
	}
	close(d);
	if(chdir("..") != 0) {
	    if(*cp) chdir(cp+1);
	    return NULL;
	}
	olddot = dot;
    }
}
