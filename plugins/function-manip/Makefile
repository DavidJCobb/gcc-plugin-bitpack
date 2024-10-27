PLUGIN=function-manip.so
INCDIR=include
SOURCES=\
        src/gcc_helpers/array_extent.cpp \
        src/gcc_helpers/dump_function.cpp \
        src/gcc_helpers/extract_pragma_kv_args.cpp \
        src/gcc_helpers/make_indexed_unbroken_for_loop.cpp \
        src/gcc_helpers/make_member_access_expr.cpp \
        src/gcc_helpers/type_is_char.cpp \
        src/gcc_helpers/type_min_max.cpp \
        src/gcc_helpers/type_name.cpp \
        src/gcc_helpers/type_size_in_bytes.cpp \
        src/gcc_helpers/type_to_string.cpp \
        src/gcc_wrappers/decl/base.cpp \
        src/gcc_wrappers/decl/field.cpp \
        src/gcc_wrappers/decl/function.cpp \
        src/gcc_wrappers/decl/label.cpp \
        src/gcc_wrappers/decl/param.cpp \
        src/gcc_wrappers/decl/result.cpp \
        src/gcc_wrappers/decl/variable.cpp \
        src/gcc_wrappers/expr/assign.cpp \
        src/gcc_wrappers/expr/base.cpp \
        src/gcc_wrappers/expr/declare.cpp \
        src/gcc_wrappers/expr/go_to_label.cpp \
        src/gcc_wrappers/expr/integer_constant.cpp \
        src/gcc_wrappers/expr/label.cpp \
        src/gcc_wrappers/expr/local_block.cpp \
        src/gcc_wrappers/expr/return_result.cpp \
        src/gcc_wrappers/expr/ternary.cpp \
        src/gcc_wrappers/flow/simple_for_loop.cpp \
        src/gcc_wrappers/list_node.cpp \
        src/gcc_wrappers/statement_list.cpp \
        src/gcc_wrappers/type.cpp \
        src/gcc_wrappers/value.cpp \
        src/lu/strings/trim.cpp \
        src/handle_kv_string.cpp \
        src/data_member.cpp \
        src/struct.cpp \
        src/handle_struct_type.cpp \
        src/parse_generate_request.cpp \
        src/generate_read_code.cpp \
        src/generate_read_func.cpp \
        src/function-manip.cpp \
		$(END)

include ../../Makefile.common

.PHONY: test
test: $(PLUGIN)
	$(CCPLUGIN) -c -o /dev/null test/test.c -Itest/