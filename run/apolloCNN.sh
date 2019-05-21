#!/bin/bash

#new terminal 
xterm -e bash -c 'roscore' &
sleep 1

#new terminal 
xterm -e bash -c 'source ~/test_perception/devel/setup.bash; rviz' &
sleep 1

#new terminal 
xterm -e bash -c 'source ~/test_perception/devel/setup.bash; roslaunch lidar_apollo_cnn_seg_detect lidar_apollo_cnn_seg_detect.launch network_definition_file:=/home/ohashi/test_perception/src/packages/perception/lidar_detector/packages/lidar_apollo_cnn_seg_detect/models/velodyne64/deploy.prototxt pretrained_model_file:=/home/ohashi/test_perception/src/packages/perception/lidar_detector/packages/lidar_apollo_cnn_seg_detect/models/velodyne64/deploy.caffemodel points_src:=/sensor/velodyne/cloud_euclidean' &
sleep 1

#new terminal 
xterm -e bash -c 'rosbag play ~/dataset/KITTI/07.bag -r 0.1 -s 5 --pause --clock' 
sleep 1


