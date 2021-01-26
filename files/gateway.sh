#!/bin/bash
set -o pipefail
set -u

insmod  coretse.ko

sleep 1s

# activate forwarding
echo 1 > /proc/sys/net/ipv4/ip_forward

IP_0="192.168.2.1"
IP_1="192.168.3.1"
FAKE_0="192.168.4.1"
FAKE_1="192.168.5.1"
MACTSE1="58:34:12:00:FC:01"
MACTSE0="58:34:12:00:FC:00"
TSE0="eth2"
TSE1="eth3"

ifconfig $TSE0 $IP_0
ifconfig $TSE1 $IP_1

# Gateway configuration
# each line adds an IP/MAC in the gateway's whitelist
echo "$IP_0 $MACTSE0" >  gateway.cfg
echo "$IP_1 $MACTSE1" >> gateway.cfg
echo "$FAKE_0 $MACTSE0" >> gateway.cfg
echo "$FAKE_1 $MACTSE1" >> gateway.cfg
./gateway_mmap

# arping dosen't pass the gateway
arp -i $TSE0 -s $IP_1 $MACTSE1
arp -i $TSE0 -s $FAKE_0 $MACTSE1
arp -i $TSE0 -s $FAKE_1 $MACTSE1
arp -i $TSE1 -s $IP_0 $MACTSE0
arp -i $TSE1 -s $FAKE_0 $MACTSE0
arp -i $TSE1 -s $FAKE_1 $MACTSE0

# flush table
iptables -F
iptables -t nat -F

iptables -t nat -A POSTROUTING -d $FAKE_1 -j SNAT --to-source $FAKE_0
#iptables -t nat -I POSTROUTING -d $FAKE_1 -j LOG --log-prefix "postrouting $FAKE_1 "
iptables -t nat -A POSTROUTING -d $FAKE_0 -j SNAT --to-source $FAKE_1
#iptables -t nat -I POSTROUTING -d $FAKE_0 -j LOG --log-prefix "postrouting $FAKE_0 "

iptables -t nat -A PREROUTING -d $FAKE_1 -j DNAT --to-destination $IP_1
#iptables -t nat -I PREROUTING -d $FAKE_1 -j LOG --log-prefix "prerouting $FAKE_1 "
iptables -t nat -A PREROUTING -d $FAKE_0 -j DNAT --to-destination $IP_0
#iptables -t nat -I PREROUTING -d $FAKE_0 -j LOG  --log-prefix "prerouting $FAKE_0 "

# we need to tell where to send FAKE IPs
route add $FAKE_1 dev $TSE0
route add $FAKE_0 dev $TSE1
