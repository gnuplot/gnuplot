/*
 * Program	: help
 * Module	: pchar.c
 * Programmer	: R. Stolfa
 *
 * Purpose :	To provide a very simple "more" like output stream for
 *		looking at the text in "HELPFILE" and all the topics
 *		listed in "DIRFILE".
 *
 * Modification History:
 *   08/27/87	Created
 */

#include	"global.h"

pchar (c)
int	c;
{
	char	in_buff[BSIZE];		/* input buffer */

	/*
	 * If this is the recursive call, do not
	 * output anything
	 */
	if (c != '\0')
		putchar (c);

	/*
	 * If this is the newline, then increment the
	 * line count
	 */
	if (c == '\n')
		lines ++;

	/*
	 * If this is the one to pause on, then do so
	 */
	if (lines == 21) {
		printf ("\nPress RETURN to continue");
		(void) fgets (in_buff, BSIZE, stdin);
		lines = 0;
	}
}
