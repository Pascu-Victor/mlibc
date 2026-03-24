// IMPORTANT: Do NOT include all-sysdeps.hpp or ansi-sysdeps.hpp here.
// Those headers declare sys_prepare_stack/sys_clone/sys_thread_exit as
// [[gnu::weak]]. In C++, the first declaration's attributes apply to the
// definition, so including them before our definitions makes our symbols
// weak — causing the GOT slots to stay null and the mlibc null-check in
// thread_create to incorrectly treat the sysdeps as missing.
//
// We manually declare only the helpers we call, without weak linkage.

#include <abi-bits/pid_t.h>
#include <mlibc/tcb.hpp>
#include <mlibc/thread.hpp>
#include <stddef.h>
#include <sys/multiproc.h>
#include <time.h>

struct timespec;

namespace mlibc {
int sys_anon_allocate(size_t size, void **pointer);
int sys_futex_wait(int *pointer, int expected, const ::timespec *time);
int sys_futex_wake(int *pointer);
} // namespace mlibc

extern "C" void __mlibc_enter_thread(void *entry, void *user_arg);

namespace mlibc {

[[noreturn]] void sys_thread_exit() {
	ker::multiproc::threadExit();
	__builtin_unreachable();
}

int sys_prepare_stack(
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
		*stack_size = 0x200000; // 2MB default
	if (!*guard_size)
		*guard_size = 0x1000; // 4KB guard page

	uintptr_t map;
	if (*stack) {
		map = (uintptr_t)*stack;
		*guard_size = 0;
	} else {
		void *p = nullptr;
		int r = sys_anon_allocate(*stack_size + *guard_size, &p);
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
	*--sp = 0;              // dummy alignment word (skipped by kernel's RSP += 16)
	*--sp = (uintptr_t)arg;
	*--sp = (uintptr_t)entry;
	*stack = (void *)sp;

	return 0;
}

int sys_clone(void *tcb, pid_t *tid_out, void *stack) {
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
		mlibc::sys_futex_wait(&tcb->tid, 0, nullptr);

	tcb->invokeThreadFunc(entry, user_arg);

	__atomic_store_n(&tcb->didExit, 1, __ATOMIC_RELEASE);
	mlibc::sys_futex_wake(&tcb->didExit);

	mlibc::sys_thread_exit();
}
