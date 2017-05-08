/***********************************************************************/
/*                                                                     */
/*                                OCaml                                */
/*                                                                     */
/*            Xavier Leroy, projet Cristal, INRIA Rocquencourt         */
/*                                                                     */
/*  Copyright 1996 Institut National de Recherche en Informatique et   */
/*  en Automatique.  All rights reserved.  This file is distributed    */
/*  under the terms of the GNU Library General Public License, with    */
/*  the special exception on linking described in file ../LICENSE.     */
/*                                                                     */
/***********************************************************************/

/* Callbacks from C to OCaml */

#ifndef CAML_CALLBACK_H
#define CAML_CALLBACK_H

#ifndef CAML_NAME_SPACE
#include "compatibility.h"
#endif
#include "mlvalues.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

void caml_init_callbacks (void);

CAMLextern value caml_callback (value closure, value arg);
CAMLextern value caml_callback2 (value closure, value arg1, value arg2);
CAMLextern value caml_callback3 (value closure, value arg1, value arg2,
                                 value arg3);
CAMLextern value caml_callbackN (value closure, int narg, value args[]);

CAMLextern value caml_callback_exn (value closure, value arg);
CAMLextern value caml_callback2_exn (value closure, value arg1, value arg2);
CAMLextern value caml_callback3_exn (value closure,
                                     value arg1, value arg2, value arg3);
CAMLextern value caml_callbackN_exn (value closure, int narg, value args[]);

#define Make_exception_result(v) ((v) | 2)
#define Is_exception_result(v) (((v) & 3) == 2)
#define Extract_exception(v) ((v) & ~3)

CAMLextern caml_root caml_named_root (char const * name);

CAMLextern void caml_main (char ** argv);
CAMLextern void caml_startup (char ** argv);

CAMLextern int caml_get_callback_depth (void);
void caml_incr_callback_depth (void);
void caml_decr_callback_depth (void);

#ifdef __cplusplus
}
#endif

#endif
