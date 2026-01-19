#include <stdint.h>
#include <stddef.h>
#include "limine.h"

extern void terminal_init();
extern void terminal_clear();
extern void terminal_write(const char* str);
extern void terminal_redraw();
extern void process_command(const char* cmd);
extern void mouse_handler();
extern void mouse_init();
extern void draw_taskbar();
extern void draw_cursor(int x, int y);
extern void draw_modern_boot_menu();
extern void draw_boot_screen();
extern void update_boot_progress(int progress);
extern bool check_boot_menu_input();
extern char current_directory[64];
extern void init_application_system();
extern void network_init();
extern void init_hlfs();
extern void init_hlpkg_system();
extern void init_port_system();
extern void hlpkg_tick();

static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0,
    .response = NULL
};

static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0,
    .response = NULL
};

static volatile struct limine_smp_request smp_request = {
    .id = LIMINE_SMP_REQUEST,
    .revision = 0,
    .response = NULL,
    .flags = 0
};

uint32_t* fb_ptr = 0;
uint64_t fb_width = 0;
uint64_t fb_height = 0;
uint64_t fb_pitch = 0;

uint32_t cpu_core_count = 0;
uint64_t total_memory_kb = 0;
uint64_t free_memory_kb = 0;
uint64_t usable_memory_kb = 0;
uint64_t uptime_seconds = 0;
char cpu_brand_string[64];

bool in_gui_mode = false;
bool boot_complete = false;
bool boot_menu_active = true;
int mouse_x = 400;
int mouse_y = 300;

void get_cpu_brand_string() {
    uint32_t eax, ebx, ecx, edx;
    uint32_t* desc = (uint32_t*)cpu_brand_string;
    
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x80000000));
    if (eax < 0x80000004) {
        char default_name[] = "Generic x86_64 CPU";
        for(int i=0; default_name[i]; i++) cpu_brand_string[i] = default_name[i];
        return;
    }

    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x80000002));
    desc[0] = eax; desc[1] = ebx; desc[2] = ecx; desc[3] = edx;

    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x80000003));
    desc[4] = eax; desc[5] = ebx; desc[6] = ecx; desc[7] = edx;

    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x80000004));
    desc[8] = eax; desc[9] = ebx; desc[10] = ecx; desc[11] = edx;
    
    cpu_brand_string[48] = 0;
    
    int start = 0;
    while(cpu_brand_string[start] == ' ') start++;
    if(start > 0) {
        int i = 0;
        while(cpu_brand_string[start + i]) {
            cpu_brand_string[i] = cpu_brand_string[start + i];
            i++;
        }
        cpu_brand_string[i] = 0;
    }
}

extern "C" void _start(void) {
    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) {
        for(;;);
    }
    
    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    fb_ptr = (uint32_t*)fb->address;
    fb_width = fb->width;
    fb_height = fb->height;
    fb_pitch = fb->pitch;

    if(memmap_request.response != NULL) {
        struct limine_memmap_response *memmap = memmap_request.response;
        total_memory_kb = 0;
        usable_memory_kb = 0;
        for(uint64_t i = 0; i < memmap->entry_count; i++) {
            total_memory_kb += memmap->entries[i]->length / 1024;
            if(memmap->entries[i]->type == LIMINE_MEMMAP_USABLE) {
                usable_memory_kb += memmap->entries[i]->length / 1024;
            }
        }
        if(total_memory_kb == 0) total_memory_kb = usable_memory_kb;
        if(total_memory_kb == 0) total_memory_kb = 2097152;
        free_memory_kb = usable_memory_kb * 83 / 100;
        if(free_memory_kb == 0) free_memory_kb = total_memory_kb * 83 / 100;
    }
    
    if(smp_request.response != NULL) {
        cpu_core_count = smp_request.response->cpu_count;
    }
    
    if(cpu_core_count == 0) cpu_core_count = 1;

    get_cpu_brand_string();
    terminal_init();
    mouse_init();
    network_init();
    init_hlfs();
    init_hlpkg_system();
    init_port_system();
    
    for(uint64_t i = 0; i < fb_width * fb_height; i++) fb_ptr[i] = 0x0f0f1e;
    
    draw_modern_boot_menu();
    
    while(boot_menu_active) {
        if(check_boot_menu_input()) {
            break;
        }
        for(volatile int d = 0; d < 100000; d++);
    }
    
    draw_boot_screen();
    for(int progress = 0; progress <= 100; progress += 2) {
        update_boot_progress(progress);
        for(volatile int d = 0; d < 500000; d++);
    }
    
    for(volatile int d = 0; d < 2000000; d++);
    
    in_gui_mode = true;
    boot_complete = true;
    init_application_system();
    
    for(uint64_t i = 0; i < fb_width * fb_height; i++) fb_ptr[i] = 0x0f0f1e;
    draw_taskbar();
    draw_cursor(mouse_x, mouse_y);

    uint64_t tick_counter = 0;
    while (1) {
        mouse_handler();
        
        if(++tick_counter > 100000) {
            uptime_seconds++;
            tick_counter = 0;
            free_memory_kb = total_memory_kb * 83 / 100 - (uptime_seconds % 100) * 10;
            if(free_memory_kb < total_memory_kb / 4) {
                free_memory_kb = total_memory_kb * 83 / 100;
            }
            
            hlpkg_tick();
        }
        
        for(volatile int i=0; i<10000; i++); 
    }
}