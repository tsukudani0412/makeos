ifndef IN_CONTAINER
.PHONY: all clean image write default
default:
	@docker compose run --rm makeos make all
all clean image write:
	@echo "--- Running command in Docker container: make $@ ---"
	@docker compose run --rm makeos make $@
%:
	@echo "--- Running command in Docker container: make $@ ---"
	@docker compose run --rm makeos make $@
else

SRCDIR  := src
BUILDDIR := build

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

VPATH = $(SRCDIR)

C_SOURCES = main.c lib.c serial.c
S_SOURCES = startup.s

OBJS = $(addprefix $(BUILDDIR)/, $(C_SOURCES:.c=.o) $(S_SOURCES:.s=.o))

TARGET = kozos

CFLAGS  = -Wall -mh -nostdinc -nostdlib -fno-builtin
CFLAGS += -I$(SRCDIR)
CFLAGS += -Os
CFLAGS += -DKOZOS

LFLAGS = -static -T $(SRCDIR)/ld.scr -L$(BUILDDIR)


.PHONY: all clean

all: $(TARGET)

$(BUILDDIR):
	@echo "Creating build directory: $@"
	@mkdir -p $(BUILDDIR)

$(TARGET): $(OBJS)
	@echo "Linking..."
	$(CC) $(OBJS) -o $(TARGET) $(CFLAGS) $(LFLAGS)
	cp $(TARGET) $(TARGET).elf
	$(STRIP) $(TARGET)
	@echo "Build finished: $(TARGET)"

$(BUILDDIR)/%.o: %.c | $(BUILDDIR)
	@echo "Compiling C: $<"
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILDDIR)/%.o: %.s | $(BUILDDIR)
	@echo "Compiling S: $<"
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	@echo "Cleaning build directory..."
	rm -rf $(BUILDDIR)
	rm -rf $(TARGET) $(TARGET).elf $(TARGET).mot

endif
