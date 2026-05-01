TARGET := m68k-monitor

CC := /opt/toolchain-m68k-elf-current/bin/m68k-elf-gcc
OBJCPY := /opt/toolchain-m68k-elf-current/bin/m68k-elf-objcopy
SIZE := /opt/toolchain-m68k-elf-current/bin/m68k-elf-size
OBJDUMP := /opt/toolchain-m68k-elf-current/bin/m68k-elf-objdump
GDB := /opt/toolchain-m68k-elf-current/bin/m68k-elf-gdb

VARIANT ?= realhw

TOOLCHAIN := /opt/toolchain-m68k-elf-current/bin

CFLAGS_COMMON := -I. -DREENTRANT_SYSCALLS_PROVIDED -D_REENT_SMALL -Wall -O0 -std=gnu99 -g

LIB_DIR := /opt/toolchain-m68k-elf-current/m68k-elf/lib

ifeq ($(VARIANT),qemu)
    CFLAGS := $(CFLAGS_COMMON) -m68020 -DPLATFORM_QEMU
    LDSCRIPT := qemu.ld
    UART_SRC := uart_qemu
    APPINIT_SRC := appinit_qemu
    AOBJ := crt0_qemu.o vector_simple.o
    LFLAGS := -m68020 -g -nostartfiles -nodefaultlibs -Wl,-T$(LDSCRIPT),-Map=$(TARGET).map
    extra_libs :=
else
    CFLAGS := $(CFLAGS_COMMON) -m68332 -DPLATFORM_REALHW
    LDSCRIPT := ram.ld
    UART_SRC := uart
    APPINIT_SRC := appinit
    AOBJ := crt0.o vector.o
    LFLAGS := -m68332 -g -nostartfiles -nodefaultlibs -Wl,-T$(LDSCRIPT),-Map=$(TARGET).map
    LIB_DIR := /opt/toolchain-m68k-elf-current/m68k-elf/lib
    extra_libs := -L$(LIB_DIR) -lc -lnosys
endif

OBJ := main.o commands.o $(APPINIT_SRC).o $(UART_SRC).o

$(TARGET).elf: $(OBJ) $(AOBJ) $(LDSCRIPT)
	@echo "---> linking $(VARIANT)..."
	$(CC) $(AOBJ) $(OBJ) $(LFLAGS) $(extra_libs) -o $@

%.o: %.S
	$(CC) -c $(CFLAGS) -Wa,-adhlns=$<.lst $< -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) -Wa,-adhlns=$<.lst $< -o $@

files: $(TARGET).elf
	@echo "---> convert to Intel HEX..."
	$(OBJCPY) -O ihex $(TARGET).elf $(TARGET).hex
	@echo "---> convert to Motorola S-Record..."
	$(OBJCPY) -O srec $(TARGET).elf $(TARGET).srec
	@echo "---> convert to binary..."
	$(OBJCPY) -O binary $(TARGET).elf $(TARGET).bin

all: files

clean:
	rm -f *.o *.hex *.srec *.bin *.elf *.map *~ *.lst

size: $(TARGET).elf
	$(SIZE) $(TARGET).elf
	@echo ""
	$(SIZE) -Ax $(TARGET).elf

run-qemu: VARIANT=qemu
run-qemu: all
	@echo "---> Running in QEMU..."
	qemu-system-m68k -M virt -cpu m68020 -kernel m68k-monitor.elf -display none

debug-qemu: VARIANT=qemu
debug-qemu:
	@echo "---> Starting QEMU with GDB server..."
	@echo "---> Connect with: $(GDB) m68k-monitor.elf -ex 'target remote localhost:1234'"
	@echo ""
	qemu-system-m68k -M virt -cpu m68020 -kernel m68k-monitor.elf -display none -s -S

.PHONY: all clean files size run-qemu debug-qemu help

help:
	@echo "Build targets:"
	@echo "  make all VARIANT=realhw       - Build for MC68331 hardware"
	@echo "  make all VARIANT=qemu         - Build for QEMU virt machine"
	@echo "  make run-qemu                - Build and run in QEMU"
	@echo "  make debug-qemu               - Build and start QEMU with GDB server"
	@echo ""
	@echo "Debugging:"
	@echo "  make debug-qemu"
	@echo "  $(GDB) m68k-monitor.elf -ex 'target remote localhost:1234'"