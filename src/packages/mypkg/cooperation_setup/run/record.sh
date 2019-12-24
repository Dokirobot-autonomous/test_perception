#!/bin/bash

source exports.sh;

read -p "Hit enter after android is connected: "

xterm -e bash -c "echo cd ohashi/bag; \
                  echo rosbag record -e \"/mks/(.*)\" -x \"(.*)/image_raw\" -e \"/tf(.*)\" ; \
                  sshpass -p bdd_path123 ssh -o StrictHostKeyChecking=no bdd@$MKS_IP" & sleep 1
xterm -e bash -c "echo cd ohashi/bag; \
                  echo rosbag record -e \"/mkz/(.*)\" -x \"(.*)/image_raw\" -e \"/tf(.*)\" ; \
                  sshpass -p bdd_path123 ssh -o StrictHostKeyChecking=no bdd@$MKZ_IP" & sleep 1
xterm -e bash -c "echo cd ohashi/bag; \
                  echo rosbag record -e \"/prius/(.*)\" -x \"(.*)/image_raw\" -e \"/tf(.*)\" ; \
                  sshpass -p mscc2019 ssh -o StrictHostKeyChecking=no mscc@$PRIUS_IP" & sleep 1
xterm -e bash -c "echo export ROS_MASTER_URI=http://$MASTER_IP:1234/;\
                  echo export ROS_IP=$MASTER_IP; \
                  echo cd ohashi/bag; \
                  echo rosbag record -a; \
                  sshpass -p bdd_path123 ssh -o StrictHostKeyChecking=no bdd@$MKS_IP" & sleep 1
#xterm -e bash -c "echo export ROS_MASTER_URI=http://$MASTER_IP:1234/;\
#                  echo export ROS_IP=$MASTER_IP; \
#                  echo cd ohashi/bag; \
#                  echo rosbag record -a; \
#                  sshpass -p ubu2jonpw ssh -o StrictHostKeyChecking=no bdd@$SURFACE_IP" & sleep 1

