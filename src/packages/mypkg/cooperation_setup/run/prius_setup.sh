#! /bin/bash

ntpdate -q 192.168.0.101

read -p "Is time sync ok? : "

xterm -e bash -c "python ../launch/prius_driver.launch" &

read -p "Make sure to turn \'XSENS\' on: "

read -p "Hit enter after calibration: "

xterm -e bash -c "roslaunch cooperation_setup prius_setup.launch" &
sleep 1

