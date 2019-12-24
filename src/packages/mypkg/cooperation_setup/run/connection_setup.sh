#!/bin/bash

source exports.sh;

xterm -e bash -c "export ROS_IP=$MASTER_IP; roscore -p 1234" &

read -p "Hit enter after android is connected: "

xterm -e bash -c "echo source ohashi/test_perception/devel/setup.bash; \
                  echo roslaunch cooperation_setup connection_client_setup.launch s:=true vehicle_name:=mks; \
                  sshpass -p bdd_path123 ssh -o StrictHostKeyChecking=no bdd@$MKS_IP" & sleep 1
xterm -e bash -c "echo 'source ohashi/test_perception/devel/setup.bash'; \
                  echo 'roslaunch cooperation_setup connection_client_setup.launch s:=true vehicle_name:=mkz'; \
                  sshpass -p bdd_path123 ssh -o StrictHostKeyChecking=no bdd@$MKZ_IP" & sleep 1
xterm -e bash -c "echo 'source ohashi/test_perception/devel/setup.bash'; \
                  echo 'roslaunch cooperation_setup connection_client_setup.launch s:=true vehicle_name:=prius'; \
                  sshpass -p mscc2019 ssh -o StrictHostKeyChecking=no mscc@$PRIUS_IP" & sleep 1

xterm -e bash -c "echo export ROS_MASTER_URI=http://$MASTER_IP:1234/;\
                  echo export ROS_IP=$MKS_IP; \
                  echo source ohashi/test_perception/devel/setup.bash; \
                  echo roslaunch cooperation_setup connection_client_setup.launch p:=true vehicle_name:=mks; \
                  sshpass -p bdd_path123 ssh -o StrictHostKeyChecking=no bdd@$MKS_IP" & sleep 1
xterm -e bash -c "echo export ROS_MASTER_URI=http://$MASTER_IP:1234/;\
                  echo export ROS_IP=$MKZ_IP; \
                  echo source ohashi/test_perception/devel/setup.bash; \
                  echo roslaunch cooperation_setup connection_client_setup.launch p:=true vehicle_name:=mkz; \
                  sshpass -p bdd_path123 ssh -o StrictHostKeyChecking=no bdd@$MKZ_IP" & sleep 1
xterm -e bash -c "echo export ROS_MASTER_URI=http://$MASTER_IP:1234/;\
                  echo export ROS_IP=$PRIUS_IP; \
                  echo source ohashi/test_perception/devel/setup.bash; \
                  echo roslaunch cooperation_setup connection_client_setup.launch p:=true vehicle_name:=prius; \
                  sshpass -p bdd_path123 ssh -o StrictHostKeyChecking=no bdd@$PRIUS_IP" & sleep 1


#xterm -e bash -c "roslaunch cooperation_setup fix_tf_publisher.launch frame_id_fix1:=prius/odom frame_id_fix2:=mkz/odom vehicle_name:=\"mkz\"" &
#sleep 1
#xterm -e bash -c "roslaunch cooperation_setup fix_tf_publisher.launch frame_id_fix1:=prius/odom frame_id_fix2:=mks/odom vehicle_name:=\"mks\"" &
#sleep 1
