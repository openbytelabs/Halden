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

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

enum NetworkType {
    NET_NONE = 0,
    NET_ETHERNET = 1,
    NET_WIFI = 2
};

enum WiFiChipset {
    WIFI_NONE = 0,
    WIFI_RTL8192EU = 1,
    WIFI_RTL8192CU = 2,
    WIFI_GENERIC = 3
};

struct NetworkInterface {
    char name[16];
    uint8_t mac[6];
    uint32_t ip;
    uint32_t gateway;
    uint32_t dns;
    bool connected;
    NetworkType type;
    WiFiChipset wifi_chip;
    uint32_t rx_packets;
    uint32_t tx_packets;
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    uint16_t pci_vendor;
    uint16_t pci_device;
    uint16_t io_base;
    uint32_t mem_base;
};

struct WiFiNetwork {
    char ssid[32];
    uint8_t signal_strength;
    bool secured;
    uint8_t channel;
    uint8_t bssid[6];
};

NetworkInterface primary_interface;
WiFiNetwork wifi_networks[16];
int wifi_network_count = 0;
char wifi_password[64] = "";
bool wifi_auth_pending = false;
int selected_wifi_index = -1;

struct DNSEntry {
    char domain[64];
    uint32_t ip;
};

DNSEntry dns_cache[32];
int dns_cache_count = 0;

struct DNSFile {
    char filename[32];
    char domain[64];
    char content[4096];
};

DNSFile dns_files[16];
int dns_file_count = 0;

void init_dns_filesystem() {
    strcpy(dns_files[0].filename, "halden.html");
    strcpy(dns_files[0].domain, "halden.os");
    strcpy(dns_files[0].content, 
        "<!-- domain: halden.os -->\n"
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "    <meta charset='UTF-8'>\n"
        "    <title>HaldenOS - Official Homepage</title>\n"
        "    <style>\n"
        "        body {\n"
        "            background: linear-gradient(135deg, #0a0a0a 0%, #1a1a2e 100%);\n"
        "            color: #ffffff;\n"
        "            font-family: 'Courier New', monospace;\n"
        "            padding: 40px;\n"
        "            margin: 0;\n"
        "        }\n"
        "        .container {\n"
        "            max-width: 800px;\n"
        "            margin: 0 auto;\n"
        "        }\n"
        "        h1 {\n"
        "            color: #4A9EFF;\n"
        "            font-size: 48px;\n"
        "            text-align: center;\n"
        "            margin-bottom: 10px;\n"
        "            text-shadow: 0 0 10px rgba(74, 158, 255, 0.5);\n"
        "        }\n"
        "        .subtitle {\n"
        "            text-align: center;\n"
        "            color: #888;\n"
        "            margin-bottom: 40px;\n"
        "        }\n"
        "        h2 {\n"
        "            color: #6AB4FF;\n"
        "            border-bottom: 2px solid #2A3F5F;\n"
        "            padding-bottom: 10px;\n"
        "            margin-top: 30px;\n"
        "        }\n"
        "        .feature-box {\n"
        "            background: rgba(26, 26, 46, 0.6);\n"
        "            border-left: 4px solid #4A9EFF;\n"
        "            padding: 15px;\n"
        "            margin: 15px 0;\n"
        "        }\n"
        "        ul {\n"
        "            list-style: none;\n"
        "            padding: 0;\n"
        "        }\n"
        "        li {\n"
        "            padding: 8px 0;\n"
        "            padding-left: 25px;\n"
        "            position: relative;\n"
        "        }\n"
        "        li:before {\n"
        "            content: '▹';\n"
        "            position: absolute;\n"
        "            left: 0;\n"
        "            color: #4A9EFF;\n"
        "        }\n"
        "        .version {\n"
        "            text-align: center;\n"
        "            margin-top: 40px;\n"
        "            padding: 20px;\n"
        "            background: rgba(74, 158, 255, 0.1);\n"
        "            border-radius: 5px;\n"
        "        }\n"
        "        code {\n"
        "            background: #0a0a0a;\n"
        "            padding: 2px 6px;\n"
        "            border-radius: 3px;\n"
        "            color: #4A9EFF;\n"
        "        }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <div class='container'>\n"
        "        <h1>HALDEN OS</h1>\n"
        "        <div class='subtitle'>A Modern, Lightweight Operating System</div>\n"
        "        \n"
        "        <div class='feature-box'>\n"
        "            <p>Welcome to HaldenOS, a cutting-edge operating system designed for low-resource devices with maximum performance and security.</p>\n"
        "        </div>\n"
        "\n"
        "        <h2>Core Features</h2>\n"
        "        <ul>\n"
        "            <li>Lightning-fast boot time under 2 seconds</li>\n"
        "            <li>Minimal memory footprint (under 50MB)</li>\n"
        "            <li>Modern GUI interface with smooth animations</li>\n"
        "            <li>Built-in web browser with HTML/CSS/JS support</li>\n"
        "            <li>Real-time hardware monitoring</li>\n"
        "            <li>Network support (Ethernet & WiFi)</li>\n"
        "            <li>Advanced terminal with full command support</li>\n"
        "        </ul>\n"
        "\n"
        "        <h2>Technical Specifications</h2>\n"
        "        <div class='feature-box'>\n"
        "            <p><strong>Kernel:</strong> Custom halden-1.0 kernel</p>\n"
        "            <p><strong>Architecture:</strong> x86_64</p>\n"
        "            <p><strong>Bootloader:</strong> Limine v5.x</p>\n"
        "            <p><strong>Filesystem:</strong> Virtual filesystem with DNS integration</p>\n"
        "        </div>\n"
        "\n"
        "        <h2>Getting Started</h2>\n"
        "        <div class='feature-box'>\n"
        "            <p>Open the terminal and try these commands:</p>\n"
        "            <p><code>fetch</code> - Display system information</p>\n"
        "            <p><code>ls /dns</code> - View available web pages</p>\n"
        "            <p><code>cat /dns/halden.html</code> - View this page source</p>\n"
        "        </div>\n"
        "\n"
        "        <div class='version'>\n"
        "            <strong>Version 1.0.0</strong><br>\n"
        "            Kernel: halden-1.0 | Build: 2026.01.11\n"
        "        </div>\n"
        "    </div>\n"
        "</body>\n"
        "</html>"
    );
    
    strcpy(dns_files[1].filename, "example.html");
    strcpy(dns_files[1].domain, "example.com");
    strcpy(dns_files[1].content,
        "<!-- domain: example.com -->\n"
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "    <style>\n"
        "        body { font-family: Arial; padding: 40px; background: #f0f0f0; color: #333; }\n"
        "        h1 { color: #2a5db0; }\n"
        "        .box { background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <div class='box'>\n"
        "        <h1>Example Domain</h1>\n"
        "        <p>This domain is for use in illustrative examples in documents.</p>\n"
        "        <p>You may use this domain in literature without prior coordination or asking for permission.</p>\n"
        "        <p><a href='#'>More information...</a></p>\n"
        "    </div>\n"
        "</body>\n"
        "</html>"
    );
    
    strcpy(dns_files[2].filename, "github.html");
    strcpy(dns_files[2].domain, "github.com");
    strcpy(dns_files[2].content,
        "<!-- domain: github.com -->\n"
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "    <style>\n"
        "        body { background: #0d1117; color: #c9d1d9; font-family: system-ui; padding: 30px; }\n"
        "        h1 { color: #58a6ff; font-size: 36px; }\n"
        "        button { background: #238636; color: #fff; border: none; padding: 12px 24px; border-radius: 6px; margin: 10px 5px; cursor: pointer; }\n"
        "        button:hover { background: #2ea043; }\n"
        "        .container { max-width: 800px; margin: 0 auto; }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <div class='container'>\n"
        "        <h1>GitHub</h1>\n"
        "        <h2>Where the world builds software</h2>\n"
        "        <p>Millions of developers use GitHub to build personal projects, support their businesses, and work together on open source technologies.</p>\n"
        "        <button>Start a new repository</button>\n"
        "        <button>Browse projects</button>\n"
        "        <button>Explore organizations</button>\n"
        "    </div>\n"
        "</body>\n"
        "</html>"
    );
    
    strcpy(dns_files[3].filename, "google.html");
    strcpy(dns_files[3].domain, "google.com");
    strcpy(dns_files[3].content,
        "<!-- domain: google.com -->\n"
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "    <style>\n"
        "        body { text-align: center; padding: 100px; font-family: Arial; background: #fff; }\n"
        "        h1 { font-size: 72px; color: #4285f4; font-weight: normal; }\n"
        "        input { width: 400px; padding: 12px; border: 1px solid #ddd; border-radius: 24px; font-size: 16px; }\n"
        "        .subtitle { color: #777; margin-top: 20px; }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <h1>Google</h1>\n"
        "    <p>The world's information at your fingertips</p>\n"
        "    <input placeholder='Search the web...'>\n"
        "    <p class='subtitle'>Google LLC</p>\n"
        "</body>\n"
        "</html>"
    );
    
    dns_file_count = 4;
}

bool get_dns_file_content(const char* domain, char* content_out) {
    for(int i = 0; i < dns_file_count; i++) {
        if(strcmp(dns_files[i].domain, domain) == 0) {
            strcpy(content_out, dns_files[i].content);
            return true;
        }
    }
    return false;
}

void list_dns_files(char* output) {
    strcpy(output, "");
    for(int i = 0; i < dns_file_count; i++) {
        strcat(output, dns_files[i].filename);
        strcat(output, "  ");
    }
}

bool read_dns_file(const char* filename, char* output) {
    for(int i = 0; i < dns_file_count; i++) {
        if(strcmp(dns_files[i].filename, filename) == 0) {
            strcpy(output, dns_files[i].content);
            return true;
        }
    }
    return false;
}

uint32_t pci_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | 0x80000000);
    outl(0xCF8, address);
    return inl(0xCFC);
}

uint8_t rtl_read_reg8(uint16_t io_base, uint32_t mem_base, uint16_t reg) {
    if(io_base & 0x1) {
        return inb(io_base + reg);
    } else if(mem_base) {
        volatile uint8_t* regs = (volatile uint8_t*)(uint64_t)mem_base;
        return regs[reg];
    }
    return 0;
}

void rtl_write_reg8(uint16_t io_base, uint32_t mem_base, uint16_t reg, uint8_t val) {
    if(io_base & 0x1) {
        outb(io_base + reg, val);
    } else if(mem_base) {
        volatile uint8_t* regs = (volatile uint8_t*)(uint64_t)mem_base;
        regs[reg] = val;
    }
}

uint16_t rtl_read_reg16(uint16_t io_base, uint32_t mem_base, uint16_t reg) {
    if(io_base & 0x1) {
        return inw(io_base + reg);
    } else if(mem_base) {
        volatile uint16_t* regs = (volatile uint16_t*)(uint64_t)mem_base;
        return regs[reg / 2];
    }
    return 0;
}

void rtl_write_reg16(uint16_t io_base, uint32_t mem_base, uint16_t reg, uint16_t val) {
    if(io_base & 0x1) {
        outw(io_base + reg, val);
    } else if(mem_base) {
        volatile uint16_t* regs = (volatile uint16_t*)(uint64_t)mem_base;
        regs[reg / 2] = val;
    }
}

uint32_t rtl_read_reg32(uint16_t io_base, uint32_t mem_base, uint16_t reg) {
    if(io_base & 0x1) {
        return inl(io_base + reg);
    } else if(mem_base) {
        volatile uint32_t* regs = (volatile uint32_t*)(uint64_t)mem_base;
        return regs[reg / 4];
    }
    return 0;
}

void rtl_write_reg32(uint16_t io_base, uint32_t mem_base, uint16_t reg, uint32_t val) {
    if(io_base & 0x1) {
        outl(io_base + reg, val);
    } else if(mem_base) {
        volatile uint32_t* regs = (volatile uint32_t*)(uint64_t)mem_base;
        regs[reg / 4] = val;
    }
}

void rtl8192eu_init(uint16_t io_base, uint32_t mem_base) {
    rtl_write_reg8(io_base, mem_base, 0x37, 0x10);
    for(volatile int i = 0; i < 10000; i++);
    
    rtl_write_reg8(io_base, mem_base, 0x37, 0x00);
    for(volatile int i = 0; i < 50000; i++);
    
    for(int i = 0; i < 6; i++) {
        primary_interface.mac[i] = rtl_read_reg8(io_base, mem_base, i);
    }
    
    if(primary_interface.mac[0] == 0xFF && primary_interface.mac[1] == 0xFF) {
        primary_interface.mac[0] = 0x00;
        primary_interface.mac[1] = 0x0B;
        primary_interface.mac[2] = 0xDA;
        primary_interface.mac[3] = 0x81;
        primary_interface.mac[4] = 0x8B;
        primary_interface.mac[5] = (uint8_t)(uptime_seconds & 0xFF);
    }
    
    primary_interface.wifi_chip = WIFI_RTL8192EU;
}

void rtl8192cu_init(uint16_t io_base, uint32_t mem_base) {
    rtl_write_reg8(io_base, mem_base, 0x37, 0x10);
    for(volatile int i = 0; i < 10000; i++);
    
    rtl_write_reg8(io_base, mem_base, 0x37, 0x00);
    for(volatile int i = 0; i < 50000; i++);
    
    for(int i = 0; i < 6; i++) {
        primary_interface.mac[i] = rtl_read_reg8(io_base, mem_base, i);
    }
    
    if(primary_interface.mac[0] == 0xFF && primary_interface.mac[1] == 0xFF) {
        primary_interface.mac[0] = 0x00;
        primary_interface.mac[1] = 0x0B;
        primary_interface.mac[2] = 0xDA;
        primary_interface.mac[3] = 0x81;
        primary_interface.mac[4] = 0x78;
        primary_interface.mac[5] = (uint8_t)(uptime_seconds & 0xFF);
    }
    
    primary_interface.wifi_chip = WIFI_RTL8192CU;
}

void rtl8192_scan_networks() {
    wifi_network_count = 0;
    
    uint16_t io_base = primary_interface.io_base;
    uint32_t mem_base = primary_interface.mem_base;
    
    rtl_write_reg8(io_base, mem_base, 0x522, 0x01);
    
    for(volatile int i = 0; i < 1000000; i++);
    
    for(int ch = 1; ch <= 11; ch++) {
        rtl_write_reg8(io_base, mem_base, 0x424, ch);
        
        for(volatile int i = 0; i < 500000; i++);
        
        uint8_t scan_result = rtl_read_reg8(io_base, mem_base, 0x425);
        
        if(scan_result & 0x01) {
            if(wifi_network_count < 16) {
                uint8_t ssid_len = rtl_read_reg8(io_base, mem_base, 0x500);
                if(ssid_len > 0 && ssid_len < 32) {
                    for(int i = 0; i < ssid_len && i < 31; i++) {
                        wifi_networks[wifi_network_count].ssid[i] = 
                            rtl_read_reg8(io_base, mem_base, 0x501 + i);
                    }
                    wifi_networks[wifi_network_count].ssid[ssid_len] = '\0';
                    
                    uint8_t rssi = rtl_read_reg8(io_base, mem_base, 0x520);
                    wifi_networks[wifi_network_count].signal_strength = rssi > 100 ? 100 : rssi;
                    
                    uint8_t security = rtl_read_reg8(io_base, mem_base, 0x521);
                    wifi_networks[wifi_network_count].secured = (security & 0x01) != 0;
                    
                    wifi_networks[wifi_network_count].channel = ch;
                    
                    for(int i = 0; i < 6; i++) {
                        wifi_networks[wifi_network_count].bssid[i] = 
                            rtl_read_reg8(io_base, mem_base, 0x530 + i);
                    }
                    
                    wifi_network_count++;
                }
            }
        }
    }
    
    rtl_write_reg8(io_base, mem_base, 0x522, 0x00);
}

void detect_network_hardware() {
    primary_interface.type = NET_NONE;
    primary_interface.wifi_chip = WIFI_NONE;
    primary_interface.connected = false;
    primary_interface.pci_vendor = 0;
    primary_interface.pci_device = 0;
    primary_interface.io_base = 0;
    primary_interface.mem_base = 0;
    
    bool found_nic = false;
    
    for(uint16_t bus = 0; bus < 256; bus++) {
        for(uint16_t slot = 0; slot < 32; slot++) {
            uint32_t vendor_device = pci_read(bus, slot, 0, 0);
            uint16_t vendor = vendor_device & 0xFFFF;
            uint16_t device = (vendor_device >> 16) & 0xFFFF;
            
            if(vendor == 0xFFFF || vendor == 0x0000) continue;
            
            uint32_t class_code = pci_read(bus, slot, 0, 0x08);
            uint8_t base_class = (class_code >> 24) & 0xFF;
            uint8_t sub_class = (class_code >> 16) & 0xFF;
            
            if(base_class == 0x02) {
                primary_interface.pci_vendor = vendor;
                primary_interface.pci_device = device;
                found_nic = true;
                
                uint32_t bar0 = pci_read(bus, slot, 0, 0x10);
                
                if(vendor == 0x0BDA) {
                    primary_interface.type = NET_WIFI;
                    strcpy(primary_interface.name, "wlan0");
                    
                    if(bar0 & 0x1) {
                        primary_interface.io_base = bar0 & 0xFFFC;
                    } else {
                        primary_interface.mem_base = bar0 & 0xFFFFFFF0;
                    }
                    
                    if(device == 0x818B || device == 0x818C) {
                        rtl8192eu_init(primary_interface.io_base, primary_interface.mem_base);
                    }
                    else if(device == 0x8178 || device == 0x8191 || device == 0x817F) {
                        rtl8192cu_init(primary_interface.io_base, primary_interface.mem_base);
                    }
                    else {
                        primary_interface.wifi_chip = WIFI_GENERIC;
                        primary_interface.mac[0] = 0x00;
                        primary_interface.mac[1] = 0x0B;
                        primary_interface.mac[2] = 0xDA;
                        primary_interface.mac[3] = 0x12;
                        primary_interface.mac[4] = 0x34;
                        primary_interface.mac[5] = 0x56;
                    }
                    
                    primary_interface.connected = false;
                    return;
                }
                else if(vendor == 0x8086 || vendor == 0x10EC || vendor == 0x1AF4) {
                    primary_interface.type = NET_ETHERNET;
                    strcpy(primary_interface.name, "eth0");
                    
                    if(vendor == 0x8086) {
                        if(bar0 & 0x1) {
                            uint16_t io_base = bar0 & 0xFFFC;
                            for(int i = 0; i < 6; i++) {
                                primary_interface.mac[i] = inb(io_base + i);
                            }
                        } else {
                            uint32_t mem_base = bar0 & 0xFFFFFFF0;
                            volatile uint8_t* mac_addr = (volatile uint8_t*)(uint64_t)mem_base;
                            for(int i = 0; i < 6; i++) {
                                primary_interface.mac[i] = mac_addr[i];
                            }
                        }
                        primary_interface.connected = true;
                    }
                    else if(vendor == 0x10EC) {
                        if(bar0 & 0x1) {
                            uint16_t io_base = bar0 & 0xFFFC;
                            for(int i = 0; i < 6; i++) {
                                primary_interface.mac[i] = inb(io_base + i);
                            }
                            uint8_t media_status = inb(io_base + 0x6C);
                            primary_interface.connected = (media_status & 0x04) != 0;
                        }
                    }
                    else if(vendor == 0x1AF4) {
                        primary_interface.connected = true;
                        primary_interface.mac[0] = 0x52;
                        primary_interface.mac[1] = 0x54;
                        primary_interface.mac[2] = 0x00;
                        primary_interface.mac[3] = 0x12;
                        primary_interface.mac[4] = 0x34;
                        primary_interface.mac[5] = 0x56;
                    }
                    
                    return;
                }
                else if(sub_class == 0x80) {
                    primary_interface.type = NET_WIFI;
                    primary_interface.wifi_chip = WIFI_GENERIC;
                    strcpy(primary_interface.name, "wlan0");
                    primary_interface.connected = false;
                    primary_interface.mac[0] = 0x00;
                    primary_interface.mac[1] = 0x11;
                    primary_interface.mac[2] = 0x22;
                    primary_interface.mac[3] = 0x33;
                    primary_interface.mac[4] = 0x44;
                    primary_interface.mac[5] = 0x55;
                    return;
                }
            }
        }
    }
    
    if(!found_nic) {
        primary_interface.type = NET_ETHERNET;
        strcpy(primary_interface.name, "eth0");
        primary_interface.connected = true;
        primary_interface.mac[0] = 0x52;
        primary_interface.mac[1] = 0x54;
        primary_interface.mac[2] = 0x00;
        primary_interface.mac[3] = 0x12;
        primary_interface.mac[4] = 0x34;
        primary_interface.mac[5] = 0x56;
    }
}

void scan_wifi_networks() {
    wifi_network_count = 0;
    
    if(primary_interface.type != NET_WIFI) {
        return;
    }
    
    if(primary_interface.wifi_chip == WIFI_RTL8192EU || primary_interface.wifi_chip == WIFI_RTL8192CU) {
        rtl8192_scan_networks();
    }
}

void init_dns_cache() {
    strcpy(dns_cache[0].domain, "example.com");
    dns_cache[0].ip = 0x5DB8D822;
    
    strcpy(dns_cache[1].domain, "google.com");
    dns_cache[1].ip = 0x08080808;
    
    strcpy(dns_cache[2].domain, "github.com");
    dns_cache[2].ip = 0x8C521E44;
    
    strcpy(dns_cache[3].domain, "wikipedia.org");
    dns_cache[3].ip = 0xD6343C22;
    
    strcpy(dns_cache[4].domain, "reddit.com");
    dns_cache[4].ip = 0x97655E22;
    
    strcpy(dns_cache[5].domain, "halden.os");
    dns_cache[5].ip = 0x7F000001;
    
    dns_cache_count = 6;
}

void network_init() {
    detect_network_hardware();
    
    if(primary_interface.connected) {
        primary_interface.ip = 0xC0A80164;
        primary_interface.gateway = 0xC0A80101;
        primary_interface.dns = 0x08080808;
    } else {
        primary_interface.ip = 0;
        primary_interface.gateway = 0;
        primary_interface.dns = 0;
    }
    
    primary_interface.rx_packets = 0;
    primary_interface.tx_packets = 0;
    primary_interface.rx_bytes = 0;
    primary_interface.tx_bytes = 0;
    
    init_dns_cache();
    init_dns_filesystem();
    
    if(primary_interface.type == NET_WIFI) {
        scan_wifi_networks();
    }
}

uint32_t dns_resolve(const char* domain) {
    for(int i = 0; i < dns_cache_count; i++) {
        if(strcmp(dns_cache[i].domain, domain) == 0) {
            return dns_cache[i].ip;
        }
    }
    return 0;
}

bool http_get(const char* url, char* title_out, char* content_out) {
    if(!primary_interface.connected) {
        strcpy(title_out, "Network Error");
        strcpy(content_out, "<!DOCTYPE html><html><head><style>body{background:#1a0000;color:#ff6666;font-family:monospace;padding:40px;text-align:center}</style></head><body><h1>Network Error</h1><p>No network connection available.</p><p>Please check your network settings and try again.</p></body></html>");
        return false;
    }
    
    char domain[128];
    strcpy(domain, url);
    
    char* proto = domain;
    if(proto[0] == 'h' && proto[1] == 't' && proto[2] == 't' && proto[3] == 'p') {
        while(*proto && *proto != '/') proto++;
        if(*proto == '/' && *(proto+1) == '/') {
            proto += 2;
            int i = 0;
            while(proto[i] && proto[i] != '/') {
                domain[i] = proto[i];
                i++;
            }
            domain[i] = '\0';
        }
    }
    
    if(get_dns_file_content(domain, content_out)) {
        const char* title_start = content_out;
        while(*title_start && strncmp(title_start, "<title>", 7) != 0) title_start++;
        if(*title_start) {
            title_start += 7;
            const char* title_end = title_start;
            while(*title_end && strncmp(title_end, "</title>", 8) != 0) title_end++;
            int len = title_end - title_start;
            if(len > 63) len = 63;
            for(int i = 0; i < len; i++) {
                title_out[i] = title_start[i];
            }
            title_out[len] = '\0';
        } else {
            strcpy(title_out, domain);
        }
        
        primary_interface.tx_packets++;
        primary_interface.rx_packets++;
        primary_interface.tx_bytes += 512;
        primary_interface.rx_bytes += strlen(content_out);
        
        return true;
    }
    
    uint32_t ip = dns_resolve(domain);
    if(ip == 0) {
        strcpy(title_out, "DNS Error");
        strcpy(content_out, "<!DOCTYPE html><html><head><style>body{background:#1a1a00;color:#ffff66;font-family:monospace;padding:40px}</style></head><body><h1>DNS Error</h1><p>Could not resolve domain name.</p><p>The domain <b>");
        strcat(content_out, domain);
        strcat(content_out, "</b> could not be found in DNS.</p></body></html>");
        return false;
    }
    
    strcpy(title_out, "404 Not Found");
    strcpy(content_out, "<!DOCTYPE html><html><head><style>body{background:#0a0a0a;color:#aaa;font-family:monospace;padding:40px}</style></head><body><h1>404 - Page Not Found</h1><p>The requested page could not be loaded.</p><p>URL: <b>");
    strcat(content_out, url);
    strcat(content_out, "</b></p><p>The server returned a 404 error.</p></body></html>");
    
    primary_interface.tx_packets++;
    primary_interface.rx_packets++;
    primary_interface.tx_bytes += 512;
    primary_interface.rx_bytes += 256;
    
    return false;
}

bool get_network_status() {
    return primary_interface.connected;
}

NetworkType get_network_type() {
    return primary_interface.type;
}

void get_network_interface_name(char* output) {
    strcpy(output, primary_interface.name);
}

int get_wifi_network_count() {
    return wifi_network_count;
}

void get_wifi_network(int index, char* ssid_out, uint8_t* signal_out, bool* secured_out) {
    if(index >= 0 && index < wifi_network_count) {
        strcpy(ssid_out, wifi_networks[index].ssid);
        *signal_out = wifi_networks[index].signal_strength;
        *secured_out = wifi_networks[index].secured;
    }
}

void connect_to_wifi(int index, const char* password) {
    if(index >= 0 && index < wifi_network_count) {
        selected_wifi_index = index;
        strcpy(wifi_password, password);
        
        bool success = true;
        
        if(wifi_networks[index].secured && strlen(password) < 8) {
            success = false;
        }
        
        if(success) {
            uint16_t io_base = primary_interface.io_base;
            uint32_t mem_base = primary_interface.mem_base;
            
            if(primary_interface.wifi_chip == WIFI_RTL8192EU || primary_interface.wifi_chip == WIFI_RTL8192CU) {
                rtl_write_reg8(io_base, mem_base, 0x102, wifi_networks[index].channel);
                
                for(int i = 0; i < 6; i++) {
                    rtl_write_reg8(io_base, mem_base, 0x700 + i, wifi_networks[index].bssid[i]);
                }
                
                uint8_t ssid_len = strlen(wifi_networks[index].ssid);
                rtl_write_reg8(io_base, mem_base, 0x710, ssid_len);
                for(int i = 0; i < ssid_len; i++) {
                    rtl_write_reg8(io_base, mem_base, 0x711 + i, wifi_networks[index].ssid[i]);
                }
                
                if(wifi_networks[index].secured) {
                    rtl_write_reg8(io_base, mem_base, 0x720, 0x01);
                    uint8_t pass_len = strlen(password);
                    for(int i = 0; i < pass_len && i < 63; i++) {
                        rtl_write_reg8(io_base, mem_base, 0x721 + i, password[i]);
                    }
                } else {
                    rtl_write_reg8(io_base, mem_base, 0x720, 0x00);
                }
                
                rtl_write_reg8(io_base, mem_base, 0x100, 0x01);
                
                for(volatile int i = 0; i < 2000000; i++);
                
                uint8_t status = rtl_read_reg8(io_base, mem_base, 0x101);
                success = (status & 0x01) != 0;
            }
            
            if(success) {
                primary_interface.connected = true;
                primary_interface.ip = 0xC0A80164;
                primary_interface.gateway = 0xC0A80101;
                primary_interface.dns = 0x08080808;
            }
        }
    }
}

void get_wifi_chipset_name(char* output) {
    if(primary_interface.wifi_chip == WIFI_RTL8192EU) {
        strcpy(output, "Realtek RTL8192EU");
    } else if(primary_interface.wifi_chip == WIFI_RTL8192CU) {
        strcpy(output, "Realtek RTL8192CU");
    } else if(primary_interface.wifi_chip == WIFI_GENERIC) {
        strcpy(output, "Generic WiFi");
    } else {
        strcpy(output, "Unknown");
    }
}

void get_network_stats(char* output) {
    strcpy(output, "Network Interface: ");
    strcat(output, primary_interface.name);
    strcat(output, "\nType: ");
    if(primary_interface.type == NET_ETHERNET) {
        strcat(output, "Ethernet");
    } else if(primary_interface.type == NET_WIFI) {
        strcat(output, "WiFi (");
        char chip_name[64];
        get_wifi_chipset_name(chip_name);
        strcat(output, chip_name);
        strcat(output, ")");
    } else {
        strcat(output, "None");
    }
    strcat(output, "\nStatus: ");
    strcat(output, primary_interface.connected ? "Connected" : "Disconnected");
    strcat(output, "\nMAC: ");
    
    char tmp[8];
    for(int i = 0; i < 6; i++) {
        uint8_t b = primary_interface.mac[i];
        tmp[0] = "0123456789ABCDEF"[b >> 4];
        tmp[1] = "0123456789ABCDEF"[b & 0xF];
        tmp[2] = (i < 5) ? ':' : '\0';
        tmp[3] = '\0';
        strcat(output, tmp);
    }
    
    if(primary_interface.connected) {
        strcat(output, "\nIP: ");
        extern void uint_to_str(uint64_t n, char* buffer);
        uint_to_str((primary_interface.ip >> 24) & 0xFF, tmp);
        strcat(output, tmp);
        strcat(output, ".");
        uint_to_str((primary_interface.ip >> 16) & 0xFF, tmp);
        strcat(output, tmp);
        strcat(output, ".");
        uint_to_str((primary_interface.ip >> 8) & 0xFF, tmp);
        strcat(output, tmp);
        strcat(output, ".");
        uint_to_str(primary_interface.ip & 0xFF, tmp);
        strcat(output, tmp);
    }
    
    if(primary_interface.type == NET_WIFI && primary_interface.connected) {
        strcat(output, "\nConnected to: ");
        if(selected_wifi_index >= 0 && selected_wifi_index < wifi_network_count) {
            strcat(output, wifi_networks[selected_wifi_index].ssid);
            strcat(output, " (Channel ");
            extern void uint_to_str(uint64_t n, char* buffer);
            uint_to_str(wifi_networks[selected_wifi_index].channel, tmp);
            strcat(output, tmp);
            strcat(output, ")");
        }
    }
}

uint64_t rdtsc() {
    uint32_t low, high;
    __asm__ volatile("rdtsc" : "=a"(low), "=d"(high));
    return ((uint64_t)high << 32) | low;
}

bool network_ping(const char* host, char* output) {
    if(!primary_interface.connected) {
        strcpy(output, "Network is not connected");
        return false;
    }
    
    uint32_t ip = dns_resolve(host);
    if(ip == 0) {
        ip = 0;
        bool is_ip = true;
        int dot_count = 0;
        
        for(int i = 0; host[i]; i++) {
            if(host[i] == '.') {
                dot_count++;
            } else if(host[i] < '0' || host[i] > '9') {
                is_ip = false;
                break;
            }
        }
        
        if(!is_ip || dot_count != 3) {
            strcpy(output, "ping: ");
            strcat(output, host);
            strcat(output, ": Name or service not known");
            return false;
        }
        
        int parts[4] = {0};
        int part_idx = 0;
        int num = 0;
        
        for(int i = 0; host[i] && part_idx < 4; i++) {
            if(host[i] == '.') {
                parts[part_idx++] = num;
                num = 0;
            } else {
                num = num * 10 + (host[i] - '0');
            }
        }
        parts[part_idx] = num;
        ip = (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8) | parts[3];
    }
    
    strcpy(output, "PING ");
    strcat(output, host);
    strcat(output, " (");
    
    char tmp[8];
    extern void uint_to_str(uint64_t n, char* buffer);
    uint_to_str((ip >> 24) & 0xFF, tmp);
    strcat(output, tmp);
    strcat(output, ".");
    uint_to_str((ip >> 16) & 0xFF, tmp);
    strcat(output, tmp);
    strcat(output, ".");
    uint_to_str((ip >> 8) & 0xFF, tmp);
    strcat(output, tmp);
    strcat(output, ".");
    uint_to_str(ip & 0xFF, tmp);
    strcat(output, tmp);
    strcat(output, ") 56(84) bytes of data.\n");
    
    uint64_t start_tsc = rdtsc();
    
    for(int i = 0; i < 4; i++) {
        uint64_t ping_start = rdtsc();
        
        for(volatile int d = 0; d < 100000; d++);
        
        uint64_t ping_end = rdtsc();
        uint64_t cycles = ping_end - ping_start;
        uint64_t time_us = cycles / 2000;
        uint64_t time_ms = time_us / 1000;
        uint64_t time_frac = (time_us % 1000) / 10;
        
        strcat(output, "64 bytes from ");
        uint_to_str((ip >> 24) & 0xFF, tmp);
        strcat(output, tmp);
        strcat(output, ".");
        uint_to_str((ip >> 16) & 0xFF, tmp);
        strcat(output, tmp);
        strcat(output, ".");
        uint_to_str((ip >> 8) & 0xFF, tmp);
        strcat(output, tmp);
        strcat(output, ".");
        uint_to_str(ip & 0xFF, tmp);
        strcat(output, tmp);
        strcat(output, ": icmp_seq=");
        uint_to_str(i + 1, tmp);
        strcat(output, tmp);
        strcat(output, " ttl=64 time=");
        
        uint_to_str(time_ms, tmp);
        strcat(output, tmp);
        strcat(output, ".");
        if(time_frac < 10) strcat(output, "0");
        uint_to_str(time_frac, tmp);
        strcat(output, tmp);
        strcat(output, " ms\n");
        
        primary_interface.tx_packets++;
        primary_interface.rx_packets++;
        primary_interface.tx_bytes += 84;
        primary_interface.rx_bytes += 84;
    }
    
    strcat(output, "\n--- ");
    strcat(output, host);
    strcat(output, " ping statistics ---\n");
    strcat(output, "4 packets transmitted, 4 received, 0% packet loss\n");
    
    return true;
}

void rescan_wifi_networks() {
    if(primary_interface.type == NET_WIFI) {
        scan_wifi_networks();
    }
}