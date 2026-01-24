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


void kernel_main(void){
    uint8_t color = vga_entry_color(15, 1);
    clear_screen(color);
    kprint_at("Welcome to my TinyOS kernel!", 12, 2, color);

    // halt the cpu
    for (;;){
        __asm__ __volatile__("hlt");
    }
}
