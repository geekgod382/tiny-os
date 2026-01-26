#include <stdint.h>

static inline uint8_t inb(uint16_t port){
    uint8_t value;
    __asm__ __volatile__ ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

// VGA text mode output
static volatile uint16_t* const VGA_BUFFER = (uint16_t*)0xB8000;
static const int VGA_COLS = 80;
static const int VGA_ROWS = 25;

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

void notepad(void){
    uint8_t color = vga_entry_color(1, 15);
    clear_screen(color);

    kprint_at("TinyOS Notepad - Type your text below:", 1, 2, color);
    kprint_at("press esc to exit", 2, 2, color);
    kprint_at("----------------------------------------", 3, 2, color);

    int row = 4, col = 2;
    for (;;){
        char c = get_keyboard_char();
        if (c == '\n'){
            row++;
            col = 2;
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
        }
        else if (c == '\033'){
            clear_screen(color);
            kprint_at("Welcome to my TinyOS kernel!", 1, 2, color);
            main_menu();
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
        }
    }

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

void main_menu(void) {
    uint8_t color = vga_entry_color(1, 15);  // white on blue

    kprint_at("TinyOS Main Menu", 3, 2, color);
    kprint_at("+-----------------+", 5, 2, color);
    kprint_at("|     Notepad     |", 6, 2, color);
    kprint_at("+-----------------+", 7, 2, color);
    kprint_at("Press 'n' to open Notepad", 9, 2, color);

    char c = get_keyboard_char();
    for (;;) {

        if (c == 'n' || c == 'N') {
            notepad();  // when notepad returns, redraw menu
            clear_screen(color);
        } else if (c == 'q' || c == 'Q') {
            break;
        }
    }
}

void kernel_main(void){
    uint8_t color = vga_entry_color(1, 15);
    clear_screen(color);
    kprint_at("Welcome to my TinyOS kernel!", 1, 2, color);

    // Set up IDT
    // idt_init();
    // kprint_at("IDT initialized successfully!", 13, 2, color);
    
    // kprint_at("Triggering INT 0 (divide by zero)...", 14, 2, color);

    // Trigger interrupt 0
    // __asm__ __volatile__("int $0");

    // Should not reach here if ISR halts
    // kprint_at("ERROR: Returned from INT 0!", 15, 2, vga_entry_color(15, 4));

    main_menu();
    
    // for (;;){
    //     __asm__ __volatile__("hlt");
    // }
}