**TinyOS Kernel**

A tiny 32-bit kernel demonstrating basic boot process

**Contents**
- **`boot.s`**: Assembly bootstrap, multiboot header, ISR stubs, and stack.
- **`kernel.c`**: C kernel entry (`kernel_main`), VGA text helpers, IDT setup, and C ISR (`isr0_c`).
- **`linker.ld`**: Linker script placing sections at 1 MiB.
- **`Makefile`**: Build rules to produce a bootable ISO with GRUB.

**Build Requirements**
- 32-bit cross toolchain (or GCC multilib): `gcc -m32`, `ld -m elf_i386`.
- GNU `make`.
- `grub-mkrescue` and `qemu-system-i386` (for creating and running the ISO).
- On Windows, use WSL, MSYS2, or install the above tools separately.

**Build**
Run from the project root:

```bash
make clean
make
make run
```

**What it does**
- Clears the VGA text buffer and prints a welcome message from `kernel_main`.

**Future updates**
- IDT/ISR
- Better text output
- Keyboard inputs

License : None