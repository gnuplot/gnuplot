/*
 * Program	: help
 * Module	: initialize.c
 * Programmer	: R. Stolfa
 *
 * Purpose :	To initialize all global data structures
 *
 * Modification History:
 *   08/31/87	Created
 */

#include	"global.h"

initialize()
{
	int	i;

	/*
	 * Catch all signals that might not free up memory....
	 */
	signal (SIGINT, catch);
	signal (SIGTERM, catch);

	Path[0] = '\0';
	cur_path[0] = '\0';

	for (i = 0; i < 3 ; i ++)
		_list[i] = NULL;
}
