PLUGIN=struct-stuff.so
INCDIR=include
SOURCES=\
        src/gcc_helpers/array_extent.cpp \
        src/gcc_helpers/bitcount_of_type.cpp \
        src/gcc_helpers/type_min_max.cpp \
        src/gcc_helpers/type_name.cpp \
        src/gcc_helpers/type_size_in_bytes.cpp \
        src/struct.cpp \
        src/handle_struct_type.cpp \
        src/struct-stuff.cpp \
		$(END)

include ../../Makefile.common

.PHONY: test
test: $(PLUGIN)
	$(CCPLUGIN) -c -o /dev/null test.c