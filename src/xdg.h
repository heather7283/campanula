#ifndef SRC_XDG_H
#define SRC_XDG_H

const char *get_xdg_config_dir(void);
const char *get_xdg_cache_dir(void);
const char *get_xdg_data_dir(void);

bool mkdir_with_parents(const char *path);

#endif /* #ifndef SRC_XDG_H */

