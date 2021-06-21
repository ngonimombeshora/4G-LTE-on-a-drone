#!/bin/bash

cd /openair-hss/scripts

Cassandra_Server_IP='172.17.0.2'
PREFIX='/openair-hss/etc'
MY_REALM='openairinterface.org'
MY_APN1='apn1.carrier.com'
MY_APN2='apn2.carrier.com'
MY_LTE_K='0c0a34601d4f07677303652c0462535b'
MY_OP_K='63bfa50ee6523365ff14c1f45f88737d'
MY_IMSI='320230100000001'
MY_USERS='200'

mkdir -p /openair-hss/logs

# provision users
./data_provisioning_users --apn $MY_APN1 --apn2 $MY_APN2 --key $MY_LTE_K --imsi-first $MY_IMSI --msisdn-first 00000001 --mme-identity mme.$MY_REALM --no-of-users $MY_USERS --realm $MY_REALM --truncate True --verbose True --cassandra-cluster $Cassandra_Server_IP
# provision mme
./data_provisioning_mme --id 3 --mme-identity mme.$MY_REALM --realm $MY_REALM --ue-reachability 1 --truncate True  --verbose True -C $Cassandra_Server_IP


declare -A HSS_CONF
HSS_CONF[@PREFIX@]=$PREFIX
HSS_CONF[@REALM@]=$MY_REALM
HSS_CONF[@HSS_FQDN@]="hss.${HSS_CONF[@REALM@]}"
HSS_CONF[@HSS_HOSTNAME@]="hss"
HSS_CONF[@cassandra_Server_IP@]=$Cassandra_Server_IP
HSS_CONF[@OP_KEY@]=$MY_OP_K
HSS_CONF[@ROAMING_ALLOWED@]='true'
for K in "${!HSS_CONF[@]}"; do	egrep -lRZ "$K" $PREFIX | xargs -0 -l sed -i -e "s|$K|${HSS_CONF[$K]}|g"; done
sed -i -e 's/#ListenOn.*$/ListenOn = "192.168.61.2";/g' $PREFIX/hss_rel14_fd.conf
./make_certs.sh hss ${HSS_CONF[@REALM@]} $PREFIX
