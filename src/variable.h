
/* The death of global variables - part one. */

#ifndef VARIABLE_H
# define VARIABLE_H

#ifdef ACTION_NULL
# undef ACTION_NULL
#endif

#ifdef ACTION_INIT
# undef ACTION_INIT
#endif

#ifdef ACTION_SHOW
# undef ACTION_SHOW
#endif

#ifdef ACTION_SET
# undef ACTION_SET
#endif

#ifdef ACTION_GET
# undef ACTION_GET
#endif

#ifndef ACTION_SAVE
# undef ACTION_SAVE
#endif

#ifdef ACTION_CLEAR
# undef ACTION_CLEAR
#endif

#define ACTION_NULL   0
#define ACTION_INIT   (1<<0)
#define ACTION_SHOW   (1<<1)
#define ACTION_SET    (1<<2)
#define ACTION_GET    (1<<3)
#define ACTION_SAVE   (1<<4)
#define ACTION_CLEAR  (1<<5)

extern char *loadpath_handler __PROTO((int, char *));

#define init_loadpath()    loadpath_handler(ACTION_INIT,NULL)
#define show_loadpath()    loadpath_handler(ACTION_SHOW,NULL)
#define set_loadpath(path) loadpath_handler(ACTION_SET,(path))
#define get_loadpath()     loadpath_handler(ACTION_GET,NULL)
#define save_loadpath()    loadpath_handler(ACTION_SAVE,NULL)
#define clear_loadpath()   loadpath_handler(ACTION_CLEAR,NULL)

#endif /* VARIABLE_H */
