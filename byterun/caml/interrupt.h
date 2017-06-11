#ifndef CAML_INTERRUPT_H
#define CAML_INTERRUPT_H

#include "mlvalues.h"
#include "platform.h"

#define Interrupt_queue_len 256

CAML_STATIC_ASSERT(Interrupt_queue_len >= Max_domains);
CAML_STATIC_ASSERT(Is_power_of_2(Interrupt_queue_len));

struct interrupt;
struct interruptor {
  atomic_uintnat* interrupt_word;
  caml_plat_mutex lock;
  caml_plat_cond cond;

  int running;
  struct interruptor* joiner;

  /* Ring buffer of pending interrupts */
  uintnat received, acknowledged;
  struct interrupt* messages[Interrupt_queue_len];
};

void caml_init_interruptor(struct interruptor* s, atomic_uintnat* interrupt_word);

void caml_handle_incoming_interrupts(struct interruptor* self);
void caml_yield_until_interrupted(struct interruptor* self);

void caml_start_interruptor(struct interruptor* self);
void caml_stop_interruptor(struct interruptor* self);
void caml_join_interruptor(struct interruptor* self, struct interuptor* target);

struct domain; // compat hack
typedef void (*interrupt_handler)(struct domain*, void*);

/* returns 0 on failure, if the target has terminated. */
CAMLcheckresult
int caml_send_interrupt(struct interruptor* self,
                        struct interruptor* target,
                        interrupt_handler handler,
                        void* data);
                         

#endif /* CAML_INTERRUPT_H */
