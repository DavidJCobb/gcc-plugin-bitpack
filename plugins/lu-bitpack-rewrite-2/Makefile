PLUGIN_NAME=lu_bitpack
PLUGIN=$(PLUGIN_NAME).so
INCDIR=include

include Makefile.gcc-wrappers
SOURCES=\
        $(GCC_WRAPPERS_SOURCES) \
        src/bitpacking/global_options.cpp \
        src/bitpacking/requested_global_options.cpp \
        src/gcc_helpers/c/at_file_scope.cpp \
        src/lu/strings/builder.cpp \
        src/lu/stringf.cpp \
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
