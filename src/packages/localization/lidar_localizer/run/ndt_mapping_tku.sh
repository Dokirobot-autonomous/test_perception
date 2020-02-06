#! /bin/bash

xterm -e bash -c "export ROS_NAMESPACE=mkz; roslaunch lidar_localizer ndt_mapping_tku.launch " &
