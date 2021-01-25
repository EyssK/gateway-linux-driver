#!/usr/bin/python3

MACTSE2 = 0x010203040506
MACTSE3 = 0x0A0B0C0D0E0F

Gateway_ctrl_reg =  0x60005000
Gateway_mac_reg =   0x60004000
Gateway_ip_reg =    0x60003000

def reg_access(add, val):
    print( "devmem2", str(add), 'w', str(val))

def set_mac (add, mac) :
    reg_access( hex(add), hex(mac & 0xFFFFFFFF))
    reg_access( hex(add+4), hex(mac >> 32))

def gate_ctrl() :
    set_mac(Gateway_ctrl_reg+0x4, MACTSE2)
    set_mac(Gateway_ctrl_reg+0xC, MACTSE3)

def stop_gateway() :
    reg_access( hex(Gateway_ctrl_reg), '0')

def start_gateway() :
    reg_access( hex(Gateway_ctrl_reg), '1')

def set_ip(ips) :
    ip_reg = Gateway_ip_reg
    for ip in ips:
       reg_access(hex(ip_reg), str(ip)) 
       ip_reg += 0x4

stop_gateway()
gate_ctrl()
ips = [ "192.168.1.2" ]
set_ip(ips)
start_gateway()

