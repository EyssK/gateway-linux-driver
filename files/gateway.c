#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/mman.h>

// #define DEBUG

#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
  __LINE__, __FILE__, errno, strerror(errno)); exit(-1); } while(0)

#define IP(_a,_b,_c,_d) ((ip_t)(((_a&0xFF)<<24)+((_b&0xFF)<<16)+((_c&0xFF)<<8)+(_d&0xFF)))
  
#define MAC(_a,_b,_c,_d,_e,_f) ((mac_t)((((mac_t)_a&0xFF)<<40)+(((mac_t)_b&0xFF)<<32)+((_c&0xFF)<<24)+((_d&0xFF)<<16)+((_e&0xFF)<<8)+(_f&0xFF)))

#ifdef DEBUG
  #define WRITEW_REG(add, val) do { \
    printf("Writing 0x%X to %p\n", val, add); \
    *((unsigned int*)(add)) = (val & 0xFFFFFFFF); \
    } while(0)
#else
  #define WRITEW_REG(add, val) do { \
    *((unsigned int*)(add)) = (val & 0xFFFFFFFF); \
  } while(0)
#endif

#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)
  
#define IP_BASE     0x60003000
#define MAC_BASE    0x60004000
#define CTRL_BASE   0x60005000

#define ENTRY_NB    256

#define MAC_TSE2    0x58341200FC02
#define MAC_TSE3    0x58341200FC03

typedef unsigned long long mac_t;
typedef unsigned int ip_t;

struct entry {
    mac_t mac;
    ip_t ip;
};

struct gateway_cfg {
    unsigned int used;
    struct entry table[ENTRY_NB];
};

void* map_one_page(int fd, off_t target) {
    void *map_base;
    /* Map one page */
    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
    if(map_base == (void *) -1) FATAL;
#ifdef DEBUG
    printf("Memory mapped 0x%X at address %p.\n", target, map_base);
#endif
    return map_base;
}

void set_mac(void* virt_add, mac_t mac, int idx) {
    WRITEW_REG(virt_add+idx*8, mac & 0xFFFFFFFF);
    WRITEW_REG(virt_add+idx*8+4, mac >> 32);
}

void set_ip(void* virt_add, ip_t ip, int idx) {
    WRITEW_REG(virt_add+idx*4, ip);
}

void stop_gateway(void* virt_add) {
    WRITEW_REG(virt_add, 1);
    msync(virt_add, MAP_SIZE, MS_SYNC);
}

void start_gateway(void* virt_add) {
    WRITEW_REG(virt_add, 0);
    msync(virt_add, MAP_SIZE, MS_SYNC);
}
    
int read_cfg_file(char* filename, struct gateway_cfg* cfg) {
    FILE* fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    
    fp = fopen(filename, "r");
    if (!fp) {
        printf("%s: Cannot open file %s\n", __func__, filename);
        return -1;
    }
    
    while ((read = getline(&line, &len, fp)) != -1) {
        char ip[4];
        char mac[6];
        ip[0] = atoi(strtok (line,"."));
        ip[1] = atoi(strtok (NULL,"."));
        ip[2] = atoi(strtok (NULL,"."));
        ip[3] = atoi(strtok (NULL," "));
        mac[0] = strtol(strtok (NULL,":"), NULL, 16);
        mac[1] = strtol(strtok (NULL,":"), NULL, 16);
        mac[2] = strtol(strtok (NULL,":"), NULL, 16);
        mac[3] = strtol(strtok (NULL,":"), NULL, 16);
        mac[4] = strtol(strtok (NULL,":"), NULL, 16);
        mac[5] = strtol(strtok (NULL," \n"),NULL, 16);
        printf("IP:\t%d.%d.%d.%d\t", ip[0],ip[1],ip[2],ip[3]);
        printf("MAC:\t%x:%x:%x:%x:%x:%x\n", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
        
        unsigned int idx = cfg->used++;
        if (idx == ENTRY_NB){
            printf("%s: too many entry\n", __func__);
            return -1;
        }
        cfg->table[idx].ip = IP(ip[0],ip[1],ip[2],ip[3]);
        cfg->table[idx].mac = MAC(mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    }
    
    fclose(fp);
    return 0;
}

int main(int argc, char **argv) {
    int fd;
    char* cfg_filename = "gateway.cfg";
    void *map_ip_base, *virt_ip_addr;
    void *map_mac_base, *virt_mac_addr;
    void *map_ctrl_base, *virt_ctrl_addr;
    unsigned int read_result, writeval;
    int access_type = 'w';
    
    if(argc>1) {
        cfg_filename = argv[1];
    }

    if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;

    // map registers
    map_ip_base = map_one_page(fd, IP_BASE);
    map_mac_base = map_one_page(fd, MAC_BASE);
    map_ctrl_base = map_one_page(fd, CTRL_BASE);

    virt_ip_addr = map_ip_base + (IP_BASE & MAP_MASK);
    virt_mac_addr = map_mac_base + (MAC_BASE & MAP_MASK);
    virt_ctrl_addr = map_ctrl_base + (CTRL_BASE & MAP_MASK);

    // read configuration
    struct gateway_cfg* cfg = malloc(sizeof(*cfg));
    if (read_cfg_file(cfg_filename, cfg)) {
        printf("Bad config file\n");
        return -1;
    }
    
    // stop gateway
    stop_gateway(virt_ctrl_addr);

    // MACTSE
    set_mac(virt_ctrl_addr+0x4, MAC_TSE2, 0);
    set_mac(virt_ctrl_addr+0xC, MAC_TSE3, 0);

    // CFG IPs and MACs
    for (int idx = 0; idx < cfg->used; idx++) {
        set_ip(virt_ip_addr, cfg->table[idx].ip, idx);
        set_mac(virt_mac_addr, cfg->table[idx].mac, idx);
    }

    // start gateway
    start_gateway(virt_ctrl_addr);
    printf("gateway started\n");

    free(cfg);
    if(munmap(map_ip_base, MAP_SIZE) == -1) FATAL;
    if(munmap(map_mac_base, MAP_SIZE) == -1) FATAL;
    if(munmap(map_ctrl_base, MAP_SIZE) == -1) FATAL;
    close(fd);
    return 0;
}
