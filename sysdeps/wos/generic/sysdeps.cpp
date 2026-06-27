#include <abi-bits/pid_t.h>
#include <algorithm>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <mlibc/all-sysdeps.hpp>
#include <mlibc/debug.hpp>
#include <mlibc/dlapi.hpp>
#include <mlibc/fsfd_target.hpp>
#include <mlibc/tcb.hpp>
#include <mlibc/thread.hpp>
#include <sched.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/callnums.h>
#include <sys/epoll.h>
#include <sys/file.h>
#include <sys/futex.h>
#include <sys/ioctl.h>
#include <sys/logging.h>
#include <sys/mman.h>
#include <sys/multiproc.h>
#include <sys/net.h>
#include <sys/poll.h>
#include <sys/process.h>
#include <sys/resource.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/time_calls.h>
#include <sys/time_ops.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/utsname.h>
#include <sys/vfs.h>
#include <sys/vmem.h>
#include <termios.h>
#include <wos/netctl.h>

#include <callnums/shm.h>

// SafeStack support: This variable is accessed by the compiler-generated code
// It needs to be in TLS storage and properly initialized
// The actual initialization will be done by the kernel when setting up TLS
extern "C" __attribute__((visibility("default"))) __thread void *__safestack_unsafe_stack_ptr =
    nullptr;

extern "C" __attribute__((weak)) char *
__cxa_demangle(const char *mangled_name, char *output_buffer, size_t *length, int *status);

namespace [[gnu::visibility("hidden")]] mlibc {

namespace {

constexpr size_t kMaxStackWords = 32;
constexpr size_t kMaxCallTraceFrames = 64;
constexpr uintptr_t kMaxFrameStride = 1024 * 1024;

bool panicActive = false;

void panicLog(uint64_t cookie, const char *message) {
	if (cookie != 0) {
		ker::logging::logExBlock(
		    cookie, "mlibc", ker::abi::sys_log::sys_log_level::PANIC, message, strlen(message)
		);
		return;
	}

	ker::logging::logEx("mlibc", ker::abi::sys_log::sys_log_level::PANIC, message, strlen(message));
}

int pselect_sleep_for_timeout_ms(int timeout_ms, const sigset_t *sigmask) {
	if (timeout_ms == 0)
		return 0;

	if (timeout_ms < 0) {
		sigset_t active_mask{};
		const sigset_t *wait_mask = sigmask;
		if (!wait_mask) {
			int64_t r = ker::process::sigprocmask(0, nullptr, &active_mask);
			if (r < 0)
				return static_cast<int>(-r);
			wait_mask = &active_mask;
		}

		int64_t r = ker::process::sigsuspend(wait_mask);
		if (r < 0)
			return static_cast<int>(-r);
		return EINTR;
	}

	sigset_t old_mask{};
	bool restore_mask = false;
	if (sigmask) {
		int64_t r = ker::process::sigprocmask(SIG_SETMASK, sigmask, &old_mask);
		if (r < 0)
			return static_cast<int>(-r);
		restore_mask = true;
	}

	timespec req{
	    .tv_sec = timeout_ms / 1000,
	    .tv_nsec = static_cast<long>((timeout_ms % 1000) * 1000000),
	};
	timespec rem{.tv_sec = 0, .tv_nsec = 0};
	uint64_t r = syscall(
	    ker::abi::callnums::time,
	    static_cast<uint64_t>(ker::abi::sys_time_ops::nanosleep),
	    reinterpret_cast<uint64_t>(&req),
	    reinterpret_cast<uint64_t>(&rem)
	);
	int result = 0;
	if (static_cast<int64_t>(r) < 0)
		result = static_cast<int>(-static_cast<int64_t>(r));

	if (restore_mask) {
		int64_t restore = ker::process::sigprocmask(SIG_SETMASK, &old_mask, nullptr);
		if (result == 0 && restore < 0)
			result = static_cast<int>(-restore);
	}

	return result;
}

int apply_wait_signal_mask(const sigset_t *sigmask, sigset_t *old_mask, bool *restore_mask) {
	if (!sigmask) {
		*restore_mask = false;
		return 0;
	}

	int64_t r = ker::process::sigprocmask(SIG_SETMASK, sigmask, old_mask);
	if (r < 0)
		return static_cast<int>(-r);
	*restore_mask = true;
	return 0;
}

int restore_wait_signal_mask(const sigset_t *old_mask, bool restore_mask) {
	if (!restore_mask)
		return 0;

	int64_t r = ker::process::sigprocmask(SIG_SETMASK, old_mask, nullptr);
	if (r < 0)
		return static_cast<int>(-r);
	return 0;
}

uintptr_t currentStackPointer() {
#if defined(__x86_64__)
	uintptr_t sp = 0;
	asm volatile("mov %%rsp, %0" : "=r"(sp));
	return sp;
#else
	return reinterpret_cast<uintptr_t>(__builtin_frame_address(0));
#endif
}

bool isAligned(uintptr_t value) { return (value % alignof(uintptr_t)) == 0; }

bool isReasonableNextFrame(uintptr_t current, uintptr_t next) {
	if (next == 0) {
		return false;
	}
	if (!isAligned(next)) {
		return false;
	}
	if (next <= current) {
		return false;
	}
	return (next - current) <= kMaxFrameStride;
}

struct PanicLine {
	char buffer[ker::abi::sys_log::JOURNAL_MESSAGE_MAX]{};
	size_t length = 0;

	void append(char c) {
		if (length < sizeof(buffer)) {
			buffer[length++] = c;
		}
	}

	void append(const char *text) {
		if (text == nullptr) {
			text = "(null)";
		}
		while (*text != '\0' && length < sizeof(buffer)) {
			buffer[length++] = *text++;
		}
	}

	void appendDecimal(size_t value, size_t minWidth = 0) {
		char digits[32]{};
		size_t count = 0;
		do {
			digits[count++] = static_cast<char>('0' + (value % 10));
			value /= 10;
		} while (value != 0 && count < sizeof(digits));

		while (count < minWidth) {
			append('0');
			--minWidth;
		}
		while (count > 0) {
			append(digits[--count]);
		}
	}

	void appendHex(uintptr_t value, size_t minWidth = 0) {
		constexpr char hexDigits[] = "0123456789abcdef";
		char digits[sizeof(uintptr_t) * 2]{};
		size_t count = 0;
		do {
			digits[count++] = hexDigits[value & 0xf];
			value >>= 4;
		} while (value != 0 && count < sizeof(digits));

		while (count < minWidth) {
			append('0');
			--minWidth;
		}
		while (count > 0) {
			append(digits[--count]);
		}
	}

	void flush(uint64_t cookie) const {
		if (cookie != 0) {
			ker::logging::logExBlock(
			    cookie, "mlibc", ker::abi::sys_log::sys_log_level::PANIC, buffer, length
			);
			return;
		}

		ker::logging::logEx("mlibc", ker::abi::sys_log::sys_log_level::PANIC, buffer, length);
	}
};

struct ResolvedSymbol {
	const char *file = nullptr;
	const char *symbol = nullptr;
	uintptr_t objectBase = 0;
	uintptr_t symbolAddress = 0;
};

bool resolveSymbol(uintptr_t pc, ResolvedSymbol *resolved) {
#if !MLIBC_BUILDING_RTLD
	__dlapi_symbol info{};
	if (__dlapi_reverse(reinterpret_cast<void *>(pc), &info) != 0) {
		return false;
	}

	resolved->file = info.file;
	resolved->symbol = info.symbol;
	resolved->objectBase = reinterpret_cast<uintptr_t>(info.base);
	resolved->symbolAddress = reinterpret_cast<uintptr_t>(info.address);
	return true;
#else
	(void)pc;
	(void)resolved;
	return false;
#endif
}

const char *maybeDemangle(const char *symbol, char **ownedBuffer) {
	*ownedBuffer = nullptr;
#if !MLIBC_BUILDING_RTLD
	if (symbol == nullptr || symbol[0] != '_' || symbol[1] != 'Z') {
		return symbol;
	}
	if (__cxa_demangle == nullptr) {
		return symbol;
	}

	int status = 0;
	char *demangled = __cxa_demangle(symbol, nullptr, nullptr, &status);
	if (status == 0 && demangled != nullptr) {
		*ownedBuffer = demangled;
		return demangled;
	}
	free(demangled);
#endif
	return symbol;
}

void dumpStackWords(uint64_t cookie) {
	auto sp = currentStackPointer();
	{
		PanicLine line;
		line.append("Stack dump: sp=0x");
		line.appendHex(sp, sizeof(uintptr_t) * 2);
		line.append('\n');
		line.flush(cookie);
	}

	auto *words = reinterpret_cast<uintptr_t *>(sp);
	for (size_t i = 0; i < kMaxStackWords; ++i) {
		PanicLine line;
		line.append("  sp+0x");
		line.appendHex(i * sizeof(uintptr_t), 3);
		line.append("  0x");
		line.appendHex(words[i], sizeof(uintptr_t) * 2);
		line.flush(cookie);
	}
}

void printCallTraceFrame(uint64_t cookie, size_t frameIndex, uintptr_t returnAddress) {
	auto pc = returnAddress == 0 ? returnAddress : returnAddress - 1;

	ResolvedSymbol resolved{};
	if (!resolveSymbol(pc, &resolved)) {
		PanicLine line;
		line.append("  #");
		line.appendDecimal(frameIndex, 2);
		line.append(" pc=0x");
		line.appendHex(pc, sizeof(uintptr_t) * 2);
		line.append(" <unresolved>");
		line.flush(cookie);
		return;
	}

	const char *file = resolved.file != nullptr ? resolved.file : "<unknown object>";
	if (resolved.symbol == nullptr) {
		auto objectOffset =
		    (resolved.objectBase != 0 && pc >= resolved.objectBase) ? pc - resolved.objectBase : 0;
		PanicLine line;
		line.append("  #");
		line.appendDecimal(frameIndex, 2);
		line.append(" pc=0x");
		line.appendHex(pc, sizeof(uintptr_t) * 2);
		line.append(' ');
		line.append(file);
		line.append("+0x");
		line.appendHex(objectOffset);
		line.append(" <no symbol>");
		line.flush(cookie);
		return;
	}

	char *demangled = nullptr;
	const char *symbol = maybeDemangle(resolved.symbol, &demangled);
	auto symbolOffset = (resolved.symbolAddress != 0 && pc >= resolved.symbolAddress)
	                        ? pc - resolved.symbolAddress
	                        : 0;
	PanicLine line;
	line.append("  #");
	line.appendDecimal(frameIndex, 2);
	line.append(" pc=0x");
	line.appendHex(pc, sizeof(uintptr_t) * 2);
	line.append(' ');
	line.append(file);
	line.append(':');
	line.append(symbol);
	line.append("+0x");
	line.appendHex(symbolOffset);
	line.flush(cookie);
#if !MLIBC_BUILDING_RTLD
	free(demangled);
#else
	(void)demangled;
#endif
}

[[gnu::noinline]]
void dumpCallTrace(uint64_t cookie) {
	auto *frame = static_cast<uintptr_t *>(__builtin_frame_address(0));
	panicLog(cookie, "Call trace:");

	for (size_t i = 0; i < kMaxCallTraceFrames && frame != nullptr; ++i) {
		auto frameAddress = reinterpret_cast<uintptr_t>(frame);
		auto nextFrame = frame[0];
		auto returnAddress = frame[1];

		if (returnAddress == 0) {
			PanicLine line;
			line.append("  #");
			line.appendDecimal(i, 2);
			line.append(" <null return address>");
			line.flush(cookie);
			break;
		}

		printCallTraceFrame(cookie, i, returnAddress);

		if (!isReasonableNextFrame(frameAddress, nextFrame)) {
			if (nextFrame != 0) {
				PanicLine line;
				line.append("  stopped: invalid frame link from 0x");
				line.appendHex(frameAddress, sizeof(uintptr_t) * 2);
				line.append(" to 0x");
				line.appendHex(nextFrame, sizeof(uintptr_t) * 2);
				line.flush(cookie);
			}
			break;
		}

		frame = reinterpret_cast<uintptr_t *>(nextFrame);
	}
}

} // namespace

bool cpu_bit_is_set(const cpu_set_t *set, size_t cpusetsize, size_t cpu) {
	size_t byte_index = cpu / CHAR_BIT;
	if (byte_index >= cpusetsize) {
		return false;
	}
	auto bytes = reinterpret_cast<const unsigned char *>(set);
	return (bytes[byte_index] & (1U << (cpu % CHAR_BIT))) != 0;
}

void cpu_bit_set(cpu_set_t *set, size_t cpusetsize, size_t cpu) {
	size_t byte_index = cpu / CHAR_BIT;
	if (byte_index >= cpusetsize) {
		return;
	}
	auto bytes = reinterpret_cast<unsigned char *>(set);
	bytes[byte_index] |= static_cast<unsigned char>(1U << (cpu % CHAR_BIT));
}

int cpuset_to_mask(size_t cpusetsize, const cpu_set_t *set, uint64_t *mask) {
	uint64_t affinity_mask = 0;
	size_t max_bits = cpusetsize * CHAR_BIT;
	for (size_t cpu = 0; cpu < max_bits; ++cpu) {
		if (!cpu_bit_is_set(set, cpusetsize, cpu)) {
			continue;
		}
		if (cpu >= 64) {
			return ENOTSUP;
		}
		affinity_mask |= 1ULL << cpu;
	}
	*mask = affinity_mask;
	return 0;
}

int mask_to_cpuset(uint64_t mask, size_t cpusetsize, cpu_set_t *set) {
	memset(set, 0, cpusetsize);
	for (size_t cpu = 0; cpu < 64; ++cpu) {
		if ((mask & (1ULL << cpu)) == 0) {
			continue;
		}
		if (cpu >= cpusetsize * CHAR_BIT) {
			return ERANGE;
		}
		cpu_bit_set(set, cpusetsize, cpu);
	}
	return 0;
}

// ---- Base sysdeps (always required) ----

void Sysdeps<LibcLog>::operator()(const char *message) {
	ker::logging::logEx("mlibc", ker::abi::sys_log::sys_log_level::INFO, message, strlen(message));
}

[[noreturn]]
void Sysdeps<LibcPanic>::operator()() {
	if (__atomic_exchange_n(&panicActive, true, __ATOMIC_ACQ_REL)) {
		uint64_t const cookie = ker::logging::beginLogBlock();
		panicLog(cookie, "\nMLIBC PANIC (recursive)\n");
		ker::logging::endLogBlock(cookie);
		sysdep<Exit>(1);
		__builtin_unreachable();
	}

	uint64_t const cookie = ker::logging::beginLogBlock();
	panicLog(cookie, "\nMLIBC PANIC\n");
	dumpStackWords(cookie);
	dumpCallTrace(cookie);
	ker::logging::endLogBlock(cookie);
	sysdep<Exit>(1);
	__builtin_unreachable();
}

pid_t Sysdeps<FutexTid>::operator()() {
	uint64_t tid = ker::multiproc::currentThreadId();
	return tid;
}

int Sysdeps<FutexWake>::operator()(int *pointer, bool all) {
	int64_t result = ker::futex::wake(pointer, all ? INT_MAX : 1);
	if (result < 0) {
		return static_cast<int>(-result);
	}
	return 0;
}

int Sysdeps<FutexWait>::operator()(int *pointer, int expected, timespec const *timeout) {
	int64_t result = ker::futex::wait(pointer, expected, timeout);
	if (result < 0) {
		return static_cast<int>(-result);
	}
	return 0;
}

int Sysdeps<AnonAllocate>::operator()(size_t size, void **pointer) {
	return sysdep<VmMap>(
	    nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0, pointer
	);
}

int Sysdeps<AnonFree>::operator()(void *pointer, size_t size) {
	return sysdep<VmUnmap>(pointer, size);
}

int Sysdeps<VmMap>::operator()(
    void *hint, size_t size, int prot, int flags, int fd, off_t offset, void **window
) {
	int64_t result = ker::vmem::map(window, size, prot, flags, fd, offset, hint);
	if (result < 0) {
		int e = static_cast<int>(-result);
		mlibc::infoLogger() << "mlibc: VmMap failed: errno=" << e << " size=" << size << " prot=0x"
		                    << frg::hex_fmt{static_cast<uint64_t>(prot)} << " flags=0x"
		                    << frg::hex_fmt{static_cast<uint64_t>(flags)} << " fd=" << fd
		                    << " offset=" << offset << " hint=" << hint << frg::endlog;
		return e;
	}
	return 0;
}

int Sysdeps<VmUnmap>::operator()(void *pointer, size_t size) {
	int result = (int)ker::vmem::free(pointer, size);
	if (result < 0) {
		return (-result);
	}
	return 0;
}

int Sysdeps<VmProtect>::operator()(void *pointer, size_t size, int prot) {
	int64_t result = ker::vmem::protect(pointer, size, prot);
	if (result < 0) {
		return (int)(-result);
	}
	return 0;
}

int Sysdeps<Msync>::operator()(void *addr, size_t length, int flags) {
	int64_t result = ker::vmem::sync(addr, length, static_cast<uint64_t>(flags));
	if (result < 0) {
		return static_cast<int>(-result);
	}
	return 0;
}

int Sysdeps<Madvise>::operator()(void *addr, size_t length, int advice) {
	(void)addr;
	(void)length;

	switch (advice) {
		case MADV_NORMAL:
		case MADV_RANDOM:
		case MADV_SEQUENTIAL:
		case MADV_WILLNEED:
			return 0;
		case MADV_DONTNEED:
		case MADV_FREE:
		case MADV_REMOVE:
		case MADV_DONTFORK:
		case MADV_DOFORK:
		case MADV_MERGEABLE:
		case MADV_UNMERGEABLE:
		case MADV_HUGEPAGE:
		case MADV_NOHUGEPAGE:
		case MADV_DONTDUMP:
		case MADV_DODUMP:
		case MADV_WIPEONFORK:
		case MADV_KEEPONFORK:
		case MADV_COLD:
		case MADV_PAGEOUT:
		case MADV_HWPOISON:
		case MADV_SOFT_OFFLINE:
			return ENOSYS;
		default:
			return EINVAL;
	}
}

int Sysdeps<PosixMadvise>::operator()(void *addr, size_t length, int advice) {
	switch (advice) {
		case POSIX_MADV_NORMAL:
			return sysdep<Madvise>(addr, length, MADV_NORMAL);
		case POSIX_MADV_RANDOM:
			return sysdep<Madvise>(addr, length, MADV_RANDOM);
		case POSIX_MADV_SEQUENTIAL:
			return sysdep<Madvise>(addr, length, MADV_SEQUENTIAL);
		case POSIX_MADV_WILLNEED:
			return sysdep<Madvise>(addr, length, MADV_WILLNEED);
		case POSIX_MADV_DONTNEED:
			return 0;
		default:
			return EINVAL;
	}
}

int Sysdeps<Sysconf>::operator()(int num, long *ret) {
	switch (num) {
		case _SC_CHILD_MAX:
			*ret = 25;
			return 0;
		case _SC_CLK_TCK:
			*ret = 100;
			return 0;
		case _SC_NPROCESSORS_CONF:
		case _SC_NPROCESSORS_ONLN: {
			auto const cpu_count = static_cast<long>(ker::multiproc::nativeThreadCount());
			*ret = cpu_count > 0 ? cpu_count : 1;
			return 0;
		}
		default:
			return EINVAL;
	}
}

int Sysdeps<Stat>::operator()(
    fsfd_target fsfdt, int fd, const char *path, int flags, struct stat *statbuf
) {
	int r = 0;
	// AT_SYMLINK_NOFOLLOW = 0x100 — when set, do NOT follow symlinks (lstat)
	bool follow_symlinks = !(flags & 0x100);
	switch (fsfdt) {
		case fsfd_target::path:
			r = follow_symlinks ? ker::abi::vfs::stat_path(path, statbuf)
			                    : ker::abi::vfs::lstat_path(path, statbuf);
			break;
		case fsfd_target::fd:
			r = ker::abi::vfs::fstat_fd(fd, statbuf);
			break;
		case fsfd_target::fd_path:
			r = ker::abi::vfs::statat_path(fd, path, flags, statbuf);
			break;
		default:
			return EINVAL;
	}
	if (r < 0) {
		return -r;
	}
	return 0;
}

int Sysdeps<TcbSet>::operator()(void *tcb) {
	if (!tcb)
		return -1;
	return ker::multiproc::setTCB(tcb);
}

[[noreturn]]
void Sysdeps<Exit>::operator()(int status) {
	ker::process::exit(status);
	__builtin_unreachable();
}

int Sysdeps<Open>::operator()(const char *pathname, int flags, mode_t mode, int *fd) {
	int r = ker::abi::vfs::open(pathname, flags, mode);
	if (r < 0)
		return -r;
	*fd = r;
	return 0;
}

int Sysdeps<OpenDir>::operator()(const char *path, int *fd) {
	return sysdep<Open>(path, O_DIRECTORY, 0, fd);
}

int
Sysdeps<ReadEntries>::operator()(int handle, void *buffer, size_t max_size, size_t *bytes_read) {
	ssize_t result = ::ker::abi::vfs::read_dir_entries(handle, buffer, max_size);
	if (result < 0) {
		return -result;
	}
	*bytes_read = result;
	return 0;
}

int Sysdeps<Read>::operator()(int fd, void *buf, size_t count, ssize_t *bytes_read) {
	ssize_t r = ker::abi::vfs::read(fd, buf, count);
	if (r < 0)
		return (int)(-r);
	if (bytes_read)
		*bytes_read = r;
	return 0;
}

int Sysdeps<Write>::operator()(int fd, const void *buf, size_t count, ssize_t *bytes_written) {
	ssize_t r = ker::abi::vfs::write(fd, buf, count);
	if (r < 0)
		return (int)(-r);
	if (bytes_written)
		*bytes_written = r;
	return 0;
}

int
Sysdeps<Writev>::operator()(int fd, const struct iovec *iovs, int iovc, ssize_t *bytes_written) {
	ssize_t total = 0;
	for (int i = 0; i < iovc; i++) {
		if (iovs[i].iov_len == 0)
			continue;
		ssize_t r = ker::abi::vfs::write(fd, iovs[i].iov_base, iovs[i].iov_len);
		if (r < 0) {
			if (total > 0)
				break;
			if (bytes_written)
				*bytes_written = 0;
			return (int)(-r);
		}
		total += r;
		if (static_cast<size_t>(r) < iovs[i].iov_len)
			break;
	}
	if (bytes_written)
		*bytes_written = total;
	return 0;
}

int Sysdeps<Readv>::operator()(int fd, const struct iovec *iovs, int iovc, ssize_t *bytes_read) {
	ssize_t total = 0;
	for (int i = 0; i < iovc; i++) {
		if (iovs[i].iov_len == 0)
			continue;
		ssize_t r = ker::abi::vfs::read(fd, iovs[i].iov_base, iovs[i].iov_len);
		if (r < 0) {
			if (total > 0)
				break;
			if (bytes_read)
				*bytes_read = 0;
			return (int)(-r);
		}
		total += r;
		if (r == 0 || static_cast<size_t>(r) < iovs[i].iov_len)
			break;
	}
	if (bytes_read)
		*bytes_read = total;
	return 0;
}

int Sysdeps<Pread>::operator()(int fd, void *buf, size_t n, off_t off, ssize_t *bytes_read) {
	ssize_t r = ker::abi::vfs::pread(fd, buf, n, off);
	if (r < 0)
		return (int)(-r);
	if (bytes_read)
		*bytes_read = r;
	return 0;
}

int Sysdeps<Seek>::operator()(int fd, off_t offset, int whence, off_t *new_offset) {
	off_t r = ker::abi::vfs::lseek(fd, offset, whence);
	if (r < 0)
		return (int)(-r);
	*new_offset = r;
	return 0;
}

int Sysdeps<Close>::operator()(int fd) {
	int r = ker::abi::vfs::close(fd);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<ClockGet>::operator()(int clock_id, time_t *secs, long *nanos) {
	timespec ts{};
	uint64_t res = ker::time::clock_gettime(clock_id, &ts);
	if ((int64_t)res < 0) {
		return (int)(-(int64_t)res);
	}
	if (secs)
		*secs = ts.tv_sec;
	if (nanos)
		*nanos = ts.tv_nsec;
	return 0;
}

namespace {
constexpr clock_t wosClockTicksPerSecond = 100;

void ticks_to_timeval(clock_t ticks, timeval *tv) {
	tv->tv_sec = ticks / wosClockTicksPerSecond;
	tv->tv_usec = (ticks % wosClockTicksPerSecond) * (1000000 / wosClockTicksPerSecond);
}
} // namespace

int Sysdeps<GetRusage>::operator()(int scope, struct rusage *usage) {
	if (!usage)
		return EFAULT;

	tms times_buf{};
	clock_t elapsed = 0;
	uint64_t res = ker::time::times(&times_buf, &elapsed);
	if ((int64_t)res < 0)
		return static_cast<int>(-static_cast<int64_t>(res));

	memset(usage, 0, sizeof(*usage));
	switch (scope) {
		case RUSAGE_SELF:
			ticks_to_timeval(times_buf.tms_utime, &usage->ru_utime);
			ticks_to_timeval(times_buf.tms_stime, &usage->ru_stime);
			return 0;
		case RUSAGE_CHILDREN:
			ticks_to_timeval(times_buf.tms_cutime, &usage->ru_utime);
			ticks_to_timeval(times_buf.tms_cstime, &usage->ru_stime);
			return 0;
		default:
			return EINVAL;
	}
}

int Sysdeps<Sleep>::operator()(time_t *secs, long *nanos) {
	timespec req{};
	req.tv_sec = secs ? *secs : 0;
	req.tv_nsec = nanos ? *nanos : 0;
	timespec rem{.tv_sec = 0, .tv_nsec = 0};

	uint64_t r = syscall(
	    ker::abi::callnums::time,
	    static_cast<uint64_t>(ker::abi::sys_time_ops::nanosleep),
	    reinterpret_cast<uint64_t>(&req),
	    reinterpret_cast<uint64_t>(&rem)
	);
	if ((int64_t)r < 0)
		return (int)(-(int64_t)r);
	if (secs)
		*secs = rem.tv_sec;
	if (nanos)
		*nanos = rem.tv_nsec;
	return 0;
}

int Sysdeps<Isatty>::operator()(int fd) {
	bool is_tty = ker::abi::vfs::isatty(fd);
	return is_tty ? 0 : ENOTTY;
}

int Sysdeps<Rmdir>::operator()(const char *path) {
	int r = ker::abi::vfs::rmdir(path);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Unlinkat>::operator()(int dirfd, const char *path, int flags) {
	int r = ker::abi::vfs::unlinkat(dirfd, path, flags);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Rename>::operator()(const char *old_path, const char *new_path) {
	int r = ker::abi::vfs::rename(old_path, new_path);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Sigprocmask>::operator()(
    int how, const sigset_t *__restrict set, sigset_t *__restrict retrieve
) {
	int64_t r = ker::process::sigprocmask(how, (const void *)set, (void *)retrieve);
	if (r < 0)
		return (int)(-r);
	return 0;
}

int Sysdeps<Sigpending>::operator()(sigset_t *set) {
	int64_t r = ker::process::sigpending((void *)set);
	if (r < 0)
		return (int)(-r);
	return 0;
}

#if !MLIBC_BUILDING_RTLD
extern "C" void __mlibc_signal_restore();
extern "C" void __mlibc_signal_restore_rt();

int Sysdeps<Sigaction>::operator()(
    int signum, const struct sigaction *__restrict act, struct sigaction *__restrict oldact
) {
	struct sigaction modified_act{};
	const struct sigaction *act_ptr = act;
	if (act) {
		modified_act = *act;
		modified_act.sa_flags |= SA_RESTORER;
		modified_act.sa_restorer =
		    (act->sa_flags & SA_SIGINFO) ? __mlibc_signal_restore_rt : __mlibc_signal_restore;
		act_ptr = &modified_act;
	}

	int64_t r = ker::process::sigaction(signum, (const void *)act_ptr, (void *)oldact);
	if (r < 0)
		return (int)(-r);
	return 0;
}
#endif // !MLIBC_BUILDING_RTLD

int Sysdeps<Fork>::operator()(pid_t *child) {
	int64_t r = ker::process::fork();
	if (r < 0)
		return (int)(-r);
	*child = (pid_t)r;
	return 0;
}

int
Sysdeps<Waitpid>::operator()(pid_t pid, int *status, int flags, struct rusage *ru, pid_t *ret_pid) {
	int64_t result = ker::process::waitpid(pid, status, flags, ru);
	if (result < 0) {
		return (int)(-result);
	}
	if (ret_pid)
		*ret_pid = (pid_t)result;
	return 0;
}

int Sysdeps<Execve>::operator()(const char *path, char *const argv[], char *const envp[]) {
	int64_t r = ker::process::execve(
	    path, const_cast<const char *const *>(argv), const_cast<const char *const *>(envp)
	);
	if (r < 0)
		return static_cast<int>(-r);
	return 0;
}

void Sysdeps<Yield>::operator()() { ker::multiproc::yield(); }

pid_t Sysdeps<GetPid>::operator()() { return ker::process::getpid(); }

int Sysdeps<Kill>::operator()(pid_t pid, int sig) {
	int64_t r = ker::process::kill((int64_t)pid, sig);
	if (r < 0)
		return (int)(-r);
	return 0;
}

int Sysdeps<Ptrace>::operator()(long req, pid_t pid, void *addr, void *data, long *out) {
	int64_t r = ker::process::ptrace(
	    static_cast<uint64_t>(req),
	    static_cast<uint64_t>(pid),
	    reinterpret_cast<uint64_t>(addr),
	    reinterpret_cast<uint64_t>(data)
	);
	if (r < 0)
		return static_cast<int>(-r);
	if (out)
		*out = r;
	return 0;
}

// ---- POSIX sysdeps ----

#if __MLIBC_POSIX_OPTION && !MLIBC_BUILDING_RTLD

int Sysdeps<Shmget>::operator()(int *shm_id, key_t key, size_t size, int shmflg) {
	auto result = static_cast<int64_t>(syscall(
	    ker::abi::callnums::shm,
	    static_cast<uint64_t>(ker::abi::shm::ops::get),
	    static_cast<uint64_t>(static_cast<int64_t>(key)),
	    size,
	    static_cast<uint64_t>(shmflg)
	));
	if (result < 0)
		return static_cast<int>(-result);
	*shm_id = static_cast<int>(result);
	return 0;
}

int Sysdeps<Shmat>::operator()(void **seg_start, int shmid, const void *shmaddr, int shmflg) {
	auto result = static_cast<int64_t>(syscall(
	    ker::abi::callnums::shm,
	    static_cast<uint64_t>(ker::abi::shm::ops::attach),
	    static_cast<uint64_t>(shmid),
	    reinterpret_cast<uint64_t>(shmaddr),
	    static_cast<uint64_t>(shmflg)
	));
	if (result < 0)
		return static_cast<int>(-result);
	*seg_start = reinterpret_cast<void *>(static_cast<uint64_t>(result));
	return 0;
}

int Sysdeps<Shmdt>::operator()(const void *shmaddr) {
	auto result = static_cast<int64_t>(syscall(
	    ker::abi::callnums::shm,
	    static_cast<uint64_t>(ker::abi::shm::ops::detach),
	    reinterpret_cast<uint64_t>(shmaddr)
	));
	if (result < 0)
		return static_cast<int>(-result);
	return 0;
}

int Sysdeps<Shmctl>::operator()(int *idx, int shmid, int cmd, struct shmid_ds *buf) {
	auto result = static_cast<int64_t>(syscall(
	    ker::abi::callnums::shm,
	    static_cast<uint64_t>(ker::abi::shm::ops::ctl),
	    static_cast<uint64_t>(shmid),
	    static_cast<uint64_t>(cmd),
	    reinterpret_cast<uint64_t>(buf)
	));
	if (result < 0)
		return static_cast<int>(-result);
	if (idx)
		*idx = static_cast<int>(result);
	return 0;
}

int
Sysdeps<Pwrite>::operator()(int fd, const void *buf, size_t n, off_t off, ssize_t *bytes_written) {
	ssize_t r = ker::abi::vfs::pwrite(fd, buf, n, off);
	if (r < 0)
		return (int)(-r);
	if (bytes_written)
		*bytes_written = r;
	return 0;
}

int Sysdeps<Access>::operator()(const char *path, int mode) {
	int r = ker::abi::vfs::access(path, mode);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Faccessat>::operator()(int dirfd, const char *pathname, int mode, int flags) {
	int r = ker::abi::vfs::faccessat(dirfd, pathname, mode, flags);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Dup>::operator()(int fd, int flags, int *newfd) {
	(void)flags;
	int r = ker::abi::vfs::dup(fd);
	if (r < 0)
		return -r;
	*newfd = r;
	return 0;
}

int Sysdeps<Dup2>::operator()(int fd, int flags, int newfd) {
	int r = ker::abi::vfs::dup2(fd, newfd, flags);
	if (r < 0)
		return -r;
	return 0;
}

int
Sysdeps<Readlink>::operator()(const char *path, void *buffer, size_t max_size, ssize_t *length) {
	ssize_t r = ker::abi::vfs::readlink(path, static_cast<char *>(buffer), max_size);
	if (r < 0)
		return static_cast<int>(-r);
	if (length)
		*length = r;
	return 0;
}

int Sysdeps<Realpath>::operator()(const char *path, char *buffer, size_t size) {
	int r = ker::abi::vfs::realpath(path, buffer, size);
	if (r < 0)
		return -r;
	return r;
}

int Sysdeps<Readlinkat>::operator()(
    int dirfd, const char *path, void *buffer, size_t max_size, ssize_t *length
) {
	if (dirfd == -100 || (path && path[0] == '/')) {
		return sysdep<Readlink>(path, buffer, max_size, length);
	}
	return ENOSYS;
}

int Sysdeps<Ftruncate>::operator()(int fd, size_t size) {
	int r = ker::abi::vfs::truncate(fd, (off_t)size);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Truncate>::operator()(const char *path, off_t length) {
	int fd = ker::abi::vfs::open(path, 1 /* O_WRONLY */, 0);
	if (fd < 0)
		return -fd;
	int r = ker::abi::vfs::truncate(fd, length);
	ker::abi::vfs::close(fd);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Openat>::operator()(int dirfd, const char *path, int flags, mode_t mode, int *fd) {
	int r = ker::abi::vfs::openat(dirfd, path, flags, mode);
	if (r < 0)
		return -r;
	*fd = r;
	return 0;
}

int Sysdeps<IfIndextoname>::operator()(unsigned int index, char *name) {
	size_t count = 0;
	int r = ker::abi::net::netctl_if_list(nullptr, &count);
	if (r < 0)
		return -r;
	if (count == 0)
		return ENODEV;

	auto *ifs = static_cast<wos_net_if_info *>(calloc(count, sizeof(wos_net_if_info)));
	if (!ifs)
		return ENOMEM;

	size_t cap = count;
	r = ker::abi::net::netctl_if_list(ifs, &cap);
	if (r < 0) {
		free(ifs);
		return -r;
	}
	for (size_t i = 0; i < cap; ++i) {
		if (ifs[i].ifindex == index) {
			strncpy(name, ifs[i].name, WOS_NET_IF_NAME_LEN);
			name[WOS_NET_IF_NAME_LEN - 1] = '\0';
			free(ifs);
			return 0;
		}
	}
	free(ifs);
	return ENODEV;
}

int Sysdeps<IfNametoindex>::operator()(const char *name, unsigned int *ret) {
	if (!name || !ret)
		return EINVAL;

	size_t count = 0;
	int r = ker::abi::net::netctl_if_list(nullptr, &count);
	if (r < 0)
		return -r;
	if (count == 0)
		return ENODEV;

	auto *ifs = static_cast<wos_net_if_info *>(calloc(count, sizeof(wos_net_if_info)));
	if (!ifs)
		return ENOMEM;

	size_t cap = count;
	r = ker::abi::net::netctl_if_list(ifs, &cap);
	if (r < 0) {
		free(ifs);
		return -r;
	}
	for (size_t i = 0; i < cap; ++i) {
		if (!strncmp(ifs[i].name, name, WOS_NET_IF_NAME_LEN)) {
			*ret = ifs[i].ifindex;
			free(ifs);
			return 0;
		}
	}
	free(ifs);
	return ENODEV;
}

int Sysdeps<Socket>::operator()(int family, int type, int protocol, int *fd) {
	int64_t r = ker::abi::net::socket(family, type, protocol);
	if (r < 0)
		return (int)(-r);
	*fd = (int)r;
	return 0;
}

int Sysdeps<MsgSend>::operator()(int fd, const struct msghdr *hdr, int flags, ssize_t *length) {
	if (!hdr || hdr->msg_iovlen == 0 || !hdr->msg_iov)
		return EINVAL;
	const iovec *iov = &hdr->msg_iov[0];
	ssize_t r;
	if (hdr->msg_name) {
		r = ker::abi::net::sendto(fd, iov->iov_base, iov->iov_len, flags, hdr->msg_name);
	} else {
		r = ker::abi::net::send(fd, iov->iov_base, iov->iov_len, flags);
	}
	if (r < 0)
		return (int)(-r);
	*length = r;
	return 0;
}

int Sysdeps<Sendto>::operator()(
    int fd,
    const void *buffer,
    size_t size,
    int flags,
    const struct sockaddr *sock_addr,
    socklen_t addr_length,
    ssize_t *length
) {
	(void)addr_length;
	ssize_t r;
	if (sock_addr) {
		r = ker::abi::net::sendto(fd, buffer, size, flags, sock_addr);
	} else {
		r = ker::abi::net::send(fd, buffer, size, flags);
	}
	if (r < 0)
		return (int)(-r);
	*length = r;
	return 0;
}

int Sysdeps<MsgRecv>::operator()(int fd, struct msghdr *hdr, int flags, ssize_t *length) {
	if (!hdr || hdr->msg_iovlen == 0 || !hdr->msg_iov)
		return EINVAL;
	const iovec *iov = &hdr->msg_iov[0];
	ssize_t r;
	if (hdr->msg_name) {
		r = ker::abi::net::recvfrom(fd, iov->iov_base, iov->iov_len, flags, hdr->msg_name);
	} else {
		r = ker::abi::net::recv(fd, iov->iov_base, iov->iov_len, flags);
	}
	if (r < 0)
		return (int)(-r);
	*length = r;
	return 0;
}

int Sysdeps<Recvfrom>::operator()(
    int fd,
    void *buffer,
    size_t size,
    int flags,
    struct sockaddr *sock_addr,
    socklen_t *addr_length,
    ssize_t *length
) {
	(void)addr_length;
	ssize_t r;
	if (sock_addr) {
		r = ker::abi::net::recvfrom(fd, buffer, size, flags, sock_addr);
	} else {
		r = ker::abi::net::recv(fd, buffer, size, flags);
	}
	if (r < 0)
		return (int)(-r);
	*length = r;
	return 0;
}

int Sysdeps<Listen>::operator()(int fd, int backlog) {
	int64_t r = ker::abi::net::listen(fd, backlog);
	if (r < 0)
		return (int)(-r);
	return 0;
}

gid_t Sysdeps<GetGid>::operator()() { return static_cast<gid_t>(ker::process::getgid()); }
gid_t Sysdeps<GetEgid>::operator()() { return static_cast<gid_t>(ker::process::getegid()); }
uid_t Sysdeps<GetUid>::operator()() { return static_cast<uid_t>(ker::process::getuid()); }
uid_t Sysdeps<GetEuid>::operator()() { return static_cast<uid_t>(ker::process::geteuid()); }

int Sysdeps<GetResuid>::operator()(uid_t *ruid, uid_t *euid, uid_t *suid) {
	int64_t r = ker::process::getresuid(ruid, euid, suid);
	if (r < 0)
		return static_cast<int>(-r);
	return 0;
}

int Sysdeps<GetResgid>::operator()(gid_t *rgid, gid_t *egid, gid_t *sgid) {
	int64_t r = ker::process::getresgid(rgid, egid, sgid);
	if (r < 0)
		return static_cast<int>(-r);
	return 0;
}

pid_t Sysdeps<GetTid>::operator()() {
	return static_cast<pid_t>(ker::multiproc::currentThreadId());
}

pid_t Sysdeps<GetPpid>::operator()() { return (pid_t)ker::process::getppid(); }

int Sysdeps<GetPgid>::operator()(pid_t pid, pid_t *pgid) {
	int64_t r = ker::process::getpgid(pid);
	if (r < 0)
		return static_cast<int>(-r);
	if (pgid)
		*pgid = static_cast<pid_t>(r);
	return 0;
}

int Sysdeps<GetSid>::operator()(pid_t pid, pid_t *sid) {
	int64_t r = ker::process::getsid(pid);
	if (r < 0)
		return static_cast<int>(-r);
	if (sid)
		*sid = static_cast<pid_t>(r);
	return 0;
}

int Sysdeps<SetPgid>::operator()(pid_t pid, pid_t pgid) {
	int64_t r = ker::process::setpgid(pid, pgid);
	if (r < 0)
		return static_cast<int>(-r);
	return 0;
}

int Sysdeps<SetUid>::operator()(uid_t uid) {
	int64_t r = ker::process::setuid(uid);
	if (r < 0)
		return static_cast<int>(-r);
	return 0;
}

int Sysdeps<SetEuid>::operator()(uid_t euid) {
	int64_t r = ker::process::seteuid(euid);
	if (r < 0)
		return static_cast<int>(-r);
	return 0;
}

int Sysdeps<SetGid>::operator()(gid_t gid) {
	int64_t r = ker::process::setgid(gid);
	if (r < 0)
		return static_cast<int>(-r);
	return 0;
}

int Sysdeps<SetEgid>::operator()(gid_t egid) {
	int64_t r = ker::process::setegid(egid);
	if (r < 0)
		return static_cast<int>(-r);
	return 0;
}

int Sysdeps<GetGroups>::operator()(size_t size, gid_t *list, int *retval) {
	int64_t r = ker::process::getgroups(size, list);
	if (r < 0)
		return static_cast<int>(-r);
	if (retval)
		*retval = static_cast<int>(r);
	return 0;
}

int Sysdeps<Pselect>::operator()(
    int num_fds,
    fd_set *read_set,
    fd_set *write_set,
    fd_set *except_set,
    const struct timespec *timeout,
    const sigset_t *sigmask,
    int *num_events
) {
	if (num_fds < 0)
		return EINVAL;

	auto fd_is_set = [](int fd, fd_set *s) -> bool {
		return (s->fds_bits[fd / 8] >> (fd % 8)) & 1;
	};
	auto fd_set_bit = [](int fd, fd_set *s) {
		s->fds_bits[fd / 8] |= static_cast<unsigned char>(1 << (fd % 8));
	};
	auto fd_zero = [](fd_set *s) { memset(s->fds_bits, 0, sizeof(fd_set)); };

	int timeout_ms = -1;
	if (timeout) {
		timeout_ms = static_cast<int>((timeout->tv_sec * 1000) + (timeout->tv_nsec / 1000000));
		timeout_ms = std::max(timeout_ms, 0);
	}

	int epfd = ker::abi::vfs::epoll_create_vfs(0);
	if (epfd < 0)
		return ENOMEM;

	int watched_fds = 0;
	for (int fd = 0; fd < num_fds; fd++) {
		uint32_t events = 0;
		if (read_set && fd_is_set(fd, read_set))
			events |= EPOLLIN;
		if (write_set && fd_is_set(fd, write_set))
			events |= EPOLLOUT;
		if (except_set && fd_is_set(fd, except_set))
			events |= EPOLLERR | EPOLLHUP;
		if (events == 0)
			continue;

		epoll_event ev;
		ev.events = events;
		ev.data.fd = fd;
		int ctl = ker::abi::vfs::epoll_ctl_vfs(epfd, EPOLL_CTL_ADD, fd, &ev);
		if (ctl < 0) {
			ker::abi::vfs::close(epfd);
			return -ctl;
		}
		watched_fds++;
	}

	if (watched_fds == 0) {
		ker::abi::vfs::close(epfd);
		int wait_result = pselect_sleep_for_timeout_ms(timeout_ms, sigmask);
		if (wait_result != 0)
			return wait_result;
		if (read_set)
			fd_zero(read_set);
		if (write_set)
			fd_zero(write_set);
		if (except_set)
			fd_zero(except_set);
		*num_events = 0;
		return 0;
	}

	epoll_event out_events[64];
	int max = watched_fds < 64 ? watched_fds : 64;
	sigset_t old_mask{};
	bool restore_mask = false;
	int mask_error = apply_wait_signal_mask(sigmask, &old_mask, &restore_mask);
	if (mask_error != 0) {
		ker::abi::vfs::close(epfd);
		return mask_error;
	}
	int ready = ker::abi::vfs::epoll_pwait_vfs(epfd, out_events, max, timeout_ms);
	int restore_error = restore_wait_signal_mask(&old_mask, restore_mask);

	if (ready == -EINTR) {
		ker::abi::vfs::close(epfd);
		*num_events = 0;
		return restore_error != 0 ? restore_error : EINTR;
	}
	if (ready < 0) {
		ker::abi::vfs::close(epfd);
		return restore_error != 0 ? restore_error : -ready;
	}
	if (restore_error != 0) {
		ker::abi::vfs::close(epfd);
		return restore_error;
	}

	if (read_set)
		fd_zero(read_set);
	if (write_set)
		fd_zero(write_set);
	if (except_set)
		fd_zero(except_set);

	int count = 0;
	if (ready > 0) {
		for (int i = 0; i < ready; i++) {
			int fd = out_events[i].data.fd;
			if (read_set && (out_events[i].events & (EPOLLIN | EPOLLHUP | EPOLLERR)))
				fd_set_bit(fd, read_set);
			if (write_set && (out_events[i].events & EPOLLOUT))
				fd_set_bit(fd, write_set);
			if (except_set && (out_events[i].events & (EPOLLERR | EPOLLHUP)))
				fd_set_bit(fd, except_set);
			count++;
		}
	}

	ker::abi::vfs::close(epfd);
	*num_events = count;
	return 0;
}

int Sysdeps<GetRlimit>::operator()(int resource, struct rlimit *limit) {
	(void)resource;
	if (limit) {
		limit->rlim_cur = RLIM_INFINITY;
		limit->rlim_max = RLIM_INFINITY;
	}
	return 0;
}

int Sysdeps<SetRlimit>::operator()(int resource, const struct rlimit *limit) {
	(void)resource;
	(void)limit;
	return 0;
}

int Sysdeps<SetPriority>::operator()(int which, id_t who, int prio) {
	(void)which;
	(void)who;
	(void)prio;
	return 0;
}

int Sysdeps<GetCwd>::operator()(char *buffer, size_t size) {
	int r = ker::abi::vfs::getcwd(buffer, size);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Chdir>::operator()(const char *path) {
	int r = ker::abi::vfs::chdir(path);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Fchdir>::operator()(int fd) {
	(void)fd;
	return 0;
}

int Sysdeps<Mkdir>::operator()(const char *path, mode_t mode) {
	int r = ker::abi::vfs::mkdir(path, static_cast<int>(mode));
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Mkdirat>::operator()(int dirfd, const char *path, mode_t mode) {
	if (dirfd == -100 || (path && path[0] == '/')) {
		return sysdep<Mkdir>(path, mode);
	}
	return ENOSYS;
}

int Sysdeps<Link>::operator()(const char *old_path, const char *new_path) {
	int r = ker::abi::vfs::link_vfs(old_path, new_path);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Linkat>::operator()(
    int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags
) {
	(void)flags;
	if ((olddirfd == AT_FDCWD || olddirfd == -100 || (oldpath && oldpath[0] == '/'))
	    && (newdirfd == AT_FDCWD || newdirfd == -100 || (newpath && newpath[0] == '/'))) {
		return sysdep<Link>(oldpath, newpath);
	}
	return ENOSYS;
}

int Sysdeps<Symlink>::operator()(const char *target, const char *linkpath) {
	int r = ker::abi::vfs::symlink(target, linkpath);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Symlinkat>::operator()(const char *target, int dirfd, const char *linkpath) {
	if (dirfd == -100 || (linkpath && linkpath[0] == '/')) {
		return sysdep<Symlink>(target, linkpath);
	}
	return ENOSYS;
}

int Sysdeps<Renameat>::operator()(
    int olddirfd, const char *old_path, int newdirfd, const char *new_path
) {
	int r = ker::abi::vfs::renameat(olddirfd, old_path, newdirfd, new_path);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Flock>::operator()(int fd, int options) {
	constexpr int WOS_FLOCK_CMD = 0x5753464c;
	int r = ker::abi::vfs::fcntl(fd, WOS_FLOCK_CMD, static_cast<uint64_t>(options));
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Fcntl>::operator()(int fd, int request, va_list args, int *result) {
	uint64_t arg = 0;
	// F_DUPFD=0, F_GETFD=1, F_SETFD=2, F_GETFL=3, F_SETFL=4, F_DUPFD_CLOEXEC=1030
	if (request == 0 || request == 2 || request == 4 || request == 1030) {
		arg = static_cast<uint64_t>(va_arg(args, int));
	}

	if (request == F_GETLK || request == F_SETLK || request == F_SETLKW || request == F_OFD_GETLK
	    || request == F_OFD_SETLK || request == F_OFD_SETLKW) {
		arg = reinterpret_cast<uint64_t>(va_arg(args, struct flock *));
	}

	int r = ker::abi::vfs::fcntl(fd, request, arg);
	if (r < 0) {
		return -r;
	}
	if (result)
		*result = r;
	return 0;
}

int Sysdeps<Ttyname>::operator()(int fd, char *buf, size_t size) {
	if (!ker::abi::vfs::isatty(fd))
		return ENOTTY;

	int pty_num = -1;
	int r = ker::abi::vfs::ioctl_vfs(fd, TIOCGPTN, reinterpret_cast<unsigned long>(&pty_num));
	if (r >= 0 && pty_num >= 0) {
		char name[32];
		int pos = 0;
		const char *prefix = "/dev/pts/";
		while (*prefix)
			name[pos++] = *prefix++;
		if (pty_num < 10) {
			name[pos++] = '0' + static_cast<char>(pty_num);
		} else {
			name[pos++] = '0' + static_cast<char>(pty_num / 10);
			name[pos++] = '0' + static_cast<char>(pty_num % 10);
		}
		name[pos] = '\0';
		if (static_cast<size_t>(pos) + 1 > size)
			return ERANGE;
		memcpy(buf, name, static_cast<size_t>(pos) + 1);
		return 0;
	}

	static const char *known_ttys[] = {"/dev/console", "/dev/tty0", "/dev/ttyS0"};
	for (const char *path : known_ttys) {
		size_t len = 0;
		const char *p = path;
		while (*p) {
			len++;
			p++;
		}
		if (len + 1 > size)
			continue;
		int tfd = ker::abi::vfs::open(path, 0 /* O_RDONLY */, 0);
		if (tfd >= 0) {
			ker::abi::vfs::close(tfd);
			memcpy(buf, path, len + 1);
			return 0;
		}
	}

	return ENOTTY;
}

int Sysdeps<Fsync>::operator()(int fd) {
	int r = ker::abi::vfs::fsync_vfs(fd);
	if (r < 0)
		return -r;
	return 0;
}

void Sysdeps<Sync>::operator()() { (void)ker::abi::vfs::sync_vfs(); }

int Sysdeps<Chmod>::operator()(const char *pathname, mode_t mode) {
	int r = ker::abi::vfs::chmod(pathname, mode);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Fchmod>::operator()(int fd, mode_t mode) {
	int r = ker::abi::vfs::fchmod(fd, mode);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Fchmodat>::operator()(int dirfd, const char *pathname, mode_t mode, int flags) {
	(void)flags;
	if (dirfd == AT_FDCWD || dirfd == -100 || (pathname && pathname[0] == '/')) {
		return sysdep<Chmod>(pathname, mode);
	}
	return ENOSYS;
}

int Sysdeps<SetSid>::operator()(pid_t *sid) {
	int64_t r = ker::process::setsid();
	if (r < 0)
		return static_cast<int>(-r);
	if (sid)
		*sid = static_cast<pid_t>(r);
	return 0;
}

int Sysdeps<Tcgetattr>::operator()(int fd, struct termios *attr) {
	if (!attr)
		return EINVAL;
	int r = ker::abi::vfs::ioctl_vfs(fd, 0x5401, reinterpret_cast<unsigned long>(attr));
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Tcsetattr>::operator()(int fd, int optional_actions, const struct termios *attr) {
	if (!attr)
		return EINVAL;
	unsigned long cmd;
	switch (optional_actions) {
		case TCSANOW:
			cmd = 0x5402;
			break;
		case TCSADRAIN:
			cmd = 0x5403;
			break;
		case TCSAFLUSH:
			cmd = 0x5404;
			break;
		default:
			return EINVAL;
	}
	int r = ker::abi::vfs::ioctl_vfs(
	    fd, cmd, reinterpret_cast<unsigned long>(const_cast<termios *>(attr))
	);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Tcflow>::operator()(int fd, int action) {
	(void)fd;
	(void)action;
	return 0;
}

int Sysdeps<Tcflush>::operator()(int fd, int queue) {
	int r = ker::abi::vfs::ioctl_vfs(fd, 0x540B /* TCFLSH */, static_cast<unsigned long>(queue));
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Tcdrain>::operator()(int fd) {
	(void)fd;
	return 0;
}

int Sysdeps<Pipe>::operator()(int *fds, int flags) {
	int r = ker::abi::vfs::pipe(fds, flags);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Socketpair>::operator()(int domain, int type_and_flags, int proto, int *fds) {
	(void)domain;
	(void)proto;
	int r = ker::abi::vfs::pipe(fds, type_and_flags & (SOCK_CLOEXEC | SOCK_NONBLOCK));
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Poll>::operator()(struct pollfd *fds, nfds_t count, int timeout, int *num_events) {
	int r = ker::abi::net::poll(fds, count, timeout);
	if (r == -EINTR) {
		*num_events = 0;
		return EINTR;
	}
	if (r < 0)
		return -r;
	*num_events = r;
	return 0;
}

int Sysdeps<Sendfile>::operator()(int outfd, int infd, off_t *offset, size_t count, ssize_t *out) {
	ssize_t r = ker::abi::vfs::sendfile(outfd, infd, offset, count);
	if (r < 0)
		return static_cast<int>(-r);
	if (out)
		*out = r;
	return 0;
}

int Sysdeps<Ioctl>::operator()(int fd, unsigned long request, void *arg, int *result) {
	// Route network ioctls (SIOC* range 0x8900-0x89FF) through net syscall
	if (request >= 0x8900 && request <= 0x89FF) {
		int r = ker::abi::net::ioctl_net(request, arg);
		if (r < 0)
			return -r;
		if (result)
			*result = 0;
		return 0;
	}
	// Route all other ioctls through VFS (device ioctl)
	int r = ker::abi::vfs::ioctl_vfs(fd, request, reinterpret_cast<unsigned long>(arg));
	if (r < 0)
		return -r;
	if (result)
		*result = r;
	return 0;
}

int Sysdeps<Tcgetwinsize>::operator()(int fd, struct winsize *winsz) {
	int result = 0;
	return sysdep<Ioctl>(fd, TIOCGWINSZ, winsz, &result);
}

int Sysdeps<Tcsetwinsize>::operator()(int fd, const struct winsize *winsz) {
	int result = 0;
	return sysdep<Ioctl>(fd, TIOCSWINSZ, const_cast<struct winsize *>(winsz), &result);
}

int Sysdeps<GetSockopt>::operator()(
    int fd, int layer, int number, void *__restrict buffer, socklen_t *__restrict size
) {
	size_t ksize = size ? *size : 0;
	int64_t r = ker::abi::net::getsockopt(fd, layer, number, buffer, &ksize);
	if (r < 0)
		return (int)(-r);
	if (size)
		*size = (socklen_t)ksize;
	return 0;
}

int
Sysdeps<SetSockopt>::operator()(int fd, int layer, int number, const void *buffer, socklen_t size) {
	int64_t r = ker::abi::net::setsockopt(fd, layer, number, buffer, size);
	if (r < 0)
		return (int)(-r);
	return 0;
}

int Sysdeps<Shutdown>::operator()(int sockfd, int how) {
	int64_t r = ker::abi::net::shutdown(sockfd, how);
	if (r < 0)
		return (int)(-r);
	return 0;
}

int Sysdeps<Accept>::operator()(
    int fd, int *newfd, struct sockaddr *addr_ptr, socklen_t *addr_length, int flags
) {
	size_t alen = addr_length ? *addr_length : 0;
	int64_t r = ker::abi::net::accept(fd, addr_ptr, &alen);
	if (r < 0)
		return (int)(-r);

	int accepted_fd = (int)r;
	if (flags & SOCK_NONBLOCK) {
		int current_flags = ker::abi::vfs::fcntl(accepted_fd, F_GETFL, 0);
		if (current_flags < 0) {
			ker::abi::vfs::close(accepted_fd);
			return -current_flags;
		}
		int set_flags = ker::abi::vfs::fcntl(accepted_fd, F_SETFL, current_flags | O_NONBLOCK);
		if (set_flags < 0) {
			ker::abi::vfs::close(accepted_fd);
			return -set_flags;
		}
	}
	if (flags & SOCK_CLOEXEC) {
		int set_fd_flags = ker::abi::vfs::fcntl(accepted_fd, F_SETFD, FD_CLOEXEC);
		if (set_fd_flags < 0) {
			ker::abi::vfs::close(accepted_fd);
			return -set_fd_flags;
		}
	}
	if (addr_length)
		*addr_length = (socklen_t)alen;
	*newfd = accepted_fd;
	return 0;
}

int Sysdeps<Bind>::operator()(int fd, const struct sockaddr *addr_ptr, socklen_t addr_length) {
	int64_t r = ker::abi::net::bind(fd, addr_ptr, addr_length);
	if (r < 0)
		return (int)(-r);
	return 0;
}

int Sysdeps<Connect>::operator()(int fd, const struct sockaddr *addr_ptr, socklen_t addr_length) {
	int64_t r = ker::abi::net::connect(fd, addr_ptr, addr_length);
	if (r < 0)
		return (int)(-r);
	return 0;
}

int Sysdeps<Sockname>::operator()(
    int fd, struct sockaddr *addr_ptr, socklen_t max_addr_length, socklen_t *actual_length
) {
	size_t alen = max_addr_length;
	int64_t r = ker::abi::net::getsockname(fd, addr_ptr, &alen);
	if (r < 0)
		return (int)(-r);
	if (actual_length)
		*actual_length = (socklen_t)alen;
	return 0;
}

int Sysdeps<Peername>::operator()(
    int fd, struct sockaddr *addr_ptr, socklen_t max_addr_length, socklen_t *actual_length
) {
	size_t alen = max_addr_length;
	int64_t r = ker::abi::net::getpeername(fd, addr_ptr, &alen);
	if (r < 0)
		return (int)(-r);
	if (actual_length)
		*actual_length = (socklen_t)alen;
	return 0;
}

int Sysdeps<GetHostname>::operator()(char *buffer, size_t bufsize) {
	int64_t r = ker::process::gethostname(buffer, bufsize);
	if (r < 0)
		return static_cast<int>(-r);
	return 0;
}

int Sysdeps<SetHostname>::operator()(const char *buffer, size_t bufsize) {
	int64_t r = ker::process::sethostname(buffer, bufsize);
	if (r < 0)
		return static_cast<int>(-r);
	return 0;
}

int Sysdeps<GetEntropy>::operator()(void *buffer, size_t length) {
	int fd = ker::abi::vfs::open("/dev/urandom", 0 /* O_RDONLY */, 0);
	if (fd < 0)
		return EIO;
	size_t total = 0;
	while (total < length) {
		ssize_t r = ker::abi::vfs::read(fd, static_cast<char *>(buffer) + total, length - total);
		if (r <= 0) {
			ker::abi::vfs::close(fd);
			return EIO;
		}
		total += static_cast<size_t>(r);
	}
	ker::abi::vfs::close(fd);
	return 0;
}

#if __MLIBC_GLIBC_OPTION
int Sysdeps<Personality>::operator()(unsigned long persona, int *out) {
	int64_t r = ker::process::personality(persona);
	if (r < 0)
		return static_cast<int>(-r);
	if (out)
		*out = static_cast<int>(r);
	return 0;
}
#endif // __MLIBC_GLIBC_OPTION

int Sysdeps<Umask>::operator()(mode_t mode, mode_t *old) {
	uint64_t prev = ker::process::setumask(static_cast<uint64_t>(mode & 0777));
	if (old)
		*old = static_cast<mode_t>(prev);
	return 0;
}

int Sysdeps<Fchownat>::operator()(
    int dirfd, const char *pathname, uid_t owner, gid_t group, int flags
) {
	if (dirfd == AT_FDCWD || dirfd == -100) {
		int r = ker::abi::vfs::chown(pathname, owner, group);
		if (r < 0)
			return -r;
		return 0;
	}
	if (flags & 0x1000 /* AT_EMPTY_PATH */) {
		int r = ker::abi::vfs::fchown(dirfd, owner, group);
		if (r < 0)
			return -r;
		return 0;
	}
	int r = ker::abi::vfs::chown(pathname, owner, group);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Sigsuspend>::operator()(const sigset_t *set) {
	int64_t r = ker::process::sigsuspend(set);
	if (r < 0)
		return static_cast<int>(-r);
	return EINTR;
}

int Sysdeps<SetGroups>::operator()(size_t size, const gid_t *list) {
	int64_t r = ker::process::setgroups(size, list);
	if (r < 0)
		return static_cast<int>(-r);
	return 0;
}

int Sysdeps<GetItimer>::operator()(int which, struct itimerval *curr_value) {
	uint64_t res = ker::time::getitimer(which, (void *)curr_value);
	if ((int64_t)res < 0)
		return (int)(-(int64_t)res);
	return 0;
}

int Sysdeps<SetItimer>::operator()(
    int which, const struct itimerval *new_value, struct itimerval *old_value
) {
	if (old_value) {
		uint64_t res = ker::time::getitimer(which, (void *)old_value);
		if ((int64_t)res < 0)
			return (int)(-(int64_t)res);
	}
	uint64_t res = ker::time::setitimer(which, (const void *)new_value);
	if ((int64_t)res < 0)
		return (int)(-(int64_t)res);
	return 0;
}

int Sysdeps<Times>::operator()(struct tms *tms, clock_t *out) {
	clock_t ret;
	uint64_t res = ker::time::times((void *)tms, (void *)&ret);
	if ((int64_t)res < 0) {
		return (int)(-(int64_t)res);
	}
	*out = ret;
	return 0;
}

int Sysdeps<Uname>::operator()(struct utsname *buf) {
	if (!buf)
		return EINVAL;
	int64_t res = ker::process::uname(buf);
	if (res < 0)
		return -res;
	return 0;
}

int Sysdeps<VmRemap>::operator()(void *pointer, size_t size, size_t new_size, void **window) {
	int64_t res = ker::vmem::remap(window, pointer, size, new_size, MREMAP_MAYMOVE);
	if (res < 0)
		return -res;
	return 0;
}

int Sysdeps<Sigaltstack>::operator()(const stack_t *ss, stack_t *oss) {
	int64_t res = ker::process::sigaltstack(ss, oss);
	if (res < 0)
		return -res;
	return 0;
}

int Sysdeps<Prctl>::operator()(int option, va_list va, int *out) {
	uint64_t arg2 = va_arg(va, uint64_t);
	uint64_t arg3 = va_arg(va, uint64_t);
	uint64_t arg4 = va_arg(va, uint64_t);
	uint64_t arg5 = va_arg(va, uint64_t);
	int64_t res = ker::process::prctl(option, arg2, arg3, arg4, arg5);
	if (res < 0)
		return -res;
	if (out)
		*out = static_cast<int>(res);
	return 0;
}

int Sysdeps<Openpt>::operator()(int oflags, int *fd) {
	(void)oflags;
	int r = ker::abi::vfs::open("/dev/ptmx", 2 /* O_RDWR */, 0);
	if (r < 0)
		return -r;
	if (fd)
		*fd = r;
	return 0;
}

int Sysdeps<Ptsname>::operator()(int masterfd, char *buffer, size_t length) {
	int pty_num = -1;
	int r = ker::abi::vfs::ioctl_vfs(
	    masterfd, 0x80045430 /* TIOCGPTN */, reinterpret_cast<unsigned long>(&pty_num)
	);
	if (r < 0)
		return -r;

	char name[32];
	int pos = 0;
	const char *prefix = "/dev/pts/";
	while (*prefix)
		name[pos++] = *prefix++;
	if (pty_num < 10) {
		name[pos++] = '0' + static_cast<char>(pty_num);
	} else {
		name[pos++] = '0' + static_cast<char>(pty_num / 10);
		name[pos++] = '0' + static_cast<char>(pty_num % 10);
	}
	name[pos] = '\0';

	if (static_cast<size_t>(pos) + 1 > length)
		return ERANGE;
	memcpy(buffer, name, static_cast<size_t>(pos) + 1);
	return 0;
}

int Sysdeps<Unlockpt>::operator()(int fd) {
	int unlock = 0;
	int r = ker::abi::vfs::ioctl_vfs(
	    fd, 0x40045431 /* TIOCSPTLCK */, reinterpret_cast<unsigned long>(&unlock)
	);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<GetAffinity>::operator()(pid_t, size_t, cpu_set_t *) { return ENOSYS; }

int Sysdeps<GetThreadaffinity>::operator()(pid_t tid, size_t cpusetsize, cpu_set_t *mask) {
	if (!mask || cpusetsize == 0) {
		return EINVAL;
	}

	int64_t result = ker::multiproc::getThreadAffinityMask((uint64_t)tid);
	if (result < 0) {
		return (int)(-result);
	}

	return mask_to_cpuset((uint64_t)result, cpusetsize, mask);
}

int Sysdeps<SetAffinity>::operator()(pid_t, size_t, const cpu_set_t *) { return ENOSYS; }

int Sysdeps<SetThreadaffinity>::operator()(pid_t tid, size_t cpusetsize, const cpu_set_t *mask) {
	if (!mask || cpusetsize == 0) {
		return EINVAL;
	}

	uint64_t affinity_mask = 0;
	if (int e = cpuset_to_mask(cpusetsize, mask, &affinity_mask); e) {
		return e;
	}

	int64_t result = ker::multiproc::setThreadAffinityMask((uint64_t)tid, affinity_mask);
	if (result < 0) {
		return (int)(-result);
	}
	return 0;
}

#endif // __MLIBC_POSIX_OPTION && !MLIBC_BUILDING_RTLD

int Sysdeps<Mount>::operator()(
    const char *source,
    const char *target,
    const char *fstype,
    unsigned long flags,
    const void *data
) {
	int r = ker::abi::vfs::mount(source, target, fstype, flags, data);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Swapon>::operator()(const char *path, int flags) {
	int64_t r = ker::vmem::swapon(path, flags);
	if (r < 0)
		return static_cast<int>(-r);
	return 0;
}

int Sysdeps<Swapoff>::operator()(const char *path) {
	int64_t r = ker::vmem::swapoff(path);
	if (r < 0)
		return static_cast<int>(-r);
	return 0;
}

int Sysdeps<Umount2>::operator()(const char *target, int flags) {
	(void)flags;
	int r = ker::abi::vfs::umount(target);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<EpollCreate>::operator()(int flags, int *fd) {
	int r = ker::abi::vfs::epoll_create_vfs(flags);
	if (r < 0)
		return -r;
	if (fd)
		*fd = r;
	return 0;
}

int Sysdeps<EpollCtl>::operator()(int epfd, int mode, int fd, epoll_event *ev) {
	int r = ker::abi::vfs::epoll_ctl_vfs(epfd, mode, fd, ev);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<EpollPwait>::operator()(
    int epfd, epoll_event *ev, int n, int timeout, const sigset_t *sigmask, int *raised
) {
	sigset_t old_mask{};
	bool restore_mask = false;
	int mask_error = apply_wait_signal_mask(sigmask, &old_mask, &restore_mask);
	if (mask_error != 0)
		return mask_error;
	int r = ker::abi::vfs::epoll_pwait_vfs(epfd, ev, n, timeout);
	int restore_error = restore_wait_signal_mask(&old_mask, restore_mask);
	if (r == -EINTR) {
		if (raised)
			*raised = 0;
		return restore_error != 0 ? restore_error : EINTR;
	}
	if (r < 0)
		return restore_error != 0 ? restore_error : -r;
	if (restore_error != 0)
		return restore_error;
	if (raised)
		*raised = r;
	return 0;
}

int Sysdeps<Statvfs>::operator()(const char *path, struct statvfs *out) {
	int r = ker::abi::vfs::statvfs_path(path, out);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Fstatvfs>::operator()(int fd, struct statvfs *out) {
	int r = ker::abi::vfs::fstatvfs_fd(fd, out);
	if (r < 0)
		return -r;
	return 0;
}

int Sysdeps<Getcpu>::operator()(int *cpu) {
	*cpu = (int)ker::multiproc::getcurrent_cpu();
	return 0;
}

int Sysdeps<Utimensat>::operator()(
    int dirfd, const char *pathname, const struct timespec *, int flags
) {
	// Timestamps are not tracked. Return ENOENT if the path doesn't exist so
	// callers like `touch` fall back to open(O_CREAT) to create the file.
	if (dirfd != AT_FDCWD && dirfd != -100)
		return ENOSYS;
	struct stat st;
	if (int e = sysdep<Stat>(fsfd_target::path, -1, pathname, flags, &st); e) {
		return e;
	}
	return 0;
}

} // namespace mlibc

__attribute__((visibility("default"))) extern "C" void frg_panic(const char *mstr) {
	mlibc::sysdep<LibcLog>(mstr);
	mlibc::sysdep<LibcPanic>();
}
