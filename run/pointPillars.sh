#!/bin/bash

#new terminal 
xterm -e bash -c 'roscore' &
sleep 1

#new terminal 
xterm -e bash -c 'source ~/test_perception/devel/setup.bash; rviz' &
sleep 1

#new terminal 
xterm -e bash -c 'source ~/test_perception/devel/setup.bash; roslaunch lidar_point_pillars lidar_point_pillars.launch pfe_onnx_file:=/home/ohashi/test_perception/src/packages/perception/lidar_detector/packages/lidar_point_pillars/model/pfe.onnx rpn_onnx_file:=/home/ohashi/test_perception/src/packages/perception/lidar_detector/packages/lidar_point_pillars/model/rpn.onnx' &
sleep 1

#new terminal 
xterm -e bash -c 'rosbag play ~/dataset/KITTI/07.bag --pause --clock' 
sleep 1


