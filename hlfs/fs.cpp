#include <stdint.h>
#include <stddef.h>

extern void* memcpy(void *dest, const void *src, size_t n);
extern void* memset(void *s, int c, size_t n);
extern size_t strlen(const char *str);
extern int strcmp(const char *s1, const char *s2);
extern int strncmp(const char *s1, const char *s2, size_t n);
extern char* strcpy(char *dest, const char *src);
extern char* strcat(char *dest, const char *src);
extern uint64_t uptime_seconds;

#define MAX_FILES 1024
#define MAX_PATH 1024
#define MAX_CONTENT_SIZE 8192

enum FileType {
    FILE_REGULAR = 0,
    FILE_DIRECTORY = 1,
    FILE_DEVICE = 2,
    FILE_SOURCE = 3
};

struct FSNode {
    char name[64];
    char path[MAX_PATH];
    FileType type;
    uint64_t size;
    char* content;
    uint32_t permissions;
    uint64_t created_time;
    uint64_t modified_time;
    bool in_use;
    int parent_index;
};

FSNode filesystem[MAX_FILES];
int fs_node_count = 0;
bool hlfs_enabled = false;
char content_storage_data[MAX_FILES][MAX_CONTENT_SIZE];
bool content_storage_used[MAX_FILES];

int find_node_by_path(const char* path) {
    for(int i = 0; i < fs_node_count; i++) {
        if(filesystem[i].in_use && strcmp(filesystem[i].path, path) == 0) {
            return i;
        }
    }
    return -1;
}

int create_node(const char* path, const char* name, FileType type, const char* content, int parent_idx, int explicit_size = -1) {
    if(fs_node_count >= MAX_FILES) return -1;
    
    int idx = fs_node_count++;
    filesystem[idx].in_use = true;
    strcpy(filesystem[idx].name, name);
    strcpy(filesystem[idx].path, path);
    filesystem[idx].type = type;
    filesystem[idx].permissions = 0755;
    filesystem[idx].created_time = uptime_seconds;
    filesystem[idx].modified_time = uptime_seconds;
    filesystem[idx].parent_index = parent_idx;
    
    if(content && (type == FILE_REGULAR || type == FILE_SOURCE)) {
        int len;
        if (explicit_size != -1) {
            len = explicit_size;
        } else {
            len = strlen(content);
        }

        if (len > MAX_CONTENT_SIZE) len = MAX_CONTENT_SIZE;

        filesystem[idx].size = len;
        content_storage_used[idx] = true;
        memcpy(content_storage_data[idx], content, len);
        filesystem[idx].content = content_storage_data[idx];
    } else {
        filesystem[idx].size = 4096;
        filesystem[idx].content = nullptr;
        content_storage_used[idx] = false;
    }
    
    return idx;
}

void init_hlfs() {
    fs_node_count = 0;
    memset(filesystem, 0, sizeof(filesystem));
    memset(content_storage_used, 0, sizeof(content_storage_used));
    
    int root = create_node("/", "/", FILE_DIRECTORY, nullptr, -1);
    
    int bin = create_node("/bin", "bin", FILE_DIRECTORY, nullptr, root);
    int boot = create_node("/boot", "boot", FILE_DIRECTORY, nullptr, root);
    int dev = create_node("/dev", "dev", FILE_DIRECTORY, nullptr, root);
    int etc = create_node("/etc", "etc", FILE_DIRECTORY, nullptr, root);
    int home = create_node("/home", "home", FILE_DIRECTORY, nullptr, root);
    create_node("/lib", "lib", FILE_DIRECTORY, nullptr, root);
    create_node("/mnt", "mnt", FILE_DIRECTORY, nullptr, root);
    create_node("/opt", "opt", FILE_DIRECTORY, nullptr, root);
    int proc = create_node("/proc", "proc", FILE_DIRECTORY, nullptr, root);
    int rootdir = create_node("/root", "root", FILE_DIRECTORY, nullptr, root);
    create_node("/run", "run", FILE_DIRECTORY, nullptr, root);
    create_node("/srv", "srv", FILE_DIRECTORY, nullptr, root);
    create_node("/sys", "sys", FILE_DIRECTORY, nullptr, root);
    create_node("/tmp", "tmp", FILE_DIRECTORY, nullptr, root);
    create_node("/usr", "usr", FILE_DIRECTORY, nullptr, root);
    int var = create_node("/var", "var", FILE_DIRECTORY, nullptr, root);
    create_node("/dns", "dns", FILE_DIRECTORY, nullptr, root);
    int hlfs_dir = create_node("/hlfs", "hlfs", FILE_DIRECTORY, nullptr, root);
    int halden_dir = create_node("/halden", "halden", FILE_DIRECTORY, nullptr, root);
    
    create_node("/bin/bash", "bash", FILE_REGULAR, "ELF executable - Bash shell", bin);
    create_node("/bin/ls", "ls", FILE_REGULAR, "ELF executable - List directory", bin);
    create_node("/bin/cat", "cat", FILE_REGULAR, "ELF executable - Concatenate files", bin);
    create_node("/bin/echo", "echo", FILE_REGULAR, "ELF executable - Echo text", bin);
    create_node("/bin/grep", "grep", FILE_REGULAR, "ELF executable - Search text", bin);
    create_node("/bin/ps", "ps", FILE_REGULAR, "ELF executable - Process status", bin);
    create_node("/bin/kill", "kill", FILE_REGULAR, "ELF executable - Kill process", bin);
    
    create_node("/dev/tty0", "tty0", FILE_DEVICE, "Character device", dev);
    create_node("/dev/tty1", "tty1", FILE_DEVICE, "Character device", dev);
    create_node("/dev/sda", "sda", FILE_DEVICE, "Block device", dev);
    create_node("/dev/sda1", "sda1", FILE_DEVICE, "Block device", dev);
    create_node("/dev/null", "null", FILE_DEVICE, "Null device", dev);
    create_node("/dev/zero", "zero", FILE_DEVICE, "Zero device", dev);
    create_node("/dev/random", "random", FILE_DEVICE, "Random device", dev);
    create_node("/dev/urandom", "urandom", FILE_DEVICE, "Random device", dev);
    
    create_node("/etc/passwd", "passwd", FILE_REGULAR, 
        "root:x:0:0:root:/root:/bin/bash\nhalden:x:1000:1000:Halden User:/home/halden:/bin/bash\n", etc);
    create_node("/etc/shadow", "shadow", FILE_REGULAR, 
        "root:!:19000:0:99999:7:::\nhalden:!:19000:0:99999:7:::\n", etc);
    create_node("/etc/group", "group", FILE_REGULAR,
        "root:x:0:\nwheel:x:10:halden\nusers:x:100:halden\n", etc);
    create_node("/etc/hostname", "hostname", FILE_REGULAR, 
        "halden-system\n", etc);
    create_node("/etc/os-release", "os-release", FILE_REGULAR,
        "NAME=\"HaldenOS\"\nVERSION=\"1.0.0\"\nID=haldenos\nID_LIKE=linux\nPRETTY_NAME=\"HaldenOS 1.0.0\"\nVERSION_ID=\"1.0.0\"\nVERSION_CODENAME=halden\nFILESYSTEM=\"HLFS\"\nKERNEL=\"1.0.0-halden\"\nARCHITECTURE=\"x86_64\"\nBUILD_DATE=\"2026-01-14\"\n", etc);
    create_node("/etc/fstab", "fstab", FILE_REGULAR,
        "/dev/sda1 / hlfs defaults 0 1\ntmpfs /dev/shm tmpfs defaults 0 0\ntmpfs /tmp tmpfs defaults 0 0\n", etc);
    create_node("/etc/hosts", "hosts", FILE_REGULAR,
        "127.0.0.1 localhost\n::1 localhost\n127.0.1.1 halden-system\n192.168.1.1 gateway\n", etc);
    create_node("/etc/resolv.conf", "resolv.conf", FILE_REGULAR,
        "nameserver 8.8.8.8\nnameserver 8.8.4.4\nnameserver 1.1.1.1\nsearch localdomain\n", etc);
    create_node("/etc/profile", "profile", FILE_REGULAR,
        "export PATH=/bin:/usr/bin:/usr/local/bin\nexport EDITOR=vi\nexport SHELL=/bin/bash\n", etc);
    create_node("/etc/bashrc", "bashrc", FILE_REGULAR,
        "alias ls='ls --color=auto'\nalias ll='ls -lah'\nalias grep='grep --color=auto'\nPS1='\\u@\\h:\\w\\$ '\n", etc);
    create_node("/etc/issue", "issue", FILE_REGULAR,
        "HaldenOS 1.0.0 \\n \\l\n\n", etc);
    create_node("/etc/motd", "motd", FILE_REGULAR,
        "Welcome to HaldenOS 1.0.0\n\nDocumentation: https://docs.halden.os\nSupport: https://support.halden.os\n\n", etc);
    
    create_node("/boot/haldenos.elf", "haldenos.elf", FILE_REGULAR, 
        "ELF 64-bit LSB executable, x86-64, version 1 (SYSV)", boot);
    create_node("/boot/limine.cfg", "limine.cfg", FILE_REGULAR,
        "TIMEOUT=0\n\n:HaldenOS\n    PROTOCOL=limine\n    KERNEL_PATH=boot:///haldenos.elf\n", boot);
    create_node("/boot/grub.cfg", "grub.cfg", FILE_REGULAR,
        "set timeout=0\nset default=0\n\nmenuentry \"HaldenOS\" {\n    multiboot /boot/haldenos.elf\n    boot\n}\n", boot);
    
    int proc_sys = create_node("/proc/sys", "sys", FILE_DIRECTORY, nullptr, proc);
    create_node("/proc/cpuinfo", "cpuinfo", FILE_REGULAR, "CPU info placeholder", proc);
    create_node("/proc/meminfo", "meminfo", FILE_REGULAR, "Memory info placeholder", proc);
    create_node("/proc/uptime", "uptime", FILE_REGULAR, "Uptime placeholder", proc);
    create_node("/proc/version", "version", FILE_REGULAR, "Version placeholder", proc);
    create_node("/proc/mounts", "mounts", FILE_REGULAR, "/dev/sda1 / hlfs rw 0 0\n", proc);
    create_node("/proc/sys/kernel.hostname", "kernel.hostname", FILE_REGULAR, "halden-system\n", proc_sys);
    
    int var_log = create_node("/var/log", "log", FILE_DIRECTORY, nullptr, var);
    create_node("/var/tmp", "tmp", FILE_DIRECTORY, nullptr, var);
    create_node("/var/log/syslog", "syslog", FILE_REGULAR, 
        "[2026-01-14 10:00:00] System boot initiated\n[2026-01-14 10:00:01] HLFS mounted successfully\n[2026-01-14 10:00:02] Network initialized\n[2026-01-14 10:00:03] hlpkg system initialized\n[2026-01-14 10:00:04] Port system initialized\n", var_log);
    create_node("/var/log/boot.log", "boot.log", FILE_REGULAR,
        "Starting HaldenOS boot sequence...\nLoading kernel modules...\nInitializing hardware...\nInitializing hlpkg system...\nInitializing port compatibility layer...\nBoot complete.\n", var_log);
    
    int home_halden = create_node("/home/halden", "halden", FILE_DIRECTORY, nullptr, home);
    create_node("/home/halden/.bashrc", ".bashrc", FILE_REGULAR,
        "alias ls='ls --color=auto'\nalias ll='ls -la'\nexport PS1='\\u@\\h:\\w\\$ '\n", home_halden);
    create_node("/home/halden/.profile", ".profile", FILE_REGULAR,
        "export PATH=$HOME/bin:/usr/local/bin:$PATH\n", home_halden);
    create_node("/home/halden/README.txt", "README.txt", FILE_REGULAR,
        "Welcome to HaldenOS!\n\nThis is your home directory.\nYou can create files and folders here.\n\nEnjoy!\n", home_halden);
    
    create_node("/root/.bashrc", ".bashrc", FILE_REGULAR,
        "alias ls='ls --color=auto'\nalias ll='ls -la'\nexport PS1='\\u@\\h:\\w# '\n", rootdir);
    create_node("/root/README.txt", "README.txt", FILE_REGULAR,
        "Root user home directory\n", rootdir);
    
    int src_dir = create_node("/hlfs/src", "src", FILE_DIRECTORY, nullptr, hlfs_dir);
    int system_dir = create_node("/hlfs/src/system", "system", FILE_DIRECTORY, nullptr, src_dir);
    int apps_dir = create_node("/hlfs/src/apps", "apps", FILE_DIRECTORY, nullptr, src_dir);
    int hlpkg_src_dir = create_node("/hlfs/src/hlpkg", "hlpkg", FILE_DIRECTORY, nullptr, src_dir);
    int port_src_dir = create_node("/hlfs/src/port", "port", FILE_DIRECTORY, nullptr, src_dir);
    
    create_node("/hlfs/src/kernel.cpp", "kernel.cpp", FILE_SOURCE,
        "#include <stdint.h>\n#include <stddef.h>\n#include \"limine.h\"\n\nextern void terminal_init();\nextern void mouse_init();\nextern void network_init();\nextern void init_hlfs();\nextern void init_hlpkg_system();\nextern void init_port_system();\n\nextern \"C\" void _start(void) {\n    terminal_init();\n    mouse_init();\n    network_init();\n    init_hlfs();\n    init_hlpkg_system();\n    init_port_system();\n    while(1);\n}\n", src_dir);
    
    create_node("/hlfs/src/system/terminal.cpp", "terminal.cpp", FILE_SOURCE,
        "#include <stdint.h>\n\nvoid terminal_init() {\n    // Initialize terminal\n}\n\nvoid terminal_write(const char* str) {\n    // Write to terminal\n}\n", system_dir);
    
    create_node("/hlfs/src/system/commands.cpp", "commands.cpp", FILE_SOURCE,
        "#include <stdint.h>\n\nvoid process_command(const char* cmd) {\n    // Process shell command\n}\n", system_dir);
    
    create_node("/hlfs/src/system/network.cpp", "network.cpp", FILE_SOURCE,
        "#include <stdint.h>\n\nvoid network_init() {\n    // Initialize network stack\n}\n", system_dir);
    
    create_node("/hlfs/src/system/applications.cpp", "applications.cpp", FILE_SOURCE,
        "#include <stdint.h>\n\nvoid init_application_system() {\n    // Initialize GUI applications\n}\n", system_dir);
    
    create_node("/hlfs/src/hlfs/fs.cpp", "fs.cpp", FILE_SOURCE,
        "#include <stdint.h>\n\nvoid init_hlfs() {\n    // Initialize HLFS filesystem\n}\n", system_dir);
    
    create_node("/hlfs/src/apps/browser.cpp", "browser.cpp", FILE_SOURCE,
        "#include <stdint.h>\n\nvoid open_browser() {\n    // Open web browser\n}\n", apps_dir);
    
    create_node("/hlfs/src/apps/terminal.cpp", "terminal.cpp", FILE_SOURCE,
        "#include <stdint.h>\n\nvoid open_terminal() {\n    // Open terminal app\n}\n", apps_dir);
    
    create_node("/hlfs/src/apps/filemanager.cpp", "filemanager.cpp", FILE_SOURCE,
        "#include <stdint.h>\n\nvoid open_filemanager() {\n    // Open file manager\n}\n", apps_dir);
    
    create_node("/hlfs/src/hlpkg/hlpkg.cpp", "hlpkg.cpp", FILE_SOURCE,
        "#include <stdint.h>\n\nvoid init_hlpkg_system() {\n    // Initialize hlpkg package management\n}\n\nint hlpkg_load(const char* path) {\n    // Load hlpkg package\n    return 0;\n}\n\nint hlpkg_execute(int package_id) {\n    // Execute hlpkg package\n    return 0;\n}\n", hlpkg_src_dir);
    
    create_node("/hlfs/src/port/port.cpp", "port.cpp", FILE_SOURCE,
        "#include <stdint.h>\n\nvoid init_port_system() {\n    // Initialize Linux compatibility layer\n}\n\nint port_load_elf(const char* path) {\n    // Load ELF binary\n    return 0;\n}\n\nuint64_t port_handle_syscall(uint32_t pid, uint64_t syscall_num, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6) {\n    // Translate Linux syscall\n    return 0;\n}\n", port_src_dir);
    
    int halden_lib = create_node("/halden/lib", "lib", FILE_DIRECTORY, nullptr, halden_dir);
    int halden_bin = create_node("/halden/bin", "bin", FILE_DIRECTORY, nullptr, halden_dir);
    int halden_etc = create_node("/halden/etc", "etc", FILE_DIRECTORY, nullptr, halden_dir);
    int halden_usr = create_node("/halden/usr", "usr", FILE_DIRECTORY, nullptr, halden_dir);
    
    create_node("/halden/lib/halden_libc.so", "halden_libc.so", FILE_REGULAR, "Halden C library", halden_lib);
    create_node("/halden/lib/halden_libm.so", "halden_libm.so", FILE_REGULAR, "Halden Math library", halden_lib);
    create_node("/halden/lib/halden_pthread.so", "halden_pthread.so", FILE_REGULAR, "Halden Pthread library", halden_lib);
    create_node("/halden/lib/halden_x11.so", "halden_x11.so", FILE_REGULAR, "Halden X11 library", halden_lib);
    create_node("/halden/lib/halden_gl.so", "halden_gl.so", FILE_REGULAR, "Halden OpenGL library", halden_lib);
    
    create_node("/halden/README.md", "README.md", FILE_REGULAR,
        "# Halden Ports Directory\n\nThis directory contains Linux compatibility libraries and translated applications.\n\n## Structure\n- /halden/lib - Compatibility libraries\n- /halden/bin - Ported binaries\n- /halden/etc - Ported configuration\n- /halden/usr - Ported user applications\n\nLinux paths are automatically translated to /halden/* paths.\n", halden_dir);
    
    create_node("/hlfs/README.md", "README.md", FILE_REGULAR,
        "# HaldenOS HLFS\n\nHalden File System - A custom filesystem for HaldenOS\n\n## Features\n- Unix-like directory structure\n- Source code storage in /hlfs/src\n- Full permissions support\n- Device file support\n- Virtual filesystem integration\n- hlpkg package management integration\n- Linux compatibility through port system\n\n## Directory Structure\n- /bin - System binaries\n- /etc - Configuration files\n- /dev - Device files\n- /home - User home directories\n- /hlfs - OS source code\n- /proc - Process information\n- /var - Variable data\n- /halden - Linux compatibility layer\n", hlfs_dir);
    
    create_node("/hlfs/Makefile", "Makefile", FILE_REGULAR,
        "TARGET = haldenos.iso\nKERNEL = haldenos.elf\nBUILD_DIR = build\n\nCC = x86_64-elf-gcc\nCFLAGS = -Wall -Wextra -O2 -ffreestanding\n\nC_FILES = kernel.cpp system/terminal.cpp system/commands.cpp system/applications.cpp system/network.cpp hlfs/fs.cpp hlpkg/hlpkg.cpp port/port.cpp apps/browser.cpp apps/terminal.cpp apps/filemanager.cpp\n\nall: $(TARGET)\n\nclean:\n\trm -rf $(TARGET) $(KERNEL) $(BUILD_DIR)\n\n.PHONY: all clean\n", hlfs_dir);
    
    create_node("/hlfs/linker.ld", "linker.ld", FILE_REGULAR,
        "OUTPUT_FORMAT(elf64-x86-64)\nOUTPUT_ARCH(i386:x86-64)\nENTRY(_start)\n\nSECTIONS\n{\n    . = 0xFFFFFFFF80000000;\n    .text : { *(.text) }\n    .data : { *(.data) }\n    .bss : { *(.bss) }\n}\n", hlfs_dir);
    
    hlfs_enabled = true;
}

bool is_hlfs_enabled() {
    return hlfs_enabled;
}

void get_filesystem_name(char* output) {
    if(hlfs_enabled) {
        strcpy(output, "HLFS (Halden File System)");
    } else {
        strcpy(output, "None");
    }
}

int get_directory_contents(const char* path, int* indices, int max_count) {
    int count = 0;
    int path_len = strlen(path);
    
    for(int i = 0; i < fs_node_count && count < max_count; i++) {
        if(!filesystem[i].in_use) continue;
        
        if(strcmp(path, "/") == 0) {
            int slash_count = 0;
            for(int j = 1; filesystem[i].path[j]; j++) {
                if(filesystem[i].path[j] == '/') slash_count++;
            }
            if(slash_count == 0 && i != 0) {
                indices[count++] = i;
            }
        } else {
            if(strncmp(filesystem[i].path, path, path_len) == 0) {
                if(filesystem[i].path[path_len] == '/') {
                    bool direct_child = true;
                    for(int j = path_len + 1; filesystem[i].path[j]; j++) {
                        if(filesystem[i].path[j] == '/') {
                            direct_child = false;
                            break;
                        }
                    }
                    if(direct_child && strcmp(filesystem[i].path, path) != 0) {
                        indices[count++] = i;
                    }
                }
            }
        }
    }
    
    return count;
}

bool read_file_content(const char* path, char* output, int max_len) {
    int idx = find_node_by_path(path);
    if(idx == -1) return false;
    
    if(filesystem[idx].type != FILE_REGULAR && filesystem[idx].type != FILE_SOURCE) {
        return false;
    }
    
    if(filesystem[idx].content) {
        int len = filesystem[idx].size;
        if(len > max_len - 1) len = max_len - 1;
        memcpy(output, filesystem[idx].content, len);
        output[len] = '\0';
        return true;
    }
    
    return false;
}

bool write_file_content(const char* path, const char* content) {
    int idx = find_node_by_path(path);
    if(idx == -1) return false;
    
    if(filesystem[idx].type != FILE_REGULAR && filesystem[idx].type != FILE_SOURCE) {
        return false;
    }
    
    if(!filesystem[idx].content) {
        content_storage_used[idx] = true;
        filesystem[idx].content = content_storage_data[idx];
    }
    
    int len = strlen(content);
    if(len > MAX_CONTENT_SIZE - 1) len = MAX_CONTENT_SIZE - 1;
    
    memcpy(filesystem[idx].content, content, len);
    filesystem[idx].content[len] = '\0';
    filesystem[idx].size = len;
    filesystem[idx].modified_time = uptime_seconds;
    
    return true;
}

bool get_file_info(int index, char* name_out, char* path_out, FileType* type_out, uint64_t* size_out) {
    if(index < 0 || index >= fs_node_count || !filesystem[index].in_use) {
        return false;
    }
    
    strcpy(name_out, filesystem[index].name);
    strcpy(path_out, filesystem[index].path);
    *type_out = filesystem[index].type;
    *size_out = filesystem[index].size;
    
    return true;
}

bool create_file_in_fs(const char* parent_path, const char* name, FileType type) {
    int parent_idx = find_node_by_path(parent_path);
    if(parent_idx == -1) return false;
    
    if(filesystem[parent_idx].type != FILE_DIRECTORY) return false;
    
    char full_path[MAX_PATH];
    strcpy(full_path, parent_path);
    if(strcmp(parent_path, "/") != 0) {
        strcat(full_path, "/");
    }
    strcat(full_path, name);
    
    if(find_node_by_path(full_path) != -1) return false;
    
    const char* default_content = "";
    if(type == FILE_REGULAR) {
        default_content = "New file\n";
    } else if(type == FILE_SOURCE) {
        default_content = "// New source file\n";
    }
    
    int idx = create_node(full_path, name, type, default_content, parent_idx);
    return idx != -1;
}

bool delete_file_from_fs(const char* path) {
    int idx = find_node_by_path(path);
    if(idx == -1 || idx == 0) return false;
    
    if(filesystem[idx].type == FILE_DIRECTORY) {
        for(int i = 0; i < fs_node_count; i++) {
            if(filesystem[i].in_use && filesystem[i].parent_index == idx) {
                return false;
            }
        }
    }
    
    filesystem[idx].in_use = false;
    if(filesystem[idx].content) {
        content_storage_used[idx] = false;
        filesystem[idx].content = nullptr;
    }
    
    return true;
}

bool rename_file_in_fs(const char* old_path, const char* new_name) {
    int idx = find_node_by_path(old_path);
    if(idx == -1 || idx == 0) return false;
    
    char parent_path[MAX_PATH];
    strcpy(parent_path, old_path);
    char* last_slash = nullptr;
    for(int i = 0; parent_path[i]; i++) {
        if(parent_path[i] == '/') {
            last_slash = &parent_path[i];
        }
    }
    
    if(last_slash) {
        *last_slash = '\0';
    } else {
        strcpy(parent_path, "/");
    }
    
    char new_path[MAX_PATH];
    strcpy(new_path, parent_path);
    if(strcmp(parent_path, "/") != 0) {
        strcat(new_path, "/");
    }
    strcat(new_path, new_name);
    
    if(find_node_by_path(new_path) != -1) return false;
    
    strcpy(filesystem[idx].name, new_name);
    strcpy(filesystem[idx].path, new_path);
    filesystem[idx].modified_time = uptime_seconds;
    
    return true;
}