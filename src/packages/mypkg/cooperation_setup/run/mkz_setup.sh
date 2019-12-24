#! /bin/bash

htpdate 192.168.0.102
ntpdate -q 192.168.0.102

read -p "Is time sync ok? : "

xterm -e bash -c "roslaunch cooperation_setup mkz_driver.launch" &
sleep 1

read -p "Hit enter after calibration: "

xterm -e bash -c "roslaunch cooperation_setup mkz_setup.launch" &
sleep 1

