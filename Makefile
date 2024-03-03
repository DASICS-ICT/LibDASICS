# compile logic
CROSS_COMPILE	?= riscv64-unknown-linux-gnu
CC				= $(CROSS_COMPILE)-gcc
OBJDUMP			= $(CROSS_COMPILE)-objdump
AR				= $(CROSS_COMPILE)-ar
RANLIB			= $(CROSS_COMPILE)-ranlib

# C flags
INCLUDE			= -Iinclude
CFLAGS			= -O2 -g $(INCLUDE) -DDASICS_LINUX -DDASICS_COPY
UCFLAGS			= -O2 -g $(INCLUDE)  
# build dir
DIR_PWD			?= 
DIR_BUILD		= build
DIR_SRC			= src
DIR_LIB			= lib
DIR_TEST		= test

# target files
# LIB FILE, stand alone, not depend on any other standard library
LIB_FILES_C		= $(wildcard $(DIR_LIB)/*/*.c)
LIB_FILES_ASM	= $(wildcard $(DIR_LIB)/*/*.S)

# SRC FILE, may used standard library
SRC_FILES_C		= $(wildcard $(DIR_SRC)/*/*.c)
SRC_FILES_ASM	= $(wildcard $(DIR_SRC)/*/*.S)

# TEST FIle
TEST_FILES		= $(wildcard $(DIR_TEST)/*.c)

# BASENAME
BASENAME		= $(notdir $(LIB_FILES_C) $(LIB_FILES_ASM) $(SRC_FILES_C) $(SRC_FILES_ASM))
OBJ_FILES		= $(addprefix build/, \
						$(addsuffix .o, $(basename $(BASENAME))))

# Lib target
LibDASICS		= $(DIR_BUILD)/LibDASICS.a


.PHONY: all

all: $(LibDASICS)

build:
	@mkdir -p $(DIR_BUILD)


# compile all files
$(DIR_BUILD)/%.o: $(DIR_SRC)/*/%.c | $(DIR_BUILD)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo + CC $@

$(DIR_BUILD)/%.o: $(DIR_SRC)/*/%.S | $(DIR_BUILD)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo + CC $@


$(DIR_BUILD)/%.o: $(DIR_LIB)/*/%.c | $(DIR_BUILD)
	@$(CC) -fno-builtin -nostdlib $(CFLAGS) -c $< -o $@
	@echo + CC $@


$(DIR_BUILD)/%.o: $(DIR_LIB)/*/%.S | $(DIR_BUILD)
	@$(CC) -fno-builtin -nostdlib $(CFLAGS) -c $< -o $@
	@echo + CC $@

# Make lib
$(LibDASICS): $(OBJ_FILES)
	@$(AR) rcs $(LibDASICS) $(OBJ_FILES) 
	@echo + AR $(LibDASICS)
	@$(RANLIB) $(LibDASICS)
	@echo +  RANLIB $(LibDASICS)


test: $(LibDASICS)
	@$(CC) $(UCFLAGS) $(TEST_FILES) -o ./build/test $(LibDASICS) -T./ld.lds
	@echo + CC ./build/test
	@$(OBJDUMP) -d ./build/test > $(DIR_BUILD)/test.txt
	@echo + OBJDUMP ./build/test


clean:
	rm -rf $(DIR_BUILD)
