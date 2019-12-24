#!/bin/bash

echo "Have you finished clock sync?"
read -p "htpdate www.google.com"

source exports.sh

xterm -e bash -c "sshpass -p bdd_path123 ssh -o StrictHostKeyChecking=no bdd@$MKS_IP" &
sleep 1

xterm -e bash -c "sshpass -p bdd_path123 ssh -o StrictHostKeyChecking=no bdd@$MKZ_IP" &
sleep 1

xterm -e bash -c "sshpass -p mscc2019 ssh -o StrictHostKeyChecking=no mscc@$PRIUS_IP" &
sleep 1
