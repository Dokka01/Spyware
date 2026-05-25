#ifndef BROWSER_HISTORY_H
#define BROWSER_HISTORY_H

/* Remplit out avec un tableau JSON des 50 dernières URLs Chrome.
   Format : [{"url":"...","title":"...","visits":N}, ...] */
void get_browser_history(char *out, size_t out_size);

#endif /* BROWSER_HISTORY_H */
