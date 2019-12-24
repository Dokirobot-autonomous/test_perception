#! /bin/bash

#xterm -e bash -c "python ../launch/prius_driver.launch" &
#sleep 1

xterm -e bash -c "roslaunch cooperation_setup prius_setup.launch" &
sleep 1

#read -p "Hit enter to create cooperation: "
#
#xterm -e bash -c "roslaunch cooperation_setup connection_client_setup.launch s:=true" &
#sleep 1
#
#xterm -e bash -c "source exports.sh; export ROS_IP=\$PRIUS_IP; roslaunch cooperation_setup connection_client_setup.launch p:=true" &
#sleep 1
