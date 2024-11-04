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
        src/gcc_wrappers/type/helpers/lookup_by_name.cpp \
        src/gcc_wrappers/type/base.cpp \
        src/gcc_wrappers/type/array.cpp \
        src/gcc_wrappers/type/container.cpp \
        src/gcc_wrappers/type/enumeration.cpp \
        src/gcc_wrappers/type/fixed_point.cpp \
        src/gcc_wrappers/type/floating_point.cpp \
        src/gcc_wrappers/type/function.cpp \
        src/gcc_wrappers/type/integral.cpp \
        src/gcc_wrappers/type/method.cpp \
        src/gcc_wrappers/type/numeric.cpp \
        src/gcc_wrappers/type/pointer.cpp \
        src/gcc_wrappers/type/record.cpp \
        src/gcc_wrappers/type/untagged_union.cpp \
        src/gcc_wrappers/builtin_types.cpp \
        src/gcc_wrappers/chain_node.cpp \
        src/gcc_wrappers/list_node.cpp \
        src/gcc_wrappers/statement_list.cpp \
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

.PHONY: testcase_a
testcase_a: TESTDIR=testcases/a
testcase_a: $(PLUGIN)
	- rm -f testcases/*.o
	- rm -f $(TESTDIR)/*.o
	$(CCPLUGIN) -c testcases/bitstreams.c -o testcases/bitstreams.o -Itestcases
	$(CCPLUGIN) -c $(TESTDIR)/test.c -o $(TESTDIR)/test.o -Itestcases
	$(CCPLUGIN) testcases/bitstreams.o $(TESTDIR)/test.o -o $(TESTDIR)/test
	$(TESTDIR)/test

.PHONY: testcase_b
testcase_b: TESTDIR=testcases/b
testcase_b: $(PLUGIN)
	- rm -f testcases/*.o
	- rm -f $(TESTDIR)/*.o
	$(CCPLUGIN) -c testcases/bitstreams.c -o testcases/bitstreams.o -Itestcases
	$(CCPLUGIN) -c $(TESTDIR)/test.c -o $(TESTDIR)/test.o -Itestcases
	$(CCPLUGIN) testcases/bitstreams.o $(TESTDIR)/test.o -o $(TESTDIR)/test
	$(TESTDIR)/test

.PHONY: testcase_bool
testcase_bool: TESTDIR=testcases/bool
testcase_bool: $(PLUGIN)
	- rm -f testcases/*.o
	- rm -f $(TESTDIR)/*.o
	$(CCPLUGIN) -c testcases/bitstreams.c -o testcases/bitstreams.o -Itestcases
	$(CCPLUGIN) -c $(TESTDIR)/test.c -o $(TESTDIR)/test.o -Itestcases
	$(CCPLUGIN) testcases/bitstreams.o $(TESTDIR)/test.o -o $(TESTDIR)/test
	$(TESTDIR)/test

.PHONY: testcase_split_struct
testcase_split_struct: TESTDIR=testcases/split_struct
testcase_split_struct: $(PLUGIN)
	- rm -f testcases/*.o
	- rm -f $(TESTDIR)/*.o
	$(CCPLUGIN) -c testcases/bitstreams.c -o testcases/bitstreams.o -Itestcases
	$(CCPLUGIN) -c $(TESTDIR)/test.c -o $(TESTDIR)/test.o -Itestcases
	$(CCPLUGIN) testcases/bitstreams.o $(TESTDIR)/test.o -o $(TESTDIR)/test
	$(TESTDIR)/test

.PHONY: testcase_split_array
testcase_split_array: TESTDIR=testcases/split_array
testcase_split_array: $(PLUGIN)
	- rm -f testcases/*.o
	- rm -f $(TESTDIR)/*.o
	$(CCPLUGIN) -c testcases/bitstreams.c -o testcases/bitstreams.o -Itestcases
	$(CCPLUGIN) -c $(TESTDIR)/test.c -o $(TESTDIR)/test.o -Itestcases
	$(CCPLUGIN) testcases/bitstreams.o $(TESTDIR)/test.o -o $(TESTDIR)/test
	$(TESTDIR)/test

.PHONY: testcase_anon_struct
testcase_anon_struct: TESTDIR=testcases/anon_struct
testcase_anon_struct: $(PLUGIN)
	- rm -f testcases/*.o
	- rm -f $(TESTDIR)/*.o
	$(CCPLUGIN) -c testcases/bitstreams.c -o testcases/bitstreams.o -Itestcases
	$(CCPLUGIN) -c $(TESTDIR)/test.c -o $(TESTDIR)/test.o -Itestcases
	$(CCPLUGIN) testcases/bitstreams.o $(TESTDIR)/test.o -o $(TESTDIR)/test
	$(TESTDIR)/test

.PHONY: testcase_opaque_buffer
testcase_opaque_buffer: TESTDIR=testcases/opaque_buffer
testcase_opaque_buffer: $(PLUGIN)
	- rm -f testcases/*.o
	- rm -f $(TESTDIR)/*.o
	$(CCPLUGIN) -c testcases/bitstreams.c -o testcases/bitstreams.o -Itestcases
	$(CCPLUGIN) -c $(TESTDIR)/test.c -o $(TESTDIR)/test.o -Itestcases
	$(CCPLUGIN) testcases/bitstreams.o $(TESTDIR)/test.o -o $(TESTDIR)/test
	$(TESTDIR)/test