sysdep_supported_options = {
    'posix': true,
    'linux': false,
    'glibc': true,
    'bsd': true,
    'wos': true,
}

rtld_dso_sources += files(
    'generic/sysdeps.cpp',
    'generic/thread.cpp',
    'sys/logging.cpp',
    'crt-x86_64' / host_machine.cpu_family() / 'signals.S',
)

libc_sources += files(
    'generic/sysdeps.cpp',
    'generic/thread.cpp',
    'generic/entry.cpp',
    'sys/logging.cpp',
    'crt-x86_64' / host_machine.cpu_family() / 'signals.S',
)

if not no_headers
    install_headers(
        'include/abi-bits/access.h',
        'include/abi-bits/auxv.h',
        'include/abi-bits/blkcnt_t.h',
        'include/abi-bits/blksize_t.h',
        'include/abi-bits/clockid_t.h',
        'include/abi-bits/dev_t.h',
        'include/abi-bits/errno.h',
        'include/abi-bits/fcntl.h',
        'include/abi-bits/fsblkcnt_t.h',
        'include/abi-bits/fsfilcnt_t.h',
        'include/abi-bits/gid_t.h',
        'include/abi-bits/in.h',
        'include/abi-bits/ino_t.h',
        'include/abi-bits/ipc.h',
        'include/abi-bits/limits.h',
        'include/abi-bits/mode_t.h',
        'include/abi-bits/mqueue.h',
        'include/abi-bits/msg.h',
        'include/abi-bits/nlink_t.h',
        'include/abi-bits/pid_t.h',
        'include/abi-bits/poll.h',
        'include/abi-bits/resource.h',
        'include/abi-bits/rlim_t.h',
        'include/abi-bits/seek-whence.h',
        'include/abi-bits/shm.h',
        'include/abi-bits/sigevent.h',
        'include/abi-bits/signal.h',
        'include/abi-bits/sigval.h',
        'include/abi-bits/socket.h',
        'include/abi-bits/socklen_t.h',
        'include/abi-bits/stat.h',
        'include/abi-bits/statfs.h',
        'include/abi-bits/statvfs.h',
        'include/abi-bits/suseconds_t.h',
        'include/abi-bits/termios.h',
        'include/abi-bits/time.h',
        'include/abi-bits/uid_t.h',
        'include/abi-bits/utsname.h',
        'include/abi-bits/vm-flags.h',
        'include/abi-bits/wait.h',
        'include/abi-bits/utmp-defines.h',
        'include/abi-bits/fd_set.h',
        'include/abi-bits/sa_family_t.h',
        'include/abi-bits/sem.h',
        'include/abi-bits/ioctls.h',
        'include/abi-bits/route.h',
        'include/abi-bits/sig-limits.h',
        'include/abi-bits/sigset_t.h',
        'include/abi-bits/epoll.h',
        subdir: 'abi-bits',
        follow_symlinks: true,
    )

    install_headers(
        'include/callnums/futex.h',
        'include/callnums/multiproc.h',
        'include/callnums/process.h',
        'include/callnums/sys_log.h',
        subdir: 'callnums',
    )

    install_headers(
        'include/sys/callnums.h',
        'include/sys/multiproc.h',
        'include/sys/net.h',
        'include/sys/process.h',
        'include/sys/syscall.h',
        'include/sys/logging.h',
        'include/sys/vfs.h',
        'include/sys/time_calls.h',
        'include/sys/time_ops.h',
        subdir: 'sys',
    )

    install_headers(
        'include/asm/ioctl.h',
        subdir: 'asm',
    )
endif

if not headers_only
    crt = custom_target(
        'crt1',
        build_by_default: true,
        command: c_compiler.cmd_array() + ['-c', '-o', '@OUTPUT@', '@INPUT@'],
        input: 'crt-x86_64/crt1.S',
        output: 'crt1.o',
        install: true,
        install_dir: get_option('libdir'),
    )

    custom_target(
        'crti',
        build_by_default: true,
        command: c_compiler.cmd_array() + ['-c', '-o', '@OUTPUT@', '@INPUT@'],
        input: 'crt-x86_64/crti.S',
        output: 'crti.o',
        install: true,
        install_dir: get_option('libdir'),
    )

    custom_target(
        'crtn',
        build_by_default: true,
        command: c_compiler.cmd_array() + ['-c', '-o', '@OUTPUT@', '@INPUT@'],
        input: 'crt-x86_64/crtn.S',
        output: 'crtn.o',
        install: true,
        install_dir: get_option('libdir'),
    )

    custom_target(
        'Scrt1',
        build_by_default: true,
        command: c_compiler.cmd_array()
        + ['-fPIE', '-c', '-o', '@OUTPUT@', '@INPUT@'],
        input: 'crt-x86_64/Scrt1.S',
        output: 'Scrt1.o',
        install: true,
        install_dir: get_option('libdir'),
    )
endif
