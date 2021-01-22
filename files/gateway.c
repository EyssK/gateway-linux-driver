#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>

#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
  __LINE__, __FILE__, errno, strerror(errno)); exit(-1); } while(0)

#define IP(_a,_b,_c,_d) ((unsigned int)(((_a&0xFF)<<24)+((_b&0xFF)<<16)+((_c&0xFF)<<8)+(_d&0xFF)))

#define WRITEW_REG(add, val) do { \
  printf("Writing 0x%X to %p\n", val, add); \
  *((unsigned int*)(add)) = (val & 0xFFFFFFFF); \
} while(0)
#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)
  
#define IP_BASE     0x60003000
#define MAC_BASE    0x60004000
#define CTRL_BASE   0x60005000

#define MAC_TSE2    0x58341200FC02
#define MAC_TSE3    0x58341200FC03

void* map_one_page(int fd, off_t target) {
    void *map_base;
      /* Map one page */
    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
    if(map_base == (void *) -1) FATAL;
    printf("Memory mapped 0x%X at address %p.\n", target, map_base); 
    return map_base;
}

void set_mac(void* virt_add, unsigned long long mac, int idx) {
    WRITEW_REG(virt_add+idx*8, mac & 0xFFFFFFFF);
    WRITEW_REG(virt_add+idx*8+4, mac >> 32);
}

void set_ip(void* virt_add, unsigned int ip, int idx) {
    WRITEW_REG(virt_add+idx*4, ip);
}

int main(int argc, char **argv) {
    int fd;
    void *map_ip_base, *virt_ip_addr;
    void *map_mac_base, *virt_mac_addr;
    void *map_ctrl_base, *virt_ctrl_addr;
    unsigned int read_result, writeval;
    int access_type = 'w';

    if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;
    printf("/dev/mem opened.\n");

    /* map registers */
    map_ip_base = map_one_page(fd, IP_BASE);
    map_mac_base = map_one_page(fd, MAC_BASE);
    map_ctrl_base = map_one_page(fd, CTRL_BASE);

    virt_ip_addr = map_ip_base + (IP_BASE & MAP_MASK);
    virt_mac_addr = map_mac_base + (MAC_BASE & MAP_MASK);
    virt_ctrl_addr = map_ctrl_base + (CTRL_BASE & MAP_MASK);

    // stop gateway
    WRITEW_REG(virt_ctrl_addr, 0);
    msync(virt_ctrl_addr, MAP_SIZE, MS_SYNC);
    printf("gateway stopped\n");

    // MACTSE
    set_mac(virt_ctrl_addr+0x4, MAC_TSE2, 0);
    set_mac(virt_ctrl_addr+0xC, MAC_TSE3, 0);
    printf("TSE MAC\n");

    // IPS
    set_ip(virt_ip_addr, IP(192,168,2,1), 0);
    set_ip(virt_ip_addr, IP(192,168,2,2), 1);
    set_ip(virt_ip_addr, IP(192,168,2,3), 2);
    set_ip(virt_ip_addr, IP(192,168,1,1), 3);
    set_ip(virt_ip_addr, IP(192,168,1,2), 4);
    set_ip(virt_ip_addr, IP(192,168,1,3), 5);
    msync(virt_ip_addr, MAP_SIZE, MS_SYNC);
    printf("IPs\n");
    
    // MACS
    set_mac(virt_mac_addr, 0xCAFECACA0000, 0);
    set_mac(virt_mac_addr, 0xCAFECACA0001, 1);
    set_mac(virt_mac_addr, 0xCAFECACA0002, 2);
    set_mac(virt_mac_addr, 0xCAFECACA0003, 3);
    set_mac(virt_mac_addr, 0xCAFECACA0004, 4);
    set_mac(virt_mac_addr, 0xCAFECACA0005, 5);
    msync(virt_mac_addr, MAP_SIZE, MS_SYNC);
    printf("MACs\n");

    // start gateway
    WRITEW_REG(virt_ctrl_addr, 1);
    msync(virt_ctrl_addr, MAP_SIZE, MS_SYNC);
    printf("gateway started\n");


    if(munmap(map_ip_base, MAP_SIZE) == -1) FATAL;
    if(munmap(map_mac_base, MAP_SIZE) == -1) FATAL;
    if(munmap(map_ctrl_base, MAP_SIZE) == -1) FATAL;
    close(fd);
    return 0;
}
