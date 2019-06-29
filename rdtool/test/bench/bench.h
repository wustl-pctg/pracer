#ifndef _BENCH_H
#define _BENCH_H

#include <iostream>
#include <chrono>

// This is a hack. I'm sharing the rd.h file between tests and the
// actual library source, so this is a separate file. Ideally the two
// should be split, and this file should be merged with one in
// $(BASE_DIR)/include.

#if defined RACEDETECT || defined INSERTSONLY
#define RD_ENABLE __om_enable_checking()
#define RD_DISABLE __om_disable_checking()

#include "rd.h"
#include "../src/print_addr.h"
#else
#define RD_ENABLE
#define RD_DISABLE
#endif

#ifdef INSERTSONLY
extern "C" void __tsan_init();
void init_inserts_only(void) __attribute__((constructor));
void init_inserts_only(void)
{
  __tsan_init();
  RD_DISABLE;
  __om_disable_instrumentation();
}

void destroy_inserts_only(void) __attribute__((destructor));
void destroy_inserts_only(void)
{
  cilk_tool_destroy();
}
#endif

#ifdef RACEDETECT
void destroy_rd(void) __attribute__((destructor));
void destroy_rd(void)
{
  assert(get_num_races_found() == 0);
  cilk_tool_destroy();
}
#endif

/* #ifdef STATS */
/* #include "stattool.h" */
/* void print_final_stats(void) __attribute__((destructor)); */
/* void print_final_stats(void) {} */
/* #endif */

#endif
