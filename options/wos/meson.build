if not wos_option
	subdir_done()
endif
libc_sources += files(
	'generic/ifaddrs.cpp',
	'generic/sched.cpp',
	'generic/vfs.cpp',
	'generic/process.cpp',
	'generic/sys-sendfile.cpp',
)

if not no_headers
	# install_headers(
	# 	'include/module.h',
	# )
	install_headers(
		'include/ifaddrs.h',
		'include/malloc.h',
	)
	install_headers(
		'include/bits/wos/wos_unistd.h',
		'include/bits/wos/wos_sched.h',
		# 'include/bits/linux/linux_stat.h',
		'include/bits/wos/cpu_set.h',
		subdir: 'bits/wos',
	)
	# install_headers(
	# 	'include/netpacket/packet.h',
	# 	subdir: 'netpacket'
	# )
	# libc provides these, not the kernel, so the Linux option shall provide them too.
	# install_headers(
	# 	'include/scsi/scsi.h',
	# 	'include/scsi/scsi_ioctl.h',
	# 	'include/scsi/sg.h',
	# 	subdir: 'scsi'
	# )
	install_headers(
		'include/sys/epoll.h',
		# 'include/sys/inotify.h',
		# 'include/sys/mount.h',
		# 'include/sys/prctl.h',
		# 'include/sys/ptrace.h',
		# 'include/sys/random.h',
		'include/sys/hcf.h',
		'include/sys/sendfile.h',
		# 'include/sys/signalfd.h',
		# 'include/sys/sysmacros.h',
		# 'include/sys/timerfd.h',
		# 'include/sys/eventfd.h',
		# 'include/sys/reboot.h',
		# 'include/sys/fsuid.h',
		# 'include/sys/vt.h',
		'include/sys/sysinfo.h',
		# 'include/sys/klog.h',
		# 'include/sys/xattr.h',
		# 'include/sys/quota.h',
		# 'include/sys/swap.h',
		'include/sys/statfs.h',
		# 'include/sys/vfs.h',
		subdir: 'sys',
	)
endif
