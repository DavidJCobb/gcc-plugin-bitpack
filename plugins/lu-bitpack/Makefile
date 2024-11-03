PLUGIN=lu-bitpack.so
INCDIR=include
SOURCES=\
        src/bitpacking/global_options.cpp \
        src/bitpacking/heritable_options.cpp \
        src/bitpacking/member_options.cpp \
        src/codegen/descriptors.cpp \
        src/codegen/in_progress_func_pair.cpp \
        src/codegen/sector_functions_generator.cpp \
        src/codegen/serialization_value.cpp \
        src/gcc_helpers/dump_function.cpp \
        src/gcc_helpers/extract_pragma_kv_args.cpp \
        src/gcc_wrappers/decl/base.cpp \
        src/gcc_wrappers/decl/field.cpp \
        src/gcc_wrappers/decl/function.cpp \
        src/gcc_wrappers/decl/label.cpp \
        src/gcc_wrappers/decl/param.cpp \
        src/gcc_wrappers/decl/result.cpp \
        src/gcc_wrappers/decl/type_def.cpp \
        src/gcc_wrappers/decl/variable.cpp \
        src/gcc_wrappers/expr/assign.cpp \
        src/gcc_wrappers/expr/base.cpp \
        src/gcc_wrappers/expr/call.cpp \
        src/gcc_wrappers/expr/declare.cpp \
        src/gcc_wrappers/expr/go_to_label.cpp \
        src/gcc_wrappers/expr/integer_constant.cpp \
        src/gcc_wrappers/expr/label.cpp \
        src/gcc_wrappers/expr/local_block.cpp \
        src/gcc_wrappers/expr/return_result.cpp \
        src/gcc_wrappers/expr/string_constant.cpp \
        src/gcc_wrappers/expr/ternary.cpp \
        src/gcc_wrappers/flow/simple_for_loop.cpp \
        src/gcc_wrappers/builtin_types.cpp \
        src/gcc_wrappers/list_node.cpp \
        src/gcc_wrappers/statement_list.cpp \
        src/gcc_wrappers/type.cpp \
        src/gcc_wrappers/value.cpp \
        src/lu/strings/handle_kv_string.cpp \
        src/lu/strings/printf_string.cpp \
        src/lu/strings/trim.cpp \
        src/pragma_handlers/debug_dump_function.cpp \
        src/pragma_handlers/debug_dump_identifier.cpp \
        src/pragma_handlers/generate_functions.cpp \
        src/pragma_handlers/heritable.cpp \
        src/pragma_handlers/set_options.cpp \
        src/lu-bitpack.cpp \
		$(END)

include ../../Makefile.common

.PHONY: test
test: $(PLUGIN)
	$(CCPLUGIN) -c -o /dev/null test/test.c -Itest

.PHONY: test_and_run
test_and_run: $(PLUGIN)
	- rm -f test/*.o
	$(CCPLUGIN) -c test/bitstreams.c -o test/bitstreams.o -Itest
	$(CCPLUGIN) -c test/test.c -o test/test.o -Itest
	$(CCPLUGIN) test/bitstreams.o test/test.o -o test/test
	test/test