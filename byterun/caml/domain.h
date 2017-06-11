#ifndef CAML_DOMAIN_H
#define CAML_DOMAIN_H

#include "mlvalues.h"
#include "domain_state.h"
#include "memory.h"

struct domain {
  struct dom_internal* internals;
  caml_domain_state* state;
};

#ifdef __GNUC__
  #define Caml_check_gc_interrupt(dom_st) \
    __builtin_expect(((uintnat)(dom_st)->young_ptr < (dom_st)->young_limit), 0)
#else
  #define Caml_check_gc_interrupt(dom_st) \
    ((uintnat)(dom_st)->young_ptr < (dom_st)->young_limit)
#endif

asize_t caml_norm_minor_heap_size (intnat);
void caml_reallocate_minor_heap(asize_t);

void caml_handle_gc_interrupt(void);

void caml_trigger_stw_gc(void);

void caml_urge_major_slice (void);

void caml_interrupt_self(void);

struct heap_stats {
  intnat pool_words;
  intnat pool_max_words;
  intnat pool_live_words;
  intnat pool_live_blocks;
  intnat pool_frag_words;
  intnat large_words;
  intnat large_max_words;
  intnat large_blocks;
};

struct gc_stats {
  uint64 minor_words;
  uint64 promoted_words;
  uint64 major_words;
  intnat minor_collections;
  struct heap_stats major_heap;
};
void caml_sample_gc_stats(struct gc_stats* buf);

CAMLextern void caml_enter_blocking_section(void);
CAMLextern void caml_leave_blocking_section(void);

CAMLextern void (*caml_enter_blocking_section_hook)(void);
CAMLextern void (*caml_leave_blocking_section_hook)(void);

void caml_init_domains(uintnat minor_heap_size);
void caml_init_domain_self(int);


struct domain* caml_domain_self();

typedef void (*domain_rpc_handler)(struct domain*, void*);

struct domain* caml_random_domain();
int caml_domain_alone();

struct domain* caml_owner_of_young_block(value);

struct domain* caml_domain_of_id(int);

CAMLcheckresult
int caml_domain_rpc(struct domain*,
                     domain_rpc_handler, void*);

#endif /* CAML_DOMAIN_H */
