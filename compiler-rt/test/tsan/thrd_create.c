// RUN: %clang_tsan -O1 %s -o %t && %run %t 2>&1 | FileCheck %s
// REQUIRES: glibc-2.30
// Regression test for C11 <threads.h> support: glibc's thrd_* wrappers reach
// the pthread entry points without going through the PLT, so before they were
// intercepted the spawned thread ran with an uninitialized ThreadState and
// crashed as soon as it entered instrumented code.
#include "test.h"

#include <threads.h>

int Global;

int Thread(void *x) {
  // Before the fix this faulted on entry to the first instrumented function.
  Global = 42;
  return 7;
}

int ExitThread(void *x) {
  thrd_exit(11);
  return 0;
}

int DetachedThread(void *x) {
  barrier_wait(&barrier);
  return 0;
}

int main() {
  thrd_t t;
  int res = 0;

  if (thrd_create(&t, Thread, NULL) != thrd_success)
    return 1;
  if (thrd_join(t, &res) != thrd_success)
    return 1;
  if (res != 7) {
    fprintf(stderr, "BAD RETVAL %d\n", res);
    return 1;
  }
  // thrd_join must establish a happens-before edge with the joined thread;
  // without it this write races with Thread's write above.
  Global++;

  // thrd_exit's value must reach thrd_join too.
  if (thrd_create(&t, ExitThread, NULL) != thrd_success)
    return 1;
  if (thrd_join(t, &res) != thrd_success)
    return 1;
  if (res != 11) {
    fprintf(stderr, "BAD EXIT RETVAL %d\n", res);
    return 1;
  }

  // A detached thread must be unregistered without a join, and must not be
  // reported as a thread leak.
  barrier_init(&barrier, 2);
  if (thrd_create(&t, DetachedThread, NULL) != thrd_success)
    return 1;
  barrier_wait(&barrier);
  if (thrd_detach(t) != thrd_success)
    return 1;

  fprintf(stderr, "DONE\n");
  return 0;
}

// CHECK-NOT: WARNING: ThreadSanitizer
// CHECK-NOT: ThreadSanitizer:DEADLYSIGNAL
// CHECK-NOT: SEGV
// CHECK: DONE
