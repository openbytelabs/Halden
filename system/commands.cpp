#include <stdint.h>
#include <stddef.h>

extern void terminal_write(const char* str);
extern uint32_t cpu_core_count;
extern uint64_t total_memory_kb;
extern uint64_t free_memory_kb;
extern uint64_t uptime_seconds;
extern char current_directory[64];
extern char cpu_brand_string[];

extern int hlpkg_get_package_count();
extern int hlpkg_get_running_count();
extern int port_get_active_count();

enum HLPKGStatus {
    PKG_NOT_LOADED = 0,
    PKG_LOADED = 1,
    PKG_RUNNING = 2,
    PKG_SUSPENDED = 3,
    PKG_ERROR = 4
};

enum PortStatus {
    PORT_INACTIVE = 0,
    PORT_LOADING = 1,
    PORT_ACTIVE = 2,
    PORT_SUSPENDED = 3,
    PORT_ERROR = 4
};

extern int hlpkg_load(const char* path);
extern int hlpkg_execute(int package_id);
extern bool hlpkg_kill(uint32_t pid);
extern int hlpkg_find_package(const char* name);
extern bool hlpkg_get_package_info(int package_id, char* name_out, char* version_out, char* author_out, uint32_t* size_out);
extern int hlpkg_get_process_list(uint32_t* pid_list, int max_count);
extern bool hlpkg_get_process_info(uint32_t pid, char* name_out, HLPKGStatus* status_out, uint32_t* mem_out, uint64_t* time_out);
extern void hlpkg_unload(int package_id);

extern int port_load_elf(const char* path);
extern bool port_kill(uint32_t pid);
extern int port_get_process_list(uint32_t* pid_list, int max_count);
extern bool port_get_process_info(uint32_t pid, char* name_out, PortStatus* status_out, uint32_t* mem_out);

extern void add_installed_app(const char* name, int app_type);
extern void refresh_all_windows();

extern void* memcpy(void *dest, const void *src, size_t n);
extern void* memset(void *s, int c, size_t n);
extern size_t strlen(const char *str);
extern int strcmp(const char *s1, const char *s2);
extern int strncmp(const char *s1, const char *s2, size_t n);
extern char* strcpy(char *dest, const char *src);
extern char* strcat(char *dest, const char *src);
extern void uint_to_str(uint64_t n, char* buffer);

void int_to_str(int64_t n, char* buffer) {
    if (n < 0) {
        buffer[0] = '-';
        uint_to_str(-n, buffer + 1);
    } else {
        uint_to_str(n, buffer);
    }
}

void hex_to_str(uint64_t n, char* buffer) {
    const char hex[] = "0123456789abcdef";
    char temp[20];
    int i = 0;
    if (n == 0) { buffer[0] = '0'; buffer[1] = 0; return; }
    while (n > 0) {
        temp[i++] = hex[n % 16];
        n /= 16;
    }
    int j = 0;
    while (i > 0) buffer[j++] = temp[--i];
    buffer[j] = 0;
}

struct File {
    char name[32];
    char content[512];
    bool is_dir;
    uint64_t size;
};

struct Disk {
    bool exists;
    char name[8];
    uint64_t size_mb;
};

#define FILE_COUNT 32
struct File files[FILE_COUNT] = {
    {"tty0", "Character device", false, 0},
    {"tty1", "Character device", false, 0},
    {"sda", "Block device", false, 0},
    {"null", "Null device", false, 0},
    {"random", "Random device", false, 0},
    {"urandom", "Random device", false, 0},
    {"passwd", "root:x:0:0:root:/root:/bin/bash\n", false, 40},
    {"shadow", "root:!:19000:0:99999:7:::\n", false, 26},
    {"hostname", "halden-system\n", false, 14},
    {"os-release", "NAME=\"HaldenOS\"\nVERSION=\"1.0.0\"\nID=haldenos\n", false, 50},
    {"fstab", "/dev/sda1 / ext4 defaults 0 1\n", false, 31},
    {"hosts", "127.0.0.1 localhost\n::1 localhost\n", false, 35},
    {"resolv.conf", "nameserver 8.8.8.8\nnameserver 8.8.4.4\n", false, 40},
    {"cpuinfo", "", false, 0},
    {"meminfo", "", false, 0},
    {"uptime", "", false, 0},
    {"version", "", false, 0},
    {"loadavg", "", false, 0},
    {"stat", "", false, 0}
};

int disk_count = 1;
struct Disk detected_disks[4] = { {true, "sda1", 20480}, {false, "", 0} };

void build_cpuinfo() {
    char* buf = files[13].content;
    strcpy(buf, "processor\t: 0\nvendor_id\t: GenuineIntel\n");
    strcat(buf, "cpu family\t: 6\nmodel\t\t: 0\nmodel name\t: ");
    strcat(buf, cpu_brand_string);
    strcat(buf, "\n");
    strcat(buf, "stepping\t: 0\nmicrocode\t: 0x0\ncpu MHz\t\t: 2000.000\n");
    strcat(buf, "cache size\t: 8192 KB\nphysical id\t: 0\nsiblings\t: ");
    char tmp[32];
    uint_to_str(cpu_core_count, tmp);
    strcat(buf, tmp);
    strcat(buf, "\ncore id\t\t: 0\ncpu cores\t: ");
    strcat(buf, tmp);
    strcat(buf, "\nflags\t\t: fpu vme de pse tsc msr pae mce cx8 apic sep mtrr\n");
    files[13].size = strlen(buf);
}

void build_meminfo() {
    char* buf = files[14].content;
    char tmp[32];
    strcpy(buf, "MemTotal:       ");
    uint_to_str(total_memory_kb, tmp);
    strcat(buf, tmp);
    strcat(buf, " kB\nMemFree:        ");
    uint_to_str(free_memory_kb, tmp);
    strcat(buf, tmp);
    strcat(buf, " kB\nMemAvailable:   ");
    uint_to_str(free_memory_kb + (total_memory_kb / 10), tmp);
    strcat(buf, tmp);
    strcat(buf, " kB\nBuffers:        ");
    uint_to_str(total_memory_kb / 50, tmp);
    strcat(buf, tmp);
    strcat(buf, " kB\nCached:         ");
    uint_to_str(total_memory_kb / 10, tmp);
    strcat(buf, tmp);
    strcat(buf, " kB\nSwapTotal:      0 kB\nSwapFree:       0 kB\n");
    files[14].size = strlen(buf);
}

void build_uptime() {
    char* buf = files[15].content;
    char tmp[32];
    uint_to_str(uptime_seconds, tmp);
    strcpy(buf, tmp);
    strcat(buf, ".00 ");
    uint64_t idle = uptime_seconds * cpu_core_count;
    uint_to_str(idle, tmp);
    strcat(buf, tmp);
    strcat(buf, ".00\n");
    files[15].size = strlen(buf);
}

void build_version() {
    char* buf = files[16].content;
    strcpy(buf, "HaldenOS version 1.0.0-halden (root@halden-system) (gcc version 11.2.0) #1 SMP PREEMPT_DYNAMIC Sat Jan 10 00:00:00 UTC 2026\n");
    files[16].size = strlen(buf);
}

void build_loadavg() {
    char* buf = files[17].content;
    strcpy(buf, "0.00 0.01 0.05 1/128 ");
    char tmp[32];
    uint_to_str(uptime_seconds % 1000 + 100, tmp);
    strcat(buf, tmp);
    strcat(buf, "\n");
    files[17].size = strlen(buf);
}

void build_stat() {
    char* buf = files[18].content;
    char tmp[32];
    strcpy(buf, "cpu  ");
    uint_to_str(uptime_seconds * 10, tmp);
    strcat(buf, tmp);
    strcat(buf, " 0 ");
    uint_to_str(uptime_seconds * 5, tmp);
    strcat(buf, tmp);
    strcat(buf, " ");
    uint_to_str(uptime_seconds * 1000, tmp);
    strcat(buf, tmp);
    strcat(buf, " 0 0 0 0 0 0\nprocesses ");
    uint_to_str(uptime_seconds / 10 + 150, tmp);
    strcat(buf, tmp);
    strcat(buf, "\nprocs_running 1\nprocs_blocked 0\n");
    files[18].size = strlen(buf);
}

extern void list_dns_files(char* output);
extern bool read_dns_file(const char* filename, char* output);

void cmd_fetch(void) {
    terminal_write("\n    ___            _      _\n");
    terminal_write("   / __\\___  __ _| | __| | ___ _ __\n");
    terminal_write("  / _\\/ _ \\/ _` | |/ _` |/ _ \\ '_ \\\n");
    terminal_write(" / / |  __/ (_| | | (_| |  __/ | | |\n");
    terminal_write(" \\/   \\___|\\__,_|_|\\__,_|\\___|_| |_|\n\n");
    terminal_write(" OS:        HaldenOS 1.0.0\n");
    terminal_write(" Kernel:    1.0.0-halden\n");
    terminal_write(" Arch:      x86_64\n");
    terminal_write(" CPU:       "); terminal_write(cpu_brand_string); terminal_write("\n");
    terminal_write(" Cores:     ");
    char s[16]; uint_to_str(cpu_core_count, s); terminal_write(s);
    terminal_write("\n Memory:    ");
    uint_to_str(total_memory_kb/1024, s); terminal_write(s); terminal_write(" MB total, ");
    uint_to_str(free_memory_kb/1024, s); terminal_write(s); terminal_write(" MB free\n");
    terminal_write(" Disks:     ");
    uint_to_str(disk_count, s); terminal_write(s); terminal_write(" detected\n");
    terminal_write(" Uptime:    ");
    uint_to_str(uptime_seconds / 3600, s); terminal_write(s); terminal_write("h ");
    uint_to_str((uptime_seconds % 3600) / 60, s); terminal_write(s); terminal_write("m ");
    uint_to_str(uptime_seconds % 60, s); terminal_write(s); terminal_write("s\n");
    terminal_write(" hlpkg:     ");
    uint_to_str(hlpkg_get_package_count(), s); terminal_write(s); terminal_write(" packages, ");
    uint_to_str(hlpkg_get_running_count(), s); terminal_write(s); terminal_write(" running\n");
    terminal_write(" ports:     ");
    uint_to_str(port_get_active_count(), s); terminal_write(s); terminal_write(" active\n\n");
}

void cmd_cd(const char* arg) {
    if(!arg || !strlen(arg)) {
        strcpy(current_directory, "/");
        return;
    }
    
    if(strcmp(arg, "..") == 0) {
        if(strcmp(current_directory, "/") == 0) return;
        
        int len = strlen(current_directory);
        for(int i = len - 1; i >= 0; i--) {
            if(current_directory[i] == '/') {
                if(i == 0) {
                    strcpy(current_directory, "/");
                } else {
                    current_directory[i] = '\0';
                }
                return;
            }
        }
        strcpy(current_directory, "/");
        return;
    }
    
    if(strcmp(arg, "/") == 0) {
        strcpy(current_directory, "/");
    } else if(strcmp(arg, "dev") == 0 || strcmp(arg, "/dev") == 0) {
        strcpy(current_directory, "/dev");
    } else if(strcmp(arg, "etc") == 0 || strcmp(arg, "/etc") == 0) {
        strcpy(current_directory, "/etc");
    } else if(strcmp(arg, "proc") == 0 || strcmp(arg, "/proc") == 0) {
        strcpy(current_directory, "/proc");
    } else if(strcmp(arg, "dns") == 0 || strcmp(arg, "/dns") == 0) {
        strcpy(current_directory, "/dns");
    } else if(strcmp(arg, "bin") == 0 || strcmp(arg, "/bin") == 0) {
        strcpy(current_directory, "/bin");
    } else if(strcmp(arg, "boot") == 0 || strcmp(arg, "/boot") == 0) {
        strcpy(current_directory, "/boot");
    } else if(strcmp(arg, "home") == 0 || strcmp(arg, "/home") == 0) {
        strcpy(current_directory, "/home");
    } else if(strcmp(arg, "lib") == 0 || strcmp(arg, "/lib") == 0) {
        strcpy(current_directory, "/lib");
    } else if(strcmp(arg, "mnt") == 0 || strcmp(arg, "/mnt") == 0) {
        strcpy(current_directory, "/mnt");
    } else if(strcmp(arg, "opt") == 0 || strcmp(arg, "/opt") == 0) {
        strcpy(current_directory, "/opt");
    } else if(strcmp(arg, "root") == 0 || strcmp(arg, "/root") == 0) {
        strcpy(current_directory, "/root");
    } else if(strcmp(arg, "run") == 0 || strcmp(arg, "/run") == 0) {
        strcpy(current_directory, "/run");
    } else if(strcmp(arg, "srv") == 0 || strcmp(arg, "/srv") == 0) {
        strcpy(current_directory, "/srv");
    } else if(strcmp(arg, "sys") == 0 || strcmp(arg, "/sys") == 0) {
        strcpy(current_directory, "/sys");
    } else if(strcmp(arg, "tmp") == 0 || strcmp(arg, "/tmp") == 0) {
        strcpy(current_directory, "/tmp");
    } else if(strcmp(arg, "usr") == 0 || strcmp(arg, "/usr") == 0) {
        strcpy(current_directory, "/usr");
    } else if(strcmp(arg, "var") == 0 || strcmp(arg, "/var") == 0) {
        strcpy(current_directory, "/var");
    } else {
        terminal_write("cd: ");
        terminal_write(arg);
        terminal_write(": No such file or directory\n");
    }
}

void cmd_ls(const char* arg) {
    if(!arg || !strlen(arg) || strcmp(arg, ".") == 0) {
        if(strcmp(current_directory, "/") == 0) {
            terminal_write("bin  boot  dev  dns  etc  home  lib  mnt  opt  proc  root  run  srv  sys  tmp  usr  var\n");
        } else if(strcmp(current_directory, "/dev") == 0) {
            for(int i = 0; i < 6; i++) { 
                terminal_write(files[i].name); 
                terminal_write("  "); 
            }
            terminal_write("\n");
        } else if(strcmp(current_directory, "/etc") == 0) {
            for(int i = 6; i < 13; i++) { 
                terminal_write(files[i].name); 
                terminal_write("  "); 
            }
            terminal_write("\n");
        } else if(strcmp(current_directory, "/proc") == 0) {
            terminal_write("cpuinfo  meminfo  uptime  version  loadavg  stat\n");
        } else if(strcmp(current_directory, "/dns") == 0) {
            char dns_list[512];
            list_dns_files(dns_list);
            terminal_write(dns_list);
            terminal_write("\n");
        } else {
            terminal_write("\n");
        }
    } else if(strcmp(arg, "-l") == 0 || strcmp(arg, "-la") == 0) {
        terminal_write("total 0\n");
        if(strcmp(current_directory, "/") == 0) {
            terminal_write("drwxr-xr-x  2 root root 4096 Jan 10 00:00 bin\n");
            terminal_write("drwxr-xr-x  2 root root 4096 Jan 10 00:00 boot\n");
            terminal_write("drwxr-xr-x  2 root root 4096 Jan 10 00:00 dev\n");
            terminal_write("drwxr-xr-x  2 root root 4096 Jan 10 00:00 dns\n");
            terminal_write("drwxr-xr-x  2 root root 4096 Jan 10 00:00 etc\n");
            terminal_write("drwxr-xr-x  2 root root 4096 Jan 10 00:00 home\n");
            terminal_write("drwxr-xr-x  2 root root 4096 Jan 10 00:00 lib\n");
        } else if(strcmp(current_directory, "/dev") == 0) {
            for(int i = 0; i < 6; i++) {
                terminal_write("crw-rw-rw-  1 root root 0 Jan 10 00:00 ");
                terminal_write(files[i].name);
                terminal_write("\n");
            }
        }
    } else {
        terminal_write(arg);
        terminal_write(": No such file or directory\n");
    }
}

void cmd_cat(const char* arg) {
    if(!arg || !strlen(arg)) {
        terminal_write("cat: missing operand\n");
        return;
    }
    
    if(strncmp(arg, "/dns/", 5) == 0 || (strcmp(current_directory, "/dns") == 0)) {
        const char* filename = arg;
        if(strncmp(arg, "/dns/", 5) == 0) {
            filename = arg + 5;
        }
        
        char content[4096];
        if(read_dns_file(filename, content)) {
            terminal_write(content);
            return;
        }
    }
    
    if(strcmp(arg, "cpuinfo") == 0 || strcmp(arg, "/proc/cpuinfo") == 0) {
        build_cpuinfo();
        terminal_write(files[13].content);
        return;
    }
    if(strcmp(arg, "meminfo") == 0 || strcmp(arg, "/proc/meminfo") == 0) {
        build_meminfo();
        terminal_write(files[14].content);
        return;
    }
    if(strcmp(arg, "uptime") == 0 || strcmp(arg, "/proc/uptime") == 0) {
        build_uptime();
        terminal_write(files[15].content);
        return;
    }
    if(strcmp(arg, "version") == 0 || strcmp(arg, "/proc/version") == 0) {
        build_version();
        terminal_write(files[16].content);
        return;
    }
    if(strcmp(arg, "loadavg") == 0 || strcmp(arg, "/proc/loadavg") == 0) {
        build_loadavg();
        terminal_write(files[17].content);
        return;
    }
    if(strcmp(arg, "stat") == 0 || strcmp(arg, "/proc/stat") == 0) {
        build_stat();
        terminal_write(files[18].content);
        return;
    }
    
    for(int i = 0; i < FILE_COUNT; i++) {
        if(strcmp(files[i].name, arg) == 0) {
            terminal_write(files[i].content);
            return;
        }
    }
    terminal_write("cat: ");
    terminal_write(arg);
    terminal_write(": No such file or directory\n");
}

void cmd_pwd(void) { 
    terminal_write(current_directory); 
    terminal_write("\n"); 
}

void cmd_whoami(void) { 
    terminal_write("root\n"); 
}

void cmd_hostname(void) { 
    terminal_write("halden-system\n"); 
}

void cmd_echo(const char* arg) {
    if(arg) terminal_write(arg);
    terminal_write("\n");
}

void cmd_uname(const char* arg) {
    if(!arg || !strlen(arg)) {
        terminal_write("HaldenOS\n");
    } else if(strcmp(arg, "-a") == 0) {
        terminal_write("HaldenOS 1.0.0-halden halden-system 1.0.0 x86_64 GNU/Linux\n");
    } else if(strcmp(arg, "-r") == 0) {
        terminal_write("1.0.0-halden\n");
    } else if(strcmp(arg, "-m") == 0) {
        terminal_write("x86_64\n");
    } else if(strcmp(arg, "-s") == 0) {
        terminal_write("HaldenOS\n");
    } else if(strcmp(arg, "-n") == 0) {
        terminal_write("halden-system\n");
    } else if(strcmp(arg, "-v") == 0) {
        terminal_write("#1 SMP PREEMPT_DYNAMIC Sat Jan 10 00:00:00 UTC 2026\n");
    }
}

void cmd_df(void) {
    terminal_write("Filesystem     1K-blocks      Used Available Use% Mounted on\n");
    for(int i = 0; i < disk_count; i++) {
        if(detected_disks[i].exists) {
            terminal_write("/dev/");
            terminal_write(detected_disks[i].name);
            terminal_write("   ");
            char s[16];
            uint_to_str(detected_disks[i].size_mb * 1024, s);
            terminal_write(s);
            terminal_write("   ");
            uint64_t used = detected_disks[i].size_mb * 1024 * 15 / 100;
            uint_to_str(used, s);
            terminal_write(s);
            terminal_write("   ");
            uint_to_str(detected_disks[i].size_mb * 1024 - used, s);
            terminal_write(s);
            terminal_write("  15% /\n");
        }
    }
    terminal_write("tmpfs          ");
    char s[16];
    uint_to_str(total_memory_kb / 2, s);
    terminal_write(s);
    terminal_write("         0   ");
    terminal_write(s);
    terminal_write("   0% /dev/shm\n");
}

void cmd_free(const char* arg) {
    bool human = arg && (strcmp(arg, "-h") == 0 || strcmp(arg, "--human") == 0);
    
    terminal_write("               total        used        free      shared  buff/cache   available\n");
    terminal_write("Mem:        ");
    char s[16];
    if(human) {
        uint_to_str(total_memory_kb / 1024, s);
        terminal_write(s);
        terminal_write("Mi      ");
        uint64_t used = total_memory_kb - free_memory_kb;
        uint_to_str(used / 1024, s);
        terminal_write(s);
        terminal_write("Mi      ");
        uint_to_str(free_memory_kb / 1024, s);
        terminal_write(s);
        terminal_write("Mi        0Mi     ");
        uint_to_str((total_memory_kb / 10) / 1024, s);
        terminal_write(s);
        terminal_write("Mi      ");
        uint_to_str((free_memory_kb + total_memory_kb / 10) / 1024, s);
        terminal_write(s);
        terminal_write("Mi\n");
    } else {
        uint_to_str(total_memory_kb, s);
        terminal_write(s);
        terminal_write("     ");
        uint64_t used = total_memory_kb - free_memory_kb;
        uint_to_str(used, s);
        terminal_write(s);
        terminal_write("     ");
        uint_to_str(free_memory_kb, s);
        terminal_write(s);
        terminal_write("         0       ");
        uint_to_str(total_memory_kb / 10, s);
        terminal_write(s);
        terminal_write("     ");
        uint_to_str(free_memory_kb + total_memory_kb / 10, s);
        terminal_write(s);
        terminal_write("\n");
    }
    terminal_write("Swap:              0           0           0\n");
}

void cmd_ps(void) {
    terminal_write("  PID TTY          TIME CMD\n");
    
    uint32_t hlpkg_pids[64];
    int hlpkg_count = hlpkg_get_process_list(hlpkg_pids, 64);
    
    char s[64];
    for(int i = 0; i < hlpkg_count; i++) {
        char name[64];
        HLPKGStatus status;
        uint32_t mem;
        uint64_t time;
        if(hlpkg_get_process_info(hlpkg_pids[i], name, &status, &mem, &time)) {
            terminal_write(" ");
            uint_to_str(hlpkg_pids[i], s);
            terminal_write(s);
            terminal_write(" tty0     00:00:00 ");
            terminal_write(name);
            terminal_write("\n");
        }
    }
    
    uint32_t port_pids[32];
    int port_count = port_get_process_list(port_pids, 32);
    
    for(int i = 0; i < port_count; i++) {
        char name[64];
        PortStatus status;
        uint32_t mem;
        if(port_get_process_info(port_pids[i], name, &status, &mem)) {
            terminal_write(" ");
            uint_to_str(port_pids[i], s);
            terminal_write(s);
            terminal_write(" tty0     00:00:00 ");
            terminal_write(name);
            terminal_write(" (port)\n");
        }
    }
}

void cmd_hlpkg_install(const char* arg) {
    if(!arg || !strlen(arg)) {
        terminal_write("hlpkg: install: missing package file\n");
        terminal_write("Usage: hlpkg install <package.hlpkg>\n");
        return;
    }
    
    if(strncmp(arg + strlen(arg) - 6, ".hlpkg", 6) != 0) {
        terminal_write("hlpkg: error: not a .hlpkg file\n");
        return;
    }
    
    terminal_write("Installing package: ");
    terminal_write(arg);
    terminal_write("\n");
    
    int pkg_id = hlpkg_load(arg);
    if(pkg_id < 0) {
        terminal_write("hlpkg: error: failed to load package\n");
        return;
    }
    
    char name[64], version[16], author[64];
    uint32_t size;
    if(hlpkg_get_package_info(pkg_id, name, version, author, &size)) {
        terminal_write("Package: ");
        terminal_write(name);
        terminal_write(" v");
        terminal_write(version);
        terminal_write(" by ");
        terminal_write(author);
        terminal_write("\n");
        
        add_installed_app(name, 1);
        refresh_all_windows();
        
        terminal_write("Successfully installed!\n");
    } else {
        terminal_write("hlpkg: error: failed to get package info\n");
    }
}

void cmd_hlpkg_run(const char* arg) {
    if(!arg || !strlen(arg)) {
        terminal_write("hlpkg: run: missing package name\n");
        terminal_write("Usage: hlpkg run <package-name>\n");
        return;
    }
    
    int pkg_id = hlpkg_find_package(arg);
    if(pkg_id < 0) {
        terminal_write("hlpkg: error: package not found: ");
        terminal_write(arg);
        terminal_write("\n");
        return;
    }
    
    int pid = hlpkg_execute(pkg_id);
    if(pid < 0) {
        terminal_write("hlpkg: error: failed to execute package\n");
        return;
    }
    
    terminal_write("Started process with PID: ");
    char s[16];
    uint_to_str(pid, s);
    terminal_write(s);
    terminal_write("\n");
}

void cmd_hlpkg_list(void) {
    terminal_write("Installed hlpkg packages:\n");
    
    int count = hlpkg_get_package_count();
    if(count == 0) {
        terminal_write("  No packages installed\n");
        return;
    }
    
    char s[16];
    uint_to_str(count, s);
    terminal_write("  Total: ");
    terminal_write(s);
    terminal_write(" package(s)\n");
}

void cmd_hlpkg(const char* arg) {
    if(!arg || !strlen(arg)) {
        terminal_write("hlpkg - HaldenOS Package Manager\n");
        terminal_write("Usage:\n");
        terminal_write("  hlpkg install <file.hlpkg>  Install a package\n");
        terminal_write("  hlpkg run <name>            Run an installed package\n");
        terminal_write("  hlpkg list                  List installed packages\n");
        return;
    }
    
    if(strncmp(arg, "install ", 8) == 0) {
        cmd_hlpkg_install(arg + 8);
    } else if(strncmp(arg, "run ", 4) == 0) {
        cmd_hlpkg_run(arg + 4);
    } else if(strcmp(arg, "list") == 0) {
        cmd_hlpkg_list();
    } else {
        terminal_write("hlpkg: unknown command: ");
        terminal_write(arg);
        terminal_write("\n");
    }
}

void cmd_ports_install(const char* arg) {
    if(!arg || !strlen(arg)) {
        terminal_write("ports: install: missing application file\n");
        terminal_write("Usage: ports install <app.AppImage>\n");
        return;
    }
    
    if(strncmp(arg + strlen(arg) - 9, ".AppImage", 9) != 0) {
        terminal_write("ports: error: not a .AppImage file\n");
        return;
    }
    
    terminal_write("Installing Linux application: ");
    terminal_write(arg);
    terminal_write("\n");
    
    int pid = port_load_elf(arg);
    if(pid < 0) {
        terminal_write("ports: error: failed to load application\n");
        return;
    }
    
    char app_name[64];
    strcpy(app_name, arg);
    int len = strlen(app_name);
    if(len > 9) {
        app_name[len - 9] = '\0';
    }
    
    add_installed_app(app_name, 2);
    refresh_all_windows();
    
    terminal_write("Successfully installed and started!\n");
    terminal_write("Process PID: ");
    char s[16];
    uint_to_str(pid, s);
    terminal_write(s);
    terminal_write("\n");
}

void cmd_ports_list(void) {
    terminal_write("Running ported applications:\n");
    
    int count = port_get_active_count();
    if(count == 0) {
        terminal_write("  No ported apps running\n");
        return;
    }
    
    char s[16];
    uint_to_str(count, s);
    terminal_write("  Total: ");
    terminal_write(s);
    terminal_write(" app(s)\n");
}

void cmd_ports(const char* arg) {
    if(!arg || !strlen(arg)) {
        terminal_write("ports - Linux Application Compatibility Layer\n");
        terminal_write("Usage:\n");
        terminal_write("  ports install <app.AppImage>  Install a Linux app\n");
        terminal_write("  ports list                    List running apps\n");
        return;
    }
    
    if(strncmp(arg, "install ", 8) == 0) {
        cmd_ports_install(arg + 8);
    } else if(strcmp(arg, "list") == 0) {
        cmd_ports_list();
    } else {
        terminal_write("ports: unknown command: ");
        terminal_write(arg);
        terminal_write("\n");
    }
}

void cmd_help(void) {
    terminal_write("Available commands:\n");
    terminal_write(" System Info:       fetch, uname, hostname, uptime\n");
    terminal_write(" Files:             ls, cd, pwd, cat\n");
    terminal_write(" Text:              echo\n");
    terminal_write(" Hardware:          df, free\n");
    terminal_write(" Processes:         ps\n");
    terminal_write(" Network:           ping\n");
    terminal_write(" Packages:          hlpkg, ports\n");
    terminal_write(" User:              whoami\n");
}

extern bool network_ping(const char* host, char* output);

void cmd_ping(const char* arg) {
    if(!arg || !strlen(arg)) {
        terminal_write("ping: usage: ping <host>\n");
        return;
    }
    
    char output[1024];
    network_ping(arg, output);
    terminal_write(output);
}

void process_command(const char* cmd) {
    if(strcmp(cmd, "fetch") == 0) cmd_fetch();
    else if(strcmp(cmd, "ls") == 0) cmd_ls(0);
    else if(strncmp(cmd, "ls ", 3) == 0) cmd_ls(cmd + 3);
    else if(strcmp(cmd, "cd") == 0) cmd_cd(0);
    else if(strncmp(cmd, "cd ", 3) == 0) cmd_cd(cmd + 3);
    else if(strcmp(cmd, "pwd") == 0) cmd_pwd();
    else if(strncmp(cmd, "cat ", 4) == 0) cmd_cat(cmd + 4);
    else if(strncmp(cmd, "echo ", 5) == 0) cmd_echo(cmd + 5);
    else if(strcmp(cmd, "whoami") == 0) cmd_whoami();
    else if(strcmp(cmd, "hostname") == 0) cmd_hostname();
    else if(strcmp(cmd, "uname") == 0) cmd_uname(0);
    else if(strncmp(cmd, "uname ", 6) == 0) cmd_uname(cmd + 6);
    else if(strcmp(cmd, "df") == 0) cmd_df();
    else if(strcmp(cmd, "free") == 0) cmd_free(0);
    else if(strncmp(cmd, "free ", 5) == 0) cmd_free(cmd + 5);
    else if(strncmp(cmd, "ping ", 5) == 0) cmd_ping(cmd + 5);
    else if(strcmp(cmd, "ps") == 0) cmd_ps();
    else if(strcmp(cmd, "hlpkg") == 0) cmd_hlpkg(0);
    else if(strncmp(cmd, "hlpkg ", 6) == 0) cmd_hlpkg(cmd + 6);
    else if(strcmp(cmd, "ports") == 0) cmd_ports(0);
    else if(strncmp(cmd, "ports ", 6) == 0) cmd_ports(cmd + 6);
    else if(strcmp(cmd, "help") == 0) cmd_help();
    else if(strcmp(cmd, "") != 0) {
        terminal_write("bash: ");
        terminal_write(cmd);
        terminal_write(": command not found\n");
    }
}