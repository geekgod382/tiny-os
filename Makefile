TARGET = kernel.bin
ISO = myos.iso

CC = gcc
AS = gcc
LD = ld

CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra \
         -fno-exceptions -fno-stack-protector -fno-pic -fno-builtin
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

OBJ = boot.o kernel.o

all: $(ISO)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(AS) -m32 -c $< -o $@

$(TARGET): $(OBJ) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJ)

$(ISO): $(TARGET) grub/grub.cfg
	rm -rf isodir
	mkdir -p isodir/boot/grub
	cp $(TARGET) isodir/boot/kernel.bin
	cp grub/grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) isodir

run: $(ISO)
	qemu-system-i386 -cdrom $(ISO)

clean:
	rm -f $(OBJ) $(TARGET) $(ISO)
	rm -rf isodir

