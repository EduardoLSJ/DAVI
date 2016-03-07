/*
 * Using UART1 module
 *
 * Tested on:
 * Microstick
 *
 * File name: rb.h
 * Author:    Eduardo Lopes
 * Info:      elopes_74@hotmail.com
 *
 * Last modification: 03-11-2013
 */

#ifndef _RB_H
#define _RB_H

#include "typedefs.h"

/*---------------------------------------------------------------------------
  MACRO: defines a ring buffer administration structure; the name of this
 *  structure is to be specified through the first macro parameter (rb),
 *  the type of element to be stored in this buffer is to be specified
 *  through the second macro parameter (type)
---------------------------------------------------------------------------*/
#define RB_CREATE(rb, type) \
   struct { \
     type *rb_start; \
     type *rb_end; \
     type *rb_in; \
     type *rb_out; \
   } rb
/*---------------------------------------------------------------------------
  MACRO: initializes the ring buffer administration structure; head and tail
 *  pointer (rb_in and rb_out, respectively) point to the beginning of the
 *  empty buffer (rb_start); the end-of-buffer pointer is set to the end
 *  element of the buffer
---------------------------------------------------------------------------*/
#define RB_INIT(rb, start, number) \
         ( (rb)->rb_in = (rb)->rb_out= (rb)->rb_start= start, \
           (rb)->rb_end = &(rb)->rb_start[number] )

/*---------------------------------------------------------------------------
  MACRO: is a support macro which makes the buffer circular; all access to
 *  the elements of the buffer (read and write operations) needs to be done
 *  through this macro
---------------------------------------------------------------------------*/
#define RB_SLOT(rb, slot) \
         ( (slot)==(rb)->rb_end? (rb)->rb_start: (slot) )

/*---------------------------------------------------------------------------
  MACRO: tests if the buffer is completely empty; in this implementation,
 *  the ring buffer is empty when head and tail pointer address the same
 *  buffer element (rb_in = rb_out)
---------------------------------------------------------------------------*/
#define RB_EMPTY(rb) ( (rb)->rb_in==(rb)->rb_out )

/*---------------------------------------------------------------------------
  MACRO: tests if the buffer is full; this is the case when tail = head + 1.
 *  Note that support macro RB_SLOT is used to ensure that the buffer is
 *  wrapped around (if required)
---------------------------------------------------------------------------*/
#define RB_FULL(rb)  ( RB_SLOT(rb, (rb)->rb_in+1)==(rb)->rb_out )

/*---------------------------------------------------------------------------
  MACRO: returns the address of the element the head pointer currently
 *  points to
---------------------------------------------------------------------------*/
#define RB_PUSHSLOT(rb) ( (rb)->rb_in )

/*---------------------------------------------------------------------------
  MACRO: returns the address of the element the tail pointer currently
 *  points to
---------------------------------------------------------------------------*/
#define RB_POPSLOT(rb)  ( (rb)->rb_out )

/*---------------------------------------------------------------------------
  MACRO: advances the head pointer of buffer ?rb? by one,
 *  taking into account possibly required buffer wrapping
---------------------------------------------------------------------------*/
#define RB_PUSHADVANCE(rb) ( (rb)->rb_in= RB_SLOT((rb), (rb)->rb_in+1) )

/*---------------------------------------------------------------------------
  MACRO: advances the tail pointer of buffer ?rb? by one, taking into account
 *  possibly required buffer wrapping
---------------------------------------------------------------------------*/
#define RB_POPADVANCE(rb)  ( (rb)->rb_out= RB_SLOT((rb), (rb)->rb_out+1) )

#endif
