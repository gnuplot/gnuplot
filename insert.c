/*
 * Program	: help
 * Module	: insert.c
 * Programmer	: R. Stolfa
 *
 * Purpose :	To uniquely insert a basename and help topic to the
 *		specified list
 *
 * Modification History:
 *   07/13/88	Created
 */

#include	"global.h"

insert (cmd, basename, subject, command)
int	cmd;
char	*basename,
	*subject,
	*command;
{
	struct	LIST	*new, *p;

	if ((strlen (basename) == 0) ||
	    (strlen (subject) == 0) ||
	    (cmd < 0) || (cmd >= 3))
		/*
		 * Bad invocation of "insert()"
		 */
		return;

	/*
	 * Build the basic LIST structure for the new
	 * entry
	 */
	new = (struct LIST *)
		malloc (sizeof (struct LIST));
	strcpy (new->base, basename);
	strcpy (new->topic, subject);
	strcpy (new->cmd, command);

	/*
	 * Prepend the new element onto the correct list
	 */
	p = _list[cmd];
	new->prev = _list[cmd];

	/*
	 * Check for uniqueness
	 */
	for (; p != NULL; p = p->prev) {
		if (strcmp (new->base, p->base) == 0) {
			free (new);
			return;
			/* NOT REACHED */
		}
	}

	/*
	 * If we get to here, we have a new item.  Fix the master
	 * pointer & go on.
	 */
	_list[cmd] = new;
}
