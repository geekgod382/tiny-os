**TinyOS Kernel**

A tiny 32-bit kernel demonstrating basic boot process, with a small ISR implementation and a persistent filesystem

**Contents**
- **`boot.s`**: Assembly bootstrap, multiboot header, ISR stubs, and stack.
- **`kernel.c`**: C kernel entry (`kernel_main`), VGA text helpers, IDT setup, and C ISR (`isr0_c`).
- **`linker.ld`**: Linker script placing sections at 1 MiB.
- **`Makefile`**: Build rules to produce a bootable ISO with GRUB.
- The different header files include functions for ATA PIO and filesystem operations

**Build Requirements**
- 32-bit cross toolchain (or GCC multilib): `gcc -m32`, `ld -m elf_i386`.
- NASM
- GNU `make`.
- `grub-mkrescue` and `qemu-system-i386` (for creating and running the ISO).
- On Windows, use WSL, MSYS2, or install the above tools separately.

**Creating a persistent driver image**
Run this command ONLY ONCE in project directory:

```bash
qemu-img create -f raw tinyfs.img 64M
```
This will create a 64 MB driver image

**Build**

```bash
make clean
make
make run
```

**What it does**
- Clears the VGA text buffer and prints a welcome message and menu from `kernel_main`.
- It has one new feature : A small notepad. Use it by pressing `n` on keyboard.

**IDT/ISR testing**
Uncomment the IDT implementation for testing ISR. This will:
- Initialise IDT/ISR
- Trigger interrupt 0 `int $0`
- Display `>>> INT 0 FIRED! <<<` after handling interrupt succesfully

**Testing persistence of filesystem**
Uncomment the file system self-test script and call the function in inside `kernel_main`

**Future updates**
- IDT/ISR (Done)
- Keyboard inputs (Done)
- Shell (Done)
- Filesystem (Done)

License : None