/*
 * Program	: help
 * Module	: input_choice.c
 * Programmer	: R. Stolfa
 *
 * Purpose :	To selectively change the current help subject
 *		based on what topic the user chooses to learn
 *		about next.
 *
 * Modification History:
 *   08/26/87	Created
 */

#include	"global.h"

input_choice ()
{
	int	done,			/* need to parse DIRFILE again */
		i,			/* temp */
		j,			/* temp */
		count,			/* num. of acronym topics that mached */
		topics;			/* num. of topics at this level */
	char	buff[BSIZE],		/* input buffer */
		tmp_path[BSIZE];	/* holding place for cur_path */
	struct	LIST	*p;		/* temp */

	done = FALSE;
	do {
		present ("HELP ", " > ");

		if (fgets (buff, BSIZE, stdin) == NULL)
			/*
			 * End help on EOF
			 */
			return (TRUE);

		/*
		 * Strip junk out of line
		 */
		for (i = 0, j = 0; i < strlen(buff); i ++) {
			if (buff[i] == '\n')
				buff[i] = '\0';
			if (!isspace(buff[i]))
				buff[j++] = toupper(buff[i]);
		}

		if (strlen(buff) == 0) {
			/*
			 * At this point, we have a request to recurse
			 * back out of the help tree by one level.
			 */
			for (i = strlen (cur_path); cur_path[i] != '/'; --i)
				;
			cur_path[i] = '\0';
			return (UP);
			/* NOT REACHED */
		}

		/*
		 * OK.  We have the topic that the user has requested.
		 * Now let's try to find some reference to it
		 */
		count = 0;
		topics = 0;
		free_list (TOPIC);
		for (p = acr_list; p != NULL ; p = p->prev) {
			if (strncmp (buff, p->topic, strlen(buff)) == 0) {
				insert (TOPIC,
					p->base, p->topic, p->cmd);
				count ++;
			}
			topics ++;
		}

		if (count == 0) {
			if (strcmp (buff, "?") != 0) {
				present ("Sorry, no documentation on ", " ");
				printf ("%s\n", buff);
			}
			if (topics > 0) {
				printf ("Additional information available:\n");
				lines = 2;
				format_help();
			}
			done = FALSE;
		} else if (count == 1) {
			if (top_list->cmd[0] == '\0') {
			   /*
			    * We have only one help subtopic, so traverse
			    * the tree down that link.
			    */
			   sprintf (cur_path, "%s/%s", cur_path,
				   top_list->base);
			   done = TRUE;
                           }
			else {
                           system(top_list->cmd);
                           return (UP);
                           }
		} else {
			/*
			 * We have several matches.  Therefore, page the
			 * HELPFILE for each to the screen and stay where
			 * we are.
			 */
			lines = 0;
			strcpy (tmp_path, cur_path);
			for (p = top_list; p != NULL ; p = p->prev) {
				if (p->cmd[0] == '\0') {
				   sprintf (cur_path, "%s/%s", tmp_path,
					   p->base);
				   gen_path(HELPFILE);
				   help();
				   strcpy (cur_path, tmp_path);
				   }
			}
		}

	} while (done != TRUE);
	return (FALSE);
}
