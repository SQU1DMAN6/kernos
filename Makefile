KERNEL = kernos
ISO_DIR = iso
BOOT_DIR = $(ISO_DIR)/boot
GRUB_DIR = $(BOOT_DIR)/grub

all: iso

kasm.o: kernel.asm
	nasm -f elf32 kernel.asm -o kasm.o

kc.o: kernel.c keyboard_map.h
	gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -c kernel.c -o kc.o

$(BOOT_DIR):
	mkdir -p $(BOOT_DIR)
	mkdir -p $(GRUB_DIR)

$(BOOT_DIR)/$(KERNEL).bin: kasm.o kc.o link.ld | $(BOOT_DIR)
	ld -m elf_i386 -T link.ld kasm.o kc.o -o $(BOOT_DIR)/$(KERNEL).bin

iso: $(BOOT_DIR)/$(KERNEL).bin
	cp grub.cfg $(GRUB_DIR)/grub.cfg
	grub-mkrescue -o $(KERNEL).iso $(ISO_DIR)

run: iso
	qemu-system-x86_64 \
	-cdrom $(KERNEL).iso \
	-display gtk \
	-serial stdio

clean:
	rm -rf *.o iso kernos.iso
