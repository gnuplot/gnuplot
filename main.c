/*
 * Program	: help
 * Module	: main.c
 * Programmer	: R. Stolfa
 *
 * Purpose :	To support a VMS-like help facility
 *
 * Modification History:
 *   08/26/87	Created
 *   07/13/88	Fixed end-of-program detection to work correctly
 */

#define		MAIN
#include	"global.h"

main (argc, argv)
int	argc;
char	*argv[];
{
	int	done;
    char *helpdir, *getenv();

        --argc; ++argv;

        if ((helpdir = getenv ("HELPDIR")) == NULL)
           strcpy(Root_Dir, ROOTDIR);
        else
           strcpy (Root_Dir, helpdir);

        if (argc >= 2 && strcmp(*argv,"-r") == 0) {	/* Absolute directory */
           strcpy(Root_Dir, *++argv);
           ++argv;
           argc += 2;
        }
				
        while (argc--) {		/* Construct relative root directory */
           strcat(Root_Dir,"/");
           strcat(Root_Dir, *argv);
           ++argv;
        }

	initialize();
	done = FALSE;

	while (done != TRUE) {
		/*
		 * Free memory to keep user memory from growing
		 */
		free_list (PRINT);
		free_list (ACRON);
		free_list (TOPIC);

		/*
		 * If we are recursing out of the help tree,
		 * do not print the help stuff...
		 */
		lines = 0;
		if (done != UP)
			help();
		scan_topics ();
		if (done != UP)
			format_help ();
		done = input_choice ();

		if ((done == UP) && (strcmp (Path, Root_Dir) == 0))
			done = TRUE;
	}
	printf ("\n");
exit(0);
}
