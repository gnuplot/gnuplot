/*
 * $Id: datablock.h,v 1.1 2012/06/19 18:11:05 sfeam Exp $
 */
void datablock_command __PROTO((void));
char **get_datablock __PROTO((char *name));
char *parse_datablock_name __PROTO((void));
void gpfree_datablock __PROTO((struct value *datablock_value));
void append_to_datablock(struct value *datablock_value, char * line);
