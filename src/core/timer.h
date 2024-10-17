#ifndef __FU_TIMER_H__
#define __FU_TIMER_H__

#include "main.h"
#include "object.h"

FU_DECLARE_TYPE(FUTimer, fu_timer)
#define FU_TYPE_TIMER (fu_timer_get_type())

uint64_t fu_timer_get_stmp();

FUTimer* fu_timer_new();
void fu_timer_start(FUTimer* timer);
uint64_t fu_timer_measure(FUTimer* timer);

FUSource* fu_timeout_source_new_microseconds(unsigned dur);
#define fu_timeout_source_new(ms) fu_timeout_source_new_microseconds(ms * 1000)

char* fu_get_current_time_utc();
char* fu_get_current_time_utc_format(const char* format);

char* fu_get_current_time_local();
char* fu_get_current_time_local_format(const char* format);
#endif /* __G_STRING_H__ */
