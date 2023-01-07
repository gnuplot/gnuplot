/* GNUPLOT - loadpath.h */

#ifndef LOADPATH_H
# define LOADPATH_H

void init_loadpath(void);
void clear_loadpath(void);
void dump_loadpath(void);
void set_var_loadpath(char *path);
char *get_loadpath(void);
char *save_loadpath(void);

#endif /* LOADPATH_H */
