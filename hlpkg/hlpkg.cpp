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

#define HLPKG_MAGIC 0x484C504B47
#define HLPKG_VERSION 1
#define MAX_PACKAGES 64
#define MAX_DEPS 16
#define MAX_NAME_LEN 64
#define MAX_PATH_LEN 256
#define MAX_BINARY_SIZE (4 * 1024 * 1024)

enum HLPKGStatus {
    PKG_NOT_LOADED = 0,
    PKG_LOADED = 1,
    PKG_RUNNING = 2,
    PKG_SUSPENDED = 3,
    PKG_ERROR = 4
};

enum HLPKGPermission {
    PERM_NONE = 0,
    PERM_FS_READ = (1 << 0),
    PERM_FS_WRITE = (1 << 1),
    PERM_NET_ACCESS = (1 << 2),
    PERM_GUI_ACCESS = (1 << 3),
    PERM_AUDIO_ACCESS = (1 << 4),
    PERM_SYSTEM_CALL = (1 << 5),
    PERM_HARDWARE_ACCESS = (1 << 6),
    PERM_FULL = 0xFFFFFFFF
};

struct HLPKGHeader {
    uint64_t magic;
    uint32_t version;
    uint32_t header_size;
    uint32_t binary_size;
    uint32_t data_size;
    uint32_t dep_count;
    uint32_t permissions;
    uint64_t signature;
    char package_name[MAX_NAME_LEN];
    char package_version[16];
    char author[MAX_NAME_LEN];
    uint64_t build_timestamp;
    uint32_t checksum;
};

struct HLPKGDependency {
    char name[MAX_NAME_LEN];
    char min_version[16];
    bool required;
};

struct HLPKGProcess {
    uint32_t pid;
    char name[MAX_NAME_LEN];
    HLPKGStatus status;
    uint8_t* binary_data;
    uint32_t binary_size;
    uint8_t* data_section;
    uint32_t data_size;
    uint32_t permissions;
    uint64_t start_time;
    uint64_t cpu_time;
    uint32_t memory_usage;
    bool in_use;
};

struct HLPKGPackage {
    HLPKGHeader header;
    HLPKGDependency dependencies[MAX_DEPS];
    uint8_t* binary_data;
    uint8_t* data_section;
    bool loaded;
    char install_path[MAX_PATH_LEN];
};

HLPKGPackage packages[MAX_PACKAGES];
int package_count = 0;

HLPKGProcess processes[MAX_PACKAGES];
int process_count = 0;
uint32_t next_pid = 1000;

uint8_t binary_storage[MAX_PACKAGES][MAX_BINARY_SIZE];
bool binary_storage_used[MAX_PACKAGES];

uint32_t calculate_checksum(const uint8_t* data, uint32_t size) {
    uint32_t checksum = 0;
    for(uint32_t i = 0; i < size; i++) {
        checksum = ((checksum << 5) + checksum) + data[i];
    }
    return checksum;
}

bool verify_signature(uint64_t signature, const HLPKGHeader* header) {
    uint64_t computed = 0;
    const uint8_t* data = (const uint8_t*)header;
    for(size_t i = 0; i < sizeof(HLPKGHeader) - sizeof(uint64_t); i++) {
        computed = ((computed << 3) + computed) + data[i];
    }
    return (computed & 0xFFFFFFFFFFFF) == (signature & 0xFFFFFFFFFFFF);
}

bool check_dependencies(const HLPKGPackage* pkg) {
    for(uint32_t i = 0; i < pkg->header.dep_count && i < MAX_DEPS; i++) {
        const HLPKGDependency* dep = &pkg->dependencies[i];
        if(!dep->required) continue;
        
        bool found = false;
        for(int j = 0; j < package_count; j++) {
            if(strcmp(packages[j].header.package_name, dep->name) == 0) {
                found = true;
                break;
            }
        }
        
        if(!found) return false;
    }
    return true;
}

int hlpkg_load(const char* path) {
    if(package_count >= MAX_PACKAGES) return -1;
    
    int storage_idx = -1;
    for(int i = 0; i < MAX_PACKAGES; i++) {
        if(!binary_storage_used[i]) {
            storage_idx = i;
            break;
        }
    }
    if(storage_idx == -1) return -1;
    
    HLPKGPackage* pkg = &packages[package_count];
    memset(pkg, 0, sizeof(HLPKGPackage));
    
    pkg->header.magic = HLPKG_MAGIC;
    pkg->header.version = HLPKG_VERSION;
    pkg->header.header_size = sizeof(HLPKGHeader);
    pkg->header.binary_size = 1024;
    pkg->header.data_size = 256;
    pkg->header.dep_count = 0;
    pkg->header.permissions = PERM_FS_READ | PERM_GUI_ACCESS;
    pkg->header.signature = 0x1234567890ABCDEF;
    strcpy(pkg->header.package_name, "sample_app");
    strcpy(pkg->header.package_version, "1.0.0");
    strcpy(pkg->header.author, "Halden Dev");
    pkg->header.build_timestamp = uptime_seconds;
    pkg->header.checksum = calculate_checksum((uint8_t*)&pkg->header, 
        sizeof(HLPKGHeader) - sizeof(uint32_t));
    
    if(pkg->header.magic != HLPKG_MAGIC) return -1;
    if(pkg->header.version != HLPKG_VERSION) return -1;
    if(!verify_signature(pkg->header.signature, &pkg->header)) return -1;
    
    binary_storage_used[storage_idx] = true;
    pkg->binary_data = binary_storage[storage_idx];
    pkg->data_section = pkg->binary_data + pkg->header.binary_size;
    
    memset(pkg->binary_data, 0x90, pkg->header.binary_size);
    memset(pkg->data_section, 0, pkg->header.data_size);
    
    if(!check_dependencies(pkg)) {
        binary_storage_used[storage_idx] = false;
        return -1;
    }
    
    strcpy(pkg->install_path, path);
    pkg->loaded = true;
    
    return package_count++;
}

int hlpkg_execute(int package_id) {
    if(package_id < 0 || package_id >= package_count) return -1;
    if(!packages[package_id].loaded) return -1;
    
    int proc_idx = -1;
    for(int i = 0; i < MAX_PACKAGES; i++) {
        if(!processes[i].in_use) {
            proc_idx = i;
            break;
        }
    }
    if(proc_idx == -1) return -1;
    
    HLPKGProcess* proc = &processes[proc_idx];
    memset(proc, 0, sizeof(HLPKGProcess));
    
    proc->pid = next_pid++;
    strcpy(proc->name, packages[package_id].header.package_name);
    proc->status = PKG_RUNNING;
    proc->binary_data = packages[package_id].binary_data;
    proc->binary_size = packages[package_id].header.binary_size;
    proc->data_section = packages[package_id].data_section;
    proc->data_size = packages[package_id].header.data_size;
    proc->permissions = packages[package_id].header.permissions;
    proc->start_time = uptime_seconds;
    proc->cpu_time = 0;
    proc->memory_usage = proc->binary_size + proc->data_size;
    proc->in_use = true;
    
    if(proc_idx >= process_count) process_count = proc_idx + 1;
    
    return proc->pid;
}

bool hlpkg_kill(uint32_t pid) {
    for(int i = 0; i < process_count; i++) {
        if(processes[i].in_use && processes[i].pid == pid) {
            processes[i].status = PKG_NOT_LOADED;
            processes[i].in_use = false;
            return true;
        }
    }
    return false;
}

bool hlpkg_suspend(uint32_t pid) {
    for(int i = 0; i < process_count; i++) {
        if(processes[i].in_use && processes[i].pid == pid) {
            if(processes[i].status == PKG_RUNNING) {
                processes[i].status = PKG_SUSPENDED;
                return true;
            }
        }
    }
    return false;
}

bool hlpkg_resume(uint32_t pid) {
    for(int i = 0; i < process_count; i++) {
        if(processes[i].in_use && processes[i].pid == pid) {
            if(processes[i].status == PKG_SUSPENDED) {
                processes[i].status = PKG_RUNNING;
                return true;
            }
        }
    }
    return false;
}

bool hlpkg_check_permission(uint32_t pid, HLPKGPermission perm) {
    for(int i = 0; i < process_count; i++) {
        if(processes[i].in_use && processes[i].pid == pid) {
            return (processes[i].permissions & perm) != 0;
        }
    }
    return false;
}

int hlpkg_get_process_list(uint32_t* pid_list, int max_count) {
    int count = 0;
    for(int i = 0; i < process_count && count < max_count; i++) {
        if(processes[i].in_use && processes[i].status == PKG_RUNNING) {
            pid_list[count++] = processes[i].pid;
        }
    }
    return count;
}

bool hlpkg_get_process_info(uint32_t pid, char* name_out, HLPKGStatus* status_out, 
                            uint32_t* mem_out, uint64_t* time_out) {
    for(int i = 0; i < process_count; i++) {
        if(processes[i].in_use && processes[i].pid == pid) {
            strcpy(name_out, processes[i].name);
            *status_out = processes[i].status;
            *mem_out = processes[i].memory_usage;
            *time_out = processes[i].cpu_time;
            return true;
        }
    }
    return false;
}

bool hlpkg_get_package_info(int package_id, char* name_out, char* version_out, 
                            char* author_out, uint32_t* size_out) {
    if(package_id < 0 || package_id >= package_count) return false;
    if(!packages[package_id].loaded) return false;
    
    strcpy(name_out, packages[package_id].header.package_name);
    strcpy(version_out, packages[package_id].header.package_version);
    strcpy(author_out, packages[package_id].header.author);
    *size_out = packages[package_id].header.binary_size + 
                packages[package_id].header.data_size;
    
    return true;
}

void hlpkg_unload(int package_id) {
    if(package_id < 0 || package_id >= package_count) return;
    if(!packages[package_id].loaded) return;
    
    for(int i = 0; i < process_count; i++) {
        if(processes[i].in_use && 
           strcmp(processes[i].name, packages[package_id].header.package_name) == 0) {
            processes[i].in_use = false;
        }
    }
    
    for(int i = 0; i < MAX_PACKAGES; i++) {
        if(packages[package_id].binary_data == binary_storage[i]) {
            binary_storage_used[i] = false;
            break;
        }
    }
    
    packages[package_id].loaded = false;
    packages[package_id].binary_data = nullptr;
    packages[package_id].data_section = nullptr;
}

void hlpkg_tick() {
    for(int i = 0; i < process_count; i++) {
        if(processes[i].in_use && processes[i].status == PKG_RUNNING) {
            processes[i].cpu_time++;
        }
    }
}

void init_hlpkg_system() {
    memset(packages, 0, sizeof(packages));
    memset(processes, 0, sizeof(processes));
    memset(binary_storage_used, 0, sizeof(binary_storage_used));
    package_count = 0;
    process_count = 0;
    next_pid = 1000;
}

int hlpkg_find_package(const char* name) {
    for(int i = 0; i < package_count; i++) {
        if(packages[i].loaded && strcmp(packages[i].header.package_name, name) == 0) {
            return i;
        }
    }
    return -1;
}

bool hlpkg_set_permission(uint32_t pid, HLPKGPermission perm, bool enable) {
    for(int i = 0; i < process_count; i++) {
        if(processes[i].in_use && processes[i].pid == pid) {
            if(enable) {
                processes[i].permissions |= perm;
            } else {
                processes[i].permissions &= ~perm;
            }
            return true;
        }
    }
    return false;
}

uint32_t hlpkg_get_total_memory_usage() {
    uint32_t total = 0;
    for(int i = 0; i < process_count; i++) {
        if(processes[i].in_use) {
            total += processes[i].memory_usage;
        }
    }
    return total;
}

int hlpkg_get_package_count() {
    int count = 0;
    for(int i = 0; i < package_count; i++) {
        if(packages[i].loaded) count++;
    }
    return count;
}

int hlpkg_get_running_count() {
    int count = 0;
    for(int i = 0; i < process_count; i++) {
        if(processes[i].in_use && processes[i].status == PKG_RUNNING) count++;
    }
    return count;
}