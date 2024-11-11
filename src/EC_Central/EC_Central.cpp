#include <iostream>
#include "../Common/common.h"
#include <librdkafka/rdkafkacpp.h>
#include <glib.h>

using namespace std;

int main(int argc, char *argv[]) {
    g_log_set_default_handler(log_handler, nullptr);

    g_message("This is a message");
    g_warning("This is a warning");
    g_critical("This is a critical message");
    g_debug("This is a debug message");
    g_info("This is an info message");
    g_debug("This is a debug message");
    g_error("This is an error message");

    return 0;
}
