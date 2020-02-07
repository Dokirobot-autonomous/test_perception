#! /bin/bash

#htpdate 192.168.0.102
#ntpdate -q 192.168.0.102

read -p "Is time sync ok? : "

python ../launch/prius_driver.py

read -p "Make sure to turn \'XSENS\' on: "

read -p "Hit enter after calibration: "

xterm -e bash -c "roslaunch cooperation_setup prius_setup.launch" &
sleep 1

