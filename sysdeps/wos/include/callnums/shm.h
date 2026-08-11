#pragma once
#include <stddef.h>
#include <stdint.h>

namespace ker::abi::shm {
enum class ops : uint64_t {
	GET,
	ATTACH,
	DETACH,
	CTL,
	get = GET,
	attach = ATTACH,
	detach = DETACH,
	ctl = CTL,
};

struct IpcPerm {
	int32_t key;
	uint32_t uid;
	uint32_t gid;
	uint32_t cuid;
	uint32_t cgid;
	uint32_t mode;
	int32_t seq;
	long unused[2];
};

struct ShmidDs {
	IpcPerm shm_perm;
	size_t shm_segsz;
	long shm_atime;
	long shm_dtime;
	long shm_ctime;
	int64_t shm_cpid;
	int64_t shm_lpid;
	unsigned long shm_nattch;
	unsigned long unused[2];
};

// Lower-case names avoid collisions with the public SysV IPC macros.
constexpr int ipc_private = 0;
constexpr int ipc_create = 01000;
constexpr int ipc_exclusive = 02000;
constexpr int ipc_remove = 0;
constexpr int ipc_stat = 2;
constexpr int shm_readonly = 010000;
} // namespace ker::abi::shm
