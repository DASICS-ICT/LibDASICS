# Common build rules; each configuration can use a separate output directory.
CROSS_COMPILE ?= riscv64-unknown-linux-gnu-
CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
RANLIB = $(CROSS_COMPILE)ranlib
OBJDUMP = $(CROSS_COMPILE)objdump

BUILD_DIR ?= build
CFLAGS = -O2 -g -MMD -Wno-c23-extensions -Wno-varargs -Iinclude -DDASICS_LINUX -DDASICS_DEBUG -DDASICS_COPY
ifdef USER_DEFINE
CFLAGS += -D$(USER_DEFINE)
endif
UCFLAGS = -O2 -g -MMD -Iinclude -Itest/include -MMD

SOURCES = $(wildcard src/*/*.c src/*/*.S lib/*/*.c lib/*/*.S)
OBJECTS = $(addprefix $(BUILD_DIR)/,$(addsuffix .o,$(basename $(SOURCES))))
HEADERS = $(wildcard include/*.h include/asm/*.h)
ARCHIVE = $(BUILD_DIR)/libDASICS.a
TEST_SOURCES = $(wildcard test/*.c)
TEST_LIBS = $(patsubst test/lib/%.c,$(BUILD_DIR)/%.so,$(wildcard test/lib/*.c))

.PHONY: all test clean
all: $(ARCHIVE)

$(BUILD_DIR)/%.o: %.c $(HEADERS) Makefile
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(if $(filter lib/%,$<),-fno-builtin -nostdlib) -c $< -o $@

$(BUILD_DIR)/%.o: %.S $(HEADERS) Makefile
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(if $(filter lib/%,$<),-fno-builtin -nostdlib) -c $< -o $@

$(ARCHIVE): $(OBJECTS)
	$(AR) rcs $@ $(OBJECTS)
	$(RANLIB) $@

$(BUILD_DIR)/%.so: test/lib/%.c $(ARCHIVE)
	$(CC) $(UCFLAGS) -fPIC -shared $< -o $@

test: $(ARCHIVE) $(TEST_LIBS)
	$(CC) $(UCFLAGS) $(TEST_SOURCES) -o $(BUILD_DIR)/test -T./ld.lds $(ARCHIVE)
	$(OBJDUMP) -d $(BUILD_DIR)/test > $(BUILD_DIR)/test.txt

clean:
	rm -rf $(BUILD_DIR)

-include $(OBJECTS:.o=.d)
