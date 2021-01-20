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

devmem2

