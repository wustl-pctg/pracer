#include <inttypes.h>
#include <stdio.h> 
#include "print_addr.h"
#include "mem_access.h"
#include "piper_rd.h"
//static int flag = 0;
// Check races on memory represented by this mem list with this read access
// Once done checking, update the mem list with this new read access
void
MemAccessList_t::check_races_and_update_with_read(uint64_t inst_addr, 
                              uint64_t addr, size_t mem_size,
                              om_node *curr_estrand, om_node *curr_hstrand) {

  DBG_TRACE(DEBUG_MEMORY, "check race w/ read addr %lx and size %lu.\n",
            addr, mem_size);
  om_assert( addr >= start_addr && 
                       (addr+mem_size) <= (start_addr+MAX_GRAIN_SIZE) );

  // check races with the writers
  // start (inclusive) index covered by this mem access;                        
  const int start = ADDR_TO_MEM_INDEX(addr);
  const int grains = SIZE_TO_NUM_GRAINS(mem_size);
  //assert(start >= 0 && start < NUM_SLOTS && (start + grains) <= NUM_SLOTS);
  DBG_TRACE(DEBUG_MEMORY, "Read has start %d and grains %d.\n", start, grains);

  // walk through the indices that this memory access cover to check races
  // against any writer found within this range 
  for(int i = start; i < (start + grains); i++) {
    MemAccess_t *writer = writers[i]; // can be NULL
    if( writer && writer->races_with(curr_estrand, curr_hstrand) ) {
      //report_race(writer->rip, inst_addr, start_addr+i, WR_RACE);
      //fprintf(stderr, "race detected at iter: %lld stage: %lld\n", g_curIterNum, g_curStageNum);
      //fprintf(stderr, "addr: %zu %zu\n", inst_addr, addr);
      //fprintf(stderr, "address key %zu\n", ADDR_TO_KEY(((uint64_t)(addr))));
      //flag++;
      //if (flag == 20) assert(0);
      //assert(0);
    }
  }

  // update the left-most reader list with this mem access
  for(int i = start; i < (start + grains); i++) {
    MemAccess_t *reader = lreaders[i];
    if(reader == NULL) {
      reader = new MemAccess_t(curr_estrand, curr_hstrand, inst_addr);
      pthread_spin_lock(&lreader_lock);
      if (lreaders[i] == NULL) {
        lreaders[i] = reader;
      }
      pthread_spin_unlock(&lreader_lock);
      if (reader == lreaders[i]) continue;
      else {
        delete reader;
        reader = lreaders[i];
      }
    }
    om_assert(reader != NULL);

    // potentially update the left-most reader; replace it if 
    // - the new reader is to the left of the old lreader 
    //   (i.e., comes first in serially execution)  OR
    // - there is a path from old lreader to this reader
    bool is_leftmost;
#if STATS > 0
    __sync_fetch_and_add(&g_num_queries, 1);
#endif
    QUERY_START;
    RDTOOL_INTERVAL_BEGIN(QUERY);
    is_leftmost = om_precedes(curr_estrand, reader->estrand) ||
      om_precedes(reader->hstrand, curr_hstrand);
    RDTOOL_INTERVAL_END(QUERY);
    QUERY_END;
    
    if(is_leftmost) {
      pthread_spin_lock(&lreader_lock);
      lreaders[i]->update_acc_info(curr_estrand, curr_hstrand, inst_addr);
      pthread_spin_unlock(&lreader_lock);
    }
  }

  // now we update the right-most readers list with this access 
  for(int i = start; i < (start + grains); i++) {
    MemAccess_t *reader = rreaders[i];
    if(reader == NULL) {
      reader = new MemAccess_t(curr_estrand, curr_hstrand, inst_addr);
      pthread_spin_lock(&rreader_lock);
      if (rreaders[i] == NULL) {
        rreaders[i] = reader;
      }
      pthread_spin_unlock(&rreader_lock);
      if (reader == rreaders[i]) continue;
      else {
        delete reader;
        reader = rreaders[i];
      }
    }
    om_assert(reader != NULL);

    // potentially update the right-most reader; replace it if 
    // - the new reader is to the right of the old rreader 
    //   (i.e., comes later in serially execution)  OR
    // - there is a path from old rreader to this reader
    // but actually the second condition subsumes the first, so if the 
    // first condition is false, there is no point checking the second
    bool is_rightmost;
#if STATS > 0
    //    __sync_fetch_and_add(&g_num_queries, 1);
#endif
    QUERY_START;
    RDTOOL_INTERVAL_BEGIN(QUERY);
    is_rightmost = om_precedes(reader->estrand, curr_estrand);
    RDTOOL_INTERVAL_END(QUERY);
    QUERY_END;

    
    if(is_rightmost) {
      pthread_spin_lock(&rreader_lock);
      rreaders[i]->update_acc_info(curr_estrand, curr_hstrand, inst_addr);
      pthread_spin_unlock(&rreader_lock);
    }
   }
}

// Check races on memory represented by this mem list with this write access
// Also, update the writers list.  
// Very similar to check_races_and_update_with_read function above.
void
MemAccessList_t::check_races_and_update_with_write(uint64_t inst_addr,
                                                   uint64_t addr, size_t mem_size,
                                                   om_node *curr_estrand, om_node *curr_hstrand) {

  DBG_TRACE(DEBUG_MEMORY, "check race w/ write addr %lx and size %lu.\n",
            addr, mem_size);
  om_assert( addr >= start_addr && 
                       (addr+mem_size) <= (start_addr+MAX_GRAIN_SIZE) );

  const int start = ADDR_TO_MEM_INDEX(addr);
  const int grains = SIZE_TO_NUM_GRAINS(mem_size);
  //assert(start >= 0 && start < NUM_SLOTS && (start + grains) <= NUM_SLOTS); 
  DBG_TRACE(DEBUG_MEMORY, "Write has start %d and grains %d.\n", start, grains);

  // now traverse through the writers list to both report race and update
  for(int i = start; i < (start + grains); i++) {
    MemAccess_t *writer = writers[i];
    if(writer == NULL) {
      writer = new MemAccess_t(curr_estrand, curr_hstrand, inst_addr);
      pthread_spin_lock(&writer_lock);
      if(writers[i] == NULL) {
        writers[i] = writer;
      }
      pthread_spin_unlock(&writer_lock);
      if(writer == writers[i]) continue;
      else { // was NULL but some other logically parallel strand updated it
          delete writer;
          writer = writers[i];
      }
    }
    // At this point, someone else may come along and write into writers[i]...
    // om_assert(writer == writers[i]);
    om_assert(writer);

    // om_assert(writer->estrand != curr_estrand && 
    //           writer->hstrand != curr_hstrand);

    // last writer exists; possibly report race and replace it
    if( writer->races_with(curr_estrand, curr_hstrand) ) { 
      // report race
      //report_race(writer->rip, inst_addr, start_addr+i, WW_RACE);
      //fprintf(stderr, "race detected at iter: %lld stage: %lld\n", g_curIterNum, g_curStageNum);
      //fprintf(stderr, "addr: %zu %zu\n", inst_addr, addr);
      //fprintf(stderr, "address key %zu\n", ADDR_TO_KEY(((uint64_t)(addr))));
      //if (++flag == 20) assert(0);
      //assert(0);
    }
    pthread_spin_lock(&writer_lock);
    // replace the last writer regardless
    writers[i]->update_acc_info(curr_estrand, curr_hstrand, inst_addr);
    pthread_spin_unlock(&writer_lock);
  }

  // Now we detect races with the lreaders
  for(int i = start; i < (start + grains); i++) {
    MemAccess_t *reader = lreaders[i];
    if( reader && reader->races_with(curr_estrand, curr_hstrand) ) {
      //report_race(reader->rip, inst_addr, start_addr+i, RW_RACE);
      //fprintf(stderr, "race detected at iter: %lld stage: %lld\n", g_curIterNum, g_curStageNum);
      //fprintf(stderr, "addr: %zu %zu\n", inst_addr, addr);
      //fprintf(stderr, "address key %zu\n", ADDR_TO_KEY(((uint64_t)(addr))));
      //if (++flag == 20) assert(0);
      //assert(0);
    }
  }
  //  Now we detect races with the rreaders
  for(int i = start; i < (start + grains); i++) {
    MemAccess_t *reader = rreaders[i];
    if( reader && reader->races_with(curr_estrand, curr_hstrand) ) {
      //report_race(reader->rip, inst_addr, start_addr+i, RW_RACE);
      //fprintf(stderr, "race detected at iter: %lld stage: %lld\n", g_curIterNum, g_curStageNum);
      //fprintf(stderr, "addr: %zu %zu\n", inst_addr, addr);
      //fprintf(stderr, "address key %zu\n", ADDR_TO_KEY(((uint64_t)(addr))));
      //if (++flag == 20) assert(0);
      //assert(0);
    }
  }
}

MemAccessList_t::MemAccessList_t(uint64_t addr, bool is_read, 
                                 om_node *estrand, om_node *hstrand, 
                                 uint64_t inst_addr, size_t mem_size) 
  : start_addr( ALIGN_BY_PREV_MAX_GRAIN_SIZE(addr) ) {
  for(int i=0; i < NUM_SLOTS; i++) {
    lreaders[i] = rreaders[i] = writers[i] = NULL;
    //    lreaders[i] = writers[i] = NULL;
  }

  const int start = ADDR_TO_MEM_INDEX(addr);
  const int grains = SIZE_TO_NUM_GRAINS(mem_size);
  DBG_TRACE(DEBUG_MEMORY, "Mem access has start %d and grains %d.\n", 
            start, grains);
  //assert(start >= 0 && start < NUM_SLOTS && (start + grains) <= NUM_SLOTS);

  // No need to acquire locks yet, since this is a new object and its
  // reference has not escaped.
  if(is_read) {
    for(int i=start; i < (start + grains); i++) {
      lreaders[i] = new MemAccess_t(estrand, hstrand, inst_addr);
      rreaders[i] = new MemAccess_t(estrand, hstrand, inst_addr);
    }
  } else {
    for(int i=start; i < (start + grains); i++) {
      writers[i] = new MemAccess_t(estrand, hstrand, inst_addr);
    }
  }

  if(pthread_spin_init(&lreader_lock, PTHREAD_PROCESS_PRIVATE) ||  
     pthread_spin_init(&rreader_lock, PTHREAD_PROCESS_PRIVATE) ||
     pthread_spin_init(&writer_lock, PTHREAD_PROCESS_PRIVATE) ) {
    fprintf(stderr, 
            "spin_lock initialization for MemAccessList_t failed.  Exit.\n");
    exit(1);
  }
}

MemAccessList_t::~MemAccessList_t() {
  for(int i=0; i < NUM_SLOTS; i++) {
    if(lreaders[i]) {
      delete lreaders[i];
      lreaders[i] = NULL;
    }
  }
  for(int i=0; i < NUM_SLOTS; i++) {
    if(rreaders[i]) {
      delete rreaders[i];
      rreaders[i] = NULL;
    }
  }
  for(int i=0; i < NUM_SLOTS; i++) {
    if(writers[i]) {
      delete writers[i];
      writers[i] = NULL;
    }
  }
  pthread_spin_destroy(&lreader_lock);
  pthread_spin_destroy(&rreader_lock);
  pthread_spin_destroy(&writer_lock);
}
