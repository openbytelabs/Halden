#include <stdint.h>
#include <stddef.h>

extern uint32_t* fb_ptr;
extern uint64_t fb_width;
extern uint64_t fb_height;

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
extern void uint_to_str(uint64_t n, char* buffer);
extern uint64_t uptime_seconds;

enum FileType {
    FILE_REGULAR = 0,
    FILE_DIRECTORY = 1,
    FILE_DEVICE = 2,
    FILE_SOURCE = 3
};

extern int get_directory_contents(const char* path, int* indices, int max_count);
extern bool get_file_info(int index, char* name_out, char* path_out, FileType* type_out, uint64_t* size_out);
extern bool read_file_content(const char* path, char* output, int max_len);
extern bool write_file_content(const char* path, const char* content);
extern bool create_file_in_fs(const char* parent_path, const char* name, FileType type);
extern bool delete_file_from_fs(const char* path);
extern bool rename_file_in_fs(const char* old_path, const char* new_name);

struct FileManagerWindow {
    int x, y;
    int w, h;
    bool minimized;
    bool maximized;
    bool dragging;
    int drag_offset_x;
    int drag_offset_y;
};

FileManagerWindow fm_win = {100, 80, 700, 500, false, false, false, 0, 0};
bool filemanager_open = false;
char current_fm_path[256] = "/";
int file_list_indices[1024];
int file_list_count = 0;
int scroll_offset_fm = 0;
int selected_file = -1;
uint64_t last_click_time = 0;
int last_clicked_file = -1;

bool viewer_open = false;
char viewer_content[8192];
char viewer_title[128];
bool viewer_editing = false;
char viewer_path[256];
int viewer_cursor_pos = 0;
int viewer_scroll = 0;

char navigation_history[32][256];
int navigation_history_count = 0;
int navigation_history_pos = -1;

enum ContextMenuType {
    CTX_NONE = 0,
    CTX_FILE = 1,
    CTX_EMPTY = 2
};

ContextMenuType context_menu_type = CTX_NONE;
int context_menu_x = 0;
int context_menu_y = 0;
char context_menu_target[256];
int context_menu_target_idx = -1;

bool new_file_dialog = false;
bool new_folder_dialog = false;
bool rename_dialog = false;
bool delete_confirm = false;
char dialog_input[64] = "";
int dialog_input_len = 0;

void load_directory() {
    memset(file_list_indices, 0, sizeof(file_list_indices));
    file_list_count = get_directory_contents(current_fm_path, file_list_indices, 1024);
    
    if (scroll_offset_fm > file_list_count) scroll_offset_fm = 0;
    selected_file = -1;
}

void push_navigation_history(const char* path) {
    if(navigation_history_pos < 31) {
        navigation_history_pos++;
        strcpy(navigation_history[navigation_history_pos], path);
        navigation_history_count = navigation_history_pos + 1;
    }
}

void navigate_back() {
    if(navigation_history_pos > 0) {
        navigation_history_pos--;
        strcpy(current_fm_path, navigation_history[navigation_history_pos]);
        load_directory();
        refresh_all_windows();
    }
}

void navigate_forward() {
    if(navigation_history_pos < navigation_history_count - 1) {
        navigation_history_pos++;
        strcpy(current_fm_path, navigation_history[navigation_history_pos]);
        load_directory();
        refresh_all_windows();
    }
}

void navigate_up() {
    if(strcmp(current_fm_path, "/") == 0) return;
    
    char* last_slash = nullptr;
    for(int i = 0; current_fm_path[i]; i++) {
        if(current_fm_path[i] == '/') {
            last_slash = &current_fm_path[i];
        }
    }
    
    if(last_slash == current_fm_path) {
        strcpy(current_fm_path, "/");
    } else if(last_slash) {
        *last_slash = '\0';
    } else {
        strcpy(current_fm_path, "/");
    }
    
    push_navigation_history(current_fm_path); 
    load_directory();
    refresh_all_windows();
}

void init_filemanager_app() {
    filemanager_open = false;
    fm_win.x = 100;
    fm_win.y = 80;
    fm_win.w = 700;
    fm_win.h = 500;
    fm_win.minimized = false;
    fm_win.maximized = false;
}

void open_filemanager() {
    filemanager_open = true;
    fm_win.minimized = false;
    strcpy(current_fm_path, "/");
    navigation_history_count = 0;
    navigation_history_pos = -1;
    push_navigation_history(current_fm_path);
    load_directory();
}

bool is_filemanager_open() {
    return filemanager_open;
}

bool is_filemanager_minimized() {
    return fm_win.minimized;
}

void set_filemanager_minimized(bool state) {
    fm_win.minimized = state;
}

void draw_file_icon(int x, int y, FileType type) {
    if(type == FILE_DIRECTORY) {
        draw_rect(x, y, 16, 14, 0xFFD700);
        draw_rect(x + 1, y + 1, 14, 12, 0x1a1a2e); 
        draw_rect(x + 2, y + 3, 12, 9, 0xFFD700);
        draw_rect(x, y - 2, 6, 2, 0xFFD700);
    } else if(type == FILE_SOURCE) {
        draw_rect(x, y, 14, 16, 0x4A9EFF);
        draw_rect(x + 1, y + 1, 12, 14, 0x1a1a2e);
        draw_string("C", x + 4, y + 4, 0x4A9EFF);
    } else {
        draw_rect(x, y, 14, 16, 0xCCCCCC);
        draw_rect(x + 1, y + 1, 12, 14, 0x1a1a2e);
        draw_rect(x + 3, y + 4, 8, 2, 0x888888);
        draw_rect(x + 3, y + 8, 8, 2, 0x888888);
        draw_rect(x + 3, y + 12, 8, 2, 0x888888);
    }
}

void draw_context_menu() {
    int menu_w = 180;
    int menu_h = (context_menu_type == CTX_FILE) ? 120 : 80;
    int menu_x = context_menu_x;
    int menu_y = context_menu_y;
    
    if(menu_x + menu_w > (int)fb_width) menu_x = fb_width - menu_w - 5;
    if(menu_y + menu_h > (int)fb_height) menu_y = fb_height - menu_h - 5;
    
    draw_rect(menu_x, menu_y, menu_w, menu_h, 0x16213e);
    draw_rect(menu_x, menu_y, menu_w, 2, 0x4A9EFF);
    draw_rect(menu_x, menu_y, 2, menu_h, 0x4A9EFF);
    draw_rect(menu_x + menu_w - 2, menu_y, 2, menu_h, 0x4A9EFF);
    draw_rect(menu_x, menu_y + menu_h - 2, menu_w, 2, 0x4A9EFF);
    
    int item_y = menu_y + 10;
    
    if(context_menu_type == CTX_FILE) {
        draw_string("Open", menu_x + 15, item_y, 0xFFFFFF);
        item_y += 25;
        draw_string("Rename", menu_x + 15, item_y, 0xFFFFFF);
        item_y += 25;
        draw_string("Delete", menu_x + 15, item_y, 0xFF6666);
        item_y += 25;
        draw_rect(menu_x + 10, item_y, menu_w - 20, 1, 0x444444);
        item_y += 10;
        draw_string("Properties", menu_x + 15, item_y, 0x888888);
    } else if(context_menu_type == CTX_EMPTY) {
        draw_string("New File", menu_x + 15, item_y, 0xFFFFFF);
        item_y += 25;
        draw_string("New Folder", menu_x + 15, item_y, 0xFFFFFF);
        item_y += 25;
        draw_string("Refresh", menu_x + 15, item_y, 0xAAAAAA);
    }
}

void draw_dialog_box(const char* title, const char* message) {
    int w = 400;
    int h = 180;
    int x = (fb_width - w) / 2;
    int y = (fb_height - h) / 2;
    
    draw_rect(x, y, w, h, 0x16213e);
    draw_rect(x, y, w, 2, 0xCCCCCC); 
    draw_rect(x, y, w, 35, 0x0f3460);
    draw_string(title, x + 15, y + 12, 0xFFFFFF);
    
    draw_string(message, x + 20, y + 55, 0xCCCCCC);
    
    draw_rect(x + 20, y + 85, w - 40, 30, 0x000000);
    draw_rect(x + 19, y + 84, w - 38, 32, 0x444444); 
    draw_rect(x + 20, y + 85, w - 40, 30, 0x1a1a2e);
    
    draw_string(dialog_input, x + 25, y + 95, 0xFFFFFF);
    
    int cursor_x = x + 25 + (int)strlen(dialog_input) * 8;
    if ((uptime_seconds % 2) == 0) {
        draw_rect(cursor_x, y + 93, 2, 14, 0xFFFFFF);
    }
    
    draw_rounded_rect(x + 20, y + 130, 80, 30, 0x238636);
    draw_string("OK", x + 45, y + 140, 0xFFFFFF);
    
    draw_rounded_rect(x + 110, y + 130, 80, 30, 0x5A1A1A);
    draw_string("Cancel", x + 125, y + 140, 0xFFFFFF);
}

void draw_viewer_window() {
    int x = 50;
    int y = 50;
    int w = fb_width - 100;
    int h = fb_height - 100;
    
    draw_rect(x, y, w, 40, 0x1a1a2e);
    draw_string(viewer_editing ? "File Editor" : "File Viewer", x + 10, y + 12, 0xFFFFFF);
    
    if(viewer_editing) {
        draw_rounded_rect(x + w - 180, y + 5, 70, 30, 0x238636);
        draw_string("Save", x + w - 160, y + 15, 0xFFFFFF);
    } else {
        draw_rounded_rect(x + w - 180, y + 5, 70, 30, 0x0f3460);
        draw_string("Edit", x + w - 160, y + 15, 0xFFFFFF);
    }
    
    draw_rounded_rect(x + w - 100, y + 5, 80, 30, 0x5A1A1A);
    draw_string("Close", x + w - 80, y + 15, 0xFFFFFF);
    
    draw_rect(x, y + 40, w, h - 40, 0x222222);
    
    draw_string(viewer_title, x + 10, y + 50, 0x4A9EFF);
    draw_rect(x + 10, y + 70, w - 20, 2, 0x444444);
    
    int cx = x + 15;
    int cy = y + 80 - (viewer_scroll * 12);
    int max_x = x + w - 15;
    int max_y = y + h - 15;
    int start_y = y + 80;
    
    for(int i = 0; viewer_content[i]; i++) {
        if(cy > max_y) break;
        
        if(cy >= start_y - 12) {
            if(viewer_content[i] == '\n') {
                if(viewer_editing && i == viewer_cursor_pos) {
                     draw_rect(cx, cy, 8, 12, 0x555555);
                }
                cx = x + 15;
                cy += 12;
            } else if(viewer_content[i] == '\t') {
                cx += 32;
            } else {
                if(cx < max_x) {
                    if(viewer_editing && i == viewer_cursor_pos) {
                        draw_rect(cx, cy, 8, 12, 0x555555);
                    }
                    draw_char(viewer_content[i], cx, cy, 0xCCCCCC);
                }
                cx += 8;
                if(cx >= max_x) {
                    cx = x + 15;
                    cy += 12;
                }
            }
        } else {
             if(viewer_content[i] == '\n') cy += 12;
             else if(cx >= max_x) cy += 12;
             if(viewer_content[i] != '\n') cx += 8;
        }
    }
    
    if(viewer_editing && viewer_cursor_pos >= (int)strlen(viewer_content)) {
        if(cy >= start_y && cy < max_y) {
            draw_rect(cx, cy, 8, 12, 0x555555);
        }
    }
}

void draw_filemanager_window() {
    if(fm_win.minimized || !filemanager_open) return;
    
    int x = fm_win.maximized ? 0 : fm_win.x;
    int y = fm_win.maximized ? 0 : fm_win.y;
    int w = fm_win.maximized ? (int)fb_width : fm_win.w;
    int h = fm_win.maximized ? (int)fb_height - 40 : fm_win.h;
    
    draw_rect(x, y, w, h, 0x2a2a3e); 
    draw_rect(x, y, w, 35, 0x16213e); 
    draw_string("File Manager", x + 10, y + 12, 0xFFFFFF);
    
    draw_rect(x + w - 70, y + 8, 20, 20, 0x444400);
    draw_string("-", x + w - 64, y + 10, 0xFFFF00);
    
    draw_rect(x + w - 45, y + 8, 20, 20, 0x004400);
    draw_string("O", x + w - 39, y + 10, 0x00FF00);
    
    draw_rect(x + w - 20, y + 8, 20, 20, 0x440000);
    draw_string("X", x + w - 14, y + 10, 0xFF0000);
    
    int tb_y = y + 40;
    draw_rounded_rect(x + 10, tb_y, 40, 28, navigation_history_pos > 0 ? 0x0f3460 : 0x222222);
    draw_string("<", x + 25, tb_y + 8, navigation_history_pos > 0 ? 0xFFFFFF : 0x666666);
    
    draw_rounded_rect(x + 55, tb_y, 40, 28, navigation_history_pos < navigation_history_count - 1 ? 0x0f3460 : 0x222222);
    draw_string(">", x + 70, tb_y + 8, navigation_history_pos < navigation_history_count - 1 ? 0xFFFFFF : 0x666666);
    
    draw_rect(x + 105, tb_y, w - 225, 28, 0x1a1a2e);
    draw_string("Path:", x + 110, tb_y + 8, 0x888888);
    draw_string(current_fm_path, x + 155, tb_y + 8, 0xFFFFFF);
    
    draw_rounded_rect(x + w - 110, tb_y, 100, 28, 0x0f3460);
    draw_string("Up Level", x + w - 90, tb_y + 8, 0xFFFFFF);
    
    draw_rect(x, y + 75, w, 2, 0x444444);
    
    int list_y = y + 80;
    int list_h = h - 85;
    int item_h = 32;
    int visible_items = list_h / item_h;
    
    if(file_list_count == 0) {
        draw_string("(Empty Folder)", x + w/2 - 50, list_y + 20, 0x666666);
    } else {
        for(int i = 0; i < visible_items; i++) {
            int file_idx = i + scroll_offset_fm;
            if(file_idx >= file_list_count) break;
            
            int idx = file_list_indices[file_idx];
            char name[64], path[256];
            FileType type;
            uint64_t size;
            
            if(!get_file_info(idx, name, path, &type, &size)) continue;
            
            int item_y = list_y + (i * item_h);
            
            if(file_idx == selected_file) {
                draw_rect(x + 2, item_y, w - 4, item_h, 0x0f3460);
            }
            
            draw_file_icon(x + 10, item_y + 8, type);
            draw_string(name, x + 35, item_y + 10, 0xFFFFFF);
            
            if(type == FILE_REGULAR || type == FILE_SOURCE) {
                char size_str[32];
                if(size < 1024) {
                    uint_to_str(size, size_str);
                    strcat(size_str, " B");
                } else if(size < 1024 * 1024) {
                    uint_to_str(size / 1024, size_str);
                    strcat(size_str, " KB");
                } else {
                    uint_to_str(size / (1024 * 1024), size_str);
                    strcat(size_str, " MB");
                }
                draw_string(size_str, x + w - 100, item_y + 10, 0xAAAAAA);
            } else if(type == FILE_DIRECTORY) {
                draw_string("<DIR>", x + w - 100, item_y + 10, 0xFFD700);
            }
        }
    }
    
    // Scrollbar
    if(file_list_count > visible_items) {
        int sb_h = list_h * visible_items / file_list_count;
        if(sb_h < 20) sb_h = 20;
        int sb_y = list_y + (scroll_offset_fm * (list_h - sb_h) / (file_list_count - visible_items));
        draw_rect(x + w - 6, list_y, 6, list_h, 0x111111);
        draw_rounded_rect(x + w - 5, sb_y, 4, sb_h, 0x888888);
    }
    
    if(context_menu_type != CTX_NONE) {
        draw_context_menu();
    }
    
    if(new_file_dialog) draw_dialog_box("New File", "Enter file name:");
    else if(new_folder_dialog) draw_dialog_box("New Folder", "Enter folder name:");
    else if(rename_dialog) draw_dialog_box("Rename", "Enter new name:");
    else if(delete_confirm) {
        int dw = 400;
        int dh = 150;
        int dx = (fb_width - dw) / 2;
        int dy = (fb_height - dh) / 2;
        
        draw_rect(dx, dy, dw, dh, 0x16213e);
        draw_rect(dx, dy, dw, 2, 0xFF6666);
        draw_string("Confirm Delete", dx + 15, dy + 12, 0xFFFFFF);
        draw_string("Are you sure you want to delete?", dx + 20, dy + 55, 0xCCCCCC);
        
        draw_rounded_rect(dx + 20, dy + 100, 80, 30, 0x8B0000);
        draw_string("Delete", dx + 35, dy + 110, 0xFFFFFF);
        draw_rounded_rect(dx + 110, dy + 100, 80, 30, 0x0f3460);
        draw_string("Cancel", dx + 125, dy + 110, 0xFFFFFF);
    }
}

void open_file_at_index(int list_idx) {
    if(list_idx < 0 || list_idx >= file_list_count) return;
    
    int idx = file_list_indices[list_idx];
    char name[64], path[256];
    FileType type;
    uint64_t size;
    
    if(!get_file_info(idx, name, path, &type, &size)) return;
    
    if(type == FILE_DIRECTORY) {
        push_navigation_history(current_fm_path);
        
        if(strcmp(current_fm_path, "/") == 0) {
            strcpy(current_fm_path, "/");
            strcat(current_fm_path, name);
        } else {
            strcat(current_fm_path, "/");
            strcat(current_fm_path, name);
        }
        
        load_directory();
        refresh_all_windows();
    } else if(type == FILE_REGULAR || type == FILE_SOURCE) {
        if(read_file_content(path, viewer_content, 8192)) {
            strcpy(viewer_title, name);
            strcpy(viewer_path, path);
            viewer_open = true;
            viewer_editing = false;
            viewer_cursor_pos = 0;
            viewer_scroll = 0;
            refresh_all_windows();
        }
    }
}

void handle_viewer_keyboard(char c) {
    int len = strlen(viewer_content);
    
    if(c == '\b') {
        if(viewer_cursor_pos > 0) {
            for(int i = viewer_cursor_pos - 1; i < len; i++) {
                viewer_content[i] = viewer_content[i + 1];
            }
            viewer_cursor_pos--;
        }
    } else if(c >= 32 || c == '\n' || c == '\t') {
        if(len < 8190) {
            for(int i = len; i >= viewer_cursor_pos; i--) {
                viewer_content[i + 1] = viewer_content[i];
            }
            viewer_content[viewer_cursor_pos] = c;
            viewer_cursor_pos++;
            viewer_content[len + 1] = '\0';
        }
    }
    refresh_all_windows();
}

void handle_viewer_click(int x, int y) {
    int vx = 50, vy = 50, vw = fb_width - 100;
    
    if(y >= vy && y <= vy + 40) {
        if(x >= vx + vw - 180 && x <= vx + vw - 110) { 
            if(viewer_editing) {
                write_file_content(viewer_path, viewer_content);
                viewer_editing = false;
            } else {
                viewer_editing = true;
                viewer_cursor_pos = strlen(viewer_content);
            }
            refresh_all_windows();
        } else if(x >= vx + vw - 100 && x <= vx + vw - 20) { 
            viewer_open = false;
            viewer_editing = false;
            refresh_all_windows();
        }
    }
}

void handle_dialog_keyboard(char c) {
    if(c == '\b') {
        if(dialog_input_len > 0) {
            dialog_input_len--;
            dialog_input[dialog_input_len] = '\0';
        }
    } else if(c == '\n') {
        bool success = false;
        if(new_file_dialog && dialog_input_len > 0) {
            success = create_file_in_fs(current_fm_path, dialog_input, FILE_REGULAR);
            new_file_dialog = false;
        } else if(new_folder_dialog && dialog_input_len > 0) {
            success = create_file_in_fs(current_fm_path, dialog_input, FILE_DIRECTORY);
            new_folder_dialog = false;
        } else if(rename_dialog && dialog_input_len > 0) {
            success = rename_file_in_fs(context_menu_target, dialog_input);
            rename_dialog = false;
        }
        if(success) load_directory();
        dialog_input_len = 0;
        memset(dialog_input, 0, 64);
    } else if(c >= 32 && c < 127 && dialog_input_len < 63) {
        dialog_input[dialog_input_len++] = c;
        dialog_input[dialog_input_len] = '\0';
    }
    refresh_all_windows();
}

void handle_context_menu_click(int x, int y) {
    int menu_w = 180;
    int menu_h = (context_menu_type == CTX_FILE) ? 120 : 80;
    int menu_x = context_menu_x;
    int menu_y = context_menu_y;
    
    if(menu_x + menu_w > (int)fb_width) menu_x = fb_width - menu_w - 5;
    if(menu_y + menu_h > (int)fb_height) menu_y = fb_height - menu_h - 5;
    
    if(x < menu_x || x > menu_x + menu_w || y < menu_y || y > menu_y + menu_h) {
        context_menu_type = CTX_NONE;
        refresh_all_windows();
        return;
    }
    
    int item_idx = (y - menu_y - 10) / 25;
    
    if(context_menu_type == CTX_FILE) {
        if(item_idx == 0) { 
            if(context_menu_target_idx != -1) open_file_at_index(context_menu_target_idx);
        } else if(item_idx == 1) { 
            rename_dialog = true;
            dialog_input_len = 0;
            memset(dialog_input, 0, 64);
        } else if(item_idx == 2) { 
            delete_confirm = true;
        }
    } else if(context_menu_type == CTX_EMPTY) {
        if(item_idx == 0) { 
            new_file_dialog = true;
            dialog_input_len = 0;
            memset(dialog_input, 0, 64);
        } else if(item_idx == 1) { 
            new_folder_dialog = true;
            dialog_input_len = 0;
            memset(dialog_input, 0, 64);
        } else if(item_idx == 2) { 
            load_directory();
        }
    }
    context_menu_type = CTX_NONE;
    refresh_all_windows();
}

void handle_filemanager_click(int x, int y) {
    if(viewer_open) {
        handle_viewer_click(x, y);
        return;
    }
    
    if(new_file_dialog || new_folder_dialog || rename_dialog) {
        int w = 400, h = 180;
        int dx = (fb_width - w) / 2, dy = (fb_height - h) / 2;
        if(y >= dy + 130 && y <= dy + 160) {
            if(x >= dx + 20 && x <= dx + 100) {
                handle_dialog_keyboard('\n');
            } else if(x >= dx + 110 && x <= dx + 190) {
                new_file_dialog = false;
                new_folder_dialog = false;
                rename_dialog = false;
                refresh_all_windows();
            }
        }
        return;
    }
    
    if(delete_confirm) {
        int w = 400, h = 150;
        int dx = (fb_width - w) / 2, dy = (fb_height - h) / 2;
        if(y >= dy + 100 && y <= dy + 130) {
            if(x >= dx + 20 && x <= dx + 100) {
                if(delete_file_from_fs(context_menu_target)) load_directory();
                delete_confirm = false;
                refresh_all_windows();
            } else if(x >= dx + 110 && x <= dx + 190) {
                delete_confirm = false;
                refresh_all_windows();
            }
        }
        return;
    }
    
    if(context_menu_type != CTX_NONE) {
        handle_context_menu_click(x, y);
        return;
    }
    
    int wx = fm_win.maximized ? 0 : fm_win.x;
    int wy = fm_win.maximized ? 0 : fm_win.y;
    int ww = fm_win.maximized ? (int)fb_width : fm_win.w;
    int wh = fm_win.maximized ? (int)fb_height - 40 : fm_win.h;
    
    if(y >= wy && y <= wy + 35) {
        if(x >= wx + ww - 70 && x <= wx + ww - 50) { 
            fm_win.minimized = true;
        } else if(x >= wx + ww - 45 && x <= wx + ww - 25) { 
            fm_win.maximized = !fm_win.maximized;
        } else if(x >= wx + ww - 20 && x <= wx + ww) { 
            filemanager_open = false;
        } else if(!fm_win.maximized) {
            fm_win.dragging = true;
            fm_win.drag_offset_x = x - x; 
            fm_win.drag_offset_y = y - y; 
        }
        refresh_all_windows();
        return;
    }
    
    int tb_y = wy + 40;
    if(y >= tb_y && y <= tb_y + 30) {
        if(x >= wx + 10 && x <= wx + 50) navigate_back();
        else if(x >= wx + 55 && x <= wx + 95) navigate_forward();
        else if(x >= wx + ww - 110 && x <= wx + ww - 10) navigate_up();
        return;
    }
    
    int list_y = wy + 80;
    int list_h = wh - 85;
    int item_h = 32;
    
    if(y >= list_y && y < list_y + list_h) {
        int clicked_item_idx = (y - list_y) / item_h;
        int clicked_data_idx = clicked_item_idx + scroll_offset_fm;
        
        if(clicked_data_idx >= 0 && clicked_data_idx < file_list_count) {
            bool is_double_click = false;
            if(clicked_data_idx == last_clicked_file && (uptime_seconds - last_click_time) <= 1) {
                is_double_click = true;
            }
            
            last_clicked_file = clicked_data_idx;
            last_click_time = uptime_seconds;
            
            if(is_double_click) {
                open_file_at_index(clicked_data_idx);
            } else {
                selected_file = clicked_data_idx;
                refresh_all_windows();
            }
        }
    }
}

void handle_filemanager_drag(int x, int y) {
    if(fm_win.dragging && !fm_win.maximized) {
        fm_win.x = x - 20; 
        fm_win.y = y - 10;
        refresh_all_windows();
    }
}

void handle_filemanager_mouseup() {
    fm_win.dragging = false;
}

void handle_filemanager_rightclick(int x, int y) {
    if(viewer_open || new_file_dialog || new_folder_dialog || rename_dialog || delete_confirm) return;
    
    if(context_menu_type != CTX_NONE) {
        context_menu_type = CTX_NONE;
        refresh_all_windows();
        return;
    }
    
    int wx = fm_win.maximized ? 0 : fm_win.x;
    int wy = fm_win.maximized ? 0 : fm_win.y;
    int wh = fm_win.maximized ? (int)fb_height - 40 : fm_win.h;
    
    int list_y = wy + 80;
    int list_h = wh - 85;
    int item_h = 32;
    
    if(y >= list_y && y < list_y + list_h) {
        int clicked_idx = (y - list_y) / item_h + scroll_offset_fm;
        
        if(clicked_idx >= 0 && clicked_idx < file_list_count) {
            int idx = file_list_indices[clicked_idx];
            char name[64], path[256];
            FileType type;
            uint64_t size;
            
            if(get_file_info(idx, name, path, &type, &size)) {
                context_menu_type = CTX_FILE;
                context_menu_x = x;
                context_menu_y = y;
                strcpy(context_menu_target, path);
                context_menu_target_idx = clicked_idx;
                selected_file = clicked_idx;
                refresh_all_windows();
            }
        } else {
            context_menu_type = CTX_EMPTY;
            context_menu_x = x;
            context_menu_y = y;
            refresh_all_windows();
        }
    }
}

void handle_filemanager_scroll(int delta) {
    if(viewer_open) {
        viewer_scroll -= delta;
        if(viewer_scroll < 0) viewer_scroll = 0;
        refresh_all_windows();
    } else if(filemanager_open && !fm_win.minimized) {
        scroll_offset_fm -= delta;
        if(scroll_offset_fm < 0) scroll_offset_fm = 0;
        if(file_list_count > 0 && scroll_offset_fm >= file_list_count) scroll_offset_fm = file_list_count - 1;
        refresh_all_windows();
    }
}

void handle_filemanager_keyboard(char c) {
    if(viewer_open && viewer_editing) {
        handle_viewer_keyboard(c);
    } else if(new_file_dialog || new_folder_dialog || rename_dialog) {
        handle_dialog_keyboard(c);
    }
}