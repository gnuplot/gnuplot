void datablock_command __PROTO((void));
char **get_datablock __PROTO((char *name));
char *parse_datablock_name __PROTO((void));
void gpfree_datablock __PROTO((struct value *datablock_value));
void append_to_datablock __PROTO((struct value *datablock_value, const char * line));
void append_multiline_to_datablock(struct value *datablock_value, const char * lines);
int datablock_size __PROTO((struct value *datablock_value));
