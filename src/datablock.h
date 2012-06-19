/*
 * $Id: datablock.h,v 0.001 2012/06/08 00:17:22 sfeam Exp $
 */
void datablock_command __PROTO((void));
char **get_datablock __PROTO((char *name));
char *parse_datablock_name __PROTO((void));
void gpfree_datablock __PROTO((struct value *datablock_value));
