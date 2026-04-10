#include <abi-bits/pid_t.h>
#include <mlibc/all-sysdeps.hpp>
#include <mlibc/tcb.hpp>
#include <mlibc/thread.hpp>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/multiproc.h>
#include <time.h>

extern "C" void __mlibc_enter_thread(void *entry, void *user_arg);

namespace mlibc {

[[noreturn]]
void Sysdeps<ThreadExit>::operator()() {
	ker::multiproc::threadExit();
	__builtin_unreachable();
}

static constexpr size_t default_stacksize = 0x200000;

int Sysdeps<PrepareStack>::operator()(
    void **stack,
    void *entry,
    void *arg,
    void *tcb,
    size_t *stack_size,
    size_t *guard_size,
    void **stack_base
) {
	(void)tcb;
	if (!*stack_size)
		*stack_size = default_stacksize;
	if (!*guard_size)
		*guard_size = 0x1000; // 4KB guard page

	uintptr_t map;
	if (*stack) {
		map = (uintptr_t)*stack;
		*guard_size = 0;
	} else {
		void *p = nullptr;
		int r = sysdep<AnonAllocate>(*stack_size + *guard_size, &p);
		if (r)
			return r;
		map = (uintptr_t)p;
	}

	*stack_base = (void *)map;

	// Push entry and user_arg onto the stack (top of region, growing down).
	// The kernel's threadCreate reads these two words and passes them as
	// RDI/RSI to __mlibc_enter_thread, then advances RSP by 16 past them.
	// An extra dummy word is pushed above them so that the resulting RSP
	// (top - 8) satisfies the x86-64 ABI requirement of RSP = 16n-8 at
	// function entry (as if a call instruction had pushed a return address).
	auto sp = reinterpret_cast<uintptr_t *>(map + *guard_size + *stack_size);
	*--sp = 0; // dummy alignment word (skipped by kernel's RSP += 16)
	*--sp = (uintptr_t)arg;
	*--sp = (uintptr_t)entry;
	*stack = (void *)sp;

	return 0;
}

int Sysdeps<Clone>::operator()(void *tcb, pid_t *tid_out, void *stack) {
	int64_t tid = ker::multiproc::threadCreate(tcb, stack, (void *)&__mlibc_enter_thread);
	if (tid < 0)
		return (int)(-tid);
	if (tid_out)
		*tid_out = (pid_t)tid;
	return 0;
}

} // namespace mlibc

// Entry point for every new thread. The kernel sets RIP here and puts
// entry/user_arg in RDI/RSI (read from the prepared stack by threadCreate).
extern "C" void __mlibc_enter_thread(void *entry, void *user_arg) {
	auto *tcb = mlibc::get_current_tcb();

	// Spin until parent stores our TID (written after sys_clone returns).
	while (!__atomic_load_n(&tcb->tid, __ATOMIC_RELAXED))
		mlibc::sysdep<FutexWait>(&tcb->tid, 0, nullptr);

	tcb->invokeThreadFunc(entry, user_arg);

	__atomic_store_n(&tcb->didExit, 1, __ATOMIC_RELEASE);
	mlibc::sysdep<FutexWake>(&tcb->didExit, true);

	mlibc::sysdep<ThreadExit>();
}
