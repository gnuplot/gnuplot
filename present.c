/*
 * Program	: help
 * Module	: present.c
 * Programmer	: R. Stolfa
 *
 * Purpose :	To generate a topics line without '/'s in it.
 *
 * Modification History:
 *   08/31/87	Created
 */

#include	"global.h"

present (str1, str2)
char	*str1,
	*str2;
{
	int	i;		/* temp */

	/*
	 * Make a line like "/vi/join/lines" more readable as
	 * " vi join lines"
	 */
	printf ("%s", str1);
	for (i = 0; i < strlen (cur_path); i ++)
		if (cur_path[i] == '/')
			putchar (' ');
		else
			putchar (cur_path[i]);
	printf ("%s", str2);
}
