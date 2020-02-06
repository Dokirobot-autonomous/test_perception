#! /bin/bash

ntpdate -q 192.168.0.101

read -p "Is time sync ok? : "

xterm -e bash -c "source ~/ohashi/test_perception/devel/setup.bash; python ~/ohashi/test_perception/src/packages/mypkg/cooperation_setup/launch/prius_driver.python" &
sleep 1

read -p "Make sure to turn \'XSENS\' on: "

read -p "Hit enter after calibration: "

xterm -e bash -c "source ~/ohashi/test_perception/devel/setup.bash; roslaunch cooperation_setup prius_setup.launch" &
sleep 1

read -p "Hit enter to connect others: "

xterm -e bash -c "source ~/ohashi/test_perception/devel/setup.bash; roslaunch cooperation_setup connection_client_setup.launch s:=true vehicle_name:=prius" &
sleep 1

source ~/ohashi/test_perception/src/packages/mypkg/cooperation_setup/run/exports.sh

xterm -e bash -c "export ROS_MASTER_URI=http://$MKZ_IP:1234/; export ROS_IP=$PRIUS_IP; source ~/ohashi/test_perception/devel/setup.bash; roslaunch cooperation_setup connection_client_setup.launch p:=true vehicle_name:=prius;" &
sleep 1

