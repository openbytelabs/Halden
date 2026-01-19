#include <stdint.h>
#include <stddef.h>

extern uint32_t* fb_ptr;
extern uint64_t fb_width;
extern uint64_t fb_height;

extern void draw_rect(int x, int y, int w, int h, uint32_t color);
extern void draw_rounded_rect(int x, int y, int w, int h, uint32_t color);
extern void draw_string(const char* str, int x, int y, uint32_t color);
extern void terminal_clear();
extern void terminal_write(const char* str);
extern void refresh_all_windows();
extern char current_directory[64];

struct TerminalWindow {
    int x, y;
    int w, h;
    bool minimized;
    bool maximized;
};

TerminalWindow term_win = {50, 50, 700, 430, false, false};
bool terminal_open = false;

void init_terminal_app() {
    terminal_open = false;
}

void open_terminal() {
    terminal_open = true;
    term_win.minimized = false;
    terminal_clear();
    terminal_write("root@halden:");
    terminal_write(current_directory);
    terminal_write("# ");
}

bool is_terminal_open() {
    return terminal_open;
}

bool is_terminal_minimized() {
    return term_win.minimized;
}

void set_terminal_minimized(bool state) {
    term_win.minimized = state;
}

void draw_terminal_app() {
    if(term_win.minimized) return;
    
    int x = term_win.maximized ? 0 : term_win.x;
    int y = term_win.maximized ? 0 : term_win.y;
    int w = term_win.maximized ? (int)fb_width : term_win.w;
    int h = term_win.maximized ? (int)fb_height - 50 : term_win.h;
    
    draw_rect(x, y, w, 35, 0x1a1a2e);
    draw_string("Terminal", x + 10, y + 12, 0xFFFFFF);
    
    draw_string("-", x + w - 70, y + 12, 0xFFFF00);
    draw_string("[]", x + w - 50, y + 12, 0x00FF00);
    draw_string("X", x + w - 20, y + 12, 0xFF0000);
    
    draw_rect(x, y + 35, w, h - 35, 0x1a1a2e);
}

void handle_terminal_click(int x, int y) {
    int wx = term_win.maximized ? 0 : term_win.x;
    int wy = term_win.maximized ? 0 : term_win.y;
    int ww = term_win.maximized ? (int)fb_width : term_win.w;
    
    if(y >= wy && y <= wy + 35) {
        if(x >= wx + ww - 70 && x <= wx + ww - 60) {
            term_win.minimized = true;
            refresh_all_windows();
        } else if(x >= wx + ww - 50 && x <= wx + ww - 30) {
            term_win.maximized = !term_win.maximized;
            refresh_all_windows();
        } else if(x >= wx + ww - 20 && x <= wx + ww - 5) {
            terminal_open = false;
            refresh_all_windows();
        }
    }
}