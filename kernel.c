#include <stdint.h>

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
void isr0_c(void){
    uint8_t color = vga_entry_color(15, 4); // white on red
    kprint_at(">>> INT 0 FIRED! <<<", 16, 2, color);
    // Halt after handling
    for(;;) __asm__ __volatile__("hlt");
}

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

void kernel_main(void){
    uint8_t color = vga_entry_color(15, 1);
    clear_screen(color);
    kprint_at("Welcome to my TinyOS kernel!", 12, 2, color);

    // Set up IDT
    idt_init();
    kprint_at("IDT initialized successfully!", 13, 2, color);
    
    kprint_at("Triggering INT 0 (divide by zero)...", 14, 2, color);

    // Trigger interrupt 0
    __asm__ __volatile__("int $0");

    // Should not reach here if ISR halts
    kprint_at("ERROR: Returned from INT 0!", 15, 2, vga_entry_color(15, 4));
    
    for (;;){
        __asm__ __volatile__("hlt");
    }
}