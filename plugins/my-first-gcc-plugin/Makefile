PLUGIN=my-first-gcc-plugin.so
SOURCES=\
        my-first-gcc-plugin.cpp \
		$(END)

include ../../Makefile.common

.PHONY: test
test: $(PLUGIN)
	$(CCPLUGIN) -c -o /dev/null test.c