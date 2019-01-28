void datablock_command(void);
char **get_datablock(char *name);
char *parse_datablock_name(void);
void gpfree_datablock(struct value *datablock_value);
void append_to_datablock(struct value *datablock_value, const char * line);
void append_multiline_to_datablock(struct value *datablock_value, const char * lines);
int datablock_size(struct value *datablock_value);
