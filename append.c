/*
 * Program	: help
 * Module	: append.c
 * Programmer	: R. Stolfa
 *
 * Purpose :	To append a basename and help topic to the
 *		specified list
 *
 * Modification History:
 *   08/27/87	Created
 *   08/31/87	Changed exit on default to be a call to "catch()"
 *	-	Streamlined the building of nodes
 */

#include	"global.h"

append (cmd, basename, subject, command)
int	cmd;
char	*basename,
	*subject,
	*command;
{
	struct	LIST	*new;

	if ((strlen (basename) == 0) ||
	    (strlen (subject) == 0) ||
	    (cmd < 0) || (cmd >= 3))
		/*
		 * Bad invocation of "append()"
		 */
		return;

	/*
	 * Build the basic LIST structure for the new
	 * entry
	 */
	new = (struct LIST *)
		malloc (sizeof (struct LIST));
  if(new==NULL){
     fprintf(stderr,"Can't malloc %d bytes\n",sizeof (struct LIST));
     exit(1);
  }
	strcpy (new->base, basename);
	strcpy (new->topic, subject);
	strcpy (new->cmd, command);

	/*
	 * Append the new element onto the correct list
	 */
	new->prev = _list[cmd];
	_list[cmd] = new;
}
