/*
 * Program	: help
 * Module	: scan_topics.c
 * Programmer	: R. Stolfa
 *
 * Purpose :	To scan the current directory for all "topic"
 *		directories in the DIRFILE file.
 *
 * Modification History:
 *   08/26/87	Created
 *   08/31/87	Changed input routine to change spaces in the topic
 *		field to be underscores.
 */

#include	"global.h"

scan_topics ()
{
	FILE	*fd;			/* DIRFILE descriptor */
	int	i,j,k,			/* temp */
		cmd,			/* directed to man page? */
		count;			/* is there any help? */
	char	buff[BSIZE],		/* for reading DIRFILE */
		help_topic[BSIZE],	/* used to parse DIRFILE lines */
		base_path[BSIZE],	/* used to parse DIRFILE lines */
		cmd_buf[BSIZE],		/* used to parse DIRFILE lines */
		prt_flag;		/* used to parse DIRFILE lines */

	count = 0;
	gen_path(DIRFILE);

	if ((fd = fopen (Path, "r")) == NULL) {
		printf ("There are no subtopics for this area.\n");
		return;
	}

	/*
	 * Here we need to read in the lines in DIRFILE
	 * that are of the format
	 * <basename><print_flag><help_topic_string>
	 * and capitalize the <help_topic_string>.
	 *
	 * if <print_flag> is a "*" then the <help_topic_string> is
	 * for viewing.
	 *
	 * if <print_flag> is a ":" then it is an acronym for lookups.
	 *
	 * if find "!" before <print_flag> then there's a command.
	 */

	while (fgets (buff, BSIZE, fd) != NULL) {

		cmd = 0;
		k = j = 0;
		for (i = 0;
		     i < strlen(buff) && buff[i] != ':' && buff[i] != '*';
		     i ++) {
			if (buff[i] == '!')
			   ++cmd;
			else if (!cmd)
			   base_path[j++] = buff[i];
			else
			   cmd_buf[k++] = buff[i];
			}
		base_path[j] = '\0';
		cmd_buf[k]   = '\0';

		if (i < strlen (buff))
			prt_flag = buff[i];
		else
			/* Bad input line */
			continue;

		strcpy (help_topic, &buff[i+1]);
		for (i = 0; i < strlen (help_topic); i ++) {
			help_topic[i] = toupper (help_topic[i]);
			if (help_topic[i] == ' ')
				help_topic[i] = '_';
			if (help_topic[i] == '\n')
				help_topic[i] = '\0';
		}

		/*
		 * At this point, we have a fairly legal line,
		 * so, let's finish it off...
		 */

		if ((strlen (base_path) == 0) || (strlen (help_topic) == 0))
			continue;
		count ++;

		if (prt_flag == '*')
			/*
			 * Append this line to the list of things to
			 * output as topics
			 */
			append (PRINT, base_path, help_topic, cmd_buf);

		/*
		 * Append this line to the list of acronymns
		 * for reference later...
		 */
		append (ACRON, base_path, help_topic, cmd_buf);
	}

	fclose (fd);

	if (count == 0) {
		printf ("There are no subtopics for this area.\n");
		return;
	}
}
