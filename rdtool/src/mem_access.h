/* -*- Mode: C++ -*- */

#ifndef _MEM_ACCESS_H
#define _MEM_ACCESS_H

#include <stdio.h>
#include <iostream>
#include <inttypes.h>
#include <pthread.h>

#include "debug_util.h"
#include "stat_util.h"
#include "rd.h" // is this necessary?
#include "om/om.h"
#include "shadow_mem.h"
#include "omrd.h"

//#define QUERY_START while (pthread_spin_trylock(&g_worker_mutexes[self].mut) != 0) { join_batch(self); }
//#define QUERY_END pthread_spin_unlock(&g_worker_mutexes[self].mut)

// Note this starts a new block
#define QUERY_START { size_t relabel_id = 0; \
      do { relabel_id = g_relabel_id;
#define QUERY_END } while ( !( (relabel_id & 0x1) == 0 &&     \
                               relabel_id == g_relabel_id)); }

#define GRAIN_SIZE 4
#define LOG_GRAIN_SIZE 2
#define MAX_GRAIN_SIZE (1 << LOG_KEY_SIZE)
#define NUM_SLOTS (MAX_GRAIN_SIZE / GRAIN_SIZE)
 
// a mask that keeps all the bits set except for the least significant bits
// that represent the max grain size
#define MAX_GRAIN_MASK (~(uint64_t)(MAX_GRAIN_SIZE-1))

// If the value is already divisible by MAX_GRAIN_SIZE, return the value; 
// otherwise return the previous / next value divisible by MAX_GRAIN_SIZE.
#define ALIGN_BY_PREV_MAX_GRAIN_SIZE(addr) ((uint64_t) (addr & MAX_GRAIN_MASK))
#define ALIGN_BY_NEXT_MAX_GRAIN_SIZE(addr) \
  ((uint64_t) ((addr+(MAX_GRAIN_SIZE-1)) & MAX_GRAIN_MASK))

// compute (addr % 16) / GRAIN_SIZE 
#define ADDR_TO_MEM_INDEX(addr) \
    (((uint64_t)addr & (uint64_t)(MAX_GRAIN_SIZE-1)) >> LOG_GRAIN_SIZE)

// compute size / GRAIN_SIZE 
#define SIZE_TO_NUM_GRAINS(size) (size >> LOG_GRAIN_SIZE) 

//extern pthread_spinlock_t* g_worker_mutexes;
extern local_mut* g_worker_mutexes;
extern __thread int self;
extern volatile size_t g_relabel_id;
extern size_t num_new_memaccess;

// Struct to hold strands corresponding to left / right readers and last writer
typedef struct MemAccess_t {

  om_node *estrand; // the strand that made this access stored in g_english
  om_node *hstrand; // the strand that made this access stored in g_hebrew
  uint64_t rip; // the instruction address of this access
  // int16_t ref_count; // number of pointers aliasing to this object
  // ref_count == 0 if only a single unique pointer to this object exists

  MemAccess_t(om_node *_estrand, om_node *_hstrand, uint64_t _rip)
    : estrand(_estrand), hstrand(_hstrand), rip(_rip)
  { }

  inline bool races_with(om_node *curr_estrand, om_node *curr_hstrand) {
    om_assert(estrand); om_assert(hstrand);
    om_assert(curr_estrand); om_assert(curr_hstrand);

    /// @todo Is it worth it to join the batch relabel here?
    //om_assert(self > -1);
    bool prec_in_english, prec_in_hebrew;

#if STATS > 0
    //    __sync_fetch_and_add(&g_num_queries, 1);
#endif
    // fprintf(stderr, "estrand: %p\n", estrand);
    // fprintf(stderr, "hstrand: %p\n", hstrand);
    // fprintf(stderr, "curr_estrand: %p\n", curr_estrand);
    // fprintf(stderr, "curr_hstrand: %p\n", curr_hstrand);

    QUERY_START;
    RDTOOL_INTERVAL_BEGIN(QUERY);
    prec_in_english = om_precedes(estrand, curr_estrand);
    prec_in_hebrew  = om_precedes(hstrand, curr_hstrand);
    RDTOOL_INTERVAL_END(QUERY);
    QUERY_END;

    // race if the ordering in english and hebrew differ
    bool has_race = (prec_in_english == !prec_in_hebrew);
#if OM_DEBUG > DEBUG_BASIC
    if(has_race) {
      DBG_TRACE(DEBUG_MEMORY, 
            "Race with estrand: %p, and curr estrand: %p, prec: %d.\n", 
            estrand, curr_estrand, prec_in_english);
      DBG_TRACE(DEBUG_MEMORY, 
            "Race with hstrand: %p, and curr hstrand: %p, prec: %d.\n", 
            hstrand, curr_hstrand, prec_in_hebrew);
    }
#endif

    return has_race;
  }
  
  inline void update_acc_info(om_node *_estrand, om_node *_hstrand, 
                              uint64_t _rip) {
    estrand = _estrand;
    hstrand = _hstrand;
    rip = _rip;
  }

  // ref counting deprecated
  // inline int16_t inc_ref_count() { ref_count++; return ref_count; }
  // inline int16_t dec_ref_count() { ref_count--; return ref_count; }

} MemAccess_t;


class MemAccessList_t {

//private:
public:
  // the smallest addr of memory locations that this MemAccessList represents
  uint64_t start_addr;
  MemAccess_t *lreaders[NUM_SLOTS];
  MemAccess_t *rreaders[NUM_SLOTS];
  MemAccess_t *writers[NUM_SLOTS];

  // locks for updating lreaders, rreaders, and writers
  pthread_spinlock_t lreader_lock;
  pthread_spinlock_t rreader_lock;
  pthread_spinlock_t writer_lock;

  // Check races on memory represented by this mem list with this read access
  // Once done checking, update the mem list with this new read access
  void check_races_and_update_with_read(uint64_t inst_addr, uint64_t addr,
                              size_t mem_size, 
                              om_node *curr_estrand, om_node *curr_hstrand);
  
  // Check races on memory represented by this mem list with this write access
  // Also, update the writers list.  
  // Very similar to check_races_and_update_with_read function above.
  void check_races_and_update_with_write(uint64_t inst_addr, uint64_t addr, 
                              size_t mem_size,
                              om_node *curr_estrand, om_node *curr_hstrand);

//public:
  // Constructor.
  //
  // addr: the memory address of the access
  // is_read: whether the initializing memory access is a read
  // estrand: the strand in g_english that made the accesss that causes this
  //          MemAccessList_t to be created 
  // estrand: the strand in g_hebrew that made the accesss that causes this
  //          MemAccessList_t to be created 
  // rip: the instruction address that access the memory that
  //            causes this MemAccessList_t to be created
  // mem_size: the size of the access 
  MemAccessList_t(uint64_t addr, bool is_read,
                  om_node *estrand, om_node *hstrand, uint64_t rip, 
                  size_t mem_size); 

  ~MemAccessList_t();


  // Check races on memory represented by this mem list with this mem access
  // Once done checking, update the mem list with the new mem access
  //
  // is_read: whether this access is a read or not
  // in_user_context: whether this access is made by user strand or runtime
  //                  strand (e.g., update / reduce)
  // inst_addr: the instruction that performs the read
  // addr: the actual memory location accessed
  // mem_size: the size of this memory access
  // curr_estrand: the strand stored in g_english corresponding to this access
  // curr_hstrand: the strand stored in g_hebrew corresponding to this access
  __attribute__((always_inline))
  void
  check_races_and_update(bool is_read, uint64_t inst_addr, uint64_t addr,
                         size_t mem_size,
                         om_node *curr_estrand, om_node *curr_hstrand) {
    if(is_read) {
      check_races_and_update_with_read(inst_addr, addr, mem_size,
                                       curr_estrand, curr_hstrand);
    } else {
      check_races_and_update_with_write(inst_addr, addr, mem_size,
                                        curr_estrand, curr_hstrand);
    }
  }

#if OM_DEBUG > DEBUG_BASIC
  void check_invariants(uint64_t current_func_id); 
#endif
}; // end of class MemAccessList_t def

#endif // _MEM_ACCESS_H
