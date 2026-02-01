#include <stdint.h>
#include "io.h"
#include "fs.h"

static volatile uint16_t* const VGA_BUFFER = (uint16_t*)0xB8000;
static const int VGA_COLS = 80;
static const int VGA_ROWS = 25;

// VGA text mode output
static uint8_t vga_entry_color(uint8_t fg, uint8_t bg){
    return fg | (bg << 4);
}

static uint16_t vga_entry(char c, uint8_t color){
    return (uint16_t)c | (uint16_t)(color << 8);
}

static void clear_screen(uint8_t color){
    for (int i = 0; i < VGA_COLS * VGA_ROWS; ++i){
        VGA_BUFFER[i] = vga_entry(' ', color);
    }
}

static void kprint_at(const char* msg, int row, int col, uint8_t color){
    int index = row * VGA_COLS + col;
    for (int i = 0; msg[i] != '\0'; ++i){
        VGA_BUFFER[index + i] = vga_entry(msg[i], color);
    }
}

static int kstrcmp(const char* a, const char* b){
    int i = 0;
    while (a[i] != '\0' && b[i] != '\0'){
        if (a[i] != b[i]){
            return (unsigned char)a[i] - (unsigned char)b[i];
        }
        i++;
    }
    return (unsigned char)a[i] - (unsigned char)b[i];
}

static void shell_print_line(const char* msg, int* row, uint8_t color){
    if (*row >= VGA_ROWS){
        clear_screen(color);
        *row = 1;
    }
    kprint_at(msg, *row, 2, color);
    (*row)++;
}


// Cursor control functions
void enable_cursor(uint8_t cursor_start, uint8_t cursor_end) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);
    
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
}

void disable_cursor() {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

void update_cursor(int row, int col) {
    uint16_t pos = row * VGA_COLS + col;
    
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}


// Notepad implementation

#define KBD_DATA_PORT 0x60
#define KBD_STATUS_PORT 0x64


static const char scancode_set1[128] = {
    [0x01] = '\033',  // Escape

    // Number row
    [0x02] = '1', [0x03] = '2', [0x04] = '3', [0x05] = '4',
    [0x06] = '5', [0x07] = '6', [0x08] = '7', [0x09] = '8',
    [0x0A] = '9', [0x0B] = '0',
    [0x0C] = '-',     // Minus
    [0x0D] = '=',     // Equals
    [0x0E] = '\b',    // Backspace
    [0x0F] = '\t',    // Tab

    // Top letter row
    [0x10] = 'q', [0x11] = 'w', [0x12] = 'e', [0x13] = 'r',
    [0x14] = 't', [0x15] = 'y', [0x16] = 'u', [0x17] = 'i',
    [0x18] = 'o', [0x19] = 'p',
    [0x1A] = '[',     // Left bracket
    [0x1B] = ']',     // Right bracket
    [0x1C] = '\n',    // Enter

    // Middle letter row
    [0x1E] = 'a', [0x1F] = 's', [0x20] = 'd', [0x21] = 'f',
    [0x22] = 'g', [0x23] = 'h', [0x24] = 'j', [0x25] = 'k',
    [0x26] = 'l',
    [0x27] = ';',     // Semicolon
    [0x28] = '\'',    // Single quote
    [0x29] = '`',     // Backtick

    // Bottom letter row
    [0x2B] = '\\',    // Backslash
    [0x2C] = 'z', [0x2D] = 'x', [0x2E] = 'c', [0x2F] = 'v',
    [0x30] = 'b', [0x31] = 'n', [0x32] = 'm',
    [0x33] = ',',     // Comma
    [0x34] = '.',     // Period
    [0x35] = '/',     // Forward slash

    [0x39] = ' ',     // Space

    [0x1D] = '\x11',  // Left Control (non-newline control code used to avoid colliding with Enter)
    [0x38] = 40,      // Left Alt

};

static uint8_t keyboard_read_scancode(){
    while (!(inb(KBD_STATUS_PORT) & 0x01));
    return inb(KBD_DATA_PORT);
}

static char get_keyboard_char(void){
    for (;;){
        uint8_t sc = keyboard_read_scancode();
        if (sc == 0x80) continue;
        if (sc == 0x1C) return '\n';
        if (sc == 0x0E) return '\b';
        if (sc == 0x01) return '\033'; // Escape

        if (sc < sizeof(scancode_set1)){
            char c = scancode_set1[sc];
            if (c) return c;
        }
    }
}

void main_menu(void);

void save_function(uint16_t* buffer){
    uint8_t color = vga_entry_color(1, 15);

    kprint_at("Please enter file name (max 23 chars):", 20, 2, color);
    char filename[24];
    for (int i = 0; i < 24; ++i) filename[i] = '\0';
    int row = 21, col = 2;
    int fpos = 0;

    enable_cursor(0, 15);
    update_cursor(row, col);

    for (;;){
        char ch = get_keyboard_char();

        if (ch == '\b'){
            if (fpos > 0){
                fpos--;
                if (col > 2) {
                    col--;
                } else if (row > 3) {
                    row--;
                    col = VGA_COLS - 1;
                }
                int idx = row * VGA_COLS + col;
                VGA_BUFFER[idx] = vga_entry(' ', color);
                update_cursor(row, col);
            }
        }
        else if (ch == '\033'){
            disable_cursor();
            clear_screen(color);
            return;
        }
        else if (ch == '\n'){
            filename[fpos] = '\0';
            /* Extract text area from the VGA buffer into a byte array */
            uint8_t data[VGA_COLS * (VGA_ROWS - 4)];
            int dpos = 0;
            for (int r = 4; r < VGA_ROWS; ++r){
                for (int c = 2; c < VGA_COLS; ++c){
                    uint16_t entry = buffer[r * VGA_COLS + c];
                    char ch2 = (char)(entry & 0xFF);
                    data[dpos++] = (uint8_t)ch2;
                }
            }
            int v = fs_write_file(filename, data, dpos);
            if (v == 0){
                kprint_at("File saved successfully!", row + 2, 2, color);
            } else {
                kprint_at("Error saving file!", row + 2, 2, color);
            }
            disable_cursor();
            return;
        }
        else {
            if (fpos < 23 && ch >= ' '){
                filename[fpos++] = ch;
                int idx = row * VGA_COLS + col;
                VGA_BUFFER[idx] = vga_entry(ch, color);
                col++;
                if (col >= VGA_COLS){
                    col = 2;
                    row++;
                }
                update_cursor(row, col);
            }
        }
    }
} 


void notepad(void){
    uint8_t color = vga_entry_color(1, 15);
    clear_screen(color);

    kprint_at("TinyOS Notepad - Type your text below:", 1, 2, color);
    kprint_at("press esc to exit", 2, 2, color);
    kprint_at("----------------------------------------", 3, 2, color);

    int row = 4, col = 2;

    enable_cursor(0, 15);
    update_cursor(row, col);

    for (;;){
        char c = get_keyboard_char();
        if (c == '\n'){
            row++;
            col = 2;
            update_cursor(row, col);
        }
        else if (c == '\b'){
            if (col > 2){
                col--;
            }
            else if ((row > 4) && (col == 2)){
                row--;
                col = VGA_COLS - 1;
            }
            int idx = row * VGA_COLS + col;
            VGA_BUFFER[idx] = vga_entry(' ', color);
            update_cursor(row, col);
        }
        else if (c == '\033'){
            disable_cursor();
            clear_screen(color);
            return;
        }

        else if (c == '\x11'){
            char x = get_keyboard_char();
            if (x == 's'){
                save_function(VGA_BUFFER);
            }
        }

        else{
            if (row >= VGA_ROWS){
                continue;
            }
            int idx = row * VGA_COLS + col;
            VGA_BUFFER[idx] = vga_entry(c, color);
            col++;
            if (col >= VGA_COLS){
                col = 2;
                row++;
            }
            update_cursor(row, col);
        }
    }

}

void shell(void){
    uint8_t color = vga_entry_color(1, 15);
    int row = 1;

    clear_screen(color);
    shell_print_line("TinyOS Shell - type 'help' for commands, 'q' to quit", &row, color);

    enable_cursor(0, 15);

    for (;;){
        if (row >= VGA_ROWS){
            clear_screen(color);
            row = 1;
            shell_print_line("TinyOS Shell - type 'help' for commands, 'q' to quit", &row, color);
        }

        char line[80];
        int len = 0;
        int col = 2;
        const char* prompt = "tinyos> ";

        kprint_at(prompt, row, col, color);
        col += 8; // length of "tinyos> "
        update_cursor(row, col);

        for (;;){
            char c = get_keyboard_char();

            if (c == '\n'){
                break;
            }
            else if (c == '\b'){
                if (len > 0 && col > 2 + 8){
                    len--;
                    col--;
                    int idx = row * VGA_COLS + col;
                    VGA_BUFFER[idx] = vga_entry(' ', color);
                    update_cursor(row, col);
                }
            }
            else if (c >= 32 && c <= 126){
                if (len < (int)(sizeof(line) - 1) && col < VGA_COLS - 1){
                    line[len++] = c;
                    int idx = row * VGA_COLS + col;
                    VGA_BUFFER[idx] = vga_entry(c, color);
                    col++;
                    update_cursor(row, col);
                }
            }
        }

        line[len] = '\0';
        row++;

        if (len == 0){
            continue;
        }

        int i = 0;
        while (line[i] == ' '){
            i++;
        }
        char* cmd = &line[i];

        int cmd_end = i;
        while (line[cmd_end] != '\0' && line[cmd_end] != ' '){
            cmd_end++;
        }
        char* arg = 0;
        if (line[cmd_end] != '\0'){
            line[cmd_end] = '\0';
            int j = cmd_end + 1;
            while (line[j] == ' '){
                j++;
            }
            if (line[j] != '\0'){
                arg = &line[j];
            }
        }

        if (kstrcmp(cmd, "help") == 0){
            shell_print_line("Available commands:", &row, color);
            shell_print_line("  help      - show this help", &row, color);
            shell_print_line("  clear     - clear the screen", &row, color);
            shell_print_line("  version   - show version info", &row, color);
            shell_print_line("  ls        - list files", &row, color);
            shell_print_line("  cat <f>   - show file contents", &row, color);
            shell_print_line("  rm <f>    - delete file", &row, color);
            shell_print_line("  notepad   - open notepad", &row, color);
            shell_print_line("  q         - return to menu", &row, color);
        }
        else if (kstrcmp(cmd, "clear") == 0){
            clear_screen(color);
            row = 1;
            shell_print_line("TinyOS Shell - type 'help' for commands, 'q' to quit", &row, color);
        }
        else if (kstrcmp(cmd, "notepad") == 0){
            notepad();
            clear_screen(color);
            row = 1;
            shell_print_line("TinyOS Shell - type 'help' for commands, 'q' to quit", &row, color);
            enable_cursor(0, 15);
        }

        else if (kstrcmp(cmd, "version") == 0){
            shell_print_line("TinyOS version 1.1", &row, color);
            shell_print_line("Built January 2025", &row, color);
        }

        else if (kstrcmp(cmd, "ls") == 0){
            int any = 0;
            for (int f = 0; f < MAX_FILES; ++f){
                if (root_dir[f].used){
                    shell_print_line(root_dir[f].name, &row, color);
                    any = 1;
                }
            }
            if (!any){
                shell_print_line("(no files)", &row, color);
            }
        }
        else if (kstrcmp(cmd, "cat") == 0){
            if (!arg || arg[0] == '\0'){
                shell_print_line("Usage: cat <filename>", &row, color);
            } else {
                uint8_t buf[512];
                int n = fs_read_file(arg, buf, sizeof(buf) - 1);
                if (n < 0){
                    shell_print_line("File not found", &row, color);
                } else {
                    buf[n] = '\0';
                    shell_print_line((char*)buf, &row, color);
                }
            }
        }

        else if (kstrcmp(cmd, "rm") == 0){
            if (!arg || arg[0] == '\0'){
                shell_print_line("Usage: rm <filename>", &row, color);
            } else {
                int r = fs_delete_file(arg);
                if (r == 0){
                    shell_print_line("File deleted", &row, color);
                } else {
                    shell_print_line("File not found", &row, color);
                }
            }
        }

        else if (kstrcmp(cmd, "q") == 0){
            disable_cursor();
            main_menu();
        }

        else {
            shell_print_line("Unknown command", &row, color);
        }
    }

    disable_cursor();
}


/* ===== IDT structures ===== */
struct idt_entry{
    uint16_t offset_low;   // lower 16 bits of handler address
    uint16_t selector;     // code segment selector
    uint8_t  zero;         // always 0
    uint8_t  type_attr;    // type and attributes
    uint16_t offset_high;  // upper 16 bits of handler address
} __attribute__((packed));

struct idt_ptr{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct idt_entry idt[256] __attribute__((aligned(8)));
static struct idt_ptr idtp __attribute__((aligned(8)));

// Assembly functions
extern void idt_load(uint32_t);
extern void isr0(void);

/* C-level ISR handler */
// void isr0_c(void){
//     uint8_t color = vga_entry_color(15, 4); // white on red
//     kprint_at(">>> INT 0 FIRED! <<<", 16, 2, color);
        // Halt after handling
//     for(;;) __asm__ __volatile__("hlt");
// }

static void idt_set_gate(int n, uint32_t handler){
    idt[n].offset_low  = handler & 0xFFFF;
    idt[n].selector    = 0x10;      // kernel code segment
    idt[n].zero        = 0;
    idt[n].type_attr   = 0x8E;      // present, ring 0, 32-bit interrupt gate
    idt[n].offset_high = (handler >> 16) & 0xFFFF;
}

static void idt_init(void){
    // Clear the IDT
    for(int i = 0; i < 256; i++){
        idt[i].offset_low = 0;
        idt[i].selector = 0;
        idt[i].zero = 0;
        idt[i].type_attr = 0;
        idt[i].offset_high = 0;
    }
    
    // Set up IDT pointer
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (uint32_t)&idt;

    // Install ISR handlers for CPU exceptions (0-31)
    for (int i = 0; i < 32; ++i) {
        idt_set_gate(i, (uint32_t)isr0);
    }

    // Load the IDT
    idt_load((uint32_t)&idtp);
}

// File system self-test
// void fs_self_test(void) {
//     uint8_t color = vga_entry_color(1, 15);
//     clear_screen(color);

//     const char* fname = "test.txt";
//     char buf[128];

//     int n = fs_read_file(fname, (uint8_t*)buf, sizeof(buf) - 1);

//     if (n >= 0) {
//         buf[n] = '\0';
//         kprint_at("FS: Found existing file:", 2, 2, color);
//         kprint_at(fname, 3, 2, color);
//         kprint_at("Contents:", 5, 2, color);
//         kprint_at(buf, 6, 2, color);
//     } else {
//         const char* msg = "Hello from TinyFS (persistent)!";
//         int rc = fs_write_file(fname, (const uint8_t*)msg, 32);

//         if (rc == 0) {
//             // Immediately read it back
//             int m = fs_read_file(fname, (uint8_t*)buf, sizeof(buf) - 1);
//             if (m >= 0) {
//                 buf[m] = '\0';
//                 kprint_at("FS: wrote and re-read test.txt:", 2, 2, color);
//                 kprint_at(buf, 3, 2, color);
//                 kprint_at("Now reboot and it should load.", 5, 2, color);
//             } else {
//                 kprint_at("FS: write OK, but read failed.", 2, 2, color);
//             }
//         } else {
//             kprint_at("FS: failed to create test.txt", 2, 2, color);
//         }
//     }
// }

void main_menu(void) {
    uint8_t color = vga_entry_color(1, 15);

    for (;;){
        clear_screen(color);
        kprint_at("Welcome to my TinyOS kernel!", 1, 2, color);
        kprint_at("TinyOS Main Menu", 3, 2, color);

        // Set up IDT
        // idt_init();
        // kprint_at("IDT initialized successfully!", 13, 2, color);
        
        // kprint_at("Triggering INT 0 (divide by zero)...", 14, 2, color);

        // Trigger interrupt 0
        // __asm__ __volatile__("int $0");

        // Should not reach here if ISR halts
        // kprint_at("ERROR: Returned from INT 0!", 15, 2, vga_entry_color(15, 4));

        kprint_at("+---------------------------+", 5, 2, color);
        kprint_at("| n - Notepad               |", 6, 2, color);
        kprint_at("| s - Shell                 |", 7, 2, color);
        kprint_at("+---------------------------+", 8, 2, color);
        kprint_at("Press keys to open the app", 10, 2, color);

        char c = get_keyboard_char();

        if (c == 'n' || c == 'N') {
            notepad();
        }
        if (c == 's' || c == 'S') {
            shell();
        }
    }
}

void kernel_main(void){
    fs_init();
    main_menu();

    for (;;){
        __asm__ __volatile__("hlt");
    }
}
