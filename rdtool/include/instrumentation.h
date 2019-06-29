#ifndef _INSTRUMENTATION_H_
#define _INSTRUMENTATION_H_

void enable_checking();
void disable_checking();

extern "C" void cilk_tool_init(void);
extern "C" void cilk_tool_destroy(void);
extern "C" void cilk_enter_begin();
extern "C" void cilk_enter_helper_begin(__cilkrts_stack_frame* sf,
                                        void* this_fn, void* rip);
extern "C" void cilk_enter_end(__cilkrts_stack_frame *sf, void *rsp);
extern "C" void cilk_steal_success(__cilkrts_worker* w, __cilkrts_worker* victim,
                                   __cilkrts_stack_frame* sf);
extern "C" void cilk_detach_begin(__cilkrts_stack_frame *parent_sf);
extern "C" void cilk_detach_end();
extern "C" void cilk_sync_begin(__cilkrts_stack_frame* sf);
extern "C" void cilk_sync_end();
extern "C" void cilk_leave_begin(__cilkrts_stack_frame* sf);
extern "C" void cilk_leave_end();
extern "C" void __attribute__((noinline))
cilk_leave_stolen(__cilkrts_worker* w,
                  __cilkrts_stack_frame *saved_sf,
                  int is_original,
                  char* stack_base);
extern "C" void cilk_sync_abandon(__cilkrts_stack_frame* sf);
extern "C" void cilk_done_with_stack(__cilkrts_stack_frame *sf_at_sync,
                                     char* stack_base);
extern "C" void cilk_continue(__cilkrts_stack_frame* sf, char* new_sp);
extern "C" void iteration_end(uint64_t flag);
extern "C" void clear_shadow_memory(size_t start, size_t end);

extern "C" void do_tool_print(void);
extern "C" void do_tool_init(void);
#endif
