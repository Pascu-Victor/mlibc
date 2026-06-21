
// for memcpy()
#include <string.h>

#include <bits/ensure.h>
#include <bits/threads.h>
#include <bits/types.h>
#include <mlibc/allocator.hpp>
#include <mlibc/threads.hpp>

#include <frg/allocation.hpp>
#include <frg/eternal.hpp>
#include <frg/vector.hpp>

namespace {

struct ExitHandler {
	void (*function)(void *);
	void *argument;
	void *dsoHandle;
};

using ExitQueue = frg::vector<ExitHandler, MemoryAllocator>;

struct ThreadExitHandler {
	void (*function)(void *);
	void *argument;
	void *dsoHandle;
	ThreadExitHandler *next;
};

__mlibc_once threadExitKeyOnce = __MLIBC_THREAD_ONCE_INITIALIZER;
__mlibc_uintptr threadExitKey;
int threadExitKeyError;
int threadExitKeyReady;

ExitQueue &getExitQueue() {
	// use frg::eternal to prevent the compiler from scheduling the destructor
	// by generating a call to __cxa_atexit().
	static frg::eternal<ExitQueue> singleton(getAllocator());
	return singleton.get();
}

void runThreadExitHandlers(void *value) {
	auto pending = static_cast<ThreadExitHandler *>(value);
	mlibc::thread_key_set(threadExitKey, nullptr);

	while (pending) {
		auto handler = pending;
		pending = handler->next;

		handler->function(handler->argument);
		frg::destruct(getAllocator(), handler);

		auto recursive = static_cast<ThreadExitHandler *>(mlibc::thread_key_get(threadExitKey));
		if (!recursive)
			continue;

		mlibc::thread_key_set(threadExitKey, nullptr);
		auto tail = recursive;
		while (tail->next)
			tail = tail->next;
		tail->next = pending;
		pending = recursive;
	}
}

void initializeThreadExitKey() {
	threadExitKeyError = mlibc::thread_key_create(&threadExitKey, runThreadExitHandlers);
	__atomic_store_n(&threadExitKeyReady, !threadExitKeyError, __ATOMIC_RELEASE);
}

int registerThreadExitHandler(void (*function)(void *), void *argument, void *dsoHandle) {
	if (mlibc::thread_once(&threadExitKeyOnce, initializeThreadExitKey))
		return -1;
	if (threadExitKeyError)
		return -1;

	auto handler = frg::construct<ThreadExitHandler>(getAllocator());
	if (!handler)
		return -1;

	handler->function = function;
	handler->argument = argument;
	handler->dsoHandle = dsoHandle;
	handler->next = static_cast<ThreadExitHandler *>(mlibc::thread_key_get(threadExitKey));

	if (mlibc::thread_key_set(threadExitKey, handler)) {
		frg::destruct(getAllocator(), handler);
		return -1;
	}

	return 0;
}

} // namespace

extern "C" int __cxa_atexit(void (*function)(void *), void *argument, void *handle) {
	ExitHandler handler;
	handler.function = function;
	handler.argument = argument;
	handler.dsoHandle = handle;
	getExitQueue().push(handler);
	return 0;
}

extern "C" int
__cxa_thread_atexit_impl(void (*function)(void *), void *argument, void *dsoHandle) {
	return registerThreadExitHandler(function, argument, dsoHandle);
}

extern "C" void __mlibc_run_thread_dtors() {
	if (!__atomic_load_n(&threadExitKeyReady, __ATOMIC_ACQUIRE))
		return;

	auto pending = static_cast<ThreadExitHandler *>(mlibc::thread_key_get(threadExitKey));
	if (pending)
		runThreadExitHandlers(pending);
}

extern "C" void __dlapi_exit();

extern "C" void __cxa_finalize(void *dso) {
	ExitQueue &eq = getExitQueue();
	for (size_t i = eq.size(); i > 0; i--) {
		auto &handler = eq[i - 1];
		if (!handler.function)
			continue;

		if (!dso || handler.dsoHandle == dso) {
			handler.function(handler.argument);
			handler.function = nullptr;
		}
	}
}

// In static builds, these should be provided by the crtbegin.o/crtend.o that
// is linked into the executable.
#ifndef MLIBC_STATIC_BUILD
// This is referenced by the compiler when generating constructors for global
// C++ objects so that it can call __cxa_finalize with a unique argument.
extern "C" {
[[gnu::visibility("hidden")]] void *__dso_handle;
}
#else
extern "C" void *__dso_handle;
#endif

[[gnu::destructor]] void __mlibc_do_destructors() {
	// In normal programs this call to __cxa_finalize is provided by libgcc.
	__cxa_finalize(&__dso_handle);
}

void __mlibc_do_finalize() {
	__mlibc_run_thread_dtors();

	// Invoke any handlers registered with atexit (NOT associated with a DSO).
	// Note that we deliberately do not invoke other handlers here, since
	// that would destroy mlibc's global objects including stdout and flushing
	// open FILEs, but we'd like those to be available to [[gnu::destructor]]
	// functions which we invoke below.
	ExitQueue &eq = getExitQueue();
	for (size_t i = eq.size(); i > 0; i--) {
		auto &handler = eq[i - 1];
		if (!handler.function)
			continue;

		if (!handler.dsoHandle) {
			handler.function(handler.argument);
			handler.function = nullptr;
		}
	}

	// Call fini/fini_array functions of each loaded object. This is necessary
	// to implement [[gnu::destructor]]. Note that C++ applications will call
	// __cxa_finalize from here.
	__dlapi_exit();
}
