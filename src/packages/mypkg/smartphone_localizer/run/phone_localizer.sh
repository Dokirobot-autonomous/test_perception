#!/bin/bash

PHONE1_NAME=android/galaxy1
PHONE_NAMES=$PHONE1_NAME


#! roscore
xterm -e bash -c 'roscore' &
sleep 2

#! rosbag
xterm -e bash -c "cd /media/ohashi/a65390d1-3a01-41f6-bcb9-97c998a6b643/dataset/UCB/191116/; roslaunch good1.launch rate:=1.0 start:=22" &
sleep 1

#! phone_localizer.launch
xterm -e bash -c "source ~/test_perception/devel/setup.bash; roslaunch smartphone_localizer phone_localizer.launch device_name:=$PHONE1_NAME pf_gps_xy_bias:=10.0" &
sleep 1

#new terminal
xterm -e bash -c "rqt_console" &
sleep 1

#new terminal
xterm -e bash -c "rqt_logger_level" &
sleep 1

#new terminal
xterm -e bash -c "rqt_graph" &
sleep 1


#! new terminal
xterm -e bash -c "source ~/test_perception/devel/setup.bash; roscd smartphone_localizer/rviz/; rviz -d phone_localizers.rviz"
sleep 1

