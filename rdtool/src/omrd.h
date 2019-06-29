#ifndef _OMRD_H
#define _OMRD_H
#include "rd.h"
#include "debug_util.h"
#include "stat_util.h"
#include <internal/abi.h> // for __cilkrts_get_nworkers();
#include <cilk/batcher.h> // for cilK_batchify
#include <cstdio> // temporary
//#include "om/om.c"
#include "stack.h"

#include "om/om.h"
#include "om/om_common.h"
#include "om/blist.h"

void join_batch(int self);

class omrd_t {
private:
    AtomicStack_t<tl_node*> m_heavy_nodes;
    om* m_ds;
    om_node* m_base;

    void add_heavy(om_node* base);

    om_node* try_insert(om_node* base);

    om_node* slow_path(__cilkrts_worker *w, om_node* base);

public:
    omrd_t();

    ~omrd_t();

    om_node* get_base();

    AtomicStack_t<tl_node*>* get_heavy_nodes();

    om* get_ds();

    om_node* insert(__cilkrts_worker* w, om_node* base);

    label_t set_heavy_threshold(label_t new_threshold);

    size_t memsize();

    //volatile size_t heavy_count;
};

extern "C" void init_lock(void);
extern omrd_t *g_english;  
extern omrd_t *g_hebrew; 
#endif
