#! /bin/bash

#xterm -e bash -c "roslaunch cooperation_setup mkz_driver.launch" &
#sleep 1

xterm -e bash -c "roslaunch cooperation_setup mkz_setup.launch" &
sleep 1

xterm -e bash -c "roslaunch cooperation_setup fix_tf_publisher.launch frame_id_fix1:=prius/odom frame_id_fix2:=mkz/odom" &
sleep 1

#read -p "Hit enter to create cooperation: "
#
#xterm -e bash -c "roslaunch cooperation_setup connection_client_setup.launch s:=true" &
#sleep 1
#
#xterm -e bash -c "source exports.sh; export ROS_IP=\$MKZ_IP; roslaunch cooperation_setup connection_client_setup.launch p:=true" &
#sleep 1
