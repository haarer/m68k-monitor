TARGET := m68k-monitor

CC := m68k-elf-gcc
OBJCPY := m68k-elf-objcopy
SIZE := m68k-elf-size
OBJDUMP := m68k-elf-objdump

CFLAGS += -m68332 -I. -DREENTRANT_SYSCALLS_PROVIDED -D_REENT_SMALL -Wall -O0 -std=gnu99 -g

LFLAGS += -m68332 -g -nostartfiles -Wl,--script=ram.ld,-Map=$(TARGET).map,--allow-multiple-definition
LIBS := -lnosys -lc

OBJ := main.o commands.o uart.o appinit.o
AOBJ := crt0.o vector.o

$(TARGET).elf: $(OBJ) $(AOBJ) ram.ld
	@echo "---> linking..."
	$(CC) $(AOBJ) $(OBJ) $(LFLAGS) $(LIBS) -o $@

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
	rm -f $(OBJ) $(AOBJ) *.hex *.srec *.bin *.elf *.map *~ *.lst

size: $(TARGET).elf
	$(SIZE) $(TARGET).elf
	@echo ""
	$(SIZE) -Ax $(TARGET).elf

.PHONY: all clean files size