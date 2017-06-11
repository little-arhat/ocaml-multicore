#include "caml/platform.h"
#include "caml/interrupt.h"
#include "caml/domain.h"
#include <stddef.h>

/* Sending interrupts between domains.

   To avoid deadlock, some rules are important:

   - Don't hold interruptor locks for long
   - Don't hold two interruptor locks at the same time
   - Continue to handle incoming interrupts even when waiting for a response */

static void waitq_init(struct waitq* q)
{
  q->head = NULL;
}

static int waitq_empty(struct waitq* q)
{
  return q->head == NULL;
}

static struct interruptor* waitq_remove(struct waitq* q)
{
  Assert (!waitq_empty(q));
  struct interruptor* s = q->head;
  q->head = s->next;
  return s;
}

static void waitq_add(struct waitq* q, struct interruptor* s)
{
  if (waitq_empty(q)) {
    q->head = q->tail = s;
  } else {
    q->tail->next = s;
  }
  s->next = NULL;
  q->tail = s;
}

void caml_init_interruptor(struct interruptor* s, atomic_uintnat* interrupt_word)
{
  s->interrupt_word = interrupt_word;
  caml_plat_mutex_init(&s->lock);
  caml_plat_cond_init(&s->cond, &s->lock);
  waitq_init(&s->interrupts);
}

/* must be called with s->lock held */
static uintnat handle_incoming(struct interruptor* s)
{
  uintnat handled = 0;
  Assert (s->running);
  while (!waitq_empty(&s->interrupts)) {
    struct interruptor* sender = waitq_remove(&s->interrupts);
    struct interrupt* req = &sender->current_interrupt;

    /* Unlock s while the handler runs, to allow other
       domains to send us messages. This is necessary to
       avoid deadlocks, since the handler might send
       interrupts */
    caml_plat_unlock(&s->lock);

    req->handler(caml_domain_self(), req->data);
    atomic_store_rel(&req->completed, 1);

    /* lock sender->lock so that we don't broadcast between check and wait */
    caml_plat_lock(&sender->lock);
    caml_plat_broadcast(&sender->cond);
    caml_plat_unlock(&sender->lock);

    caml_plat_lock(&s->lock);
    handled++;
  }
  return handled;
}

void caml_start_interruptor(struct interruptor* s)
{
  caml_plat_lock(&s->lock);
  Assert (waitq_empty(&s->interrupts));
  Assert (!s->running);
  s->running = 1;
  caml_plat_unlock(&s->lock);
}

void caml_stop_interruptor(struct interruptor* s)
{
  caml_plat_lock(&s->lock);
  while (handle_incoming(s) != 0) { }
  s->running = 0;
  caml_plat_unlock(&s->lock);
}

void caml_handle_incoming_interrupts(struct interruptor* s)
{
  caml_plat_lock(&s->lock);
  handle_incoming(s);
  caml_plat_unlock(&s->lock);
}

void caml_yield_until_interrupted(struct interruptor* s)
{
  caml_plat_lock(&s->lock);
  while (handle_incoming(s) == 0) {
    caml_plat_wait(&s->cond);
  }
  caml_plat_unlock(&s->lock);
}

static const uintnat INTERRUPT_MAGIC = (uintnat)(-1); //FIXME dup
int caml_send_interrupt(struct interruptor* self,
                         struct interruptor* target,
                         interrupt_handler handler,
                         void* data)
{
  struct interrupt* req = &self->current_interrupt;
  int i;

  caml_plat_lock(&target->lock);
  if (!target->running) {
    caml_plat_unlock(&target->lock);
    return 0;
  }
  req->handler = handler;
  req->data = data;
  atomic_store_rel(&req->completed, 0);
  waitq_add(&target->interrupts, self);
  /* Signal the condition variable, in case the target is
     itself waiting for an interrupt to be processed elsewhere */
  caml_plat_broadcast(&target->cond); // OPT before/after unlock? elide?
  caml_plat_unlock(&target->lock);

  atomic_store_rel(target->interrupt_word, INTERRUPT_MAGIC);

  /* Often, interrupt handlers are fast, so spin for a bit before waiting */
  for (i=0; i<1000; i++) {
    if (atomic_load_acq(&req->completed)) {
      return 1;
    }
    cpu_relax();
  }

  caml_plat_lock(&self->lock);
  while (1) {
    handle_incoming(self);
    if (atomic_load_acq(&req->completed)) break;
    caml_plat_wait(&self->cond);
  }
  caml_plat_unlock(&self->lock);
  return 1;
}
