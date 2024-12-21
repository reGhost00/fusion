#pragma once

#include "main.h"
#include "object.h"

typedef struct _FUError {
    int code;
    char* message;
} FUError;

typedef enum _EFURandAlgorithm {
    EFU_RAND_ALGORITHM_DEFAULT,
    EFU_RAND_ALGORITHM_LCG,
    EFU_RAND_ALGORITHM_MT,
    EFU_RAND_ALGORITHM_CNT
} EFURandAlgorithm;

FU_DECLARE_TYPE(FURand, fu_rand)
#define FU_TYPE_RAND (fu_rand_get_type())

FU_DECLARE_TYPE(FUTimer, fu_timer)
#define FU_TYPE_TIMER (fu_timer_get_type())
//
//  error

FUError* fu_error_new_take(int code, char** msg);
FUError* fu_error_new_from_code(int code);
void fu_error_free(FUError* err);
//
//  random

FURand* fu_rand_new(EFURandAlgorithm type);
int fu_rand_int_range(FURand* self, int min, int max);
//
//  timer

FUTimer* fu_timer_new();
void fu_timer_start(FUTimer* timer);
uint64_t fu_timer_measure(FUTimer* timer);
//
//  timeout source

FUSource* fu_timeout_source_new_microseconds(unsigned dur);
#define fu_timeout_source_new(ms) fu_timeout_source_new_microseconds(ms * 1000)

char* fu_get_current_time_utc();
char* fu_get_current_time_utc_format(const char* format);

char* fu_get_current_time_local();
char* fu_get_current_time_local_format(const char* format);

char* fu_base64_encode(const uint8_t* data, size_t len);
uint8_t* fu_base64_decode(const char* data, size_t* len);