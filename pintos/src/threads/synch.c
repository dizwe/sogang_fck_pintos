/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
*/

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
     decrement it.

   - up or "V": increment the value (and wake up one waiting
     thread, if any). */
void
sema_init (struct semaphore *sema, unsigned value) 
{
  ASSERT (sema != NULL);

  sema->value = value;
  list_init (&sema->waiters);
}

/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. */
void
sema_down (struct semaphore *sema) 
{
  enum intr_level old_level;

  ASSERT (sema != NULL);
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  while (sema->value == 0) 
    {
      if(thread_mlfqs) 
	list_push_back (&sema->waiters, &thread_current()->elem);

      else{
        list_insert_ordered (&sema->waiters, &thread_current ()->elem, thread_priority_comp, NULL);}
      thread_block ();
    }
  sema->value--;
  intr_set_level (old_level);
}

/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */
bool
sema_try_down (struct semaphore *sema) 
{
  enum intr_level old_level;
  bool success;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  if (sema->value > 0) 
    {
      sema->value--;
      success = true; 
    }
  else
    success = false;
  intr_set_level (old_level);

  return success;
}

/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up one thread of those waiting for SEMA, if any.

   This function may be called from an interrupt handler. */
void
sema_up (struct semaphore *sema) 
{
  enum intr_level old_level;
  struct thread * t_thread,* max_thread;
  struct list_elem * t_elem,* max_elem;

  ASSERT (sema != NULL);

//  max_thread->priority = -1;

  old_level = intr_disable ();
  if(thread_mlfqs){
  if(!list_empty(&sema->waiters))
    thread_unblock(list_entry(list_pop_front(&sema->waiters), struct thread, elem));
  sema->value++;
  intr_set_level(old_level);
  return;
  }

  sema->value++;
  if (!list_empty (&sema->waiters)){
//    max_thread = list_begin(&sema->waiters);
    max_elem = list_begin(&sema->waiters);
    max_thread = list_entry(max_elem, struct thread, elem);
    
//    t_elem = list_next(max_elem); 
    for(t_elem = list_begin(&sema->waiters); t_elem != list_end(&sema->waiters); t_elem = list_next(t_elem)){
        t_thread = list_entry(t_elem, struct thread, elem);
	if(t_thread->priority > max_thread->priority){
	  max_thread = t_thread;
	  max_elem = t_elem;
	}
  }
    list_remove(max_elem);
    thread_unblock(max_thread);
  }
//  sema->value++;
  intr_set_level (old_level);
//  thread_yield();
}

static void sema_test_helper (void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */
void
sema_self_test (void) 
{
  struct semaphore sema[2];
  int i;

  printf ("Testing semaphores...");
  sema_init (&sema[0], 0);
  sema_init (&sema[1], 0);
  thread_create ("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
  for (i = 0; i < 10; i++) 
    {
      sema_up (&sema[0]);
      sema_down (&sema[1]);
    }
  printf ("done.\n");
}

/* Thread function used by sema_self_test(). */
static void
sema_test_helper (void *sema_) 
{
  struct semaphore *sema = sema_;
  int i;

  for (i = 0; i < 10; i++) 
    {
      sema_down (&sema[0]);
      sema_up (&sema[1]);
    }
}

/* Initializes LOCK.  A lock can be held by at most a single
   thread at any given time.  Our locks are not "recursive", that
   is, it is an error for the thread currently holding a lock to
   try to acquire that lock.

   A lock is a specialization of a semaphore with an initial
   value of 1.  The difference between a lock and such a
   semaphore is twofold.  First, a semaphore can have a value
   greater than 1, but a lock can only be owned by a single
   thread at a time.  Second, a semaphore does not have an owner,
   meaning that one thread can "down" the semaphore and then
   another one "up" it, but with a lock the same thread must both
   acquire and release it.  When these restrictions prove
   onerous, it's a good sign that a semaphore should be used,
   instead of a lock. */
void
lock_init (struct lock *lock)
{
  ASSERT (lock != NULL);

  lock->holder = NULL;
  sema_init (&lock->semaphore, 1);
  if(!thread_mlfqs){lock->lock_priority = PRI_MIN;}
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
lock_acquire (struct lock *lock)
{
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (!lock_held_by_current_thread (lock));
 /*lock_acquire하는 동안 다른 interrupt가 방해할 수 없도록 interrupt disable */
  if(thread_mlfqs){
    sema_down(&lock->semaphore);
    lock->holder = thread_current();
    return;
  } 

  /*  현재 thread의 lock이 Argument로 넘어온 lock인 것으로 설정한다 
    그리고 만약 그 lock이 NULL이면 lock_donation을 할 필요가 없다. */
  struct thread * cur_thread = thread_current();
  struct thread * cur_holder = lock->holder;
  struct lock * cur_lock = lock;
  
  /* 현재 lock의 holder가 NULL이면 lock을 하고 있는 thread가 없는 것이므로 현재 lock의 priority도
  자연스럽게 현재 thread의 priority가 된다. 왜냐하면 현재 thread가 lock을 잡을 것이기 때문이다.*/

  if(cur_holder == NULL) 
    cur_lock->lock_priority = cur_thread->priority;

  cur_thread->lock_already = lock;

    while(cur_holder != NULL){
      if(cur_holder->priority >= cur_thread->priority) break;
      cur_holder->flag_priority = 1;
      cur_holder->priority = cur_thread->priority;
//      compare_and_yield(cur_holder, cur_thread->priority);
//      lock_release_list_insert_ordered(cur_holder);

//      if (lock_donation(lock) == 4) break ;
      
    /* lock에 적힌 priority보다 현재 thread의 priority가 더 크면 당연스럽게 
	donate가 발생했을 것이므로 이렇게 해준다. */

      if(cur_lock -> lock_priority < cur_thread -> priority){
	cur_lock -> lock_priority = cur_thread -> priority;
	}

      if((cur_holder -> lock_already != NULL)){
	cur_lock = cur_holder-> lock_already;
	cur_holder = cur_lock -> holder;
        }
      else {
	break;
	}
      }
    

  sema_down(&lock->semaphore);

  cur_thread = thread_current();
  lock->holder = cur_thread;

  cur_thread->lock_already = NULL;
  list_insert_ordered(&cur_thread->lock_list, &lock->lock_elem, lock_compare, NULL);
}

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   hread.

   This function will not sleep, so it may be called within an
   interrupt handler. */
bool
lock_try_acquire (struct lock *lock)
{
  bool success;

  ASSERT (lock != NULL);
  ASSERT (!lock_held_by_current_thread (lock));

  success = sema_try_down (&lock->semaphore);
  if (success)
    lock->holder = thread_current ();
  return success;
}

/* Releases LOCK, which must be owned by the current thread.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */
void
lock_release (struct lock *lock) 
{
  ASSERT (lock != NULL);
  ASSERT (lock_held_by_current_thread (lock));

  lock->holder = NULL;
  sema_up(&lock->semaphore);

  if(thread_mlfqs) return;

  struct thread * cur_thread = thread_current();
  struct semaphore cur_sema = lock->semaphore;
  struct list cur_waiter_list = cur_sema.waiters;

  list_remove(&lock->lock_elem);
//  lock->lock_priority = PRI_MIN;
  
  /* lock_list가 empty라면 현재 thread에서 lock을 기다렸던 친구들이 없다는 것! 
  그러므로 그냥 본인의 original priority만 돌려받으면 그만이다. */ 

  if(list_empty(&cur_thread->lock_list)){
    cur_thread->flag_priority = 0;  // donation 받은거 없슴 결백함.
    cur_thread->priority = cur_thread->original_priority;  // original priority 받고
    compare_and_yield(cur_thread, cur_thread->original_priority);
    }
  /* lock_list가 empty가 아니라면 donation이 발생했을 확률이 있다. 
    lock_list에서 가장 priority가 높은 녀석을 찾으서 현재 thread와 같으면
    그녀석이 기부천사이다 */
//  else if(cur_thread->flag_priority > 0){
     else{
      list_sort(&cur_thread->lock_list ,lock_compare, NULL);
      struct lock * donation_angel = list_entry(list_front(&cur_thread->lock_list), struct lock, lock_elem);
      
      cur_thread->priority = donation_angel->lock_priority;
      compare_and_yield(cur_thread, donation_angel->lock_priority);
//      lock_release_list_insert_ordered(cur_thread);
      }
    }


 
/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */
bool
lock_held_by_current_thread (const struct lock *lock) 
{
  ASSERT (lock != NULL);

  return lock->holder == thread_current ();
}

/* One semaphore in a list. */
struct semaphore_elem 
  {
    struct list_elem elem;              /* List element. */
    struct semaphore semaphore;         /* This semaphore. */
  };

/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */
void
cond_init (struct condition *cond)
{
  ASSERT (cond != NULL);

  list_init (&cond->waiters);
}

/* Atomically releases LOCK and waits for COND to be signaled by
   some other piece of code.  After COND is signaled, LOCK is
   reacquired before returning.  LOCK must be held before calling
   this function.

   The monitor implemented by this function is "Mesa" style, not
   "Hoare" style, that is, sending and receiving a signal are not
   an atomic operation.  Thus, typically the caller must recheck
   the condition after the wait completes and, if necessary, wait
   again.

   A given condition variable is associated with only a single
   lock, but one lock may be associated with any number of
   condition variables.  That is, there is a one-to-many mapping
   from locks to condition variables.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
cond_wait (struct condition *cond, struct lock *lock) 
{
  struct semaphore_elem waiter;

  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));
  
  sema_init (&waiter.semaphore, 0);
  if(!thread_mlfqs){
  waiter.semaphore.sema_priority = thread_current()->priority;
  list_insert_ordered (&cond->waiters, &waiter.elem, cond_compare, NULL);
  }
  else{
  list_push_back(&cond->waiters, &waiter.elem);
  }

  lock_release (lock);
  sema_down (&waiter.semaphore);
  lock_acquire (lock);
}

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_signal (struct condition *cond, struct lock *lock UNUSED) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));

  if (!list_empty (&cond->waiters)) 
    sema_up (&list_entry (list_pop_front (&cond->waiters),
                          struct semaphore_elem, elem)->semaphore);
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_broadcast (struct condition *cond, struct lock *lock) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);

  while (!list_empty (&cond->waiters))
    cond_signal (cond, lock);
}

bool lock_compare(const struct list_elem *a, const struct list_elem *b, void *aux){
  return list_entry(a, struct lock, lock_elem)->lock_priority > list_entry(b, struct lock, lock_elem)->lock_priority;
}

bool cond_compare(const struct list_elem *a, const struct list_elem *b, void *aux){
  return list_entry(a, struct semaphore_elem, elem)->semaphore.sema_priority > list_entry(b, struct semaphore_elem, elem)->semaphore.sema_priority;
}
