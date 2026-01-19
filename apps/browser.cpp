#include <stdint.h>
#include <stddef.h>

extern uint32_t* fb_ptr;
extern uint64_t fb_width;
extern uint64_t fb_height;
extern int mouse_x;
extern int mouse_y;

extern void draw_rect(int x, int y, int w, int h, uint32_t color);
extern void draw_rounded_rect(int x, int y, int w, int h, uint32_t color);
extern void draw_char(char c, int x, int y, uint32_t color);
extern void draw_string(const char* str, int x, int y, uint32_t color);
extern void* memset(void *s, int c, size_t n);
extern size_t strlen(const char *str);
extern int strcmp(const char *s1, const char *s2);
extern int strncmp(const char *s1, const char *s2, size_t n);
extern char* strcpy(char *dest, const char *src);
extern char* strcat(char *dest, const char *src);
extern void refresh_all_windows();

extern bool http_get(const char* url, char* title_out, char* content_out);
extern bool get_network_status();

struct BrowserWindow {
    int x, y;
    int w, h;
    bool minimized;
    bool maximized;
};

BrowserWindow browser_win = {80, 60, 720, 480, false, false};
bool browser_open = false;

char address_bar[128] = "halden.os";
char page_title[64] = "HaldenOS Browser";
char page_content[4096] = "";
int scroll_offset = 0;
bool address_bar_focused = false;

uint32_t hex_to_color(const char* hex) {
    uint32_t color = 0;
    int i = (hex[0] == '#') ? 1 : 0;
    for(int j = 0; j < 6 && hex[i]; j++, i++) {
        char c = hex[i];
        uint8_t val = 0;
        if(c >= '0' && c <= '9') val = c - '0';
        else if(c >= 'a' && c <= 'f') val = c - 'a' + 10;
        else if(c >= 'A' && c <= 'F') val = c - 'A' + 10;
        color = (color << 4) | val;
    }
    return color;
}

void find_tag_content(const char* html, const char* tag, char* output, int max_len) {
    char open_tag[32];
    char close_tag[32];
    strcpy(open_tag, "<");
    strcat(open_tag, tag);
    
    strcpy(close_tag, "</");
    strcat(close_tag, tag);
    strcat(close_tag, ">");
    
    const char* start = html;
    while(*start) {
        bool match = true;
        for(int i = 0; open_tag[i]; i++) {
            if(start[i] != open_tag[i]) {
                match = false;
                break;
            }
        }
        if(match) {
            while(*start && *start != '>') start++;
            if(*start == '>') start++;
            
            const char* end = start;
            while(*end) {
                bool close_match = true;
                for(int i = 0; close_tag[i]; i++) {
                    if(end[i] != close_tag[i]) {
                        close_match = false;
                        break;
                    }
                }
                if(close_match) {
                    int len = end - start;
                    if(len > max_len - 1) len = max_len - 1;
                    for(int i = 0; i < len; i++) {
                        output[i] = start[i];
                    }
                    output[len] = '\0';
                    return;
                }
                end++;
            }
        }
        start++;
    }
    output[0] = '\0';
}

void find_style_property(const char* styles, const char* prop, char* value, int max_len) {
    const char* p = styles;
    while(*p) {
        bool match = true;
        for(int i = 0; prop[i]; i++) {
            if(p[i] != prop[i]) {
                match = false;
                break;
            }
        }
        if(match && p[strlen(prop)] == ':') {
            p += strlen(prop) + 1;
            while(*p == ' ') p++;
            
            int i = 0;
            while(*p && *p != ';' && *p != '}' && i < max_len - 1) {
                value[i++] = *p++;
            }
            value[i] = '\0';
            return;
        }
        p++;
    }
    value[0] = '\0';
}

void render_html(const char* html, int x, int y, int max_w, int max_h) {
    char body[3072];
    char style[1024];
    char bg_color[16];
    char text_color[16];
    
    find_tag_content(html, "body", body, 3072);
    find_tag_content(html, "style", style, 1024);
    
    if(strlen(body) == 0) {
        strcpy(body, html);
    }
    
    uint32_t bg = 0x2a2a3e;
    uint32_t fg = 0xCCCCCC;
    
    if(strlen(style) > 0) {
        find_style_property(style, "background", bg_color, 16);
        if(strlen(bg_color) > 0 && bg_color[0] == '#') {
            bg = hex_to_color(bg_color);
        }
        
        find_style_property(style, "color", text_color, 16);
        if(strlen(text_color) > 0 && text_color[0] == '#') {
            fg = hex_to_color(text_color);
        }
    }
    
    draw_rect(x, y, max_w, max_h, bg);
    
    int cx = x;
    int cy = y - scroll_offset;
    int max_x = x + max_w;
    int max_y = y + max_h;
    
    const char* p = body;
    bool in_tag = false;
    bool in_h1 = false;
    bool in_h2 = false;
    bool in_link = false;
    bool in_button = false;
    char tag_name[16];
    int tag_idx = 0;
    
    while(*p) {
        if(*p == '<') {
            in_tag = true;
            tag_idx = 0;
            p++;
            
            bool is_closing = (*p == '/');
            if(is_closing) p++;
            
            while(*p && *p != '>' && *p != ' ' && tag_idx < 15) {
                tag_name[tag_idx++] = *p++;
            }
            tag_name[tag_idx] = '\0';
            
            if(strcmp(tag_name, "h1") == 0) {
                if(!is_closing) {
                    in_h1 = true;
                    if(cx > x) { cx = x; cy += 12; }
                    cy += 8;
                } else {
                    in_h1 = false;
                    cx = x;
                    cy += 16;
                }
            } else if(strcmp(tag_name, "h2") == 0) {
                if(!is_closing) {
                    in_h2 = true;
                    if(cx > x) { cx = x; cy += 12; }
                    cy += 6;
                } else {
                    in_h2 = false;
                    cx = x;
                    cy += 14;
                }
            } else if(strcmp(tag_name, "p") == 0 || strcmp(tag_name, "li") == 0) {
                if(!is_closing) {
                    if(cx > x) { cx = x; cy += 12; }
                    if(strcmp(tag_name, "li") == 0) {
                        if(cy >= y && cy < max_y) {
                            draw_string("- ", cx, cy, fg);
                        }
                        cx += 16;
                    }
                } else {
                    cx = x;
                    cy += 14;
                }
            } else if(strcmp(tag_name, "a") == 0) {
                in_link = !is_closing;
            } else if(strcmp(tag_name, "button") == 0) {
                if(!is_closing) {
                    in_button = true;
                    if(cx > x) { cx = x; cy += 12; }
                    cy += 4;
                } else {
                    in_button = false;
                    cx = x;
                    cy += 18;
                }
            } else if(strcmp(tag_name, "br") == 0) {
                cx = x;
                cy += 12;
            } else if(strcmp(tag_name, "ul") == 0 || strcmp(tag_name, "ol") == 0) {
                if(!is_closing) {
                    cx = x;
                    cy += 8;
                } else {
                    cx = x;
                    cy += 8;
                }
            }
            
            while(*p && *p != '>') p++;
            if(*p == '>') p++;
            continue;
        }
        
        if(*p == '\n' || *p == '\r') {
            p++;
            continue;
        }
        
        if(*p == ' ' && (p[1] == ' ' || p[1] == '\n' || p[1] == '<')) {
            p++;
            continue;
        }
        
        if(cy >= y && cy < max_y) {
            uint32_t color = fg;
            if(in_h1) color = 0x4A9EFF;
            else if(in_h2) color = 0x6AB4FF;
            else if(in_link) color = 0x4A9EFF;
            else if(in_button) color = 0xFFFFFF;
            
            if(in_button && cx == x + 4) {
                draw_rounded_rect(cx - 2, cy - 2, 120, 16, 0x238636);
            }
            
            draw_char(*p, cx, cy, color);
        }
        
        cx += 8;
        if(cx >= max_x - 8) {
            cx = x;
            cy += 12;
        }
        
        p++;
    }
}

void init_browser_app() {
    browser_open = false;
}

void open_browser() {
    browser_open = true;
    browser_win.minimized = false;
    strcpy(address_bar, "halden.os");
    http_get(address_bar, page_title, page_content);
}

bool is_browser_open() {
    return browser_open;
}

bool is_browser_minimized() {
    return browser_win.minimized;
}

void set_browser_minimized(bool state) {
    browser_win.minimized = state;
}

void draw_browser_window() {
    if(browser_win.minimized) return;
    
    int x = browser_win.maximized ? 0 : browser_win.x;
    int y = browser_win.maximized ? 0 : browser_win.y;
    int w = browser_win.maximized ? (int)fb_width : browser_win.w;
    int h = browser_win.maximized ? (int)fb_height - 50 : browser_win.h;
    
    draw_rect(x, y, w, 35, 0x1a1a2e);
    draw_string("Browser", x + 10, y + 12, 0xFFFFFF);
    
    draw_string("-", x + w - 70, y + 12, 0xFFFF00);
    draw_string("[]", x + w - 50, y + 12, 0x00FF00);
    draw_string("X", x + w - 20, y + 12, 0xFF0000);
    
    draw_rect(x, y + 35, w, h - 35, 0x2a2a3e);
    
    draw_rect(x + 10, y + 45, w - 20, 30, address_bar_focused ? 0x3A5F8F : 0x1a1a2e);
    draw_string(address_bar, x + 15, y + 55, 0xFFFFFF);
    
    if(address_bar_focused) {
        int cursor_x = x + 15 + (int)strlen(address_bar) * 8;
        draw_rect(cursor_x, y + 55, 2, 10, 0xFFFFFF);
    }
    
    draw_rounded_rect(x + w - 90, y + 45, 80, 30, 0x0f3460);
    draw_string("Go", x + w - 65, y + 55, 0xFFFFFF);
    
    draw_rect(x + 10, y + 85, w - 20, 2, 0x444444);
    
    render_html(page_content, x + 10, y + 95, w - 20, h - 130);
    
    if(!get_network_status()) {
        draw_rect(x + 10, y + h - 35, w - 20, 25, 0x5A1A1A);
        draw_string("Network: Disconnected", x + 20, y + h - 25, 0xFF6666);
    } else {
        draw_rect(x + 10, y + h - 35, w - 20, 25, 0x1A3A1A);
        draw_string("Network: Connected", x + 20, y + h - 25, 0x66FF66);
    }
}

void navigate_to_url() {
    http_get(address_bar, page_title, page_content);
    scroll_offset = 0;
    address_bar_focused = false;
    refresh_all_windows();
}

void handle_browser_click(int x, int y) {
    int wx = browser_win.maximized ? 0 : browser_win.x;
    int wy = browser_win.maximized ? 0 : browser_win.y;
    int ww = browser_win.maximized ? (int)fb_width : browser_win.w;
    int wh = browser_win.maximized ? (int)fb_height - 50 : browser_win.h;
    
    if(y >= wy && y <= wy + 35) {
        if(x >= wx + ww - 70 && x <= wx + ww - 60) {
            browser_win.minimized = true;
            refresh_all_windows();
            return;
        } else if(x >= wx + ww - 50 && x <= wx + ww - 30) {
            browser_win.maximized = !browser_win.maximized;
            refresh_all_windows();
            return;
        } else if(x >= wx + ww - 20 && x <= wx + ww - 5) {
            browser_open = false;
            refresh_all_windows();
            return;
        }
    }
    
    if(y >= wy + 45 && y <= wy + 75) {
        if(x >= wx + 10 && x <= wx + ww - 100) {
            address_bar_focused = true;
            refresh_all_windows();
            return;
        } else if(x >= wx + ww - 90 && x <= wx + ww - 10) {
            navigate_to_url();
            return;
        }
    } else {
        if(address_bar_focused) {
            address_bar_focused = false;
            refresh_all_windows();
        }
    }
}

void handle_browser_keyboard(char c) {
    if(!address_bar_focused) return;
    
    int len = (int)strlen(address_bar);
    
    if(c == '\b') {
        if(len > 0) {
            address_bar[len - 1] = '\0';
            refresh_all_windows();
        }
    } else if(c == '\n') {
        navigate_to_url();
    } else if(c >= 32 && c < 127 && len < 126) {
        address_bar[len] = c;
        address_bar[len + 1] = '\0';
        refresh_all_windows();
    }
}