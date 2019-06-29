#include <iostream>
#include <cstdlib> // for getenv

#include <internal/abi.h>
#include <cilk/batcher.h>
#include "rd.h"
#include "print_addr.h"
#include "shadow_mem.h"
#include "mem_access.h"

#include "stack.h"
#include "stat_util.h"

#include "omrd.cpp" /// Hack @todo fix linking errors

int g_tool_init = 0;

typedef struct FrameData_s {
  om_node* current_english;
  om_node* current_hebrew;
  om_node* cont_english;
  om_node* cont_hebrew;
  om_node* sync_english;
  om_node* sync_hebrew;
  uint32_t flags;
} FrameData_t;

AtomicStack_t<FrameData_t>* frames;

FrameData_t* get_frame() { return frames[self].head(); }

// XXX Need to synchronize access to it when update
static ShadowMem<MemAccessList_t> shadow_mem;

void init_strand(__cilkrts_worker* w, FrameData_t* init)
{
  self = w->self;
  t_worker = w;

  FrameData_t* f;
  if (init) {
    f = frames[w->self].head();
    assert(!(init->flags & FRAME_HELPER_MASK));
    assert(!init->current_english);
    assert(!init->current_hebrew);

    //    pthread_spin_lock(&g_worker_mutexes[self]);
    f->current_english = init->cont_english;
    f->current_hebrew = init->cont_hebrew;
    f->sync_english = init->sync_english;
    f->sync_hebrew = init->sync_hebrew;
    f->cont_english = NULL;
    f->cont_hebrew = NULL;
    f->flags = init->flags;
    //    pthread_spin_unlock(&g_worker_mutexes[self]);
  }  else {
    assert(frames[w->self].empty());
    frames[w->self].push();
    f = frames[w->self].head();
    f->flags = 0;

    f->current_english = g_english->get_base();
    f->current_hebrew = g_hebrew->get_base();
    f->cont_english = NULL;
    f->cont_hebrew = NULL;
    f->sync_english = NULL;
    f->sync_hebrew = NULL;
  }
}

extern "C" void do_tool_init(void) 
{
  DBG_TRACE(DEBUG_CALLBACK, "cilk_tool_init called.\n");
  int p = __cilkrts_get_nworkers();
#if STATS > 1
  size_t num_events = NUM_INTERVAL_TYPES * p;
  g_timing = new struct padded_time[num_events]();
  // g_timing_events = new struct padded_time[num_events];
  // g_timing_event_starts = new struct padded_time[num_events];
  // g_timing_event_ends = new struct padded_time[num_events];
  for (int i = 0; i < num_events; ++i) {
    g_timing[i].start = g_timing[i].elapsed = 0;
    // g_timing_events[i].t = EMPTY_TIME_POINT;
    // g_timing_event_starts[i].t = EMPTY_TIME_POINT;
    // g_timing_event_ends[i].t = EMPTY_TIME_POINT;
  }
#endif
  frames = new AtomicStack_t<FrameData_t>[p];
  //  g_worker_mutexes = new pthread_spinlock_t[p];
  g_worker_mutexes = (local_mut*)memalign(64, sizeof(local_mut)*p);
  for (int i = 0; i < p; ++i) {
    pthread_spin_init(&g_worker_mutexes[i].mut, PTHREAD_PROCESS_PRIVATE);
  }
  //pthread_spin_init(&g_relabel_mutex, PTHREAD_PROCESS_PRIVATE);
  __cilkrts_mutex_init(&g_relabel_mutex);
  g_english = new omrd_t();
  g_hebrew = new omrd_t();

  char *s_thresh = getenv("HEAVY_THRESHOLD");
  if (s_thresh) {
    label_t thresh = (label_t)atol(getenv("HEAVY_THRESHOLD"));
    g_english->set_heavy_threshold(thresh);
    g_hebrew->set_heavy_threshold(thresh);
  }

  // XXX: Can I assume this is called before everything?
  read_proc_maps();
  g_tool_init = 1;


  //  RDTOOL_INTERVAL_BEGIN(TOOL);
  // g_timing[0].start = rdtsc();
  // fprintf(stderr, "rdtsc: %llu\n", rdtsc());
}

extern size_t g_num_relabel_lock_tries;

extern "C" void do_tool_print(void)
{
  DBG_TRACE(DEBUG_CALLBACK, "cilk_tool_print called.\n");
  // g_timing[0].elapsed = rdtsc() - g_timing[0].start;
  // fprintf(stderr, "rdtsc: %llu\n", rdtsc());
  // int self = 0;
  //  RDTOOL_INTERVAL_END(TOOL);
  // fprintf(stderr, "real: %lu\n", g_timing[0].elapsed);
  // fprintf(stderr, "real2: %lf\n", g_timing[0].elapsed / (CPU_MHZ * 1000));
  // fprintf(stderr, "Time: %lf\n", GET_TIME_MS(TOOL));

#if STATS > 0
  int p = __cilkrts_get_nworkers();
#endif
#if STATS > 1
  for (int itype = 0; itype < NUM_INTERVAL_TYPES; ++itype) {
    char s[32];
    if (g_interval_strings[itype] == NULL) continue;
    strncpy(s, g_interval_strings[itype], 31);
    strcat(s, ":");
    fprintf(stderr, "%-20s\t", s);
    
    long double total = 0;
    for (int i = 0; i < p; ++i) {
      int self = i;
      long double t = GET_TIME_MS(itype);
      fprintf(stderr, "% 10.2Lf", t);
      total += t;
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "Total %s:\t%Lf\n", g_interval_strings[itype], total);
  }

  // fprintf(stderr, "\nTotal relabel lock acquires:\t%zu\n",
  // g_num_relabel_lock_tries);
  // double insert_time = 0;
  // double query_time = 0;
  // for (int i = 0; i < p; ++i) {
  //   int self = i;
  //   fprintf(stderr, "Index for %d is %d\n", i, TIDX(INSERT));
  //   insert_time += (g_timing_events[TIDX(INSERT)].tv_sec * 1000.0) +
  //     (g_timing_events[TIDX(INSERT)].tv_nsec / 1000000.0);
  //   query_time += (g_timing_events[TIDX(QUERY)].tv_sec * 1000.0) +
  //     (g_timing_events[TIDX(QUERY)].tv_nsec / 1000000.0);
  // }
  // fprintf(stderr, "%-20s\t%lf\n", "Total insert time:", insert_time);
  // fprintf(stderr, "%-20s\t%lf\n", "Total query time:", query_time);


#endif
#if STATS > 0  
  std::cout << "Num relabels: " << g_num_relabels << std::endl;

  size_t min = (unsigned long)-1;
  size_t max = 0;
  size_t total = 0;
  for (int i = 0; i < g_num_relabels; ++i) {
    int n = g_heavy_node_info[i];
    if (n < min) min = n;
    if (n > max) max = n;
    total += n;
  }
  fprintf(stderr, "Heavy node min: %zu\n", min);
  fprintf(stderr, "Heavy node max: %zu\n", max);
  fprintf(stderr, "Heavy node median: %zu\n", g_heavy_node_info[g_num_relabels/2]);
  fprintf(stderr, "Heavy node mean: %.2lf\n", total / (double)g_num_relabels);
  fprintf(stderr, "Heavy node total: %zu\n", total);
  // std::cout << "Num inserts: " << g_num_inserts << std::endl;
  // std::cout << "Avg size per relabel: " << (double)g_relabel_size / (double)g_num_relabels << std::endl;
  std::cout << "English OM memory used: " << g_english->memsize() << std::endl;
  std::cout << "Hebrew OM memory used: " << g_hebrew->memsize() << std::endl;
  size_t sssize = 0;
  for (int i = 0; i < p; ++i) sssize += frames[i].memsize();
  std::cout << "Shadow stack(s) total size: " << sssize << std::endl;
  // fprintf(stderr, "--- Thread Sanitizer Stats ---\n");
  // fprintf(stderr, "Reads: %zu\n", g_num_reads);
  // fprintf(stderr, "Writes: %zu\n", g_num_writes);
  // fprintf(stderr, "Total Accesses: %zu\n", g_num_reads + g_num_writes);
  // fprintf(stderr, "Total Queries: %zu\n", g_num_reads + g_num_queries);
  //  fprintf(stderr, "MemAccess_ts allocated: %zu\n", num_new_memaccess);
#endif

}

extern "C" void do_tool_destroy(void) 
{
  DBG_TRACE(DEBUG_CALLBACK, "cilk_tool_destroy called.\n");

  do_tool_print();
  delete g_hebrew;
  delete g_english;
  delete[] frames;

  for (int i = 0; i < __cilkrts_get_nworkers(); ++i) {
    pthread_spin_destroy(&g_worker_mutexes[i].mut);
  }
  //pthread_spin_destroy(&g_relabel_mutex);
  __cilkrts_mutex_destroy(t_worker, &g_relabel_mutex);

  //  delete[] g_worker_mutexes;
  free(g_worker_mutexes);
#if STATS > 1
  delete[] g_timing;
  // delete[] g_timing_events;
  // delete[] g_timing_event_starts;
  // delete[] g_timing_event_ends;
#endif
  //  delete_proc_maps(); /// @todo shakespeare has problems compiling print_addr.cpp
}

extern "C" om_node* get_current_english()
{
  __cilkrts_worker* w = __cilkrts_get_tls_worker();
  if (!frames) return NULL;
  return frames[w->self].head()->current_english;
}

extern "C" om_node* get_current_hebrew()
{
  __cilkrts_worker* w = __cilkrts_get_tls_worker();
  if (!frames) return NULL;
  return frames[w->self].head()->current_hebrew;
}


extern "C" void do_steal_success(__cilkrts_worker* w, __cilkrts_worker* victim,
                                 __cilkrts_stack_frame* sf)
{
  DBG_TRACE(DEBUG_CALLBACK, 
            "%d: do_steal_success, stole %p from %d.\n", w->self, sf, victim->self);
  frames[w->self].reset();
  FrameData_t* loot = frames[victim->self].steal_top(frames[w->self]);
  om_assert(!(loot->flags & FRAME_HELPER_MASK));
  init_strand(w, loot);
}

extern "C" void cilk_return_to_first_frame(__cilkrts_worker* w,
                                           __cilkrts_worker* team,
                                           __cilkrts_stack_frame* sf)
{
  DBG_TRACE(DEBUG_CALLBACK, 
            "%d: Transfering shadow stack %p to original worker %d.\n", 
            w->self, sf, team->self);
  if (w->self != team->self) frames[w->self].transfer(frames[team->self]);
}

extern "C" void do_enter_begin()
{
  if (self == -1) return;
  frames[self].push();
  DBG_TRACE(DEBUG_CALLBACK, "%d: do_enter_begin, new size: %d.\n", 
            self, frames[self].size());
  FrameData_t* parent = frames[self].ancestor(1);
  FrameData_t* f = frames[self].head();
  f->flags = 0;
  f->cont_english = f->cont_hebrew = NULL;
  f->sync_english = f->sync_hebrew = NULL;
  f->current_english = parent->current_english;
  f->current_hebrew = parent->current_hebrew;
}

extern "C" void do_enter_helper_begin(__cilkrts_stack_frame* sf, void* this_fn, void* rip)
{ 
  DBG_TRACE(DEBUG_CALLBACK, "%d: do_enter_helper_begin, sf: %p, parent: %p.\n", 
            self, sf, sf->call_parent);
}

// XXX: the doc says the sf is frame pointer to parent frame, but actually
// it's the pointer to the helper shadow frame. 
extern "C" void do_detach_begin(__cilkrts_stack_frame* sf)
{
  __cilkrts_worker* w = __cilkrts_get_tls_worker();

  // can't be empty now that we init strand in do_enter_end
  om_assert(!frames[w->self].empty());

  frames[w->self].push_helper();

  DBG_TRACE(DEBUG_CALLBACK, 
            "%d: do_detach_begin, sf: %p, parent: %p, stack size: %d.\n", 
            self, sf, sf->call_parent, frames[self].size());

  FrameData_t* parent = frames[w->self].ancestor(1);
  FrameData_t* f = frames[w->self].head();
  assert(parent + 1 == f);

  assert(!(parent->flags & FRAME_HELPER_MASK));
  assert(f->flags & FRAME_HELPER_MASK);
  //  f->flags = FRAME_HELPER_MASK;

  if (!parent->sync_english) { // first of spawn group
    om_assert(!parent->sync_hebrew);
    parent->sync_english = g_english->insert(w, parent->current_english);
    parent->sync_hebrew = g_hebrew->insert(w, parent->current_hebrew);

    DBG_TRACE(DEBUG_CALLBACK, 
              "%d:  do_detach_begin, first of spawn group, "
              "setting parent sync_eng %p and sync heb %p.\n",
              self, parent->sync_english, parent->sync_hebrew);
  } else {
    DBG_TRACE(DEBUG_CALLBACK, 
              "%d: do_detach_begin, NOT first spawn, "
              "parent sync eng %p and sync heb: %p.\n", 
              self, parent->sync_english);
  }

  f->current_english = g_english->insert(w, parent->current_english);
  parent->cont_english = g_english->insert(w, f->current_english);

  parent->cont_hebrew = g_hebrew->insert(w, parent->current_hebrew);
  f->current_hebrew = g_hebrew->insert(w, parent->cont_hebrew);
  
  // DBG_TRACE(DEBUG_CALLBACK, 
  //           "do_detach_begin, f curr eng: %p and parent cont eng: %p.\n", 
  //           f->current_english, parent->cont_english);
  // DBG_TRACE(DEBUG_CALLBACK, 
  //           "do_detach_begin, f curr heb: %p and parent cont heb: %p.\n", 
  //           f->current_hebrew, parent->cont_hebrew);

  f->cont_english = NULL;
  f->cont_hebrew = NULL;
  f->sync_english = NULL; 
  f->sync_hebrew = NULL;
  parent->current_english = NULL; 
  parent->current_hebrew = NULL;
}

/* return 1 if we are entering top-level user frame and 0 otherwise */
extern "C" int do_enter_end (__cilkrts_stack_frame* sf, void* rsp)
{
  __cilkrts_worker* w = sf->worker;
  if (__cilkrts_get_batch_id(w) != -1) return 0;

  if (frames[w->self].empty()) {
    self = w->self;
    init_strand(w, NULL); // enter_counter already set to 1 in init_strand

    DBG_TRACE(DEBUG_CALLBACK,
              "%d: do_enter_end, sf %p, parent %p, size %d.\n",
              self, sf, sf->call_parent, frames[self].size());
    return 1;
  } else {
    DBG_TRACE(DEBUG_CALLBACK,
              "%d: do_enter_end, sf %p, parent %p, size %d.\n",
              self, sf, sf->call_parent, frames[self].size());
    return 0;
  }
}

extern "C" void do_sync_begin (__cilkrts_stack_frame* sf)
{
  DBG_TRACE(DEBUG_CALLBACK, "%d: do_sync_begin, sf %p, size %d.\n", 
            self, sf, frames[self].size());

  om_assert(self != -1);
  om_assert(!frames[self].empty());

  if (__cilkrts_get_batch_id(sf->worker) != -1) return;

  FrameData_t* f = frames[self].head();

  // XXX: This should never happen
  // if (f->flags & FRAME_HELPER_MASK) { // this is a stolen frame, and this worker will be returning to the runtime shortly
  //  assert(0);
  //printf("do_sync_begin error.\n");
  //  return; /// @todo is this correct? this only seems to happen on the initial frame, when it is stolen
  // }

  assert(!(f->flags & FRAME_HELPER_MASK));
  om_assert(f->current_english); 
  om_assert(f->current_hebrew);
  om_assert(!f->cont_english); 
  om_assert(!f->cont_hebrew);

  if (f->sync_english) { // spawned
    om_assert(f->sync_hebrew);
    //    DBG_TRACE(DEBUG_CALLBACK, "function spawned (in cilk_sync_begin) (worker %d)\n", self);

    f->current_english = f->sync_english;
    f->current_hebrew = f->sync_hebrew;
    // DBG_TRACE(DEBUG_CALLBACK, 
    //           "do_sync_begin, setting eng to %p.\n", f->current_english);
    // DBG_TRACE(DEBUG_CALLBACK, 
    //           "do_sync_begin, setting heb to %p.\n", f->current_hebrew);
    f->sync_english = NULL; 
    f->sync_hebrew = NULL;
  } else { // function didn't spawn, or this is a function-ending sync
    om_assert(!f->sync_english); 
    om_assert(!f->sync_hebrew);
  }
}

extern "C" void do_sync_end()
{
  DBG_TRACE(DEBUG_CALLBACK, "%d: do_sync_end, size %d.\n", 
            self, frames[self].size());
}

// Noticed that the frame was stolen.
extern "C" void do_leave_stolen(__cilkrts_stack_frame* sf)
{
  DBG_TRACE(DEBUG_CALLBACK, 
            "%d: do_leave_stolen, spawning sf %p, size before pop: %d.\n",
            self, sf, frames[self].size());
  if (frames[self].size() <= 1) return;

  FrameData_t* child = frames[self].head();
  FrameData_t* parent = frames[self].ancestor(1);

  // If there is a helper frame on top, this is a real steal. If not,
  // then this is a full frame returning.
  if (child->flags & FRAME_HELPER_MASK) { // Real steal
    om_assert(!(parent->flags & FRAME_HELPER_MASK));
    om_assert(!parent->current_english); om_assert(!parent->current_hebrew);
    om_assert(parent->cont_english); om_assert(parent->cont_hebrew);
    om_assert(parent->sync_english); om_assert(parent->sync_hebrew);

    /** I would expect this would work: */
    // parent->current_english = parent->cont_english;
    // parent->current_hebrew = parent->cont_hebrew;
    // parent->cont_english = parent->cont_hebrew = NULL;

    /** Instead, I seem to have to do the following. Why? */
    parent->current_english = parent->sync_english;
    parent->current_hebrew = parent->sync_hebrew;
    parent->cont_english = parent->cont_hebrew = NULL;
    parent->sync_english = parent->sync_hebrew = NULL;

  } else { // full frame returning
    om_assert(sf == NULL);
    om_assert(!parent->cont_english);
    om_assert(!parent->cont_hebrew);
    parent->current_english = child->current_english;
    parent->current_hebrew = child->current_hebrew;
  }
  frames[self].pop();
}

/* return 1 if we are leaving last frame and 0 otherwise */
extern "C" int do_leave_begin (__cilkrts_stack_frame *sf)
{
  DBG_TRACE(DEBUG_CALLBACK, "%d: do_leave_begin, sf %p, size: %d.\n", 
            self, sf, frames[self].size());
  return frames[sf->worker->self].empty();
}

extern "C" int do_leave_end()
{
  DBG_TRACE(DEBUG_CALLBACK, "%d: do_leave_end, size before pop %d.\n", 
            self, frames[self].size());
  assert(t_worker);
  if (__cilkrts_get_batch_id(t_worker) != -1) return 0;

  om_assert(self != -1);
  om_assert(!frames[self].empty());

  FrameData_t* child = frames[self].head();
  if (frames[self].size() > 1) {
    //    DBG_TRACE(DEBUG_CALLBACK, "cilk_leave_end(%d): popping and changing nodes.\n", self);
    FrameData_t* parent = frames[self].ancestor(1);
    if (!(child->flags & FRAME_HELPER_MASK)) { // parent called current
      //      DBG_TRACE(DEBUG_CALLBACK, "cilk_leave_end(%d): returning from call.\n", self);

      om_assert(!parent->cont_english);
      om_assert(!parent->cont_hebrew);
      parent->current_english = child->current_english;
      parent->current_hebrew = child->current_hebrew;
    } else { // parent spawned current
      //      DBG_TRACE(DEBUG_CALLBACK, "cilk_leave_end(%d): parent spawned child.\n", self);
      om_assert(!parent->current_english); om_assert(!parent->current_hebrew);
      om_assert(parent->cont_english); om_assert(parent->cont_hebrew);
      om_assert(parent->sync_english); om_assert(parent->sync_hebrew);
      parent->current_english = parent->cont_english;
      parent->current_hebrew = parent->cont_hebrew;
      parent->cont_english = parent->cont_hebrew = NULL;
    }
    frames[self].pop();
  }
  // we are leaving the last frame if the shadow stack is empty
  return frames[self].empty();
}

// called by record_memory_read/write, with the access broken down 
// into 8-byte aligned memory accesses
void
record_mem_helper(bool is_read, uint64_t inst_addr, uint64_t addr,
                  uint32_t mem_size)
{
  om_assert(self != -1);
  FrameData_t *f = frames[self].head();
  MemAccessList_t *val = shadow_mem.find( ADDR_TO_KEY(addr) );
  //  MemAccess_t *acc = NULL;
  MemAccessList_t *mem_list = NULL;

  if( val == NULL ) {
    // not in shadow memory; create a new MemAccessList_t and insert
    mem_list = new MemAccessList_t(addr, is_read, f->current_english, 
                                   f->current_hebrew, inst_addr, mem_size);
    val = shadow_mem.insert(ADDR_TO_KEY(addr), mem_list);
    // insert failed; someone got to the slot first;
    // delete new mem_list and fall through to check race with it 
    if( val != mem_list ) { delete mem_list; }
    else { return; } // else we are done
  }
  // check for race and possibly update the existing MemAccessList_t 
  om_assert(val != NULL);
  val->check_races_and_update(is_read, inst_addr, addr, mem_size, 
                              f->current_english, f->current_hebrew);
}

// XXX: We can only read 1,2,4,8,16 bytes; optimize later
/* Deprecated
   extern "C" void do_read(uint64_t inst_addr, uint64_t addr, size_t mem_size)
   {
   DBG_TRACE(DEBUG_MEMORY, "record read of %lu bytes at addr %p and rip %p.\n", 
   mem_size, addr, inst_addr);

   // handle the prefix
   uint64_t next_addr = ALIGN_BY_NEXT_MAX_GRAIN_SIZE(addr); 
   size_t prefix_size = next_addr - addr;
   om_assert(prefix_size >= 0 && prefix_size < MAX_GRAIN_SIZE);

   if(prefix_size >= mem_size) { // access falls within a max grain sized block
   record_mem_helper(true, inst_addr, addr, mem_size);
   } else { 
   om_assert( prefix_size <= mem_size );
   if(prefix_size) { // do the prefix first
   record_mem_helper(true, inst_addr, addr, prefix_size);
   mem_size -= prefix_size;
   }
   addr = next_addr;
   // then do the rest of the max-grain size aligned blocks
   uint32_t i;
   for(i = 0; (i+MAX_GRAIN_SIZE) < mem_size; i += MAX_GRAIN_SIZE) {
   record_mem_helper(true, inst_addr, addr + i, MAX_GRAIN_SIZE);
   }
   // trailing bytes
   record_mem_helper(true, inst_addr, addr+i, mem_size-i);
   }
   }

   // XXX: We can only read 1,2,4,8,16 bytes; optimize later
   extern "C" void do_write(uint64_t inst_addr, uint64_t addr, size_t mem_size)
   {
   DBG_TRACE(DEBUG_MEMORY, "record write of %lu bytes at addr %p and rip %p.\n", 
   mem_size, addr, inst_addr);

  // handle the prefix
  uint64_t next_addr = ALIGN_BY_NEXT_MAX_GRAIN_SIZE(addr); 
  size_t prefix_size = next_addr - addr;
  om_assert(prefix_size >= 0 && prefix_size < MAX_GRAIN_SIZE);

  if(prefix_size >= mem_size) { // access falls within a max grain sized block
    record_mem_helper(false, inst_addr, addr, mem_size);
  } else {
    om_assert( prefix_size <= mem_size );
    if(prefix_size) { // do the prefix first
      record_mem_helper(false, inst_addr, addr, prefix_size);
      mem_size -= prefix_size;
    }
    addr = next_addr;
    // then do the rest of the max-grain size aligned blocks
    uint32_t i=0;
    for(i=0; (i+MAX_GRAIN_SIZE) < mem_size; i += MAX_GRAIN_SIZE) {
      record_mem_helper(false, inst_addr, addr + i, MAX_GRAIN_SIZE);
    }
    // trailing bytes
    record_mem_helper(false, inst_addr, addr+i, mem_size-i);
  }
}
*/

// clear the memory block at [start-end) (end is exclusive).
extern "C" void
clear_shadow_memory(size_t start, size_t end) {

  DBG_TRACE(DEBUG_MEMORY, "Clear shadow memory %p--%p (%u).\n", 
            start, end, end-start);
  om_assert(ALIGN_BY_NEXT_MAX_GRAIN_SIZE(end) == end); 
  om_assert(start < end);
  //  om_assert(end-start < 4096);

  while(start != end) {
    shadow_mem.erase( ADDR_TO_KEY(start) );
    start += MAX_GRAIN_SIZE;
  }
}
