#ifndef COMMON_H
#define COMMON_H

#include <glib.h>

void log_handler(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message,
                 gpointer user_data);

#endif //COMMON_H
