#!/bin/bash

cd /openair-mme/scripts

# Using the same interface name and the same IP address for S11 and S10 is not allowed.
# Starting a virtual interface on loopback for S10
ifconfig lo:s10 127.0.0.10 up
echo "ifconfig lo:s10 127.0.0.10 up --> OK"

INSTANCE=1
PREFIX='/openair-mme/etc'
MY_REALM='openairinterface.org'

declare -A MME_CONF

MME_CONF[@MME_S6A_IP_ADDR@]="192.168.61.3"
MME_CONF[@INSTANCE@]=$INSTANCE
MME_CONF[@PREFIX@]=$PREFIX
MME_CONF[@REALM@]=$MY_REALM
MME_CONF[@PID_DIRECTORY@]='/var/run'
MME_CONF[@MME_FQDN@]="mme.${MME_CONF[@REALM@]}"
MME_CONF[@HSS_HOSTNAME@]='hss'
MME_CONF[@HSS_FQDN@]="${MME_CONF[@HSS_HOSTNAME@]}.${MME_CONF[@REALM@]}"
MME_CONF[@HSS_IP_ADDR@]="192.168.61.2"
MME_CONF[@HSS_REALM@]=$MY_REALM
MME_CONF[@MCC@]='655'
MME_CONF[@MNC@]='25'
MME_CONF[@MME_GID@]='32768'
MME_CONF[@MME_CODE@]='3'
MME_CONF[@TAC_0@]='1'
MME_CONF[@TAC_1@]='5'
MME_CONF[@TAC_2@]='6'
MME_CONF[@MME_INTERFACE_NAME_FOR_S1_MME@]='enx0024322405d3'
MME_CONF[@MME_IPV4_ADDRESS_FOR_S1_MME@]='192.168.61.3/24'
MME_CONF[@MME_INTERFACE_NAME_FOR_S11@]='enx0024322405d3'
MME_CONF[@MME_IPV4_ADDRESS_FOR_S11@]='192.168.61.3/24'
MME_CONF[@MME_INTERFACE_NAME_FOR_S10@]='lo:s10'
MME_CONF[@MME_IPV4_ADDRESS_FOR_S10@]='127.0.0.10/24'
MME_CONF[@OUTPUT@]='CONSOLE'
MME_CONF[@SGW_IPV4_ADDRESS_FOR_S11_0@]='192.168.61.4'
MME_CONF[@PEER_MME_IPV4_ADDRESS_FOR_S10_0@]='0.0.0.0/24'
MME_CONF[@PEER_MME_IPV4_ADDRESS_FOR_S10_1@]='0.0.0.0/24'

TAC_SGW_TEST='7'
tmph=`echo "$TAC_SGW_TEST / 256" | bc`
tmpl=`echo "$TAC_SGW_TEST % 256" | bc`
MME_CONF[@TAC-LB_SGW_TEST_0@]=`printf "%02x\n" $tmpl`
MME_CONF[@TAC-HB_SGW_TEST_0@]=`printf "%02x\n" $tmph`

MME_CONF[@MCC_SGW_0@]=${MME_CONF[@MCC@]}
MME_CONF[@MNC3_SGW_0@]=`printf "%03d\n" $(echo ${MME_CONF[@MNC@]} | sed -e "s/^0*//")`
TAC_SGW_0='1'
tmph=`echo "$TAC_SGW_0 / 256" | bc`
tmpl=`echo "$TAC_SGW_0 % 256" | bc`
MME_CONF[@TAC-LB_SGW_0@]=`printf "%02x\n" $tmpl`
MME_CONF[@TAC-HB_SGW_0@]=`printf "%02x\n" $tmph`

MME_CONF[@MCC_MME_0@]=${MME_CONF[@MCC@]}
MME_CONF[@MNC3_MME_0@]=`printf "%03d\n" $(echo ${MME_CONF[@MNC@]} | sed -e "s/^0*//")`
TAC_MME_0='5'
tmph=`echo "$TAC_MME_0 / 256" | bc`
tmpl=`echo "$TAC_MME_0 % 256" | bc`
MME_CONF[@TAC-LB_MME_0@]=`printf "%02x\n" $tmpl`
MME_CONF[@TAC-HB_MME_0@]=`printf "%02x\n" $tmph`

MME_CONF[@MCC_MME_1@]=${MME_CONF[@MCC@]}
MME_CONF[@MNC3_MME_1@]=`printf "%03d\n" $(echo ${MME_CONF[@MNC@]} | sed -e "s/^0*//")`
TAC_MME_1='6'
tmph=`echo "$TAC_MME_1 / 256" | bc`
tmpl=`echo "$TAC_MME_1 % 256" | bc`
MME_CONF[@TAC-LB_MME_1@]=`printf "%02x\n" $tmpl`
MME_CONF[@TAC-HB_MME_1@]=`printf "%02x\n" $tmph`

for K in "${!MME_CONF[@]}"; do 
  egrep -lRZ "$K" $PREFIX | xargs -0 -l sed -i -e "s|$K|${MME_CONF[$K]}|g"
  ret=$?;[[ ret -ne 0 ]] && echo "Tried to replace $K with ${MME_CONF[$K]}"
done

# Generate freeDiameter certificate
./check_mme_s6a_certificate $PREFIX mme.${MME_CONF[@REALM@]}
