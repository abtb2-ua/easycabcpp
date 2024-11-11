#include "Common.h"
#include <iostream>
#include <glib.h>
#include <cstdlib>
#include <sys/time.h>

using namespace std;


void log_handler(const gchar *log_domain, const GLogLevelFlags log_level, const gchar *message,
                 gpointer user_data) {
    const char *GREEN = "\x1b[38;5;156m";
    const char *RESET = "\x1b[0m";
    const char *BOLD = "\x1b[1m";
    const char *BLUE = "\x1b[38;5;75m";
    const char *ORANGE = "\x1b[38;5;215m";
    const char *PURPLE = "\x1b[38;5;135m";
    const char *RED = "\x1b[38;5;9m";
    const char *PINK = "\x1b[38;5;219m";

    if ((log_level & G_LOG_LEVEL_MASK) == G_LOG_LEVEL_DEBUG) {
        if (getenv("G_MESSAGES_DEBUG") == nullptr)
            return;
    }

    struct timeval now = {};
    gettimeofday(&now, nullptr);
    const int hours = static_cast<int>(now.tv_sec / 3600 % 24) + 2; // UTC + 2
    const int minutes = static_cast<int>(now.tv_sec / 60 % 60);
    const int seconds = static_cast<int>(now.tv_sec) % 60;
    const int milliseconds = static_cast<int>(now.tv_usec) / 1000;

    printf("%s[%02i:%02i:%02i.%03i]%s ", BLUE, hours, minutes, seconds, milliseconds, RESET);

    switch (log_level & G_LOG_LEVEL_MASK) {
        case G_LOG_LEVEL_CRITICAL:
            printf("%s** CRITICAL **%s", PURPLE, RESET);
        break;
        case G_LOG_LEVEL_WARNING:
            printf("%s** WARNING **%s", ORANGE, RESET);
        break;
        case G_LOG_LEVEL_MESSAGE:
            printf("%s%sMessage%s", GREEN, BOLD, RESET);
        break;
        case G_LOG_LEVEL_DEBUG:
            printf("%s%sDebug%s", PINK, BOLD, RESET);
        break;
        case G_LOG_LEVEL_INFO:
            printf("%s%sInfo%s", BLUE, BOLD, RESET);
        break;
        case G_LOG_LEVEL_ERROR:
            printf("%s** ERROR **%s", RED, RESET);
        break;

        default:
            break;
    }

    printf(": %s\n", message);

    if (log_level == 6 || log_level == G_LOG_LEVEL_ERROR) {
        exit(1);
    }
}

