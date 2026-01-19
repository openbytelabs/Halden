TARGET = haldenos.iso
KERNEL = haldenos.elf
BUILD_DIR = build

CC = x86_64-elf-gcc
CFLAGS = -Wall -Wextra -O2 -pipe -I. -ffreestanding -fno-stack-protector -fno-stack-check -fno-lto -fno-pie -fno-pic -m64 -mabi=sysv -mno-80387 -mno-mmx -mno-sse -mno-sse2 -mno-red-zone -mcmodel=large
LDFLAGS = -nostdlib -static -z max-page-size=0x1000 -T linker.ld

C_FILES = kernel.cpp \
          system/utils.cpp \
          system/terminal.cpp \
          system/commands.cpp \
          system/applications.cpp \
          system/network.cpp \
          hlfs/fs.cpp \
          hlpkg/hlpkg.cpp \
          port/port.cpp \
          apps/browser.cpp \
          apps/terminal.cpp \
          apps/filemanager.cpp

OBJ = $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(C_FILES))

all: $(TARGET)

$(KERNEL): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

$(OBJ): limine.h

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

limine.h:
	curl -LO https://github.com/limine-bootloader/limine/raw/v5.x-branch/limine.h

limine:
	git clone https://github.com/limine-bootloader/limine.git --branch=v5.x-branch-binary --depth=1
	$(MAKE) -C limine

$(TARGET): $(KERNEL) limine limine.h
	rm -rf iso_root
	mkdir -p iso_root
	cp $(KERNEL) iso_root/
	cp limine.cfg limine/limine-bios.sys limine/limine-bios-cd.bin limine/limine-uefi-cd.bin iso_root/
	mkdir -p iso_root/EFI/BOOT
	cp limine/BOOTX64.EFI iso_root/EFI/BOOT/ 2>/dev/null || true
	xorriso -as mkisofs -b limine-bios-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table --efi-boot limine-uefi-cd.bin -efi-boot-part --efi-boot-image --protective-msdos-label iso_root -o $(TARGET)
	./limine/limine bios-install $(TARGET) 2>/dev/null || true

clean:
	rm -rf iso_root $(TARGET) $(KERNEL) $(BUILD_DIR) limine limine.h

run: $(TARGET)
	qemu-system-x86_64 -cdrom $(TARGET) -m 2G

debug: $(TARGET)
	qemu-system-x86_64 -cdrom $(TARGET) -m 2G -s -S

test: $(TARGET)
	qemu-system-x86_64 -cdrom $(TARGET) -m 4G -smp 4 -enable-kvm

.PHONY: all clean run debug test