/*
 * Program	: help
 * Module	: free_list.c
 * Programmer	: R. Stolfa
 *
 * Purpose :	To return to system memory a list of LIST structures
 *
 * Modification History:
 *   08/27/87	Created
 *   08/31/87	Changed default exit to be a call to "catch()"
 */

#include	"global.h"

free_list (type)
int	type;
{
	struct	LIST	*q;

	/*
	 * Check for valid type
	 */
	if ((type < 0) || (type >= 3)) {
		printf ("free_list:  error in type parameter %d\n", type);
		catch ();
		/* NOT REACHED */
	}

	/*
	 * Clear the header
	 */
	q = _list[type];
	_list[type] = NULL;

	/*
	 * Clear the list
	 */
	for ( ; q != NULL; q = q->prev)
		free (q);
}
