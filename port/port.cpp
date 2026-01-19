#include <stdint.h>
#include <stddef.h>

extern void* memcpy(void *dest, const void *src, size_t n);
extern void* memset(void *s, int c, size_t n);
extern size_t strlen(const char *str);
extern int strcmp(const char *s1, const char *s2);
extern int strncmp(const char *s1, const char *s2, size_t n);
extern char* strcpy(char *dest, const char *src);
extern char* strcat(char *dest, const char *src);

#define ELF_MAGIC 0x464C457F
#define MAX_PORTS 32
#define MAX_SYSCALLS 512
#define MAX_LIBS 64

enum PortStatus {
    PORT_INACTIVE = 0,
    PORT_LOADING = 1,
    PORT_ACTIVE = 2,
    PORT_SUSPENDED = 3,
    PORT_ERROR = 4
};

enum LinuxSyscall {
    SYS_READ = 0,
    SYS_WRITE = 1,
    SYS_OPEN = 2,
    SYS_CLOSE = 3,
    SYS_STAT = 4,
    SYS_FSTAT = 5,
    SYS_LSEEK = 8,
    SYS_MMAP = 9,
    SYS_MPROTECT = 10,
    SYS_MUNMAP = 11,
    SYS_BRK = 12,
    SYS_IOCTL = 16,
    SYS_ACCESS = 21,
    SYS_SOCKET = 41,
    SYS_CONNECT = 42,
    SYS_ACCEPT = 43,
    SYS_BIND = 49,
    SYS_LISTEN = 50,
    SYS_CLONE = 56,
    SYS_FORK = 57,
    SYS_EXECVE = 59,
    SYS_EXIT = 60,
    SYS_WAIT4 = 61,
    SYS_KILL = 62,
    SYS_GETPID = 39,
    SYS_GETUID = 102,
    SYS_GETGID = 104,
    SYS_GETTIMEOFDAY = 96,
    SYS_NANOSLEEP = 35
};

struct ELFHeader {
    uint32_t magic;
    uint8_t class_type;
    uint8_t endianness;
    uint8_t version;
    uint8_t os_abi;
    uint8_t abi_version;
    uint8_t padding[7];
    uint16_t type;
    uint16_t machine;
    uint32_t elf_version;
    uint64_t entry_point;
    uint64_t program_header_offset;
    uint64_t section_header_offset;
    uint32_t flags;
    uint16_t header_size;
    uint16_t program_header_size;
    uint16_t program_header_count;
    uint16_t section_header_size;
    uint16_t section_header_count;
    uint16_t string_table_index;
};

struct ProgramHeader {
    uint32_t type;
    uint32_t flags;
    uint64_t offset;
    uint64_t virtual_address;
    uint64_t physical_address;
    uint64_t file_size;
    uint64_t memory_size;
    uint64_t alignment;
};

struct LinuxPath {
    char linux_path[256];
    char halden_path[256];
};

struct PortedProcess {
    uint32_t pid;
    char name[64];
    PortStatus status;
    ELFHeader elf_header;
    uint64_t entry_point;
    uint64_t base_address;
    uint32_t memory_size;
    bool in_use;
    LinuxPath path_mappings[16];
    int path_count;
};

struct LibraryMapping {
    char linux_lib[64];
    char halden_lib[64];
    bool loaded;
};

struct SyscallTranslation {
    LinuxSyscall linux_syscall;
    uint64_t (*halden_handler)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
    bool enabled;
};

PortedProcess ported_processes[MAX_PORTS];
int ported_count = 0;
uint32_t next_port_pid = 2000;

LibraryMapping library_map[MAX_LIBS] = {
    {"libc.so.6", "halden_libc.so", true},
    {"libm.so.6", "halden_libm.so", true},
    {"libpthread.so.0", "halden_pthread.so", true},
    {"libdl.so.2", "halden_dl.so", true},
    {"librt.so.1", "halden_rt.so", true},
    {"libX11.so.6", "halden_x11.so", true},
    {"libGL.so.1", "halden_gl.so", true},
    {"libstdc++.so.6", "halden_stdcpp.so", true}
};
int library_count = 8;

LinuxPath default_paths[] = {
    {"/lib", "/halden/lib"},
    {"/usr/lib", "/halden/usr/lib"},
    {"/usr/local/lib", "/halden/usr/local/lib"},
    {"/bin", "/halden/bin"},
    {"/usr/bin", "/halden/usr/bin"},
    {"/etc", "/halden/etc"},
    {"/home", "/halden/home"},
    {"/tmp", "/halden/tmp"},
    {"/var", "/halden/var"}
};
int default_path_count = 9;

uint64_t port_sys_read(uint64_t fd, uint64_t buf, uint64_t count, 
                       uint64_t, uint64_t, uint64_t) {
    return 0;
}

uint64_t port_sys_write(uint64_t fd, uint64_t buf, uint64_t count, 
                        uint64_t, uint64_t, uint64_t) {
    return count;
}

uint64_t port_sys_open(uint64_t pathname, uint64_t flags, uint64_t mode, 
                       uint64_t, uint64_t, uint64_t) {
    return 3;
}

uint64_t port_sys_close(uint64_t fd, uint64_t, uint64_t, 
                        uint64_t, uint64_t, uint64_t) {
    return 0;
}

uint64_t port_sys_mmap(uint64_t addr, uint64_t length, uint64_t prot, 
                       uint64_t flags, uint64_t fd, uint64_t offset) {
    return 0x40000000 + (length & 0xFFFFF000);
}

uint64_t port_sys_munmap(uint64_t addr, uint64_t length, uint64_t, 
                         uint64_t, uint64_t, uint64_t) {
    return 0;
}

uint64_t port_sys_brk(uint64_t addr, uint64_t, uint64_t, 
                      uint64_t, uint64_t, uint64_t) {
    return addr ? addr : 0x50000000;
}

uint64_t port_sys_getpid(uint64_t, uint64_t, uint64_t, 
                         uint64_t, uint64_t, uint64_t) {
    return next_port_pid - 1;
}

uint64_t port_sys_exit(uint64_t status, uint64_t, uint64_t, 
                       uint64_t, uint64_t, uint64_t) {
    return 0;
}

SyscallTranslation syscall_table[MAX_SYSCALLS];

void init_syscall_table() {
    memset(syscall_table, 0, sizeof(syscall_table));
    
    syscall_table[SYS_READ].linux_syscall = SYS_READ;
    syscall_table[SYS_READ].halden_handler = port_sys_read;
    syscall_table[SYS_READ].enabled = true;
    
    syscall_table[SYS_WRITE].linux_syscall = SYS_WRITE;
    syscall_table[SYS_WRITE].halden_handler = port_sys_write;
    syscall_table[SYS_WRITE].enabled = true;
    
    syscall_table[SYS_OPEN].linux_syscall = SYS_OPEN;
    syscall_table[SYS_OPEN].halden_handler = port_sys_open;
    syscall_table[SYS_OPEN].enabled = true;
    
    syscall_table[SYS_CLOSE].linux_syscall = SYS_CLOSE;
    syscall_table[SYS_CLOSE].halden_handler = port_sys_close;
    syscall_table[SYS_CLOSE].enabled = true;
    
    syscall_table[SYS_MMAP].linux_syscall = SYS_MMAP;
    syscall_table[SYS_MMAP].halden_handler = port_sys_mmap;
    syscall_table[SYS_MMAP].enabled = true;
    
    syscall_table[SYS_MUNMAP].linux_syscall = SYS_MUNMAP;
    syscall_table[SYS_MUNMAP].halden_handler = port_sys_munmap;
    syscall_table[SYS_MUNMAP].enabled = true;
    
    syscall_table[SYS_BRK].linux_syscall = SYS_BRK;
    syscall_table[SYS_BRK].halden_handler = port_sys_brk;
    syscall_table[SYS_BRK].enabled = true;
    
    syscall_table[SYS_GETPID].linux_syscall = SYS_GETPID;
    syscall_table[SYS_GETPID].halden_handler = port_sys_getpid;
    syscall_table[SYS_GETPID].enabled = true;
    
    syscall_table[SYS_EXIT].linux_syscall = SYS_EXIT;
    syscall_table[SYS_EXIT].halden_handler = port_sys_exit;
    syscall_table[SYS_EXIT].enabled = true;
}

bool verify_elf_header(const ELFHeader* header) {
    if(header->magic != ELF_MAGIC) return false;
    if(header->class_type != 2) return false;
    if(header->machine != 0x3E) return false;
    return true;
}

void translate_path(const char* linux_path, char* halden_path, 
                   const PortedProcess* proc) {
    for(int i = 0; i < proc->path_count; i++) {
        const LinuxPath* mapping = &proc->path_mappings[i];
        int len = strlen(mapping->linux_path);
        if(strncmp(linux_path, mapping->linux_path, len) == 0) {
            strcpy(halden_path, mapping->halden_path);
            strcat(halden_path, linux_path + len);
            return;
        }
    }
    
    for(int i = 0; i < default_path_count; i++) {
        int len = strlen(default_paths[i].linux_path);
        if(strncmp(linux_path, default_paths[i].linux_path, len) == 0) {
            strcpy(halden_path, default_paths[i].halden_path);
            strcat(halden_path, linux_path + len);
            return;
        }
    }
    
    strcpy(halden_path, "/halden");
    strcat(halden_path, linux_path);
}

const char* map_library(const char* linux_lib) {
    for(int i = 0; i < library_count; i++) {
        if(library_map[i].loaded && 
           strcmp(library_map[i].linux_lib, linux_lib) == 0) {
            return library_map[i].halden_lib;
        }
    }
    return nullptr;
}

int port_load_elf(const char* path) {
    if(ported_count >= MAX_PORTS) return -1;
    
    int proc_idx = -1;
    for(int i = 0; i < MAX_PORTS; i++) {
        if(!ported_processes[i].in_use) {
            proc_idx = i;
            break;
        }
    }
    if(proc_idx == -1) return -1;
    
    PortedProcess* proc = &ported_processes[proc_idx];
    memset(proc, 0, sizeof(PortedProcess));
    
    proc->elf_header.magic = ELF_MAGIC;
    proc->elf_header.class_type = 2;
    proc->elf_header.endianness = 1;
    proc->elf_header.version = 1;
    proc->elf_header.machine = 0x3E;
    proc->elf_header.type = 2;
    proc->elf_header.entry_point = 0x400000;
    
    if(!verify_elf_header(&proc->elf_header)) return -1;
    
    proc->pid = next_port_pid++;
    strcpy(proc->name, "ported_app");
    proc->status = PORT_ACTIVE;
    proc->entry_point = proc->elf_header.entry_point;
    proc->base_address = 0x400000;
    proc->memory_size = 0x100000;
    proc->in_use = true;
    
    for(int i = 0; i < default_path_count && i < 16; i++) {
        proc->path_mappings[i] = default_paths[i];
    }
    proc->path_count = default_path_count < 16 ? default_path_count : 16;
    
    if(proc_idx >= ported_count) ported_count = proc_idx + 1;
    
    return proc->pid;
}

uint64_t port_handle_syscall(uint32_t pid, uint64_t syscall_num, 
                             uint64_t arg1, uint64_t arg2, uint64_t arg3,
                             uint64_t arg4, uint64_t arg5, uint64_t arg6) {
    if(syscall_num >= MAX_SYSCALLS) return -1;
    
    SyscallTranslation* trans = &syscall_table[syscall_num];
    if(!trans->enabled || !trans->halden_handler) return -1;
    
    return trans->halden_handler(arg1, arg2, arg3, arg4, arg5, arg6);
}

bool port_kill(uint32_t pid) {
    for(int i = 0; i < ported_count; i++) {
        if(ported_processes[i].in_use && ported_processes[i].pid == pid) {
            ported_processes[i].status = PORT_INACTIVE;
            ported_processes[i].in_use = false;
            return true;
        }
    }
    return false;
}

bool port_suspend(uint32_t pid) {
    for(int i = 0; i < ported_count; i++) {
        if(ported_processes[i].in_use && ported_processes[i].pid == pid) {
            if(ported_processes[i].status == PORT_ACTIVE) {
                ported_processes[i].status = PORT_SUSPENDED;
                return true;
            }
        }
    }
    return false;
}

bool port_resume(uint32_t pid) {
    for(int i = 0; i < ported_count; i++) {
        if(ported_processes[i].in_use && ported_processes[i].pid == pid) {
            if(ported_processes[i].status == PORT_SUSPENDED) {
                ported_processes[i].status = PORT_ACTIVE;
                return true;
            }
        }
    }
    return false;
}

int port_get_process_list(uint32_t* pid_list, int max_count) {
    int count = 0;
    for(int i = 0; i < ported_count && count < max_count; i++) {
        if(ported_processes[i].in_use && ported_processes[i].status == PORT_ACTIVE) {
            pid_list[count++] = ported_processes[i].pid;
        }
    }
    return count;
}

bool port_get_process_info(uint32_t pid, char* name_out, PortStatus* status_out, 
                           uint32_t* mem_out) {
    for(int i = 0; i < ported_count; i++) {
        if(ported_processes[i].in_use && ported_processes[i].pid == pid) {
            strcpy(name_out, ported_processes[i].name);
            *status_out = ported_processes[i].status;
            *mem_out = ported_processes[i].memory_size;
            return true;
        }
    }
    return false;
}

bool port_add_path_mapping(uint32_t pid, const char* linux_path, 
                           const char* halden_path) {
    for(int i = 0; i < ported_count; i++) {
        if(ported_processes[i].in_use && ported_processes[i].pid == pid) {
            if(ported_processes[i].path_count >= 16) return false;
            
            LinuxPath* mapping = &ported_processes[i].path_mappings[
                ported_processes[i].path_count];
            strcpy(mapping->linux_path, linux_path);
            strcpy(mapping->halden_path, halden_path);
            ported_processes[i].path_count++;
            return true;
        }
    }
    return false;
}

bool port_add_library_mapping(const char* linux_lib, const char* halden_lib) {
    if(library_count >= MAX_LIBS) return false;
    
    strcpy(library_map[library_count].linux_lib, linux_lib);
    strcpy(library_map[library_count].halden_lib, halden_lib);
    library_map[library_count].loaded = true;
    library_count++;
    return true;
}

void init_port_system() {
    memset(ported_processes, 0, sizeof(ported_processes));
    ported_count = 0;
    next_port_pid = 2000;
    init_syscall_table();
}

int port_get_active_count() {
    int count = 0;
    for(int i = 0; i < ported_count; i++) {
        if(ported_processes[i].in_use && ported_processes[i].status == PORT_ACTIVE) {
            count++;
        }
    }
    return count;
}

uint32_t port_get_total_memory() {
    uint32_t total = 0;
    for(int i = 0; i < ported_count; i++) {
        if(ported_processes[i].in_use) {
            total += ported_processes[i].memory_size;
        }
    }
    return total;
}

bool port_enable_syscall(LinuxSyscall syscall, bool enable) {
    if((uint32_t)syscall >= MAX_SYSCALLS) return false;
    syscall_table[syscall].enabled = enable;
    return true;
}