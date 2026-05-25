#ifndef INSTALLED_APPS_H
#define INSTALLED_APPS_H

/* Remplit out avec un tableau JSON des applications installées.
   Format : [{"name":"...","version":"..."}, ...] */
void get_installed_apps(char *out, size_t out_size);

#endif /* INSTALLED_APPS_H */
