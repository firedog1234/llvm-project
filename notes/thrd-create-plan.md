# Fix TSan crash with C11 `<threads.h>` (llvm/llvm-project#199585)

## Context

[Issue #199585](https://github.com/llvm/llvm-project/issues/199585) (open, unassigned, labels `c11` + `compiler-rt:tsan`): a program that uses C11 `<threads.h>` (`thrd_create`/`thrd_join`) instead of raw `pthread_create` crashes with a TSan-internal SEGV as soon as the spawned thread does anything (e.g. hits an `assert`). The same program using `pthread_create` directly works fine. This has been a known gap since at least 2022 (originally reported against upstream ThreadSanitizer before it moved into llvm-project).

**Root cause** (confirmed by reading the code, not guessing): TSan intercepts `pthread_create` via standard `dlsym(RTLD_NEXT, ...)`-based ELF symbol interposition (`compiler-rt/lib/interception/interception_linux.cpp`). glibc's `thrd_create` (added glibc 2.28) is implemented as a thin wrapper that calls `pthread_create` through an **internal/hidden alias**, not through the public dynamically-resolved symbol — so TSan's interceptor never fires for that call. Consequently the new thread never runs through TSan's `__tsan_thread_start_func` trampoline (`compiler-rt/lib/tsan/rtl/tsan_interceptors_posix.cpp:1046-1078`), which is the *only* place a real `ThreadState` gets placement-newed into the thread's TLS buffer (via `ThreadStart`, `tsan_rtl_thread.cpp:170-228`). The thread's `ThreadState` stays zero-initialized (`is_inited=false`, `shadow_stack_pos=nullptr`).

Ordinary intercepted libc calls tolerate this (`MustIgnoreInterceptor`, `tsan_interceptors.h:48-55`, forwards straight to `REAL()` when `!thr->is_inited`), but **compiler-instrumented code does not check this at all** — e.g. `CurrentStackId` (`tsan_rtl.cpp:932-949`) unconditionally writes `thr->shadow_stack_pos[0] = pc`, a null-pointer write on such a thread → SIGSEGV. This exactly matches the reported symptom (crash triggered by ordinary instrumented code / `assert` internals in the child thread).

No sanitizer in this tree (tsan, asan, msan, ubsan) currently intercepts any C11 `<threads.h>` function on any platform (confirmed via exhaustive grep). The closest precedent for "a libc wrapper bypasses an existing interceptor by calling an internal alias" is the `clone` interceptor at `tsan_interceptors_posix.cpp:2464-2497`, which reimplements the needed bookkeeping around `REAL(clone)` rather than relying on a higher-level interceptor.

**Goal of this change:** add TSan interceptors for `thrd_create` (fixes the crash) and `thrd_join`/`thrd_detach`/`thrd_exit` (closes the same interposition-bypass gap for join/detach/exit, which would otherwise silently produce missed happens-before edges / false-positive races once `thrd_create` alone is fixed). `mtx_*`/`cnd_*`/`tss_*`/C11 `call_once` are explicitly out of scope (separate, lower-severity issue — worth filing separately, not fixing here).

## What to study first

- `compiler-rt/lib/tsan/rtl/tsan_interceptors_posix.cpp:1037-1178` — the `ThreadParam` struct, `__tsan_thread_start_func` trampoline, and the `pthread_create`/`pthread_join` interceptors. This is the template being adapted.
- `compiler-rt/lib/tsan/rtl/tsan_rtl_thread.cpp` — `ThreadCreate`/`ThreadStart`/`ThreadJoin`, to understand what bookkeeping a new thread's registration actually requires.
- `compiler-rt/lib/tsan/rtl/tsan_interceptors_posix.cpp:2464-2497` — the `clone` interceptor, as precedent for "reimplement bookkeeping around `REAL()` of a lower-level primitive instead of trusting a higher interceptor to fire."
- `compiler-rt/lib/interception/interception_linux.cpp` — how `dlsym(RTLD_NEXT, ...)` interposition works and why internal glibc aliases bypass it.
- `compiler-rt/lib/sanitizer_common/sanitizer_platform.h:41-45` — `SANITIZER_GLIBC` macro (`defined(__GLIBC__)`), the correct guard for this glibc-specific fix (musl/Bionic/Apple/*BSD don't have this bug the same way, and Bionic/Apple don't even provide `<threads.h>`).
- `compiler-rt/lib/sanitizer_common/sanitizer_errno_codes.h` — currently defines `errno_EBUSY`/`errno_EINVAL` but not `errno_EAGAIN`, which is needed for correct `thrd_create` return-code translation.
- `compiler-rt/test/tsan/pthread_mutex_clocklock.cpp` and `pthread_key.cpp` — the test pattern to copy (`// RUN: %clang_tsan ...`, `// REQUIRES: glibc-2.30`, `#include "test.h"`, `CHECK`/`CHECK-NOT`). `glibc-2.30` is already a defined lit feature (`compiler-rt/test/lit.common.cfg.py:749-763`) and is safely ≥ 2.28 (when glibc added `<threads.h>`), so no lit-config changes are needed.

## Implementation plan

### 1. Refactor `pthread_create`'s interceptor to expose a shared helper

In `tsan_interceptors_posix.cpp`, extract everything from the current `pthread_create` interceptor *except* the `attr`-specific setup (default-attr init, `pthread_attr_getdetachstate`, `AdjustStackSize`) into a static helper:

```cpp
static int DoPthreadCreate(ThreadState *thr, uptr pc, void *th, void *attr,
                            void *(*callback)(void *), void *param,
                            bool detached) {
  MaybeSpawnBackgroundThread();
  if (ctx->after_multithreaded_fork) { /* existing Report/Die block, unchanged */ }
  ThreadParam p;
  p.callback = callback;
  p.param = param;
  p.tid = kMainTid;
  int res = -1;
  {
    ScopedIgnoreInterceptors ignore;
    ThreadIgnoreBegin(thr, pc);
    res = REAL(pthread_create)(th, attr, __tsan_thread_start_func, &p);
    ThreadIgnoreEnd(thr);
  }
  if (res == 0) {
    p.tid = ThreadCreate(thr, pc, *(uptr *)th, detached);
    CHECK_NE(p.tid, kMainTid);
    p.created.Post();
    p.started.Wait();
  }
  AdaptiveDelay::AfterThreadCreation();
  return res;
}
```

`pthread_create`'s interceptor becomes a thin wrapper around `DoPthreadCreate`, keeping only the attr handling. This is a pure refactor — no behavior change; existing pthread_create tests must still pass unmodified.

### 2. New `thrd_create` interceptor, gated `#if SANITIZER_GLIBC`

`thrd_start_t` is `int (*)(void*)` (returns `int`, not `void*`), so add a small adapter mirroring glibc's own internal `thread_wrapper_function`.

**Lifetime caveat (caught in review — do not skip):** the naive design of stack-allocating the adapter's param struct in the `thrd_create` interceptor and passing a pointer to it into `DoPthreadCreate` is **unsafe**, unlike `ThreadParam` in the existing `pthread_create` path. Tracing `__tsan_thread_start_func` (tsan_interceptors_posix.cpp:1046-1078): the child only calls the user callback *after* `p->started.Post()` — which is the same event that unblocks the parent's `p.started.Wait()` and lets `DoPthreadCreate`/the interceptor return to its caller. For plain `pthread_create`, `ThreadParam`'s fields are read out *before* that handshake completes, so the parent frame is guaranteed alive. But if `thrd_create` passes a pointer to a *stack-local* adapter struct, and the adapter itself is what reads `callback`/`param` (via `thrd_callback_wrapper`), that read happens *after* the parent has already returned — i.e. after the `thrd_create` interceptor's stack frame may have been reused by the caller's next statement. That's a use-after-scope race, not merely a style nit.

**Fix: heap-allocate the adapter struct with `InternalAlloc`/`InternalFree`** (the sanitizer-safe allocation helpers already used throughout this file, e.g. tsan_interceptors_posix.cpp:666,732,873 — not `new`/`malloc`, which would re-enter the intercepted allocator), and free it from inside the child's wrapper after copying out the fields it needs, with a matching free on the interceptor's failure path (thread never started, so nothing will free it otherwise):

```cpp
struct ThrdParam {
  int (*callback)(void *arg);
  void *param;
};

static void *thrd_callback_wrapper(void *arg) {
  ThrdParam *p = static_cast<ThrdParam *>(arg);
  int (*callback)(void *) = p->callback;
  void *param = p->param;
  InternalFree(p);
  int res = callback(param);
  return reinterpret_cast<void *>(static_cast<uptr>(res));
}

TSAN_INTERCEPTOR(int, thrd_create, void *th, int (*callback)(void *), void *param) {
  SCOPED_INTERCEPTOR_RAW(thrd_create, th, callback, param);
  auto *tp = static_cast<ThrdParam *>(InternalAlloc(sizeof(ThrdParam)));
  tp->callback = callback;
  tp->param = param;
  int res = DoPthreadCreate(thr, pc, th, nullptr, thrd_callback_wrapper, tp,
                             /*detached=*/false);
  if (res != 0) InternalFree(tp);  // thread never started; wrapper never ran
  if (res == 0) return 0;                    // thrd_success
  if (res == errno_EAGAIN) return 3;         // thrd_nomem
  return 2;                                  // thrd_error
}
```

- `th` (`thrd_t*`) is treated as opaque `void*`, matching how `pthread_create`'s interceptor already treats `pthread_t*` — valid because glibc's `thrd_t` is `typedef pthread_t thrd_t`, and the existing code deliberately avoids typed libc-header parameters in interceptors.
- Return-code translation mirrors glibc's actual `thrd_create` implementation (`err == EAGAIN → thrd_nomem`, else `thrd_error`, `0 → thrd_success`) — hardcode the three C11 constants as bare literals with a comment (don't `#include <threads.h>`; no interceptor in this file includes target libc headers). These literal values (`thrd_success=0, thrd_error=2, thrd_nomem=3`) match the well-known glibc/musl C11 enum ordering but **must be double-checked against actual glibc `<threads.h>` source during implementation** rather than trusted from this plan — not independently verifiable in this environment (macOS has no C11 `<threads.h>`).
- Add `errno_EAGAIN` (value `11`) to `compiler-rt/lib/sanitizer_common/sanitizer_errno_codes.h` next to `errno_EBUSY`/`errno_EINVAL` in the non-Haiku branch (matches existing convention; unused/undefined on Haiku is fine since this call site is `SANITIZER_GLIBC`-gated). Confirmed via grep: `EAGAIN` is not currently defined anywhere else in `sanitizer_errno_codes.h`.

### 3. Companion interceptors: `thrd_join` (required), `thrd_detach`/`thrd_exit` (recommended, same patch)

`thrd_join` is not optional: glibc's `thrd_join` bypasses interception the same way (internal `__pthread_join` alias). Without it, fixing only `thrd_create` trades the SEGV for a silent correctness regression — the child thread is never `ThreadJoin()`'d in TSan's `ThreadRegistry`, so the happens-before edge from child to joiner is never established, producing false-positive races on code that's actually race-free.

```cpp
TSAN_INTERCEPTOR(int, thrd_join, void *th, int *res) {
  SCOPED_INTERCEPTOR_RAW(thrd_join, th, res);
  Tid tid = ThreadConsumeTid(thr, pc, (uptr)th);
  ThreadIgnoreBegin(thr, pc);
  void *pthread_res = nullptr;
  int ret = BLOCK_REAL(pthread_join)(th, &pthread_res);   // BLOCK_REAL, not REAL — needed for deadlock detector
  ThreadIgnoreEnd(thr);
  if (ret != 0) return 2;  // thrd_error
  ThreadJoin(thr, pc, tid);
  if (res) *res = static_cast<int>(reinterpret_cast<uptr>(pthread_res));
  return 0;  // thrd_success
}
```

`thrd_detach`/`thrd_exit` are cheap 3-6 line mirrors of `pthread_detach`/`pthread_exit` with the same return-code translation pattern; include them in the same patch unless reviewers push back on scope (they can be split into a fast follow-up without weakening the fix for the reported crash — `thrd_create`+`thrd_join` are the load-bearing pair).

### 4. Registration

In `InitializeInterceptors()`, right after the existing `TSAN_INTERCEPT(pthread_exit);` (`tsan_interceptors_posix.cpp` ~line 3075):

```cpp
#if SANITIZER_GLIBC
  TSAN_INTERCEPT(thrd_create);
  TSAN_INTERCEPT(thrd_join);
  TSAN_INTERCEPT(thrd_detach);
  TSAN_INTERCEPT(thrd_exit);
#endif
```

Wrap the interceptor *definitions* themselves in the same `#if SANITIZER_GLIBC` (not just the registration), so no non-glibc platform that happens to export a same-named symbol could pick these up unexpectedly at link time.

### 5. Tests — new files in `compiler-rt/test/tsan/`

**`thrd_create.c`** — direct regression test for the reported SEGV:
```c
// RUN: %clang_tsan -O1 %s -o %t && %run %t 2>&1 | FileCheck %s
// REQUIRES: glibc-2.30
#include "test.h"
#include <threads.h>
#include <assert.h>

int Global;

int Thread(void *x) {
  Global = 42;
  assert(Global == 42);
  return 0;
}

int main() {
  thrd_t t;
  assert(thrd_create(&t, Thread, NULL) == thrd_success);
  int res;
  assert(thrd_join(t, &res) == thrd_success);
  fprintf(stderr, "DONE\n");
}
// CHECK-NOT: WARNING: ThreadSanitizer
// CHECK-NOT: SEGV
// CHECK: DONE
```

**`thrd_join_race.c`** — positive-detection test proving instrumentation genuinely works post-fix (not just "doesn't crash"), mirroring `simple_race.c`: two `thrd_create`d threads racing on a global via `barrier_wait` (from `test.h`), expecting `CHECK: WARNING: ThreadSanitizer: data race`. Use `%deflake %run %t` (not plain `%run %t`) for the RUN line, matching `simple_race.c`'s convention — a genuine race test without `%deflake` is flaky. Optionally add a second scenario in the same file (or a `thrd_detach.c`) proving a properly-joined access produces `CHECK-NOT: WARNING`, confirming `thrd_join`'s happens-before edge is established correctly.

## Risk / edge cases

- **musl**: unverified whether musl's `thrd_create` has the same bypass; gate strictly to `SANITIZER_GLIBC` (not `SANITIZER_LINUX`) to avoid touching musl with unverified semantics. `REQUIRES: glibc-2.30` on the new tests already excludes musl test configs.
- **Android/Bionic, Apple, *BSD**: none provide `<threads.h>`; `SANITIZER_GLIBC` guard correctly excludes all of them, so `TSAN_INTERCEPT` never looks for a missing symbol there.
- **ASan/MSan have the same gap**, but `pthread_create` interceptors are independently duplicated per-sanitizer (no shared layer to fix once) and the failure mode there is less severe (no `ThreadState`-null-deref SEGV analogous to TSan's `CurrentStackId`). Out of scope here — worth filing as a separate follow-up issue, not blocking this fix.

## Plan review notes

This plan was independently checked against the codebase by a second pass (not just asserted). Confirmed correct on re-verification:
- Passing `nullptr` as `attr` to `REAL(pthread_create)` is fine — POSIX/glibc treat `NULL` attr as "use defaults"; the existing `myattr` dance in `pthread_create`'s interceptor exists only to satisfy `pthread_attr_getdetachstate`'s non-null requirement, not because `pthread_create` itself needs it.
- No existing `#if SANITIZER_GLIBC` block or symbol name collides with the new interceptors' insertion point.
- All helpers referenced (`IsStateDetached`, `ThreadConsumeTid`, `BLOCK_REAL`, `ThreadJoin`, `AdjustStackSize`) are already defined/visible earlier in the same translation unit.
- `errno_EAGAIN` is genuinely absent from `sanitizer_errno_codes.h` today.
- `thrd_join` being load-bearing (not deferrable) holds up given `ThreadJoin`'s role in establishing the happens-before edge in `tsan_rtl_thread.cpp`.

The one substantive bug this review caught — the `ThrdParam` stack-lifetime race — is already folded into the design above (§2, "Lifetime caveat"). Two open items to resolve during implementation, not before: verify the `thrd_success`/`thrd_error`/`thrd_nomem` literal values against real glibc/musl source (couldn't be checked from this environment), and use `%deflake %run %t` in the race test's RUN line.

## Verification

1. Build compiler-rt/tsan (see `[[libcxx_build_test_workflow]]`-style cmake/ninja setup already used for libc++ in this environment, adapted to the `compiler-rt` target — e.g. `ninja tsan` or the relevant check target).
2. Run the original issue's repro directly: `clang -std=c11 -fsanitize=thread -g repro.c && ./a.out` — should exit cleanly instead of SEGV-ing.
3. Run the new lit tests: `ninja check-tsan` (or `llvm-lit compiler-rt/test/tsan/thrd_create.c compiler-rt/test/tsan/thrd_join_race.c -v`).
4. Run the full existing `check-tsan` suite to confirm the `DoPthreadCreate` refactor didn't regress `pthread_create`/`pthread_join` behavior (existing `pthread_*.c`/`.cpp` tests should be unaffected).
