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

H8WRITE = /work/tools/h8write/h8write
H8WRITE_SERDEV = /dev/ttyUSB0

VPATH = $(SRCDIR)

C_SOURCES = main.c lib.c serial.c vector.c
S_SOURCES = startup.s

OBJS = $(addprefix $(BUILDDIR)/, $(C_SOURCES:.c=.o) $(S_SOURCES:.s=.o))

TARGET = kzload

CFLAGS  = -Wall -mh -nostdinc -nostdlib -fno-builtin
CFLAGS += -I$(SRCDIR)
CFLAGS += -Os
CFLAGS += -DKZLOAD

LFLAGS = -static -T $(SRCDIR)/ld.scr -L$(BUILDDIR)


.PHONY: all image write clean

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

$(TARGET).mot: $(TARGET)
	@echo "Creating MOT file..."
	$(OBJCOPY) -O srec $(TARGET) $(TARGET).mot

image: $(TARGET).mot

write: $(TARGET).mot
	$(H8WRITE) -3069 -f20 $(TARGET).mot $(H8WRITE_SERDEV)

clean:
	@echo "Cleaning build directory..."
	rm -rf $(BUILDDIR)
	rm -rf $(TARGET) $(TARGET).elf $(TARGET).mot

make_h8write :
								gcc /work/tools/h8write/h8write.c -o /work/tools/h8write/h8write
endif
