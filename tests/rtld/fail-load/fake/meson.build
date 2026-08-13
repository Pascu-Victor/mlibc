lib_fake = shared_library('fake', 'empty.c', install_dir: 'fake', link_args: mlibc_link_args)
lib_native_fake = shared_library('native-fake', 'empty.c', native: true, install_dir: 'fake', link_args: mlibc_link_args)

test_link_with += [lib_fake]
test_native_link_with += [lib_native_fake]

malformed_elf = files('make-malformed-elf.py')
malformed_libfoo = custom_target('malformed-libfoo',
	input: lib_fake,
	output: 'libfoo.so',
	command: [py, malformed_elf, '@INPUT@', '@OUTPUT@'],
	build_by_default: true,
	install: true,
	install_dir: 'fake',
)
test_depends += [malformed_libfoo]

configure_file(input: 'fake-lib', output: 'libnative-foo.so', install_dir: 'fake', copy: true)
