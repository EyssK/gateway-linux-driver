#!/bin/bash

make_ip () {
    let IP=$(($1<<24))+$(($2<<16))+$(($3<<8))+$4
    printf "0x%x" $IP
}

IP1=$(make_ip 192 168 1 1)
IP2=$(make_ip 192 168 1 2)

MACUP="0x0102"
MACLOW="0x03040506"

MACTSE2UP="0x00"
MACTSE2LOW="0x00"

MACTSE3UP="0x00"
MACTSE3LOW="0x00"

##############

gateway_ctrl_reg="0x60005000"

#Stop gateway
devmem2 $gateway_ctrl_reg 0

#Configure TSE macs
devmem2 $(($gateway_ctrl_reg+4)) MACTSE2LOW
devmem2 $(($gateway_ctrl_reg+8)) MACTSE2UP
devmem2 $(($gateway_ctrl_reg+12)) MACTSE3LOW
devmem2 $(($gateway_ctrl_reg+16)) MACTSE3UP

