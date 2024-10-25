#pragma once

/** @brief Represents levels of logging */
typedef enum _EFULogLevel {
    EFU_LOG_LEVEL_CRITICAL,
    EFU_LOG_LEVEL_ERROR,
    EFU_LOG_LEVEL_WARNING,
    EFU_LOG_LEVEL_INFO,
    EFU_LOG_LEVEL_DEBUG,
    EFU_LOG_LEVEL_CNT
} EFULogLevel;

// A function pointer for a console to hook into the logger.
typedef void (*FULogFunc)(EFULogLevel level, const char* message);

void fu_log_set_handler(FULogFunc handler);
void fu_log_reset_handler();

void fu_log(EFULogLevel level, const char* format, ...);

#define fu_critical(message, ...) fu_log(EFU_LOG_LEVEL_CRITICAL, message, ##__VA_ARGS__);
#define fu_error(message, ...) fu_log(EFU_LOG_LEVEL_ERROR, message, ##__VA_ARGS__);

#ifndef NDEBUG
#define fu_warning(message, ...) fu_log(EFU_LOG_LEVEL_WARNING, message, ##__VA_ARGS__);
#define fu_info(message, ...) fu_log(EFU_LOG_LEVEL_INFO, message, ##__VA_ARGS__);
#define fu_debug(message, ...) fu_log(EFU_LOG_LEVEL_DEBUG, message, ##__VA_ARGS__);
#else
#define fu_warning(message, ...)
#define fu_info(message, ...)
#define fu_debug(message, ...)
#endif
