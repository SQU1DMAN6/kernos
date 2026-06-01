KERNEL = kernos
ISO_DIR = iso
BOOT_DIR = $(ISO_DIR)/boot
GRUB_DIR = $(BOOT_DIR)/grub
GRUB_MKRESCUE := $(shell command -v grub-mkrescue 2>/dev/null || command -v grub2-mkrescue 2>/dev/null)
CC = gcc
CFLAGS = -Iinclude \
	-m32 \
	-ffreestanding \
	-fno-pie \
	-fno-stack-protector \
	-nostdlib

all: iso

kasm.o: kernel.asm
	nasm -f elf32 kernel.asm -o kasm.o


kc.o: kernel.c include/keyboard_map.h include/terminal.h include/idt.h include/pic.h
	gcc -Iinclude -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -c kernel.c -o kc.o

drivers/keyboard_map.o: drivers/keyboard_map.c include/keyboard_map.h
	gcc -Iinclude -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -c drivers/keyboard_map.c -o drivers/keyboard_map.o

drivers/framebuffer.o: drivers/framebuffer.c include/framebuffer.h
	gcc $(CFLAGS) -c drivers/framebuffer.c -o drivers/framebuffer.o

fonts/kernos8x16.o: fonts/kernos8x16.c include/kernos8x16.h
	gcc $(CFLAGS) -c fonts/kernos8x16.c -o fonts/kernos8x16.o

terminal/terminal.o: terminal/terminal.c include/terminal.h include/keyboard_map.h
	gcc -Iinclude -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -c terminal/terminal.c -o terminal/terminal.o

arch/i386/idt.o: arch/i386/idt.c include/idt.h include/pic.h include/io.h
	gcc -Iinclude -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -c arch/i386/idt.c -o arch/i386/idt.o

arch/i386/pic.o: arch/i386/pic.c include/pic.h include/io.h
	gcc -Iinclude -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -c arch/i386/pic.c -o arch/i386/pic.o

arch/i386/io.o: arch/i386/io.c include/io.h
	gcc -Iinclude -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -c arch/i386/io.c -o arch/i386/io.o

arch/i386/keyboard.o: arch/i386/keyboard.c include/keyboard.h include/keyboard_map.h include/terminal.h include/io.h
	gcc -Iinclude -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -c arch/i386/keyboard.c -o arch/i386/keyboard.o

arch/i386/power.o: arch/i386/power.c include/power.h include/io.h
	gcc -Iinclude -m32 -ffreestanding -fno-pie \
	-fno-stack-protector -nostdlib \
	-c arch/i386/power.c -o arch/i386/power.o

arch/i386/timer.o: arch/i386/timer.c include/timer.h include/io.h
	gcc -Iinclude -m32 -ffreestanding -fno-pie \
	-fno-stack-protector -nostdlib \
	-c arch/i386/timer.c -o arch/i386/timer.o

lib/string.o: lib/string.c include/string.h
	gcc -Iinclude -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -c lib/string.c -o lib/string.o

terminal/shell.o: terminal/shell.c include/shell.h include/terminal.h include/string.h
	gcc -Iinclude -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -c terminal/shell.c -o terminal/shell.o

mm/heap.o: mm/heap.c include/memory.h
	gcc -Iinclude -m32 -ffreestanding -fno-pie \
	-fno-stack-protector -nostdlib \
	-c mm/heap.c -o mm/heap.o

fs/vfs.o: fs/vfs.c
	$(CC) $(CFLAGS) -c fs/vfs.c -o fs/vfs.o

fs/ramfs.o: fs/ramfs.c
	$(CC) $(CFLAGS) -c fs/ramfs.c -o fs/ramfs.o

$(BOOT_DIR):
	mkdir -p $(BOOT_DIR)
	mkdir -p $(GRUB_DIR)

$(BOOT_DIR)/$(KERNEL).bin: \
	kasm.o \
	kc.o \
	drivers/keyboard_map.o \
	drivers/framebuffer.o \
	fonts/kernos8x16.o \
	terminal/terminal.o \
	terminal/shell.o \
	lib/string.o \
	arch/i386/idt.o \
	arch/i386/pic.o arch/i386/io.o \
	arch/i386/keyboard.o \
	arch/i386/power.o \
	arch/i386/timer.o \
	mm/heap.o \
	fs/vfs.o \
	fs/ramfs.o \
	link.ld | $(BOOT_DIR)

	ld -m elf_i386 -T link.ld \
	kasm.o \
	kc.o \
	drivers/keyboard_map.o \
	drivers/framebuffer.o \
	fonts/kernos8x16.o \
	terminal/terminal.o \
	terminal/shell.o \
	lib/string.o \
	arch/i386/idt.o \
	arch/i386/pic.o \
	arch/i386/io.o \
	arch/i386/keyboard.o \
	arch/i386/power.o \
	arch/i386/timer.o \
	mm/heap.o \
	fs/vfs.o \
	fs/ramfs.o \
	-o $(BOOT_DIR)/$(KERNEL).bin

iso: $(BOOT_DIR)/$(KERNEL).bin
	cp grub.cfg $(GRUB_DIR)/grub.cfg
	$(GRUB_MKRESCUE) -o $(KERNEL).iso $(ISO_DIR)

run: iso
	qemu-system-x86_64 \
	-cdrom $(KERNEL).iso \
	-display sdl \
	-serial stdio

clean:
	rm -rf *.o */*.o */*/*.o iso kernos.iso
