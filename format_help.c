/*
 * Program	: help
 * Module	: format_help.c
 * Programmer	: R. Stolfa
 *
 * Purpose :	To format the list of available help topics in a
 *		neat and readable format
 *
 * Modification History:
 *   08/26/87	Created
 *   09/02/87	Fixed for more than one line of text (oops!),
 *		streamlined.
 */

#include	"global.h"

format_help ()
{
	struct	LIST	*p;		/* temporary LIST pointer */
	register	int	cur_col;

	/*
	 * Screen columns go from 0 to 79
	 */
	cur_col = 0;

	pchar ('\n');
	for (p = prt_list; p != NULL; p = p->prev) {
		/*
		 * If the addition of the current topic to the screen
		 * will cause there to be wraparound, skip to the next
		 * line.
		 */
		cur_col = (cur_col + 8) -
			  ((cur_col + 8) % 8) +
			  strlen(p->topic);
		if (cur_col > 79)  {
			cur_col = strlen(p->topic) + 8;
			pchar ('\n');
		}
		printf ("\t%s", p->topic);
	}
	pchar ('\n');
	pchar ('\n');
}
