/*
 * Program	: help
 * Module	: catch.c
 * Programmer	: R. Stolfa
 *
 * Purpose :	To handle all user signals.
 *
 * Modification History:
 *   08/31/87	Created
 */

#include	"global.h"

#ifdef __TURBOC__
  void catch()
#else
  int	catch()
#endif
{
	/*
	 * Free the in memory lists to keep user memory from
	 * growing.
	 */
	free_list (PRINT);
	free_list (ACRON);
	free_list (TOPIC);

	/*
	 * ...Neat up the screen and exit
	 */
	putchar ('\n');
	exit (0);
}
