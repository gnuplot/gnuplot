/*
 * Program	: help
 * Module	: global.h
 * Programmer	: R. Stolfa
 *
 * Modification History:
 *   08/27/87	Created
 *   07/13/88	Cleaned up for distribution
 */

#include	<stdio.h>
#ifdef __TURBOC__
#include <stdlib.h>
#endif
#include	<signal.h>
#include	<ctype.h>
#undef		toupper			/* to get the non-macro version */

#define		TRUE		1
#define		FALSE		0
#define		UP		2

#define		BSIZE		80

#define		PRINT		0
#define		ACRON		1
#define		TOPIC		2

/*
 * ROOTDIR is the anchor point for the help tree.  It should be a
 * publically accessable directory, with all it's submembers being
 * readable and executable by all.
 */
#define		ROOTDIR		"/usr/local/help"

/*
 * HELPFILE is the basename of the file that will contain the
 * text of the actual help information.  It should have
 * permissions 444.
 */
#define		HELPFILE	"/TEXT"

/*
 * DIRFILE is the basename of a file that contains the help
 * index to file name mappings for the current level of the
 * help tree.  The format of the data contained in this file
 * is as follows....
 *
 * <basename><print_flag><help_index_string>
 *
 * where:
 *	<basename>	relative directory name for help
 *	<print_flag>	is a:
 *				* for printable
 *				: for acronym
 *	<help_index_string>
 *			text to present as a choice (or use as an
 *			acronym) at any level in the help tree
 */
#define		DIRFILE		"/DIR"

/*
 * LIST structure.
 *
 * This is the standard format of help file lists.
 */
struct	LIST {
	char	base[BSIZE];
	char	topic[BSIZE];
	char	cmd[BSIZE];
	struct	LIST	*prev;
};
#define		prt_list	(_list[PRINT])
#define		acr_list	(_list[ACRON])
#define		top_list	(_list[TOPIC])

/*------------------------------------------------------------*/

/*
 * MACROS
 */

#define	gen_path(x)	sprintf (Path, "%s%s%s", Root_Dir, cur_path, (x))

/*------------------------------------------------------------*/

/*
 * Variables
 */

#ifdef	MAIN
#define	extern	/* global */
#endif

extern	struct	LIST	*_list[3];	/* list of printable topics */
extern	char		Path[BSIZE],	/* true path to help file */
			cur_path[BSIZE];/* curent help path */
extern	int		lines;		/* number of lines on the screen */
#ifdef __TURBOC__
          void catch();	/* interrupt handler */
#else
          int catch();
#endif
extern	char		Root_Dir[BSIZE];/* location of root directory */
