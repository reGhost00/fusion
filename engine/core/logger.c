#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// custom
#include "_base.h"
#include "logger.h"

static void fu_log_default_handler(EFULogLevel level, const char* buffer)
{
    if (EFU_LOG_LEVEL_CRITICAL == level) {
        printf("\x1b[1;41m%s\x1b[0;49m", buffer);
        abort();
    }
    if (EFU_LOG_LEVEL_ERROR == level) {
        printf("\x1b[91;49m%s\x1b[0m", buffer);
        return;
    }
    if (EFU_LOG_LEVEL_WARNING == level) {
        printf("\x1b[93;49m%s\x1b[0m", buffer);
        return;
    }
    if (EFU_LOG_LEVEL_INFO == level) {
        printf("\x1b[96;49m%s\x1b[0m", buffer);
        return;
    }
    puts(buffer);
}

static FULogFunc defLogHandler = fu_log_default_handler;
/**
 * g_log:
 * @log_domain: (nullable): the log domain, usually `G_LOG_DOMAIN`, or `NULL`
 *   for the default
 * @log_level: the log level, either from [type@GLib.LogLevelFlags]
 *   or a user-defined level
 * @format: the message format. See the `printf()` documentation
 * @...: the parameters to insert into the format string
 *
 * Logs an error or debugging message.
 *
 * If the log level has been set as fatal, [func@GLib.BREAKPOINT] is called
 * to terminate the program. See the documentation for [func@GLib.BREAKPOINT] for
 * details of the debugging options this provides.
 *
 * If [func@GLib.log_default_handler] is used as the log handler function, a new-line
 * character will automatically be appended to @..., and need not be entered
 * manually.
 *
 * If [structured logging is enabled](logging.html#using-structured-logging) this will
 * output via the structured log writer function (see [func@GLib.log_set_writer_func]).
 */
void fu_log(EFULogLevel level, const char* format, ...)
{
    static const char* defLevelPrefix[] = { "CRITICAL", "ERROR", "WARNING", NULL };
    static char buf1[3991];
    static char buf2[4011];
    va_list args;
    va_start(args, format);

    if (FU_UNLIKELY(!defLogHandler))
        defLogHandler = fu_log_default_handler;
    memset(buf1, 0, sizeof(buf1));
    vsnprintf(buf1, sizeof(buf1) - 1, format, args);
    if (EFU_LOG_LEVEL_WARNING < level) {
        defLogHandler(level, buf1);
        va_end(args);
        return;
    }

    memset(buf2, 0, sizeof(buf2));
    snprintf(buf2, sizeof(buf2) - 1, "[%s] %s", defLevelPrefix[level], buf1);
    defLogHandler(level, buf2);
    va_end(args);
}

void fu_log_set_handler(FULogFunc handler)
{
    fu_return_if_fail(handler);
    defLogHandler = handler;
}

void fu_log_reset_handler()
{
    defLogHandler = fu_log_default_handler;
}
