#include <stdint.h>
#include <stddef.h>

extern uint32_t* fb_ptr;
extern uint64_t fb_width;
extern uint64_t fb_height;
extern bool in_gui_mode;
extern int mouse_x;
extern int mouse_y;
extern char current_directory[64];

extern void put_pixel(int x, int y, uint32_t color);
extern void draw_rect(int x, int y, int w, int h, uint32_t color);
extern void draw_rounded_rect(int x, int y, int w, int h, uint32_t color);
extern void draw_char(char c, int x, int y, uint32_t color);
extern void draw_string(const char* str, int x, int y, uint32_t color);
extern void terminal_clear();
extern void terminal_write(const char* str);
extern void terminal_redraw();
extern void draw_cursor(int x, int y);
extern void draw_background_logo();
extern void* memset(void *s, int c, size_t n);
extern size_t strlen(const char *str);
extern int strcmp(const char *s1, const char *s2);
extern char* strcpy(char *dest, const char *src);
extern char* strcat(char *dest, const char *src);

void refresh_all_windows();
void handle_click(int x, int y, bool right_click);

extern void draw_browser_window();
extern void handle_browser_click(int x, int y);
extern void handle_browser_keyboard(char c);
extern bool is_browser_open();
extern bool is_browser_minimized();
extern void set_browser_minimized(bool state);
extern void open_browser();
extern struct BrowserWindow {
    int x, y, w, h;
    bool minimized;
    bool maximized;
} browser_win;

extern void draw_filemanager_window();
extern bool is_filemanager_open();
extern bool is_filemanager_minimized();
extern void set_filemanager_minimized(bool state);
extern void handle_filemanager_click(int x, int y);
extern void handle_filemanager_rightclick(int x, int y);
extern void handle_filemanager_keyboard(char c);
extern void open_filemanager();
extern bool viewer_open;
extern void draw_viewer_window();
extern struct FileManagerWindow {
    int x, y, w, h;
    bool minimized;
} fm_win;

extern void draw_terminal_app();
extern bool is_terminal_open();
extern bool is_terminal_minimized();
extern void set_terminal_minimized(bool state);
extern void handle_terminal_click(int x, int y);
extern void open_terminal();
extern struct TerminalWindow {
    int x, y, w, h;
    bool minimized;
    bool maximized;
} term_win;

extern bool get_network_status();
extern void get_network_interface_name(char* output);
extern int get_network_type();
extern int get_wifi_network_count();
extern void get_wifi_network(int index, char* ssid_out, uint8_t* signal_out, bool* secured_out);
extern void connect_to_wifi(int index, const char* password);

extern bool browser_open;
extern bool terminal_open;
extern bool filemanager_open;

struct InstalledApp {
    char name[32];
    int type;
    bool active;
};

#define MAX_INSTALLED_APPS 16
InstalledApp installed_apps[MAX_INSTALLED_APPS];
int installed_app_count = 0;

void add_installed_app(const char* name, int app_type) {
    if(installed_app_count >= MAX_INSTALLED_APPS) return;
    
    for(int i = 0; i < installed_app_count; i++) {
        if(strcmp(installed_apps[i].name, name) == 0) {
            return;
        }
    }
    
    strcpy(installed_apps[installed_app_count].name, name);
    installed_apps[installed_app_count].type = app_type;
    installed_apps[installed_app_count].active = false;
    installed_app_count++;
}

enum AppID {
    APP_NONE = 0,
    APP_TERMINAL = 1,
    APP_BROWSER = 2,
    APP_FILEMANAGER = 3
};

AppID focused_app = APP_NONE;

enum KeyboardLayout {
    LAYOUT_QWERTY = 0,
    LAYOUT_TRQ = 1
};

extern KeyboardLayout current_layout;

void close_terminal() {
    terminal_open = false;
    focused_app = APP_NONE;
}

void close_browser() {
    browser_open = false;
    focused_app = APP_NONE;
}

void close_filemanager() {
    filemanager_open = false;
    focused_app = APP_NONE;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ( "inw %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ( "outw %0, %1" : : "a"(val), "Nd"(port) );
}

void system_shutdown() {
    outw(0x604, 0x2000);
    outw(0xB004, 0x2000);
    outb(0x4004, 0x3400);
    
    const char* s5 = "Shutdown";
    for(int i = 0; s5[i]; i++) {
        outb(0xB2, s5[i]);
    }
    
    outw(0x1000, 0x2000);
    outb(0xf4, 0x00);
    
    while(1) {
        __asm__ volatile("cli; hlt");
    }
}

void system_reboot() {
    __asm__ volatile("cli");
    
    uint8_t temp;
    do {
        temp = inw(0x64);
        if(temp & 0x01) {
            inw(0x60);
        }
    } while(temp & 0x02);
    
    outb(0x64, 0xFE);
    
    while(1) {
        __asm__ volatile("hlt");
    }
}

bool app_menu_open = false;
bool power_menu_open = false;
bool network_menu_open = false;
bool settings_menu_open = false;
bool wifi_password_prompt = false;
int selected_wifi_network = -1;
char wifi_password_input[64] = "";
int wifi_password_len = 0;

enum DraggingApp {
    DRAG_NONE = 0,
    DRAG_TERMINAL = 1,
    DRAG_BROWSER = 2,
    DRAG_FILEMANAGER = 3
};

DraggingApp current_drag = DRAG_NONE;
int drag_offset_x = 0;
int drag_offset_y = 0;

void update_window_drag(int x, int y) {
    if(current_drag == DRAG_TERMINAL && !term_win.maximized) {
        term_win.x = x - drag_offset_x;
        term_win.y = y - drag_offset_y;
        if(term_win.x < 0) term_win.x = 0;
        if(term_win.y < 0) term_win.y = 0;
        if(term_win.x + term_win.w > (int)fb_width) term_win.x = fb_width - term_win.w;
        if(term_win.y + term_win.h > (int)fb_height - 50) term_win.y = fb_height - 50 - term_win.h;
        refresh_all_windows();
    } else if(current_drag == DRAG_BROWSER && !browser_win.maximized) {
        browser_win.x = x - drag_offset_x;
        browser_win.y = y - drag_offset_y;
        if(browser_win.x < 0) browser_win.x = 0;
        if(browser_win.y < 0) browser_win.y = 0;
        if(browser_win.x + browser_win.w > (int)fb_width) browser_win.x = fb_width - browser_win.w;
        if(browser_win.y + browser_win.h > (int)fb_height - 50) browser_win.y = fb_height - 50 - browser_win.h;
        refresh_all_windows();
    } else if(current_drag == DRAG_FILEMANAGER) {
        fm_win.x = x - drag_offset_x;
        fm_win.y = y - drag_offset_y;
        if(fm_win.x < 0) fm_win.x = 0;
        if(fm_win.y < 0) fm_win.y = 0;
        if(fm_win.x + fm_win.w > (int)fb_width) fm_win.x = fb_width - fm_win.w;
        if(fm_win.y + fm_win.h > (int)fb_height - 50) fm_win.y = fb_height - 50 - fm_win.h;
        refresh_all_windows();
    }
}

void stop_window_drag() {
    current_drag = DRAG_NONE;
}

void handle_gui_click(int x, int y, bool right_click) {
    handle_click(x, y, right_click);
}


void draw_taskbar() {
    draw_rect(0, fb_height - 50, fb_width, 50, 0x1a1a2e);
    
    draw_rounded_rect(10, fb_height - 42, 90, 34, power_menu_open ? 0x2A3F5F : 0x0f3460);
    draw_string("HaldenOS", 20, fb_height - 30, 0xFFFFFF);
    
    draw_rounded_rect(110, fb_height - 42, 80, 34, app_menu_open ? 0x2A3F5F : 0x0f3460);
    draw_string("Apps", 130, fb_height - 30, 0xFFFFFF);
    
    int task_x = 210;
    if(is_terminal_open()) {
        draw_rounded_rect(task_x, fb_height - 42, 100, 34, is_terminal_minimized() ? 0x0f3460 : 0x2A3F5F);
        draw_string("Terminal", task_x + 15, fb_height - 30, 0xFFFFFF);
        task_x += 110;
    }
    if(is_browser_open()) {
        draw_rounded_rect(task_x, fb_height - 42, 100, 34, is_browser_minimized() ? 0x0f3460 : 0x2A3F5F);
        draw_string("Browser", task_x + 15, fb_height - 30, 0xFFFFFF);
        task_x += 110;
    }
    if(is_filemanager_open()) {
        draw_rounded_rect(task_x, fb_height - 42, 100, 34, is_filemanager_minimized() ? 0x0f3460 : 0x2A3F5F);
        draw_string("Files", task_x + 20, fb_height - 30, 0xFFFFFF);
        task_x += 110;
    }
    
    for(int i = 0; i < installed_app_count; i++) {
        if(installed_apps[i].active) {
            draw_rounded_rect(task_x, fb_height - 42, 100, 34, 0x2A3F5F);
            int name_len = strlen(installed_apps[i].name);
            int center_offset = (100 - name_len * 8) / 2;
            draw_string(installed_apps[i].name, task_x + center_offset, fb_height - 30, 0xFFFFFF);
            task_x += 110;
        }
    }
    
    char net_name[32];
    get_network_interface_name(net_name);
    int net_type = get_network_type();
    
    int net_box_width = 140;
    int net_x = fb_width - net_box_width - 20;
    
    draw_rounded_rect(net_x, fb_height - 42, net_box_width, 34, network_menu_open ? 0x2A3F5F : 0x0f3460);
    
    if(get_network_status()) {
        draw_rect(net_x + 10, fb_height - 35, 8, 8, 0x00FF00);
        
        if(net_type == 1) {
            draw_string("Ethernet", net_x + 25, fb_height - 30, 0xFFFFFF);
        } else if(net_type == 2) {
            draw_string("WiFi", net_x + 25, fb_height - 30, 0xFFFFFF);
        }
    } else {
        draw_rect(net_x + 10, fb_height - 35, 8, 8, 0xFF0000);
        draw_string("No Network", net_x + 25, fb_height - 30, 0x888888);
    }
}

void draw_network_menu() {
    int menu_w = 280;
    int menu_h = 350;
    int menu_x = fb_width - menu_w - 20;
    int menu_y = fb_height - 50 - menu_h - 10;
    
    draw_rounded_rect(menu_x, menu_y, menu_w, menu_h, 0x16213e);
    draw_rect(menu_x, menu_y, menu_w, 40, 0x1a1a2e);
    draw_string("Network", menu_x + 15, menu_y + 15, 0xFFFFFF);
    
    char iface[32];
    get_network_interface_name(iface);
    int net_type = get_network_type();
    
    draw_string("Interface:", menu_x + 15, menu_y + 50, 0xCCCCCC);
    draw_string(iface, menu_x + 110, menu_y + 50, 0xFFFFFF);
    
    draw_string("Status:", menu_x + 15, menu_y + 70, 0xCCCCCC);
    if(get_network_status()) {
        draw_string("Connected", menu_x + 110, menu_y + 70, 0x00FF00);
    } else {
        draw_string("Disconnected", menu_x + 110, menu_y + 70, 0xFF0000);
    }
    
    if(net_type == 2) {
        draw_rect(menu_x, menu_y + 90, menu_w, 2, 0x444444);
        draw_string("WiFi Networks:", menu_x + 15, menu_y + 100, 0xCCCCCC);
        
        int wifi_count = get_wifi_network_count();
        int y_offset = menu_y + 95;
        
        for(int i = 0; i < wifi_count && i < 5; i++) {
            char ssid[32];
            uint8_t signal;
            bool secured;
            get_wifi_network(i, ssid, &signal, &secured);
            
            y_offset += 40;
            draw_rounded_rect(menu_x + 10, y_offset, menu_w - 20, 35, 0x1a1a2e);
            draw_string(ssid, menu_x + 20, y_offset + 12, 0xFFFFFF);
            
            if(secured) {
                draw_string("*", menu_x + menu_w - 40, y_offset + 12, 0xFFFF00);
            }
            
            char sig_str[8];
            sig_str[0] = '0' + (signal / 10);
            sig_str[1] = '0' + (signal % 10);
            sig_str[2] = '%';
            sig_str[3] = '\0';
            draw_string(sig_str, menu_x + menu_w - 60, y_offset + 12, signal > 50 ? 0x00FF00 : 0xFFFF00);
        }
    }
}

void draw_power_menu() {
    int menu_w = 200;
    int menu_h = 140;
    int menu_x = 10;
    int menu_y = fb_height - 50 - menu_h - 10;
    
    draw_rounded_rect(menu_x, menu_y, menu_w, menu_h, 0x16213e);
    draw_rect(menu_x, menu_y, menu_w, 40, 0x1a1a2e);
    draw_string("Power", menu_x + 15, menu_y + 15, 0xFFFFFF);
    
    draw_rounded_rect(menu_x + 10, menu_y + 50, menu_w - 20, 35, 0x2A3F5F);
    draw_string("Shutdown", menu_x + 50, menu_y + 62, 0xFFFFFF);
    
    draw_rounded_rect(menu_x + 10, menu_y + 95, menu_w - 20, 35, 0x2A3F5F);
    draw_string("Reboot", menu_x + 60, menu_y + 107, 0xFFFFFF);
}

void draw_settings_menu() {
    int menu_w = 280;
    int menu_h = 200;
    int menu_x = fb_width - menu_w - 20 - 150;
    int menu_y = fb_height - 50 - menu_h - 10;
    
    draw_rounded_rect(menu_x, menu_y, menu_w, menu_h, 0x16213e);
    draw_rect(menu_x, menu_y, menu_w, 40, 0x1a1a2e);
    draw_string("Settings", menu_x + 15, menu_y + 15, 0xFFFFFF);
    
    draw_string("Keyboard Layout", menu_x + 15, menu_y + 55, 0xCCCCCC);
    
    if(current_layout == LAYOUT_QWERTY) {
        draw_string("Current: QWERTY", menu_x + 15, menu_y + 75, 0xFFFFFF);
    } else {
        draw_string("Current: Turkish Q", menu_x + 15, menu_y + 75, 0xFFFFFF);
    }
    
    draw_rounded_rect(menu_x + 15, menu_y + 95, menu_w - 30, 30, 0x0f3460);
    draw_string("Switch Layout", menu_x + 60, menu_y + 105, 0xFFFFFF);
    
    draw_rect(menu_x, menu_y + 140, menu_w, 2, 0x444444);
    draw_string("Display: 1024x768", menu_x + 15, menu_y + 155, 0xCCCCCC);
}

void draw_app_menu() {
    int menu_w = 250;
    int base_h = 180;
    int menu_h = base_h + (installed_app_count * 40);
    int menu_x = 110;
    int menu_y = fb_height - 50 - menu_h - 10;
    
    draw_rounded_rect(menu_x, menu_y, menu_w, menu_h, 0x16213e);
    draw_rect(menu_x, menu_y, menu_w, 40, 0x1a1a2e);
    draw_string("Applications", menu_x + 15, menu_y + 15, 0xFFFFFF);
    
    draw_rounded_rect(menu_x + 10, menu_y + 50, menu_w - 20, 35, 0x2A3F5F);
    draw_string("Terminal", menu_x + 70, menu_y + 62, 0xFFFFFF);
    
    draw_rounded_rect(menu_x + 10, menu_y + 95, menu_w - 20, 35, 0x2A3F5F);
    draw_string("Browser", menu_x + 70, menu_y + 107, 0xFFFFFF);
    
    draw_rounded_rect(menu_x + 10, menu_y + 140, menu_w - 20, 35, 0x2A3F5F);
    draw_string("File Manager", menu_x + 60, menu_y + 152, 0xFFFFFF);
    
    int y_offset = menu_y + 185;
    for(int i = 0; i < installed_app_count; i++) {
        draw_rounded_rect(menu_x + 10, y_offset, menu_w - 20, 35, 0x2A3F5F);
        
        int name_len = strlen(installed_apps[i].name);
        int center_offset = (menu_w - 20 - name_len * 8) / 2;
        draw_string(installed_apps[i].name, menu_x + 10 + center_offset, y_offset + 12, 0xFFFFFF);
        
        if(installed_apps[i].type == 1) {
            draw_string("[H]", menu_x + menu_w - 40, y_offset + 12, 0x00FFFF);
        } else if(installed_apps[i].type == 2) {
            draw_string("[P]", menu_x + menu_w - 40, y_offset + 12, 0xFF00FF);
        }
        
        y_offset += 40;
    }
}

void draw_wifi_password_dialog() {
    int dialog_w = 400;
    int dialog_h = 150;
    int dialog_x = (fb_width - dialog_w) / 2;
    int dialog_y = (fb_height - dialog_h) / 2;
    
    draw_rounded_rect(dialog_x, dialog_y, dialog_w, dialog_h, 0x16213e);
    draw_rect(dialog_x, dialog_y, dialog_w, 40, 0x1a1a2e);
    draw_string("WiFi Password", dialog_x + 15, dialog_y + 15, 0xFFFFFF);
    
    char ssid[32];
    uint8_t signal;
    bool secured;
    get_wifi_network(selected_wifi_network, ssid, &signal, &secured);
    
    draw_string("Network: ", dialog_x + 20, dialog_y + 55, 0xCCCCCC);
    draw_string(ssid, dialog_x + 90, dialog_y + 55, 0xFFFFFF);
    
    draw_rect(dialog_x + 20, dialog_y + 75, dialog_w - 40, 30, 0x1a1a2e);
    
    for(int i = 0; i < wifi_password_len; i++) {
        draw_char('*', dialog_x + 25 + i * 8, dialog_y + 85, 0xFFFFFF);
    }
    
    if(wifi_password_len < 63) {
        draw_rect(dialog_x + 25 + wifi_password_len * 8, dialog_y + 85, 2, 10, 0xFFFFFF);
    }
    
    draw_rounded_rect(dialog_x + dialog_w - 120, dialog_y + 115, 100, 25, 0x0f3460);
    draw_string("Connect", dialog_x + dialog_w - 100, dialog_y + 122, 0xFFFFFF);
}

void refresh_all_windows() {
    if(!in_gui_mode) return;
    
    for(uint64_t i = 0; i < fb_width * fb_height; i++) {
        fb_ptr[i] = 0x0f0f1e;
    }
    
    draw_background_logo();
    
    if(is_terminal_open() && !is_terminal_minimized()) {
        draw_terminal_app();
        terminal_redraw();
    }
    
    if(is_browser_open() && !is_browser_minimized()) {
        draw_browser_window();
    }
    
    if(is_filemanager_open() && !is_filemanager_minimized()) {
        draw_filemanager_window();
        if(viewer_open) {
            draw_viewer_window();
        }
    }
    
    draw_taskbar();
    
    if(app_menu_open) draw_app_menu();
    if(power_menu_open) draw_power_menu();
    if(network_menu_open) draw_network_menu();
    if(settings_menu_open) draw_settings_menu();
    if(wifi_password_prompt) draw_wifi_password_dialog();
    
    draw_cursor(mouse_x, mouse_y);
}

void handle_wifi_password_input(char c) {
    if(c == '\n') {
        connect_to_wifi(selected_wifi_network, wifi_password_input);
        wifi_password_prompt = false;
        network_menu_open = false;
        refresh_all_windows();
    } else if(c == 27) {
        wifi_password_prompt = false;
        refresh_all_windows();
    } else if(c == '\b') {
        if(wifi_password_len > 0) {
            wifi_password_len--;
            wifi_password_input[wifi_password_len] = '\0';
            refresh_all_windows();
        }
    } else if(c >= 32 && c < 127 && wifi_password_len < 63) {
        wifi_password_input[wifi_password_len] = c;
        wifi_password_len++;
        wifi_password_input[wifi_password_len] = '\0';
        refresh_all_windows();
    }
}

void handle_click(int x, int y, bool right_click) {
    if(wifi_password_prompt) {
        int dialog_w = 400;
        int dialog_h = 150;
        int dialog_x = (fb_width - dialog_w) / 2;
        int dialog_y = (fb_height - dialog_h) / 2;
        
        if(x >= dialog_x + dialog_w - 120 && x <= dialog_x + dialog_w - 20 &&
           y >= dialog_y + 115 && y <= dialog_y + 140) {
            connect_to_wifi(selected_wifi_network, wifi_password_input);
            wifi_password_prompt = false;
            network_menu_open = false;
            refresh_all_windows();
        }
        return;
    }
    
    if(y >= fb_height - 50) {
        if(x >= 10 && x <= 100) {
            power_menu_open = !power_menu_open;
            app_menu_open = false;
            network_menu_open = false;
            settings_menu_open = false;
            refresh_all_windows();
            return;
        }
        
        if(x >= 110 && x <= 190) {
            app_menu_open = !app_menu_open;
            power_menu_open = false;
            network_menu_open = false;
            settings_menu_open = false;
            refresh_all_windows();
            return;
        }
        
        int task_x = 210;
        if(is_terminal_open()) {
            if(x >= task_x && x <= task_x + 100) {
                set_terminal_minimized(!is_terminal_minimized());
                focused_app = APP_TERMINAL;
                refresh_all_windows();
                return;
            }
            task_x += 110;
        }
        if(is_browser_open()) {
            if(x >= task_x && x <= task_x + 100) {
                set_browser_minimized(!is_browser_minimized());
                focused_app = APP_BROWSER;
                refresh_all_windows();
                return;
            }
            task_x += 110;
        }
        if(is_filemanager_open()) {
            if(x >= task_x && x <= task_x + 100) {
                set_filemanager_minimized(!is_filemanager_minimized());
                focused_app = APP_FILEMANAGER;
                refresh_all_windows();
                return;
            }
            task_x += 110;
        }
        
        int net_box_width = 140;
        int net_x = fb_width - net_box_width - 20;
        if(x >= net_x && x <= net_x + net_box_width) {
            network_menu_open = !network_menu_open;
            app_menu_open = false;
            power_menu_open = false;
            settings_menu_open = false;
            refresh_all_windows();
            return;
        }
        
        return;
    }
    
    if(app_menu_open) {
        int menu_w = 250;
        int base_h = 180;
        int menu_h = base_h + (installed_app_count * 40);
        int menu_x = 110;
        int menu_y = fb_height - 50 - menu_h - 10;
        
        if(x >= menu_x && x <= menu_x + menu_w && y >= menu_y && y <= menu_y + menu_h) {
            if(y >= menu_y + 50 && y <= menu_y + 85) {
                open_terminal();
                focused_app = APP_TERMINAL;
                app_menu_open = false;
                refresh_all_windows();
                return;
            } else if(y >= menu_y + 95 && y <= menu_y + 130) {
                open_browser();
                focused_app = APP_BROWSER;
                app_menu_open = false;
                refresh_all_windows();
                return;
            } else if(y >= menu_y + 140 && y <= menu_y + 175) {
                open_filemanager();
                focused_app = APP_FILEMANAGER;
                app_menu_open = false;
                refresh_all_windows();
                return;
            }
            
            int y_offset = menu_y + 185;
            for(int i = 0; i < installed_app_count; i++) {
                if(y >= y_offset && y <= y_offset + 35) {
                    installed_apps[i].active = !installed_apps[i].active;
                    app_menu_open = false;
                    refresh_all_windows();
                    return;
                }
                y_offset += 40;
            }
            return;
        }
    }
    
    if(power_menu_open) {
        int menu_w = 200;
        int menu_h = 140;
        int menu_x = 10;
        int menu_y = fb_height - 50 - menu_h - 10;
        
        if(x >= menu_x && x <= menu_x + menu_w && y >= menu_y && y <= menu_y + menu_h) {
            if(y >= menu_y + 50 && y <= menu_y + 85) {
                system_shutdown();
            } else if(y >= menu_y + 95 && y <= menu_y + 130) {
                system_reboot();
            }
            return;
        }
    }
    
    if(settings_menu_open) {
        int menu_w = 280;
        int menu_h = 200;
        int settings_x = fb_width - menu_w - 20 - 150;
        int menu_x = settings_x - 20;
        int menu_y = fb_height - 50 - menu_h - 10;
        
        if(x >= menu_x && x <= menu_x + menu_w && y >= menu_y && y <= menu_y + menu_h) {
            int switch_btn_y = menu_y + 145;
            if(y >= switch_btn_y && y <= switch_btn_y + 30) {
                current_layout = (current_layout == LAYOUT_QWERTY) ? LAYOUT_TRQ : LAYOUT_QWERTY;
                refresh_all_windows();
            }
            return;
        }
    }
    
    if(network_menu_open) {
        int menu_w = 280;
        int menu_h = 350;
        int menu_x = fb_width - menu_w - 20;
        int menu_y = fb_height - 50 - menu_h - 10;
        
        if(x >= menu_x && x <= menu_x + menu_w && y >= menu_y && y <= menu_y + menu_h) {
            int net_type = get_network_type();
            if(net_type == 2) {
                int wifi_count = get_wifi_network_count();
                int y_offset = menu_y + 95;
                
                for(int i = 0; i < wifi_count && i < 5; i++) {
                    if(y >= y_offset && y <= y_offset + 35) {
                        char ssid[32];
                        uint8_t signal;
                        bool secured;
                        get_wifi_network(i, ssid, &signal, &secured);
                        
                        if(secured) {
                            selected_wifi_network = i;
                            wifi_password_prompt = true;
                            wifi_password_len = 0;
                            memset(wifi_password_input, 0, 64);
                            refresh_all_windows();
                        } else {
                            connect_to_wifi(i, "");
                            network_menu_open = false;
                            refresh_all_windows();
                        }
                        return;
                    }
                    y_offset += 40;
                }
            }
            return;
        }
    }
    
    bool click_handled = false;
    
    if (focused_app == APP_TERMINAL && is_terminal_open() && !is_terminal_minimized()) {
        int wx = term_win.maximized ? 0 : term_win.x;
        int wy = term_win.maximized ? 0 : term_win.y;
        int ww = term_win.maximized ? (int)fb_width : term_win.w;
        int wh = term_win.maximized ? (int)fb_height - 50 : term_win.h;
        
        if(x >= wx && x <= wx + ww && y >= wy && y <= wy + wh) {
            handle_terminal_click(x, y);
            if(y <= wy + 35 && !term_win.maximized) {
                if(x >= wx + ww - 20 && x <= wx + ww - 5) {
                    close_terminal();
                    refresh_all_windows();
                    return;
                } else if(x < wx + ww - 80) {
                    current_drag = DRAG_TERMINAL;
                    drag_offset_x = x - wx;
                    drag_offset_y = y - wy;
                }
            }
            click_handled = true;
        }
    } else if (focused_app == APP_BROWSER && is_browser_open() && !is_browser_minimized()) {
        int wx = browser_win.maximized ? 0 : browser_win.x;
        int wy = browser_win.maximized ? 0 : browser_win.y;
        int ww = browser_win.maximized ? (int)fb_width : browser_win.w;
        int wh = browser_win.maximized ? (int)fb_height - 50 : browser_win.h;
        
        if(x >= wx && x <= wx + ww && y >= wy && y <= wy + wh) {
            handle_browser_click(x, y);
            if(y <= wy + 35 && !browser_win.maximized) {
                if(x >= wx + ww - 20 && x <= wx + ww - 5) {
                    close_browser();
                    refresh_all_windows();
                    return;
                } else if(x < wx + ww - 80) {
                    current_drag = DRAG_BROWSER;
                    drag_offset_x = x - wx;
                    drag_offset_y = y - wy;
                }
            }
            click_handled = true;
        }
    } else if (focused_app == APP_FILEMANAGER && is_filemanager_open() && !is_filemanager_minimized()) {
        if(x >= fm_win.x && x <= fm_win.x + fm_win.w && y >= fm_win.y && y <= fm_win.y + fm_win.h) {
            if(right_click) handle_filemanager_rightclick(x, y);
            else handle_filemanager_click(x, y);
            if(y <= fm_win.y + 35) {
                if(x >= fm_win.x + fm_win.w - 20 && x <= fm_win.x + fm_win.w - 5) {
                    close_filemanager();
                    refresh_all_windows();
                    return;
                } else if(x < fm_win.x + fm_win.w - 80) {
                    current_drag = DRAG_FILEMANAGER;
                    drag_offset_x = x - fm_win.x;
                    drag_offset_y = y - fm_win.y;
                }
            }
            click_handled = true;
        }
    }
    
    if (!click_handled) {
        if (is_terminal_open() && !is_terminal_minimized()) {
            int wx = term_win.maximized ? 0 : term_win.x;
            int wy = term_win.maximized ? 0 : term_win.y;
            int ww = term_win.maximized ? (int)fb_width : term_win.w;
            int wh = term_win.maximized ? (int)fb_height - 50 : term_win.h;
            
            if(x >= wx && x <= wx + ww && y >= wy && y <= wy + wh) {
                focused_app = APP_TERMINAL;
                if(y <= wy + 35 && !term_win.maximized) {
                    if(x >= wx + ww - 20 && x <= wx + ww - 5) {
                        close_terminal();
                        refresh_all_windows();
                        return;
                    } else if(x < wx + ww - 80) {
                        current_drag = DRAG_TERMINAL;
                        drag_offset_x = x - wx;
                        drag_offset_y = y - wy;
                    }
                } else {
                    handle_terminal_click(x, y);
                }
                refresh_all_windows();
                return;
            }
        }
        
        if (is_browser_open() && !is_browser_minimized()) {
            int wx = browser_win.maximized ? 0 : browser_win.x;
            int wy = browser_win.maximized ? 0 : browser_win.y;
            int ww = browser_win.maximized ? (int)fb_width : browser_win.w;
            int wh = browser_win.maximized ? (int)fb_height - 50 : browser_win.h;
            
            if(x >= wx && x <= wx + ww && y >= wy && y <= wy + wh) {
                focused_app = APP_BROWSER;
                if(y <= wy + 35 && !browser_win.maximized) {
                    if(x >= wx + ww - 20 && x <= wx + ww - 5) {
                        close_browser();
                        refresh_all_windows();
                        return;
                    } else if(x < wx + ww - 80) {
                        current_drag = DRAG_BROWSER;
                        drag_offset_x = x - wx;
                        drag_offset_y = y - wy;
                    }
                } else {
                    handle_browser_click(x, y);
                }
                refresh_all_windows();
                return;
            }
        }
        
        if (is_filemanager_open() && !is_filemanager_minimized()) {
            if(x >= fm_win.x && x <= fm_win.x + fm_win.w && y >= fm_win.y && y <= fm_win.y + fm_win.h) {
                focused_app = APP_FILEMANAGER;
                if(y <= fm_win.y + 35) {
                    if(x >= fm_win.x + fm_win.w - 20 && x <= fm_win.x + fm_win.w - 5) {
                        close_filemanager();
                        refresh_all_windows();
                        return;
                    } else if(x < fm_win.x + fm_win.w - 80) {
                        current_drag = DRAG_FILEMANAGER;
                        drag_offset_x = x - fm_win.x;
                        drag_offset_y = y - fm_win.y;
                    }
                } else {
                    if(right_click) handle_filemanager_rightclick(x, y);
                    else handle_filemanager_click(x, y);
                }
                refresh_all_windows();
                return;
            }
        }
    }
    
    app_menu_open = false;
    power_menu_open = false;
    network_menu_open = false;
    settings_menu_open = false;
    refresh_all_windows();
}

extern void init_browser_app();
extern void init_terminal_app();
extern void init_filemanager_app();

void init_application_system() {
    init_terminal_app();
    init_browser_app();
    init_filemanager_app();
    
    for(int i = 0; i < MAX_INSTALLED_APPS; i++) {
        installed_apps[i].active = false;
        installed_apps[i].type = 0;
        installed_apps[i].name[0] = '\0';
    }
    installed_app_count = 0;
}

bool is_app_focused() {
    if(wifi_password_prompt) return true;
    if(is_browser_open() && !is_browser_minimized()) return true;
    if(is_filemanager_open() && !is_filemanager_minimized()) return true;
    return false;
}

void handle_app_keyboard(char c) {
    if(wifi_password_prompt) {
        handle_wifi_password_input(c);
        return;
    }
    
    if (focused_app == APP_BROWSER && is_browser_open()) {
        handle_browser_keyboard(c);
    } else if (focused_app == APP_FILEMANAGER && is_filemanager_open()) {
        handle_filemanager_keyboard(c);
    } else {
        if(is_filemanager_open() && !is_filemanager_minimized()) {
            handle_filemanager_keyboard(c);
            return;
        }
        if(is_browser_open() && !is_browser_minimized()) {
            handle_browser_keyboard(c);
        }
    }
}