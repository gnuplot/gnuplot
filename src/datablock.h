/*
 * $Id: datablock.h,v 1.2 2014/03/23 13:27:27 markisch Exp $
 */
void datablock_command __PROTO((void));
char **get_datablock __PROTO((char *name));
char *parse_datablock_name __PROTO((void));
void gpfree_datablock __PROTO((struct value *datablock_value));
void append_to_datablock(struct value *datablock_value, const char * line);
