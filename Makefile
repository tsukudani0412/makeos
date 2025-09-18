ifndef IN_CONTAINER
.PHONY: all clean image write bootloader os default
default:
	@docker compose run --rm makeos make all
all clean image write bootloader os:
	@echo "--- Running command in Docker container: make $@"
	@docker compose run --rm makeos make $@
%:
	@echo "--- Running command in Docker container: make $@"
	@docker compose run --rm makeos make $@
else

PREFIX  = /usr/local
ARCH    = h8300-elf
BINDIR  = $(PREFIX)/bin
ADDNAME = $(ARCH)-

AR      = $(BINDIR)/$(ADDNAME)ar
AS      = $(BINDIR)/$(ADDNAME)as
CC      = $(BINDIR)/$(ADDNAME)gcc
LD      = $(BINDIR)/$(ADDNAME)ld
NM      = $(BINDIR)/$(ADDNAME)nm
OBJCOPY = $(BINDIR)/$(ADDNAME)objcopy
OBJDUMP = $(BINDIR)/$(ADDNAME)objdump
RANLIB  = $(BINDIR)/$(ADDNAME)ranlib
STRIP   = $(BINDIR)/$(ADDNAME)strip

H8WRITE = /work/tools/h8write/h8write
H8WRITE_SERDEV = /dev/ttyUSB0


BL_SRCDIR    := src/bootloader
BL_BUILDDIR  := build/bootloader
BL_TARGET    := kzload

BL_C_SOURCES = main.c lib.c serial.c vector.c xmodem.c elf.c interrupt.c
BL_S_SOURCES = startup.S intr.S
BL_OBJS      = $(addprefix $(BL_BUILDDIR)/, $(BL_C_SOURCES:.c=.o) $(BL_S_SOURCES:.S=.o))

BL_CFLAGS    = -Wall -mh -nostdinc -nostdlib -fno-builtin
BL_CFLAGS   += -I$(BL_SRCDIR)
BL_CFLAGS   += -Os
BL_CFLAGS   += -DKZLOAD

BL_LFLAGS    = -static -T $(BL_SRCDIR)/ld.scr -L$(BL_BUILDDIR)


OS_SRCDIR    := src/os
OS_BUILDDIR  := build/os
OS_TARGET    := kozos

OS_C_SOURCES = main.c lib.c serial.c interrupt.c kozos.c syscall.c memory.c test10_1.c
OS_S_SOURCES = startup.S
OS_OBJS      = $(addprefix $(OS_BUILDDIR)/, $(OS_C_SOURCES:.c=.o) $(OS_S_SOURCES:.S=.o))

OS_CFLAGS    = -Wall -mh -nostdinc -nostdlib -fno-builtin
OS_CFLAGS   += -I$(OS_SRCDIR)
OS_CFLAGS   += -Os
OS_CFLAGS   += -DKOZOS

OS_LFLAGS    = -static -T $(OS_SRCDIR)/ld.scr -L$(OS_BUILDDIR)


.PHONY: all bootloader os image write clean

all: bootloader os


bootloader: $(BL_TARGET)

$(BL_TARGET): $(BL_OBJS)
	@echo "Linking bootloader..."
	$(CC) $(BL_OBJS) -o $@ $(BL_CFLAGS) $(BL_LFLAGS)
	cp $@ $(BL_TARGET).elf
	$(STRIP) $@
	@echo "Bootloader build finished: $@"

$(BL_BUILDDIR)/%.o: $(BL_SRCDIR)/%.c | $(BL_BUILDDIR)
	@echo "Compiling C for bootloader: $<"
	$(CC) -c $(BL_CFLAGS) -o $@ $<

$(BL_BUILDDIR)/%.o: $(BL_SRCDIR)/%.s | $(BL_BUILDDIR)
	@echo "Compiling S for bootloader: $<"
	$(CC) -c $(BL_CFLAGS) -o $@ $<

$(BL_BUILDDIR)/%.o: $(BL_SRCDIR)/%.S | $(BL_BUILDDIR)
	@echo "Compiling S for bootloader: $<"
	$(CC) -c $(BL_CFLAGS) -o $@ $<

os: $(OS_TARGET)

$(OS_TARGET): $(OS_OBJS)
	@echo "Linking OS..."
	$(CC) $(OS_OBJS) -o $@ $(OS_CFLAGS) $(OS_LFLAGS)
	cp $@ $(OS_TARGET).elf
	$(STRIP) $@
	@echo "OS build finished: $@"

$(OS_BUILDDIR)/%.o: $(OS_SRCDIR)/%.c | $(OS_BUILDDIR)
	@echo "Compiling C for OS: $<"
	$(CC) -c $(OS_CFLAGS) -o $@ $<

$(BL_BUILDDIR)/%.o: $(OS_SRCDIR)/%.s | $(OS_BUILDDIR)
	@echo "Compiling S for OS: $<"
	$(CC) -c $(OS_CFLAGS) -o $@ $<

$(OS_BUILDDIR)/%.o: $(OS_SRCDIR)/%.S | $(OS_BUILDDIR)
	@echo "Compiling S for OS: $<"
	$(CC) -c $(OS_CFLAGS) -o $@ $<

$(BL_BUILDDIR) $(OS_BUILDDIR):
	@echo "Creating build directory: $@"
	@mkdir -p $@


$(BL_TARGET).mot: $(BL_TARGET)
	@echo "Creating MOT file for bootloader..."
	$(OBJCOPY) -O srec $< $@

image: $(BL_TARGET).mot

write: $(BL_TARGET).mot
	$(H8WRITE) -3069 -f20 $< $(H8WRITE_SERDEV)


clean:
	@echo "Cleaning build directories and artifacts..."
	rm -rf build
	rm -f $(BL_TARGET) $(BL_TARGET).elf $(BL_TARGET).mot
	rm -f $(OS_TARGET) $(OS_TARGET).elf


make_h8write :
	gcc /work/tools/h8write/h8write.c -o /work/tools/h8write/h8write
endif
