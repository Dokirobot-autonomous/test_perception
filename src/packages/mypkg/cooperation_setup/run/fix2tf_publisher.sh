#! /bin/bash

xterm -e bash -c "roslaunch cooperation_setup fix_tf_publisher.launch frame_id_fix1:=prius/odom frame_id_fix2:=mkz/odom vehicle_name:=\"mkz\"" &
sleep 1
xterm -e bash -c "roslaunch cooperation_setup fix_tf_publisher.launch frame_id_fix1:=prius/odom frame_id_fix2:=mks/odom vehicle_name:=\"mks\"" &
sleep 1
