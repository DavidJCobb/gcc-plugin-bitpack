PLUGIN=lu_bitpack.so
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
        src/attribute_handlers/bitpack_string.cpp \
        src/attribute_handlers/bitpack_transforms.cpp \
        src/attribute_handlers/generic_bitpacking_data_option.cpp \
        src/attribute_handlers/generic_type_or_decl.cpp \
        src/attribute_handlers/test.cpp \
        src/attribute_handlers/no_op.cpp \
        src/bitpacking/data_options/computed.cpp \
        src/bitpacking/data_options/requested.cpp \
        src/bitpacking/data_options/x_options.cpp \
        src/bitpacking/attribute_attempted_on.cpp \
        src/bitpacking/global_options.cpp \
        src/bitpacking/mark_for_invalid_attributes.cpp \
        src/bitpacking/transform_function_validation_helpers.cpp \
        src/bitpacking/verify_bitpack_attributes.cpp \
        src/codegen/decl_descriptor.cpp \
        src/codegen/divide_items_by_sectors.cpp \
        src/codegen/serialization_item.cpp \
        src/gcc_helpers/c/at_file_scope.cpp \
        src/gcc_helpers/dump_function.cpp \
        src/gcc_helpers/extract_pragma_kv_args.cpp \
        src/gcc_helpers/stringify_fully_qualified_accessor.cpp \
        src/lu/strings/builder.cpp \
        src/lu/strings/handle_kv_string.cpp \
        src/lu/strings/printf_string.cpp \
        src/lu/strings/trim.cpp \
        src/pragma_handlers/debug_dump_as_serialization_item.cpp \
        src/pragma_handlers/debug_dump_function.cpp \
        src/pragma_handlers/debug_dump_identifier.cpp \
        src/pragma_handlers/enable.cpp \
        src/pragma_handlers/generate_functions.cpp \
        src/pragma_handlers/set_options.cpp \
        src/xmlgen/is_valid_name.cpp \
        src/xmlgen/string_as_cdata.cpp \
        src/xmlgen/write_string_as_attribute_value.cpp \
        src/xmlgen/write_string_as_text_content.cpp \
        src/xmlgen/xml_element.cpp \
        src/debugprint.cpp \
        src/lu-bitpack.cpp \
		$(END)

include ../../Makefile.common

.PHONY: test
test: $(PLUGIN)
	$(CCPLUGIN) -c -o /dev/null test/test.c -Itest

.PHONY: testcase
testcase: TESTDIR=testcases/$(testname)
testcase: PLUGINARGS=-fplugin-arg-lu_bitpack-xml-out=$(TESTDIR)/test.xml
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
