#include <dlfcn.h>
#include <stdlib.h>
#include "piper_rd.h"
#include "debug_util.h"
//#include "stack.h"
#include "shadow_mem.h"
#include "mem_access.h"
#include <stdio.h>
#include "instrumentation.h"

//#define TRACEINFO

__thread static volatile int checking_disabled = 1;
//__thread __cilkrts_worker* t_worker = NULL;
//__thread int self = -1;
//static bool check_enable_instrumentation = true;
//__thread static bool clear_stack = false;
static ShadowMem<MemAccessList_t> shadow_mem;
//static bool instrumentation = false;
//local_mut printMut;

size_t new_memaccess[32];

/*
typedef struct FrameData_s {
    om_node* current_english;
    om_node* current_hebrew;
    om_node* cont_english;
    om_node* cont_hebrew;
    om_node* sync_english;
    om_node* sync_hebrew;
    uint32_t flags;
} FrameData_t;*/

//AtomicStack_t<FrameData_t>* frames = NULL;

/*
void print(__cilkrts_stack_frame* sf) {
    __cilkrts_worker *w = __cilkrts_get_tls_worker();
    if (w != NULL) {
        printf("worker: %d ", w->self);
    }
    printf("sf: %ld ", (long)sf);
    if (sf != NULL) {
        printf("sf parent: %ld ", (long)sf->call_parent);
    }
    printf("low watermark: %lu ", stack_low_watermark);
    printf("\n");
}*/


// clear the memory block at [start-end) (end is exclusive).
extern "C" void
clear_shadow_memory(size_t start, size_t end) {
    return;
    if (start == size_t(-1) && end == size_t(-1)) return;
    //fprintf(stderr, "clear shadow memory\n");
    //fprintf(stderr, "start key %zu end key %zu\n", ADDR_TO_KEY(start), ADDR_TO_KEY(end));
    //fprintf(stderr, "start %zu end %zu\n", (start), (end));
    //fprintf(stderr, "iter: %lld stage: %lld\n", g_curIterNum, g_curStageNum);
    om_assert(ALIGN_BY_NEXT_MAX_GRAIN_SIZE(end) == end); 
    assert(start <= end);

    while(start != end) {
        shadow_mem.erase( ADDR_TO_KEY(start) );
        start += MAX_GRAIN_SIZE;
        //if (start == end - 16)
        //    fprintf(stderr, "%zu %zu\n", start, end);
        //if (start == end)
        //    fprintf(stderr, "%zu %zu\n", start, end); 
        //if (start == end + 16)
        //    fprintf(stderr, "%zu %zu\n", start, end);
        //fprintf(stderr, "start %zu end %zu\n", start, end);
    }
    shadow_mem.erase(ADDR_TO_KEY(end)); //include end
}

//static volatile bool g_flag = false;
//static uint64_t address;
extern "C" void iteration_end(uint64_t flag) {
    //log the low_watermark
    //g_rdc.setHighWatermark(g_curIterNum, flag & ((uint64_t)(-1) << 4));
    uint64_t res = (uint64_t)__builtin_frame_address(0);
    //fprintf(stderr, "iteration end res:  %zu\n", res);
    //fprintf(stderr, "iteration end flag: %zu\n", ADDR_TO_KEY((flag & ((uint64_t)(-1) << 4))));
    //fprintf(stderr, "indexCur: %d\n", indexCur);
    if (g_rdc.getLowWatermark(g_curIterNum) > res)
        g_rdc.setLowWatermark(g_curIterNum, res);

    //address = g_rdc.getHighWatermark(g_curIterNum) + 16;
    //g_flag = true;
    //fprintf(stderr, "address===========%zu\n", address);
    //fprintf(stderr, "address update: %zu\n", res);
    //clear the shadow memory here for current iteration     
/*  int address = 0;
    unsigned long long low_watermark = g_rdc.getLowWatermark(g_curIterNum);
    if (low_watermark > &address)
        low_watermark = &address;

    clear_shadow_memory(low_watermark, g_rdc.getHighWatermark(g_curIterNum));*/
}

/*
__attribute__((always_inline))
static void enable_instrumentation() {
    DBG_TRACE(DEBUG_BASIC, "Enable instrumentation.\n");
    instrumentation = true;
}

__attribute__((always_inline))
static void disable_instrumentation() {
    DBG_TRACE(DEBUG_BASIC, "Disable instrumentation.\n");
    instrumentation = false;
}*/

__attribute__((always_inline))
void enable_checking() {
#ifdef PRINTINFO
    //fprintf(stderr, "enable checking\n");
#endif
    checking_disabled = 0;
    om_assert(checking_disabled >= 0);
    //__sync_synchronize();
}

__attribute__((always_inline))
void disable_checking() {
#ifdef PRINTINFO
    //fprintf(stderr, "disable checking\n");
#endif
    om_assert(checking_disabled >= 0);
    checking_disabled = 1;
    //__sync_synchronize();
}


__attribute__((always_inline))
static bool should_check() {
    return (checking_disabled == 0);
}

/*
typedef void*(*malloc_t)(size_t);
static malloc_t real_malloc = NULL;


extern "C" void* malloc(size_t s) {

    // DBG_TRACE(DEBUG_CALLBACK, "malloc called.\n");
    // make it 8-byte aligned; easier to erase from shadow mem
    uint64_t new_size = ALIGN_BY_NEXT_MAX_GRAIN_SIZE(s);
    
    // Don't try to init, since that needs malloc.  
    if (real_malloc==NULL) {
        real_malloc = (malloc_t)dlsym(RTLD_NEXT, "malloc");
        char *error = dlerror();
        if (error != NULL) {
          fputs(error, stderr);
          fflush(stderr);
          abort();
        }
    }
    void *r = real_malloc(new_size);

    //  if (t_worker && t_worker && __cilkrts_get_batch_id(t_worker) != -1) return r;
    //if (t_worker && __cilkrts_get_batch_id(t_worker) != -1) return r;
    
    if(should_check() && r != NULL) {
        __cilkrts_worker *w = __cilkrts_get_tls_worker();
        
        if (w && __cilkrts_get_batch_id(w) != -1) return r;

        clear_shadow_memory((size_t)r, (size_t)r+new_size);
    }

    return r;
}*/

/*
typedef void*(*calloc_t)(size_t, size_t);
static calloc_t real_calloc = NULL;

extern "C" void* empty_calloc(size_t number, size_t size) {
    fprintf(stderr, "empty_calloc\n");
    return NULL;
}

extern "C" void* calloc(size_t number, size_t size) {
    
    // DBG_TRACE(DEBUG_CALLBACK, "malloc called.\n");
    // make it 8-byte aligned; easier to erase from shadow mem
    uint64_t new_size = ALIGN_BY_NEXT_MAX_GRAIN_SIZE(size * number);
    size = 8;
    number = new_size / size;

    // Don't try to init, since that needs malloc.  
    if (real_calloc==NULL) {
        real_calloc = empty_calloc;
        fprintf(stderr, "real_calloc %d\n", real_calloc);
        real_calloc = (calloc_t)dlsym(RTLD_NEXT, "calloc");
        fprintf(stderr, "real_calloc %d\n", real_calloc);
        char *error = dlerror();
        if (error != NULL) {
          fputs(error, stderr);
          fflush(stderr);
          abort();
        }
    }

    void *r = real_calloc(number, size);
    //  if (t_worker && t_worker && __cilkrts_get_batch_id(t_worker) != -1) return r;
    //if (t_worker && __cilkrts_get_batch_id(t_worker) != -1) return r;
    
    if(should_check() && r != NULL) {
        __cilkrts_worker *w = __cilkrts_get_tls_worker();
        
        if (w && __cilkrts_get_batch_id(w) != -1) return r;

        clear_shadow_memory((size_t)r, (size_t)((size_t)r+number*size));
    }

    return r;
}*/
/*
extern "C" void* __real_calloc(size_t, size_t);

extern "C" void* __wrap_calloc(size_t number, size_t size) {
    uint64_t new_size = ALIGN_BY_NEXT_MAX_GRAIN_SIZE(size * number);
    size = 8;
    number = new_size / size;

    void *r = __real_calloc(number, size);

     if(should_check() && r != NULL) {
        __cilkrts_worker *w = __cilkrts_get_tls_worker();
        
        if (w && __cilkrts_get_batch_id(w) != -1) return r;

        clear_shadow_memory((size_t)r, (size_t)((size_t)r+number*size));
    }
    //fprintf(stderr, "__wrap_calloc start addr %zu, %zu\n", ADDR_TO_KEY((size_t)r), ADDR_TO_KEY((size_t)((size_t)r+number*size)));
    return r; 
}

extern "C" void* __real_malloc(size_t);

extern "C" void* __wrap_malloc(size_t size) {
    uint64_t new_size = ALIGN_BY_NEXT_MAX_GRAIN_SIZE(size);

    void *r = __real_malloc(new_size);

     if(should_check() && r != NULL) {
        __cilkrts_worker *w = __cilkrts_get_tls_worker();
        
        if (w && __cilkrts_get_batch_id(w) != -1) return r;

        clear_shadow_memory((size_t)r, (size_t)((size_t)r + new_size));
    }
    //fprintf(stderr, "__wrap_malloc start addr %zu, %zu\n", ADDR_TO_KEY((size_t)r), ADDR_TO_KEY((size_t)((size_t)r + new_size)));
    return r;
}

extern "C" void* __real_realloc(void*, size_t);

extern "C" void* __wrap_realloc(void *p, size_t size) {
    //fprintf(stderr, "call __wrap_realloc size: %zu\n", size);
    uint64_t new_size = ALIGN_BY_NEXT_MAX_GRAIN_SIZE(size);

    void *r = __real_realloc(p, new_size);

     if(should_check() && r != NULL) {
        __cilkrts_worker *w = __cilkrts_get_tls_worker();
        
        if (w && __cilkrts_get_batch_id(w) != -1) return r;

        clear_shadow_memory((size_t)r, (size_t)((size_t)r + new_size));
    }
    //fprintf(stderr, "__wrap_realloc start addr %zu, %zu\n", ADDR_TO_KEY((size_t)r), ADDR_TO_KEY((size_t)((size_t)r + new_size)));
    return r;
}
//bool g_inited = false;
*/
/*
extern "C" void cilk_tool_init(void) {
#ifdef PRINTINFO
    std::cout << "cilk tool init" << std::endl;
#endif
   //init
    //int p = __cilkrts_get_nworkers();
    //frames = new AtomicStack_t<FrameData_t>[p];
    pthread_spin_init(&printMut.mut, PTHREAD_PROCESS_PRIVATE);
    g_inited = true;
}

extern "C" void cilk_tool_destroy(void) {
#ifdef PRINTINFO
    std::cout << "cilk tool destroy" << std::endl;
#endif
    //destroy (disable instrumentation)
    //delete[] frames;
    //disable_instrumentation();
}

extern "C" void cilk_enter_begin() {
#ifdef PRINTINFO
    std::cout << "cilk enter begin" << std::endl;
#endif
    if (!g_inited) return;
    //do enter begin
    //if (self == -1) return;
    //frames[self].push();
    //FrameData_t* f = frames[self].head();
    //f->flags = 0;
#ifdef TRACEINFO
    pthread_spin_lock(&printMut.mut);
    std::cout << "cilk enter begin" << std::endl;
    print(NULL);
    //frames[self].print(self, NULL);
    //std::cout << "watermark: " << stack_low_watermark << std::endl;
    pthread_spin_unlock(&printMut.mut);
#endif
}

extern "C" void cilk_enter_helper_begin(__cilkrts_stack_frame* sf,
                                        void* this_fn, void* rip) {
#ifdef PRINTINFO
    std::cout << "cilk enter helper begin" << std::endl;
#endif
    //do enter helper begin

#ifdef TRACEINFO
    pthread_spin_lock(&printMut.mut);


    std::cout << "cilk enter helper begin" << std::endl;
    print(sf);
    //frames[self].print(self, sf);
    //std::cout << "watermark: " << stack_low_watermark << std::endl;
    pthread_spin_unlock(&printMut.mut);
#endif
}

extern "C" void cilk_enter_end(__cilkrts_stack_frame *sf, void *rsp) {
#ifdef PRINTINFO
    std::cout << "cilk enter end" << std::endl;
#endif
    //if (!(sf->worker)) return;
    //__cilkrts_worker* w = sf->worker;

    //if (frames[w->self].empty()) {
    //    self = w->self;
    //    t_worker = w;

    //    frames[w->self].push();
    //    FrameData_t* f = frames[w->self].head();
    //    f->flags = 0;

    //    if (__builtin_expect(check_enable_instrumentation, 0)) {
    //        check_enable_instrumentation = false;
    //        enable_instrumentation();
    //        stack_low_watermark = (uint64_t)(-1);
    //    }
    //}
#ifdef TRACEINFO
    pthread_spin_lock(&printMut.mut);
    //print(sf);
    std::cout << "cilk enter end" << std::endl;
    print(sf);
    //frames[self].print(self, sf);
    //std::cout << "watermark: " << stack_low_watermark << std::endl;
    pthread_spin_unlock(&printMut.mut);
#endif
}

extern "C" void cilk_steal_success(__cilkrts_worker* w, __cilkrts_worker* victim,
                                   __cilkrts_stack_frame* sf) {
#ifdef PRINTINFO
    std::cout << "cilk steal success" << std::endl;
#endif
    //frames[w->self].reset();
    //FrameData_t* loot = frames[victim->self].steal_top(frames[w->self]);
    //self = w->self;
    //t_worker = w;
    //FrameData_t* f = frames[w->self].head();
    //f->flags = loot->flags;
#ifdef TRACEINFO
    pthread_spin_lock(&printMut.mut);

    std::cout << "cilk steal success" << std::endl;
    print(sf);
    //frames[self].print(self, sf);
    //std::cout << "watermark: " << stack_low_watermark << std::endl;
    pthread_spin_unlock(&printMut.mut);
#endif
    stack_low_watermark = (uint64_t)(-1);

#ifdef TRACEINFO
    pthread_spin_lock(&printMut.mut);
    print(sf);
    pthread_spin_unlock(&printMut.mut);
#endif
}

extern "C" void cilk_detach_begin(__cilkrts_stack_frame *parent_sf) {
#ifdef PRINTINFO
    std::cout << "cilk detach begin" << std::endl;
#endif
    //__cilkrts_worker* w = __cilkrts_get_tls_worker();
    //frames[w->self].push_helper();
#ifdef TRACEINFO
    pthread_spin_lock(&printMut.mut);

    std::cout << "cilk detach begin" << std::endl;
    print(parent_sf);
    //frames[self].print(self, parent_sf);
    //std::cout << "watermark: " << stack_low_watermark << std::endl;
    pthread_spin_unlock(&printMut.mut);
#endif
}

extern "C" void cilk_detach_end() {
#ifdef PRINTINFO
    std::cout << "cilk detach end" << std::endl;
#endif
#ifdef TRACEINFO
    pthread_spin_lock(&printMut.mut);

    std::cout << "cilk detach end" << std::endl;
    print(NULL);
    //frames[self].print(self, NULL);
    //std::cout << "watermark: " << stack_low_watermark << std::endl;
    pthread_spin_unlock(&printMut.mut);
#endif
}

extern "C" void cilk_sync_begin(__cilkrts_stack_frame* sf) {
#ifdef PRINTINFO
    std::cout << "cilk sync begin" << std::endl;
#endif
    //FrameData_t* f = frames[self].head();
    //assert(!(f->flags & FRAME_HELPER_MASK));
#ifdef TRACEINFO
    pthread_spin_lock(&printMut.mut);

    std::cout << "cilk sync begin" << std::endl;
    print(sf);
    //frames[self].print(self, sf);
    //std::cout << "watermark: " << stack_low_watermark << std::endl;
    pthread_spin_unlock(&printMut.mut);
#endif
}

extern "C" void cilk_sync_end() {
#ifdef PRINTINFO
    std::cout << "cilk sync end" << std::endl;
#endif
#ifdef TRACEINFO
    pthread_spin_lock(&printMut.mut);

    std::cout << "cilk sync end" << std::endl;
    print(NULL);
    //frames[self].print(self, NULL);
    //std::cout << "watermark: " << stack_low_watermark << std::endl;
    pthread_spin_unlock(&printMut.mut);
#endif
}

extern "C" void cilk_leave_begin(__cilkrts_stack_frame* sf) {
#ifdef PRINTINFO
    std::cout << "cilk leave begin" << std::endl;
#endif
    //if (frames[sf->worker->self].empty()) {
    //    check_enable_instrumentation = true;
    //    disable_instrumentation();
    //}

    if(sf->flags & CILK_FRAME_DETACHED) {
        // This is the point where we need to set flag to clear accesses to stack
        // out of the shadow memory.  The spawn helper of this leave_begin
        // is about to return, and we want to clear stack accesses below
        // (and including) spawn helper's stack frame.  Set the flag here
        // and the stack will be cleared in tsan_func_exit.
        clear_stack = true;
    }
#ifdef TRACEINFO
    pthread_spin_lock(&printMut.mut);

    std::cout << "cilk leave begin" << std::endl;
    print(sf);
    //frames[self].print(self, sf);
    //std::cout << "watermark: " << stack_low_watermark << std::endl;
    pthread_spin_unlock(&printMut.mut);
#endif
}

extern "C" void cilk_leave_end() {
#ifdef PRINTINFO
    std::cout << "cilk leave end" << std::endl;
#endif
    //if (frames[self].size() > 1) {
    //    frames[self].pop();
    //}
#ifdef TRACEINFO
    pthread_spin_lock(&printMut.mut);

    std::cout << "cilk leave end" << std::endl;
    print(NULL);
    //frames[self].print(self, NULL);
    //std::cout << "watermark: " << stack_low_watermark << std::endl;
    pthread_spin_unlock(&printMut.mut);
#endif
}*/

extern "C" void __attribute__((noinline))
cilk_leave_stolen(__cilkrts_worker* w,
                  __cilkrts_stack_frame *saved_sf,
                  int is_original,
                  char* stack_base) {
    if (clear_stack) {
        clear_shadow_memory(stack_low_watermark, stack_high_watermark);
        clear_stack = false;
    }
}

/*
extern "C" void __attribute__((noinline))
cilk_leave_stolen(__cilkrts_worker* w,
                  __cilkrts_stack_frame *saved_sf,
                  int is_original,
                  char* stack_base) {
#ifdef PRINTINFO
    std::cout << "cilk leave stolen" << std::endl;
#endif
    //assert(t_worker);
    
    if (instrumentation) {
        om_assert(clear_stack == true);
        uint64_t stack_high_watermark;
        /// @todo is the 'is_original' necessary?

        if (is_original)
            stack_high_watermark = (uint64_t)__builtin_frame_address(3);
        else
            stack_high_watermark = (uint64_t)stack_base;

        std::cout << "hig watermark: " << stack_high_watermark << std::endl;

        clear_shadow_memory(stack_low_watermark, stack_high_watermark);
        // We're jumping back to the runtime, but keep this in case we do a
        // provably good steal
        stack_low_watermark = stack_high_watermark;
        clear_stack = 0;
    }
#ifdef TRACEINFO
    pthread_spin_lock(&printMut.mut);

    std::cout << "cilk leave stolen" << std::endl;
    print(saved_sf);
    //frames[self].print(self, saved_sf);
    //std::cout << "watermark: " << stack_low_watermark << std::endl;
    pthread_spin_unlock(&printMut.mut);
#endif

    om_assert(clear_stack == true);
    uint64_t stack_high_watermark;
    /// @todo is the 'is_original' necessary?

    if (is_original)
        stack_high_watermark = (uint64_t)__builtin_frame_address(3);
    else
        stack_high_watermark = (uint64_t)stack_base;

    //std::cout << "hig watermark: " << stack_high_watermark << std::endl;

    clear_shadow_memory(stack_low_watermark, stack_high_watermark);
    // We're jumping back to the runtime, but keep this in case we do a
    // provably good steal
    stack_low_watermark = stack_high_watermark;
    clear_stack = 0;


    //if (frames[self].size() > 1)
    //    frames[self].pop();
#ifdef TRACEINFO
    pthread_spin_lock(&printMut.mut);
    print(saved_sf);
    //std::cout << "cilk leave stolen" << std::endl;
    //frames[self].print(self, saved_sf);
    //std::cout << "watermark: " << stack_low_watermark << std::endl;
    pthread_spin_unlock(&printMut.mut);
#endif
}*/

/*
extern "C" void cilk_sync_abandon(__cilkrts_stack_frame* sf) {
#ifdef PRINTINFO
    std::cout << "cilk sync abandon" << std::endl;
#endif
#ifdef TRACEINFO
    pthread_spin_lock(&printMut.mut);

    std::cout << "cilk sync abandon" << std::endl;
    print(sf);
    //frames[self].print(self, sf);
    //std::cout << "watermark: " << stack_low_watermark << std::endl;
    pthread_spin_unlock(&printMut.mut);
#endif
}

extern "C" void cilk_done_with_stack(__cilkrts_stack_frame *sf_at_sync,
                                     char* stack_base){
#ifdef PRINTINFO
    std::cout << "cilk done with stack" << std::endl;
#endif
    uint64_t stack_high_watermark = (uint64_t)stack_base;

    //std::cout << "hig watermark: " << stack_high_watermark << std::endl;
    clear_shadow_memory(stack_low_watermark, stack_high_watermark);
    stack_low_watermark = ((uint64_t)-1);
    clear_stack = 0;
#ifdef TRACEINFO
    pthread_spin_lock(&printMut.mut);

    std::cout << "cilk done with stack" << std::endl;
    print(sf_at_sync);
    //frames[self].print(self, sf_at_sync);
    //std::cout << "watermark: " << stack_low_watermark << std::endl;
    pthread_spin_unlock(&printMut.mut);
#endif
}

extern "C" void cilk_continue(__cilkrts_stack_frame* sf, char* new_sp) {
#ifdef PRINTINFO
    std::cout << "cilk continue" << std::endl;
#endif
    if (new_sp != NULL) {
        stack_low_watermark = (uint64_t) new_sp; //GET_SP(sf);
    }
#ifdef TRACEINFO
    pthread_spin_lock(&printMut.mut);

    std::cout << "cilk continue" << std::endl;
    print(sf);
    //frames[self].print(self, sf);
    //std::cout << "watermark: " << stack_low_watermark << std::endl;
    pthread_spin_unlock(&printMut.mut);
#endif
}

extern "C" void cilk_return_to_first_frame(__cilkrts_worker* w,
                                           __cilkrts_worker* team,
                                           __cilkrts_stack_frame* sf)
{
#ifdef PRINTINFO
    std::cout << "return to first frame" << std::endl;
#endif
    DBG_TRACE(DEBUG_CALLBACK,
              "%d: Transfering shadow stack %p to original worker %d.\n",
              w->self, sf, team->self);
    //pthread_spin_lock(&printMut.mut);
    //std::cout << "return to first frame1" << std::endl;
    //frames[self].print(self, sf);
    //frames[w->self].print(w->self, sf);
    //std::cout << "watermark: " << stack_low_watermark << std::endl;
    //pthread_spin_unlock(&printMut.mut);

    //std::cout << w->self << std::endl;
    //std::cout << team->self << std::endl;
    //if (w->self != team->self) frames[w->self].transfer(frames[team->self]);

    //pthread_spin_lock(&printMut.mut);
    //std::cout << "return to first frame2" << std::endl;
    //frames[self].print(self, sf);
    //std::cout << "watermark: " << stack_low_watermark << std::endl;
    //pthread_spin_unlock(&printMut.mut);
}

extern "C" void cilk_spawn_prepare() {
    //frames[self].print(self, NULL);
}

extern "C" void cilk_spawn_or_continue(int in_continuation) {
    //frames[self].print(self, NULL);
}*/

extern "C" void cilk_tool_init(void) {

}

extern "C" void cilk_tool_destroy(void) {

}

extern "C" void cilk_enter_begin() {

}

extern "C" void cilk_enter_helper_begin(__cilkrts_stack_frame* sf,
                                        void* this_fn, void* rip) {

}

extern "C" void cilk_enter_end(__cilkrts_stack_frame *sf, void *rsp) {

}

extern "C" void cilk_steal_success(__cilkrts_worker* w, __cilkrts_worker* victim,
                                   __cilkrts_stack_frame* sf) {

}

extern "C" void cilk_detach_begin(__cilkrts_stack_frame *parent_sf) {

}

extern "C" void cilk_detach_end() {

}

extern "C" void cilk_sync_begin(__cilkrts_stack_frame* sf) {

}

extern "C" void cilk_sync_end() {

}

extern "C" void cilk_leave_begin(__cilkrts_stack_frame* sf) {

}

extern "C" void cilk_leave_end() {

}

extern "C" void cilk_sync_abandon(__cilkrts_stack_frame* sf) {

}

extern "C" void cilk_done_with_stack(__cilkrts_stack_frame *sf_at_sync,
                                     char* stack_base){

}

extern "C" void cilk_continue(__cilkrts_stack_frame* sf, char* new_sp) {

}

extern "C" void cilk_return_to_first_frame(__cilkrts_worker* w,
                                           __cilkrts_worker* team,
                                           __cilkrts_stack_frame* sf)
{

}

extern "C" void cilk_spawn_prepare() {
}

extern "C" void cilk_spawn_or_continue(int in_continuation) {
}


void record_mem_helper(bool is_read, uint64_t inst_addr, uint64_t addr,
                  uint32_t mem_size)
{
    //fprintf(stderr, "record mem helper\n");
    //om_assert(self != -1);
    //FrameData_t *f = frames[self].head();
    MemAccessList_t *val = shadow_mem.find( ADDR_TO_KEY(addr) );
    //  MemAccess_t *acc = NULL;
    MemAccessList_t *mem_list = NULL;
    //fprintf(stderr, "val: %d key: %zu\n", val, ADDR_TO_KEY(addr)); 
    if( val == NULL ) {
        // not in shadow memory; create a new MemAccessList_t and insert
        //mem_list = new MemAccessList_t(addr, is_read, g_rdc.getDownFirstNode(g_curIterNum, g_curStageNum),
        //                               g_rdc.getRightFirstNode(g_curIterNum, g_curStageNum), inst_addr, mem_size);
        mem_list = new MemAccessList_t(addr, is_read, curDown, curRight, inst_addr, mem_size);
        //new_memaccess[__cilkrts_get_tls_worker()->self]++;
        val = shadow_mem.insert(ADDR_TO_KEY(addr), mem_list);
        // insert failed; someone got to the slot first;
        // delete new mem_list and fall through to check race with it
        if( val != mem_list ) { delete mem_list; }
        else { return; } // else we are done
    }
    // check for race and possibly update the existing MemAccessList_t
    om_assert(val != NULL);
    //val->check_races_and_update(is_read, inst_addr, addr, mem_size,
    //                            g_rdc.getDownFirstNode(g_curIterNum, g_curStageNum), g_rdc.getRightFirstNode(g_curIterNum, g_curStageNum));
    val->check_races_and_update(is_read, inst_addr, addr, mem_size, curDown, curRight);
}

static inline void tsan_read(void *addr, size_t mem_size, void *rip) {
//    return;
    if (should_check()) {
        disable_checking();
        //if (g_curIterNum == 1025) fprintf(stderr, "this is a reader: %zu %d %zu\n", (uint64_t)addr, g_flag, address);
        //if (g_flag && address == (uint64_t)addr) fprintf(stderr, "found it!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        //fprintf(stderr, "tsan_read at iter: %lld stage: %lld\n", g_curIterNum, g_curStageNum);
        //fprintf(stderr, "address key %zu\n", ADDR_TO_KEY(((uint64_t)(addr))));
        //const int start = ADDR_TO_MEM_INDEX(addr); 
        //const int grains = SIZE_TO_NUM_GRAINS(mem_size);
        //if ((start + grains) > NUM_SLOTS) {
        //    fprintf(stderr, "addr: %zu\n", (uint64_t)addr);
        //}
        //assert(start >= 0 && start < NUM_SLOTS && (start + grains) <= NUM_SLOTS);
//        reader++;
        record_mem_helper(true, (uint64_t)rip, (uint64_t)addr, mem_size);
        enable_checking();
    }
}

static inline void tsan_write(void *addr, size_t mem_size, void *rip) {
    //return;
    if (should_check()) {
        disable_checking();
        
        //if (g_curIterNum == 1025) fprintf(stderr, "this is a writer: %zu, %d, %zu\n", (uint64_t)addr, g_flag, address);
        //if (g_flag && address == (uint64_t)addr) fprintf(stderr, "found it!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        
        //fprintf(stderr, "tsan_read at iter: %lld stage: %lld\n", g_curIterNum, g_curStageNum);
        
        //fprintf(stderr, "address key %zu\n", ADDR_TO_KEY(((uint64_t)(addr))));
        //const int start = ADDR_TO_MEM_INDEX(addr); 
        //const int grains = SIZE_TO_NUM_GRAINS(mem_size);
        //if ((start + grains) > NUM_SLOTS) {
        //    fprintf(stderr, "addr: %zu\n", (uint64_t)addr);
        //}
        //assert(start >= 0 && start < NUM_SLOTS && (start + grains) <= NUM_SLOTS);
        //writer++;
        record_mem_helper(false, (uint64_t)rip, (uint64_t)addr, mem_size);
        enable_checking();
    }
}

extern "C" void __tsan_func_exit() {
    //return;
    if (should_check()) {
        disable_checking();
        //fprintf(stderr, "res: approaching\n");
        uint64_t res = (uint64_t) __builtin_frame_address(0);
        //fprintf(stderr, "res: %zu\n", res);
        //if (g_rdc.getLowWatermark(g_curIterNum) > res)
        //    g_rdc.setLowWatermark(g_curIterNum, res);

        enable_checking();
    }
}




/*
extern "C" void __tsan_func_exit() {
    if (!g_inited) return;
    int checking_status = checking_disabled;
    //disable_checking();
    checking_disabled = 1;
    uint64_t res = (uint64_t) __builtin_frame_address(0);

    //pthread_spin_lock(&printMut.mut);
    //std::cout << "tsan func exit" << std::endl;
    //frames[self].print(self, NULL);
    //std::cout << "res: " << res << std::endl;
    //std::cout << "watermark: " << stack_low_watermark << std::endl;
    //pthread_spin_unlock(&printMut.mut);

    if(stack_low_watermark > res) {
        stack_low_watermark = res;
    }

    if(clear_stack) {
        // the spawn helper that's exiting is calling tsan_func_exit,
        // so the spawn helper's base pointer is the stack_high_watermark
        // to clear (stack grows downward)
        uint64_t stack_high_watermark = (uint64_t)__builtin_frame_address(1);
        //std::cout << "hig watermark: " << stack_high_watermark << std::endl;
        clear_shadow_memory(stack_low_watermark, stack_high_watermark);
        // now the high watermark becomes the low watermark
        stack_low_watermark = stack_high_watermark;
        clear_stack = 0;
    }
    //enable_checking();
    checking_disabled = checking_status;
}*/

void do_tool_print() {
  size_t num_lreader_runs, num_rreader_runs, num_writer_runs;
  MemAccess_t *last_lreader, *last_rreader, *last_writer;

  fprintf(stderr, "Getting shadow_mem_stats\n");
    
  num_lreader_runs = num_rreader_runs = num_writer_runs = 0;
  last_lreader = last_rreader = last_writer = NULL;

  for (uint64_t i = 0; i < (1<<(48 - LOG_TBL_SIZE - LOG_KEY_SIZE)); ++i) {
    ShadowMem<MemAccessList_t>::shadow_tbl *volatile *dest = &(shadow_mem.shadow_dir[i]);
    if (!dest || !(*dest)) {
      if (last_lreader) num_lreader_runs++;
      if (last_rreader) num_rreader_runs++;
      if (last_writer) num_writer_runs++;
      last_lreader = last_rreader = last_writer = NULL;
      continue;
    }
      
    for (uint64_t j = 0; j < (1<<LOG_TBL_SIZE); ++j) {
      MemAccessList_t** slot =  &(*dest)->shadow_entries[j];
      if (!slot || !(*slot)) {
        if (last_lreader) num_lreader_runs++;
        if (last_rreader) num_rreader_runs++;
        if (last_writer) num_writer_runs++;
        last_lreader = last_rreader = last_writer = NULL;
        continue;
      }

      for (uint64_t k = 0; k < NUM_SLOTS; ++k) {
        if (last_lreader && last_lreader != (*slot)->lreaders[k]) {
          num_lreader_runs++;
        }
        if (last_rreader && last_rreader != (*slot)->rreaders[k]) {
          num_rreader_runs++;
        }
        if (last_writer && last_writer != (*slot)->writers[k]) {
          num_writer_runs++;
        }
        last_lreader = (*slot)->lreaders[k];
        last_rreader = (*slot)->rreaders[k];
        last_writer = (*slot)->writers[k];
      } // k
    } // j
  } // i

  // might have ended on a run
  if (last_lreader) num_lreader_runs++;
  if (last_rreader) num_rreader_runs++;
  if (last_writer) num_writer_runs++;
          
  fprintf(stderr, "Runs: %zu, %zu, %zu\n",
          num_lreader_runs, num_rreader_runs, num_writer_runs);

  size_t sum = 0;
  for (int i = 0; i < 32; i++) 
      sum += new_memaccess[i];

  fprintf(stderr, "new_memaccess: %zu\n", sum);
 
}

void do_tool_init() {
    for (int i = 0; i < 32; i++)
        new_memaccess[i] = 0;
}



extern "C" void __tsan_init() {}
extern "C" void __tsan_func_entry(void *pc) {}
extern "C" void __tsan_vptr_read(void **vptr_p) {}
extern "C" void __tsan_vptr_update(void **vptr_p, void *new_val) {}
extern "C" void
__tsan_read1(void *addr) { tsan_read(addr, 1, __builtin_return_address(0)); }
extern "C" void
__tsan_read2(void *addr) { tsan_read(addr, 2, __builtin_return_address(0)); }
extern "C" void
__tsan_read4(void *addr) { tsan_read(addr, 4, __builtin_return_address(0)); }
extern "C" void
__tsan_read8(void *addr) { tsan_read(addr, 8, __builtin_return_address(0)); }
extern "C" void
__tsan_read16(void *addr) { tsan_read(addr, 16, __builtin_return_address(0)); }
extern "C" void
__tsan_write1(void *addr) { tsan_write(addr, 1, __builtin_return_address(0)); }
extern "C" void
__tsan_write2(void *addr) { tsan_write(addr, 2, __builtin_return_address(0)); }
extern "C" void
__tsan_write4(void *addr) { tsan_write(addr, 4, __builtin_return_address(0)); }
extern "C" void
__tsan_write8(void *addr) { tsan_write(addr, 8, __builtin_return_address(0)); }
extern "C" void
__tsan_write16(void *addr) { tsan_write(addr, 16, __builtin_return_address(0)); }
