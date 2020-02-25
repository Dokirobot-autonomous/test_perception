#! /bin/bash

htpdate www.google.com

#ntpdate -q 192.168.0.101

#sudo systemctl restart chrony.service
#sudo systemctl start chrony.service
#sudo systemctl stop chrony.service

read -p "Is time sync ok? : "

xterm -e bash -c "source ~/ohashi/test_perception/devel/setup.bash; roslaunch cooperation_setup mkz_driver.launch" &
sleep 1

read -p "Hit enter after android_time measured: "

xterm -e bash -c "source ~/ohashi/test_perception/devel/setup.bash; roslaunch cooperation_setup mkz_setup.launch" &
sleep 1

read -p "Hit enter to connect others: "

xterm -e bash -c "source ~/ohashi/test_perception/devel/setup.bash; roslaunch cooperation_setup connection_client_setup.launch s:=true vehicle_name:=mkz" & 
sleep 1

source ~/ohashi/test_perception/src/packages/mypkg/cooperation_setup/run/exports.sh

xterm -e bash -c "export ROS_MASTER_URI=http://$MKZ_IP:1234/; export ROS_IP=$MKZ_IP; source ~/ohashi/test_perception/devel/setup.bash; roslaunch cooperation_setup connection_client_setup.launch p:=true vehicle_name:=mkz;" &
sleep 1

