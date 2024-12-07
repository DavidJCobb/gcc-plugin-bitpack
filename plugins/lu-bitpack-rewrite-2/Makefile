PLUGIN_NAME=lu_bitpack
PLUGIN=$(PLUGIN_NAME).so
INCDIR=include

include Makefile.gcc-wrappers
SOURCES=\
        $(GCC_WRAPPERS_SOURCES) \
        src/attribute_handlers/helpers/bp_attr_context.cpp \
        src/attribute_handlers/helpers/type_transitively_has_attribute.cpp \
        src/attribute_handlers/bitpack_as_opaque_buffer.cpp \
        src/attribute_handlers/bitpack_bitcount.cpp \
        src/attribute_handlers/bitpack_default_value.cpp \
        src/attribute_handlers/bitpack_range.cpp \
        src/attribute_handlers/bitpack_stat_category.cpp \
        src/attribute_handlers/bitpack_string.cpp \
        src/attribute_handlers/bitpack_tagged_id.cpp \
        src/attribute_handlers/bitpack_transforms.cpp \
        src/attribute_handlers/bitpack_union_external_tag.cpp \
        src/attribute_handlers/bitpack_union_internal_tag.cpp \
        src/attribute_handlers/generic_bitpacking_data_option.cpp \
        src/attribute_handlers/generic_type_or_decl.cpp \
        src/attribute_handlers/test.cpp \
        src/attribute_handlers/no_op.cpp \
        src/bitpacking/attribute_attempted_on.cpp \
        src/bitpacking/data_options/typed.cpp \
        src/bitpacking/data_options.cpp \
        src/bitpacking/get_union_bitpacking_info.cpp \
        src/bitpacking/global_options.cpp \
        src/bitpacking/mark_for_invalid_attributes.cpp \
        src/bitpacking/requested_global_options.cpp \
        src/bitpacking/transform_function_validation_helpers.cpp \
        src/bitpacking/verify_bitpack_attributes_on_type_finished.cpp \
        src/bitpacking/verify_union_external_tag.cpp \
        src/bitpacking/verify_union_internal_tag.cpp \
        src/bitpacking/verify_union_members.cpp \
        src/gcc_helpers/c/at_file_scope.cpp \
        src/lu/strings/builder.cpp \
        src/lu/strings/handle_kv_string.cpp \
        src/lu/strings/trim.cpp \
        src/lu/stringf.cpp \
        src/pragma_handlers/debug_dump_bp_data_options.cpp \
        src/pragma_handlers/debug_dump_function.cpp \
        src/pragma_handlers/debug_dump_identifier.cpp \
        src/pragma_handlers/enable.cpp \
        src/pragma_handlers/set_options.cpp \
        src/basic_global_state.cpp \
        src/debugprint.cpp \
        src/pragma_parse_exception.cpp \
        src/$(PLUGIN_NAME).cpp \
		$(END)

include ../../Makefile.common

.PHONY: test
test: $(PLUGIN)
	$(CCPLUGIN) -c -o /dev/null testcases/basic/test.c -Itest

.PHONY: testcase
testcase: TESTDIR=testcases/$(testname)
testcase: PLUGINARGS=-fplugin-arg-$(PLUGIN_NAME)-xml-out=$(TESTDIR)/test.xml
testcase: $(PLUGIN)
ifdef test
	$(error Specify a testname as make testname=name testcase)
else
	- rm -f testcases/*.o
	- rm -f $(TESTDIR)/*.o
	$(CCPLUGIN) $(PLUGINARGS) -c testcases/bitstreams.c -o testcases/bitstreams.o -Itestcases
	$(CCPLUGIN) $(PLUGINARGS) -c $(TESTDIR)/test.c -o $(TESTDIR)/test.o -Itestcases
	$(CCPLUGIN) $(PLUGINARGS) testcases/bitstreams.o $(TESTDIR)/test.o -o $(TESTDIR)/test
	$(TESTDIR)/test
endif
