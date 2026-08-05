// RUN: %clang_tsan -O1 %s -o %t && %deflake %run %t 2>&1 | FileCheck %s
// REQUIRES: glibc-2.30
// Threads created through C11 thrd_create must be instrumented like any other,
// so a genuine race between two of them is still reported.
#include "test.h"

#include <threads.h>

int Global;

int Thread1(void *x) {
  barrier_wait(&barrier);
  Global = 42;
  return 0;
}

int Thread2(void *x) {
  Global = 43;
  barrier_wait(&barrier);
  return 0;
}

int main() {
  barrier_init(&barrier, 2);
  thrd_t t[2];
  thrd_create(&t[0], Thread1, NULL);
  thrd_create(&t[1], Thread2, NULL);
  thrd_join(t[0], NULL);
  thrd_join(t[1], NULL);
  return 0;
}

// CHECK: WARNING: ThreadSanitizer: data race
