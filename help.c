/*
 * Program	: help
 * Module	: help.c
 * Programmer	: R. Stolfa
 *
 * Purpose :	To more the current "HELPFILE" to the screen.
 *
 * Modification History:
 *   08/27/87	Created
 *   09/02/87	Added ("Q"|"q") commands
 */

#include	"global.h"

help ()
{
	FILE	*fd;		/* help text file */
	int	c;		/* temp */

	gen_path (HELPFILE);

	if ((fd = fopen (Path, "r")) == NULL) {
		printf ("There is no help text on this subject\n");
		return;
	}

	/*
	 * Note what help subject we are looking at
	 */
	if (strlen (cur_path) != 0) {
		present ("TOPIC: ", " ");
		pchar ('\n');
	}

	/*
	 * Reset the number of lines displayed, and output
	 * the new help file text.
	 */
	while ((c = getc (fd)) != EOF)
		pchar(c);

	fclose (fd);

	pchar ('\n');
	lines ++;
}
