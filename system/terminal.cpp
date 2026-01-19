#include <stdint.h>
#include <stddef.h>

extern uint32_t* fb_ptr;
extern uint64_t fb_width;
extern uint64_t fb_height;
extern uint64_t fb_pitch;
extern bool in_gui_mode;
extern bool boot_complete;
extern bool boot_menu_active;
extern uint32_t cpu_core_count;
extern uint64_t total_memory_kb;
extern uint64_t free_memory_kb;
extern uint64_t uptime_seconds;
extern char cpu_brand_string[];

char current_directory[64] = "/";
extern int mouse_x;
extern int mouse_y;

extern void* memcpy(void *dest, const void *src, size_t n);
extern void* memset(void *s, int c, size_t n);
extern size_t strlen(const char *str);
extern int strcmp(const char *s1, const char *s2);
extern int strncmp(const char *s1, const char *s2, size_t n);
extern char* strcpy(char *dest, const char *src);
extern char* strcat(char *dest, const char *src);
extern void uint_to_str(uint64_t n, char* buffer);
extern void process_command(const char* cmd);
extern void handle_gui_click(int x, int y, bool right_click);
extern void refresh_all_windows();
extern bool is_hlfs_enabled();
extern void get_filesystem_name(char* output);

static uint8_t font8x8_basic[128][8] = {
    {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0},
    {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, 
    {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00}, 
    {0x6C,0x6C,0x00,0x00,0x00,0x00,0x00,0x00}, 
    {0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00}, 
    {0x18,0x7E,0xC0,0x7E,0x06,0x7E,0x18,0x00}, 
    {0x00,0xC6,0xCC,0x18,0x30,0x66,0xC6,0x00}, 
    {0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00}, 
    {0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00}, 
    {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00}, 
    {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00}, 
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, 
    {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00}, 
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30}, 
    {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00}, 
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}, 
    {0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,0x00}, 
    {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00}, 
    {0x18,0x38,0x18,0x18,0x18,0x18,0x3C,0x00}, 
    {0x3C,0x66,0x06,0x0C,0x18,0x30,0x7E,0x00}, 
    {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00}, 
    {0x0C,0x1C,0x2C,0x4C,0x7E,0x0C,0x0C,0x00}, 
    {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00}, 
    {0x1C,0x30,0x60,0x7C,0x66,0x66,0x3C,0x00}, 
    {0x7E,0x06,0x0C,0x18,0x30,0x30,0x30,0x00}, 
    {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00}, 
    {0x3C,0x66,0x66,0x3E,0x06,0x66,0x3C,0x00}, 
    {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00}, 
    {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30}, 
    {0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00}, 
    {0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00}, 
    {0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00}, 
    {0x3C,0x66,0x06,0x0C,0x18,0x00,0x18,0x00}, 
    {0x3C,0x66,0x6E,0x6E,0x6E,0x60,0x3E,0x00}, 
    {0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00}, 
    {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00}, 
    {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00}, 
    {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00}, 
    {0x7E,0x60,0x60,0x78,0x60,0x60,0x7E,0x00}, 
    {0x7E,0x60,0x60,0x78,0x60,0x60,0x60,0x00}, 
    {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3C,0x00}, 
    {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00}, 
    {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, 
    {0x06,0x06,0x06,0x06,0x06,0x66,0x3C,0x00}, 
    {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00}, 
    {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00}, 
    {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00}, 
    {0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00}, 
    {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, 
    {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00}, 
    {0x3C,0x66,0x66,0x66,0x6A,0x6C,0x36,0x00}, 
    {0x7C,0x66,0x66,0x7C,0x6C,0x66,0x66,0x00}, 
    {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00}, 
    {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, 
    {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, 
    {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00}, 
    {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00}, 
    {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00}, 
    {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00}, 
    {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00}, 
    {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00}, 
    {0x00,0x60,0x30,0x18,0x0C,0x06,0x00,0x00}, 
    {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00}, 
    {0x18,0x3C,0x66,0x00,0x00,0x00,0x00,0x00}, 
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF}, 
    {0x18,0x18,0x08,0x00,0x00,0x00,0x00,0x00}, 
    {0x00,0x00,0x3C,0x06,0x3E,0x66,0x3E,0x00}, 
    {0x60,0x60,0x7C,0x66,0x66,0x66,0x7C,0x00}, 
    {0x00,0x00,0x3C,0x60,0x60,0x60,0x3C,0x00}, 
    {0x06,0x06,0x3E,0x66,0x66,0x66,0x3E,0x00}, 
    {0x00,0x00,0x3C,0x66,0x7E,0x60,0x3C,0x00}, 
    {0x1C,0x30,0x78,0x30,0x30,0x30,0x30,0x00}, 
    {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x3C}, 
    {0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x00}, 
    {0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00}, 
    {0x18,0x00,0x18,0x18,0x18,0x18,0x18,0x30}, 
    {0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0x00}, 
    {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, 
    {0x00,0x00,0xec,0xfe,0xd6,0xd6,0xd6,0x00}, 
    {0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x00}, 
    {0x00,0x00,0x3C,0x66,0x66,0x66,0x3C,0x00}, 
    {0x00,0x00,0x7C,0x66,0x66,0x7C,0x60,0x60}, 
    {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x06}, 
    {0x00,0x00,0x7C,0x66,0x60,0x60,0x60,0x00}, 
    {0x00,0x00,0x3E,0x60,0x3C,0x06,0x7C,0x00}, 
    {0x30,0x30,0x78,0x30,0x30,0x30,0x1C,0x00}, 
    {0x00,0x00,0x66,0x66,0x66,0x66,0x3E,0x00}, 
    {0x00,0x00,0x66,0x66,0x66,0x3C,0x18,0x00}, 
    {0x00,0x00,0x63,0x6b,0x6b,0x7f,0x36,0x00}, 
    {0x00,0x00,0x66,0x3C,0x18,0x3C,0x66,0x00}, 
    {0x00,0x00,0x66,0x66,0x66,0x3E,0x06,0x3C}, 
    {0x00,0x00,0x7E,0x0C,0x18,0x30,0x7E,0x00}, 
    {0x0C,0x18,0x18,0x70,0x18,0x18,0x0C,0x00}, 
    {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18}, 
    {0x30,0x18,0x18,0x0E,0x18,0x18,0x30,0x00}, 
    {0x36,0x6C,0x00,0x00,0x00,0x00,0x00,0x00}, 
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00} 
};

void put_pixel(int x, int y, uint32_t color) {
    if(x < 0 || x >= (int)fb_width || y < 0 || y >= (int)fb_height) return;
    fb_ptr[y * (fb_pitch / 4) + x] = color;
}

void draw_rect(int x, int y, int w, int h, uint32_t color) {
    for(int j = 0; j < h; j++) {
        for(int i = 0; i < w; i++) {
            put_pixel(x + i, y + j, color);
        }
    }
}

void draw_rounded_rect(int x, int y, int w, int h, uint32_t color) {
    draw_rect(x+2, y, w-4, h, color);
    draw_rect(x, y+2, w, h-4, color);
    put_pixel(x+1, y+1, color);
    put_pixel(x+w-2, y+1, color);
    put_pixel(x+1, y+h-2, color);
    put_pixel(x+w-2, y+h-2, color);
}

void draw_char(char c, int x, int y, uint32_t color) {
    if(c < 0 || c > 127) return;
    for(int i=0; i<8; i++) {
        for(int j=0; j<8; j++) {
            if((font8x8_basic[(int)c][i] >> (7-j)) & 1) {
                put_pixel(x+j, y+i, color);
            }
        }
    }
}

void draw_string(const char* str, int x, int y, uint32_t color) {
    int offset = 0;
    while(*str) {
        draw_char(*str, x + offset, y, color);
        offset += 8;
        str++;
    }
}

void draw_background_logo() {
    for(int i = 0; i < 50; i++) {
        int x = (i * 31337 + 12345) % fb_width;
        int y = (i * 54321 + 67890) % fb_height;
        int size = (i % 3) + 1;
        
        for(int dy = 0; dy < size; dy++) {
            for(int dx = 0; dx < size; dx++) {
                put_pixel(x + dx, y + dy, 0x1a1a2e);
            }
        }
    }
}

int boot_menu_selection = 0;

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

void draw_modern_boot_menu() {
    for(uint64_t i = 0; i < fb_width * fb_height; i++) {
        fb_ptr[i] = 0x000000;
    }
    
    int center_x = fb_width / 2;
    int center_y = fb_height / 2;
    
    const char* logo = "HALDEN";
    int logo_x = center_x - 60;
    int logo_y = center_y - 100;
    
    for(int i = 0; logo[i]; i++) {
        for(int dy = 0; dy < 24; dy++) {
            for(int dx = 0; dx < 16; dx++) {
                if((font8x8_basic[(int)logo[i]][dy / 3] >> (7 - dx / 2)) & 1) {
                    put_pixel(logo_x + i * 20 + dx, logo_y + dy, 0xFFFFFF);
                }
            }
        }
    }
    
    draw_rect(center_x - 150, logo_y + 50, 300, 2, 0x444444);
    
    const char* options[] = {"Boot HaldenOS", "System Info"};
    int option_y = logo_y + 80;
    
    for(int i = 0; i < 2; i++) {
        uint32_t color = (i == boot_menu_selection) ? 0x00FFFF : 0xAAAAAA;
        if(i == boot_menu_selection) {
            draw_rect(center_x - 160, option_y + i * 40 - 5, 320, 30, 0x1a1a2e);
        }
        draw_string(options[i], center_x - 56, option_y + i * 40, color);
    }
    
    draw_string("Use Arrow Keys and Enter to select", center_x - 136, fb_height - 50, 0x666666);
}

bool check_boot_menu_input() {
    uint8_t status = inb(0x64);
    if(status & 0x01) {
        uint8_t scancode = inb(0x60);
        
        if(scancode == 0x48) {
            if(boot_menu_selection > 0) {
                boot_menu_selection--;
                draw_modern_boot_menu();
            }
        } else if(scancode == 0x50) {
            if(boot_menu_selection < 1) {
                boot_menu_selection++;
                draw_modern_boot_menu();
            }
        } else if(scancode == 0x1C) {
            if(boot_menu_selection == 0) {
                boot_menu_active = false;
                return true;
            } else if(boot_menu_selection == 1) {
                extern void draw_system_info();
                draw_system_info();
                
                while(true) {
                    status = inb(0x64);
                    if(status & 0x01) {
                        scancode = inb(0x60);
                        if(scancode == 0x21) {
                            draw_modern_boot_menu();
                            break;
                        }
                    }
                    for(volatile int d = 0; d < 100000; d++);
                }
            }
        }
    }
    return false;
}

void draw_boot_screen() {
    for(uint64_t i = 0; i < fb_width * fb_height; i++) {
        fb_ptr[i] = 0x000000;
    }
    
    const char* logo = "HALDEN";
    int logo_width = strlen(logo) * 16;
    int logo_x = fb_width / 2 - logo_width / 2;
    int logo_y = fb_height / 2 - 40;
    
    for(int i = 0; logo[i]; i++) {
        for(int dy = 0; dy < 20; dy++) {
            for(int dx = 0; dx < 15; dx++) {
                if((font8x8_basic[(int)logo[i]][dy / 2] >> (7 - dx / 2)) & 1) {
                    put_pixel(logo_x + i * 16 + dx, logo_y + dy, 0x4A9EFF);
                }
            }
        }
    }
}

void update_boot_progress(int progress) {
    int bar_width = 300;
    int bar_height = 3;
    int bar_x = fb_width / 2 - bar_width / 2;
    int bar_y = fb_height / 2 + 20;
    
    draw_rect(bar_x, bar_y, bar_width, bar_height, 0x1A1A1A);
    
    int filled_width = (bar_width * progress) / 100;
    if(filled_width > 0) {
        draw_rect(bar_x, bar_y, filled_width, bar_height, 0x4A9EFF);
    }
}

static int term_x = 0;
static int term_y = 0;
static char terminal_buffer[4096];
static int terminal_buffer_len = 0;

void terminal_init() {
    term_x = 10;
    term_y = 10;
    memset(terminal_buffer, 0, 4096);
    terminal_buffer_len = 0;
}

void terminal_clear() {
    if(in_gui_mode) {
        draw_rect(50, 80, 700, 400, 0x1a1a2e);
        term_x = 60;
        term_y = 90;
    } else {
        for(uint64_t i = 0; i < fb_width * fb_height; i++) fb_ptr[i] = 0x0f0f1e; 
        term_x = 10;
        term_y = 10;
    }
    memset(terminal_buffer, 0, 4096);
    terminal_buffer_len = 0;
}

void terminal_write(const char* str) {
    int max_x = in_gui_mode ? 740 : (int)fb_width - 8;
    int max_y = in_gui_mode ? 470 : (int)fb_height - 10;
    int start_x = in_gui_mode ? 60 : 10;
    
    while(*str && terminal_buffer_len < 4095) {
        terminal_buffer[terminal_buffer_len++] = *str;
        str++;
    }
    terminal_buffer[terminal_buffer_len] = '\0';
    
    term_x = start_x;
    term_y = in_gui_mode ? 90 : 10;
    
    for(int i = 0; i < terminal_buffer_len; i++) {
        if(terminal_buffer[i] == '\n') {
            term_x = start_x;
            term_y += 10;
        } else {
            if(term_x < max_x && term_y < max_y) {
                draw_char(terminal_buffer[i], term_x, term_y, 0xCCCCCC);
            }
            term_x += 8;
            if(term_x >= max_x) {
                term_x = start_x;
                term_y += 10;
            }
        }
    }
}

void terminal_redraw() {
    if(!in_gui_mode) return;
    
    draw_rect(50, 80, 700, 400, 0x1a1a2e);
    
    int max_x = 740;
    int max_y = 470;
    int start_x = 60;
    
    term_x = start_x;
    term_y = 90;
    
    for(int i = 0; i < terminal_buffer_len; i++) {
        if(terminal_buffer[i] == '\n') {
            term_x = start_x;
            term_y += 10;
        } else {
            if(term_x < max_x && term_y < max_y) {
                draw_char(terminal_buffer[i], term_x, term_y, 0xCCCCCC);
            }
            term_x += 8;
            if(term_x >= max_x) {
                term_x = start_x;
                term_y += 10;
            }
        }
    }
}

uint32_t cursor_saved_pixels[16 * 11];
int cursor_saved_x = -1;
int cursor_saved_y = -1;
bool cursor_saved = false;

void save_cursor_area(int x, int y) {
    if(x < 0 || y < 0 || x >= (int)fb_width - 11 || y >= (int)fb_height - 16) return;
    
    cursor_saved_x = x;
    cursor_saved_y = y;
    cursor_saved = true;
    
    int idx = 0;
    for(int dy = 0; dy < 16; dy++) {
        for(int dx = 0; dx < 11; dx++) {
            int px = x + dx;
            int py = y + dy;
            
            if(px < 0 || px >= (int)fb_width || py < 0 || py >= (int)fb_height) {
                cursor_saved_pixels[idx++] = 0x0f0f1e;
            } else {
                cursor_saved_pixels[idx++] = fb_ptr[py * (fb_pitch / 4) + px];
            }
        }
    }
}

void restore_cursor_area() {
    if(!cursor_saved) return;
    if(cursor_saved_x < 0 || cursor_saved_y < 0) return;
    
    int idx = 0;
    for(int dy = 0; dy < 16; dy++) {
        for(int dx = 0; dx < 11; dx++) {
            int px = cursor_saved_x + dx;
            int py = cursor_saved_y + dy;
            
            if(px >= 0 && px < (int)fb_width && py >= 0 && py < (int)fb_height) {
                fb_ptr[py * (fb_pitch / 4) + px] = cursor_saved_pixels[idx];
            }
            idx++;
        }
    }
    
    cursor_saved = false;
}

void draw_cursor(int x, int y) {
    if(x < 0 || y < 0 || x >= (int)fb_width - 11 || y >= (int)fb_height - 16) return;
    
    const int cursor_data[16] = {
        0b1100000000000000,
        0b1110000000000000,
        0b1111000000000000,
        0b1111100000000000,
        0b1111110000000000,
        0b1111111000000000,
        0b1111111100000000,
        0b1111111110000000,
        0b1111111111000000,
        0b1111111000000000,
        0b1110110000000000,
        0b1100011000000000,
        0b1000001100000000,
        0b0000001100000000,
        0b0000000110000000,
        0b0000000000000000
    };
    
    restore_cursor_area();
    save_cursor_area(x, y);
    
    for(int dy = 0; dy < 16; dy++) {
        for(int dx = 0; dx < 11; dx++) {
            int px = x + dx;
            int py = y + dy;
            
            if(px < 0 || px >= (int)fb_width || py < 0 || py >= (int)fb_height) continue;
            
            if((cursor_data[dy] >> (15 - dx)) & 1) {
                if(dx == 0 || dy == 0) {
                    put_pixel(px, py, 0x000000);
                } else {
                    put_pixel(px, py, 0xFFFFFF);
                }
            }
        }
    }
}

char kbd_buffer[128];
int kbd_idx = 0;
static uint8_t mouse_cycle = 0;
static int8_t mouse_byte[3];
static bool mouse_left_btn = false;
static bool mouse_right_btn = false;
static bool shift_pressed = false;
static bool ctrl_pressed = false;

enum KeyboardLayout {
    LAYOUT_QWERTY = 0,
    LAYOUT_TRQ = 1
};

KeyboardLayout current_layout = LAYOUT_QWERTY;

extern void handle_app_keyboard(char c);
extern bool is_app_focused();
extern void update_window_drag(int mouse_x, int mouse_y);
extern void stop_window_drag();

char scancode_to_ascii_qwerty(uint8_t sc, bool shift) {
    if(sc >= 0x80) return 0;
    
    if(shift) {
        const char* map = "\0\033!@#$%^&*()_+\b\tQWERTYUIOP{}\n\0ASDFGHJKL:\"~\0|ZXCVBNM<>?\0*\0 ";
        if(sc < 58) return map[sc];
    } else {
        const char* map = "\0\0331234567890-=\b\tqwertyuiop[]\n\0asdfghjkl;'`\0\\zxcvbnm,./\0*\0 ";
        if(sc < 58) return map[sc];
    }
    
    if(sc == 0x34) return shift ? '>' : '.';
    if(sc == 0x33) return shift ? '<' : ',';
    
    return 0;
}

char scancode_to_ascii_trq(uint8_t sc, bool shift) {
    if(sc >= 0x80) return 0;
    
    if(shift) {
        const char* map = "\0\033!'^+%&/()=?_\b\tQWERTYUIOPGU\n\0ASDFGHJKL:;*\0<ZXCVBNM>|\0*\0 ";
        if(sc < 58) return map[sc];
    } else {
        const char* map = "\0\01234567890*-\b\tqwertyuiopgu\n\0asdfghjkl:;*\0<zxcvbnm>|\0*\0 ";
        if(sc < 58) return map[sc];
    }
    
    if(sc == 0x1A) return shift ? 'G' : 'g';
    if(sc == 0x1B) return shift ? 'U' : 'u';
    if(sc == 0x27) return shift ? 'I' : 'i';
    if(sc == 0x28) return shift ? ';' : ':';
    if(sc == 0x29) return '"';
    if(sc == 0x2B) return shift ? '|' : '<';
    if(sc == 0x33) return shift ? ':' : '.';
    if(sc == 0x34) return shift ? '/' : ',';
    if(sc == 0x35) return shift ? '?' : '.';
    
    return 0;
}

char scancode_to_ascii(uint8_t sc, bool shift) {
    if(current_layout == LAYOUT_TRQ) {
        return scancode_to_ascii_trq(sc, shift);
    }
    return scancode_to_ascii_qwerty(sc, shift);
}

void mouse_handler() {
    if(!boot_complete) return;
    
    uint8_t status = inb(0x64);
    
    if(status & 0x20) {
        uint8_t b = inb(0x60);
        
        if(mouse_cycle == 0) {
            if(!(b & 0x08)) return;
            mouse_byte[0] = b;
            mouse_cycle = 1;
        } else if(mouse_cycle == 1) {
            mouse_byte[1] = b;
            mouse_cycle = 2;
        } else if(mouse_cycle == 2) {
            mouse_byte[2] = b;
            mouse_cycle = 0;
            
            if(in_gui_mode) {
                int8_t dx = mouse_byte[1];
                int8_t dy = -mouse_byte[2];
                
                if(dx < -50) dx = -50;
                if(dx > 50) dx = 50;
                if(dy < -50) dy = -50;
                if(dy > 50) dy = 50;
                
                int new_x = mouse_x + dx;
                int new_y = mouse_y + dy;
                
                if(new_x < 0) new_x = 0;
                if(new_x >= (int)fb_width - 11) new_x = (int)fb_width - 11;
                if(new_y < 0) new_y = 0;
                if(new_y >= (int)fb_height - 16) new_y = (int)fb_height - 16;
                
                bool left_btn = mouse_byte[0] & 0x01;
                bool right_btn = mouse_byte[0] & 0x02;
                
                if(mouse_x != new_x || mouse_y != new_y) {
                    mouse_x = new_x;
                    mouse_y = new_y;
                    
                    if(left_btn && mouse_left_btn) {
                        update_window_drag(mouse_x, mouse_y);
                    } else {
                        draw_cursor(mouse_x, mouse_y);
                    }
                }
                
                if(left_btn && !mouse_left_btn) {
                    handle_gui_click(mouse_x, mouse_y, false);
                }
                if(!left_btn && mouse_left_btn) {
                    stop_window_drag();
                }
                if(right_btn && !mouse_right_btn) {
                    handle_gui_click(mouse_x, mouse_y, true);
                }
                
                mouse_left_btn = left_btn;
                mouse_right_btn = right_btn;
            }
        }
    } else {
        if(status & 0x01) {
            uint8_t b = inb(0x60);
            
            if(b == 0x2A || b == 0x36) {
                shift_pressed = true;
            } else if(b == 0xAA || b == 0xB6) {
                shift_pressed = false;
            } else if(b == 0x1D) {
                ctrl_pressed = true;
            } else if(b == 0x9D) {
                ctrl_pressed = false;
            }
            
            if(b < 0x80) {
                char c = scancode_to_ascii(b, shift_pressed);
                
                if(in_gui_mode && is_app_focused()) {
                    handle_app_keyboard(c);
                    return;
                }
                
                if(c == '\b') {
                    if(kbd_idx > 0) {
                        kbd_idx--;
                        kbd_buffer[kbd_idx] = 0;
                        if(terminal_buffer_len > 0) terminal_buffer_len--;
                        terminal_buffer[terminal_buffer_len] = 0;
                        
                        if(in_gui_mode) {
                            draw_rect(50, 80, 700, 400, 0x1a1a2e);
                        }
                        
                        term_x = in_gui_mode ? 60 : 10;
                        term_y = in_gui_mode ? 90 : 10;
                        
                        for(int i = 0; i < terminal_buffer_len; i++) {
                            if(terminal_buffer[i] == '\n') {
                                term_x = in_gui_mode ? 60 : 10;
                                term_y += 10;
                            } else {
                                draw_char(terminal_buffer[i], term_x, term_y, 0xCCCCCC);
                                term_x += 8;
                            }
                        }
                    }
                } else if(c == '\n') {
                    if(terminal_buffer_len < 4095) {
                        terminal_buffer[terminal_buffer_len++] = '\n';
                        terminal_buffer[terminal_buffer_len] = '\0';
                    }
                    process_command(kbd_buffer);
                    kbd_idx = 0;
                    memset(kbd_buffer, 0, 128);
                    
                    terminal_write("root@halden:");
                    terminal_write(current_directory);
                    terminal_write("# ");
                } else if(c) {
                    if(kbd_idx < 127) {
                        kbd_buffer[kbd_idx++] = c;
                        char tmp[2] = {c, 0};
                        terminal_write(tmp);
                    }
                }
            }
        }
    }
}

static void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if(type == 0) {
        while(timeout--) if((inb(0x64) & 1) == 1) return;
    } else {
        while(timeout--) if((inb(0x64) & 2) == 0) return;
    }
}

static void mouse_write(uint8_t write) {
    mouse_wait(1);
    outb(0x64, 0xD4);
    mouse_wait(1);
    outb(0x60, write);
}

static uint8_t mouse_read() {
    mouse_wait(0);
    return inb(0x60);
}

void mouse_init() {
    uint8_t status;
    mouse_wait(1);
    outb(0x64, 0xA8);
    mouse_wait(1);
    outb(0x64, 0x20);
    mouse_wait(0);
    status = (inb(0x60) | 2);
    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, status);
    mouse_write(0xF6);
    mouse_read();
    mouse_write(0xF4);
    mouse_read();
}

void draw_system_info() {
    for(uint64_t i = 0; i < fb_width * fb_height; i++) fb_ptr[i] = 0x0a0a0a;
    
    int start_y = 60;
    int start_x = 100;
    
    draw_string("SYSTEM INFORMATION", fb_width / 2 - 72, start_y, 0x00FFFF);
    draw_rect(start_x, start_y + 20, fb_width - 200, 2, 0x444444);
    
    start_y += 40;
    
    draw_string("Filesystem:", start_x, start_y, 0xFFFFFF);
    start_y += 20;
    char fs_name[64];
    get_filesystem_name(fs_name);
    draw_string("  Type:         ", start_x + 20, start_y, 0xCCCCCC);
    draw_string(fs_name, start_x + 140, start_y, 0xCCCCCC);
    start_y += 15;
    draw_string("  Status:       ", start_x + 20, start_y, 0xCCCCCC);
    if(is_hlfs_enabled()) {
        draw_string("Active", start_x + 140, start_y, 0x00FF00);
    } else {
        draw_string("Inactive", start_x + 140, start_y, 0xFF0000);
    }
    
    start_y += 25;
    draw_string("CPU Information:", start_x, start_y, 0xFFFFFF);
    start_y += 20;
    
    uint32_t eax, ebx, ecx, edx;
    char cpu_vendor[13];
    
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
    *((uint32_t*)(cpu_vendor + 0)) = ebx;
    *((uint32_t*)(cpu_vendor + 4)) = edx;
    *((uint32_t*)(cpu_vendor + 8)) = ecx;
    cpu_vendor[12] = '\0';
    
    draw_string("  Vendor:       ", start_x + 20, start_y, 0xCCCCCC);
    draw_string(cpu_vendor, start_x + 140, start_y, 0xCCCCCC);
    start_y += 15;
    draw_string("  Model:        ", start_x + 20, start_y, 0xCCCCCC);
    draw_string(cpu_brand_string, start_x + 140, start_y, 0xCCCCCC);
    start_y += 15;
    draw_string("  Cores:        ", start_x + 20, start_y, 0xCCCCCC);
    char cores_str[16];
    uint_to_str(cpu_core_count, cores_str);
    draw_string(cores_str, start_x + 140, start_y, 0xCCCCCC);
    start_y += 15;
    draw_string("  Architecture: x86_64", start_x + 20, start_y, 0xCCCCCC);
    
    start_y += 25;
    draw_string("Display Information:", start_x, start_y, 0xFFFFFF);
    start_y += 20;
    draw_string("  Framebuffer:  ", start_x + 20, start_y, 0xCCCCCC);
    char res_str[32];
    uint_to_str(fb_width, res_str);
    strcat(res_str, "x");
    char h_str[16];
    uint_to_str(fb_height, h_str);
    strcat(res_str, h_str);
    draw_string(res_str, start_x + 140, start_y, 0xCCCCCC);
    start_y += 15;
    draw_string("  Color Depth:  32-bit", start_x + 20, start_y, 0xCCCCCC);
    start_y += 15;
    draw_string("  Pitch:        ", start_x + 20, start_y, 0xCCCCCC);
    uint_to_str(fb_pitch, res_str);
    strcat(res_str, " bytes");
    draw_string(res_str, start_x + 140, start_y, 0xCCCCCC);
    
    start_y += 25;
    draw_string("Memory Information:", start_x, start_y, 0xFFFFFF);
    start_y += 20;
    draw_string("  Total RAM:    ", start_x + 20, start_y, 0xCCCCCC);
    char ram_str[32];
    uint_to_str(total_memory_kb / 1024, ram_str);
    strcat(ram_str, " MB");
    draw_string(ram_str, start_x + 140, start_y, 0xCCCCCC);
    start_y += 15;
    draw_string("  Free RAM:     ", start_x + 20, start_y, 0xCCCCCC);
    uint_to_str(free_memory_kb / 1024, ram_str);
    strcat(ram_str, " MB");
    draw_string(ram_str, start_x + 140, start_y, 0xCCCCCC);
    start_y += 15;
    draw_string("  Used RAM:     ", start_x + 20, start_y, 0xCCCCCC);
    uint_to_str((total_memory_kb - free_memory_kb) / 1024, ram_str);
    strcat(ram_str, " MB");
    draw_string(ram_str, start_x + 140, start_y, 0xCCCCCC);
    
    start_y += 25;
    draw_string("System:", start_x, start_y, 0xFFFFFF);
    start_y += 20;
    draw_string("  OS:           HaldenOS 1.0.0", start_x + 20, start_y, 0xCCCCCC);
    start_y += 15;
    draw_string("  Kernel:       1.0.0-halden", start_x + 20, start_y, 0xCCCCCC);
    start_y += 15;
    draw_string("  Architecture: x86_64", start_x + 20, start_y, 0xCCCCCC);
    start_y += 15;
    draw_string("  Bootloader:   Limine", start_x + 20, start_y, 0xCCCCCC);
    
    draw_string("Press F to return to boot menu", fb_width / 2 - 120, fb_height - 40, 0xFFFF00);
}