PLUGIN=function-manip.so
INCDIR=include
SOURCES=\
        src/gcc_helpers/array_extent.cpp \
        src/gcc_helpers/dump_function.cpp \
        src/gcc_helpers/extract_pragma_kv_args.cpp \
        src/gcc_helpers/type_is_char.cpp \
        src/gcc_helpers/type_min_max.cpp \
        src/gcc_helpers/type_name.cpp \
        src/gcc_helpers/type_size_in_bytes.cpp \
        src/lu/strings/trim.cpp \
        src/handle_kv_string.cpp \
        src/data_member.cpp \
        src/struct.cpp \
        src/handle_struct_type.cpp \
        src/function-manip.cpp \
		$(END)

include ../../Makefile.common

.PHONY: test
test: $(PLUGIN)
	$(CCPLUGIN) -c -o /dev/null test/test.c -Itest/