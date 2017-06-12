#ifndef CAML_INTERRUPT_H
#define CAML_INTERRUPT_H

#include "mlvalues.h"
#include "platform.h"

struct waitq {
  struct interruptor* head;
  struct interruptor* tail; /* well-defined only if head != NULL */
};

struct domain; // compat hack
typedef void (*interrupt_handler)(struct domain*, void*);
struct interrupt {
  interrupt_handler handler;
  void* data;
  atomic_uintnat completed;
};

struct interruptor {
  atomic_uintnat* interrupt_word;
  caml_plat_mutex lock;
  caml_plat_cond cond;

  int running;
  int64 generation;
  struct waitq joiners;
  int64 join_target_generation;

  /* Queue of domains trying to send interrupts here */
  struct waitq interrupts;
  struct interrupt current_interrupt;

  /* Next pointer for wait queues.
     Touched only when the queue is locked */
  struct interruptor* next;
};

void caml_init_interruptor(struct interruptor* s, atomic_uintnat* interrupt_word);

void caml_handle_incoming_interrupts(struct interruptor* self);
void caml_yield_until_interrupted(struct interruptor* self);

void caml_start_interruptor(struct interruptor* self);
void caml_stop_interruptor(struct interruptor* self);
int caml_join_interruptor(struct interruptor* self, struct interruptor* target, int64 target_gen);

/* returns 0 on failure, if the target has terminated. */
CAMLcheckresult
int caml_send_interrupt(struct interruptor* self,
                        struct interruptor* target,
                        interrupt_handler handler,
                        void* data);


#endif /* CAML_INTERRUPT_H */
