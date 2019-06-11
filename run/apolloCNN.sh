#!/bin/bash


#new terminal 
xterm -e bash -c 'roscore' &
sleep 1

#new terminal 
xterm -e bash -c 'source ~/test_perception/devel/setup.bash; rviz' &
sleep 1

#new terminal 
xterm -e bash -c "source ~/test_perception/devel/setup.bash; roslaunch lidar_apollo_cnn_seg_detect lidar_apollo_cnn_seg_detect.launch network_definition_file:=/home/ohashi/test_perception/src/packages/perception/lidar_detector/packages/lidar_apollo_cnn_seg_detect/models/velodyne64/deploy.prototxt pretrained_model_file:=/home/ohashi/test_perception/src/packages/perception/lidar_detector/packages/lidar_apollo_cnn_seg_detect/models/velodyne64/deploy.caffemodel points_src:=/sensor/velodyne/cloud_euclidean marker_display_duration:=1.0" &
sleep 1

#new terminal 
xterm -e bash -c 'rosbag play /media/ohashi/a65390d1-3a01-41f6-bcb9-97c998a6b643/dataset/KITTI/odometry/raw/07.bag -r 0.1 -s 5 --pause --clock' 
#xterm -e bash -c 'rosbag play /media/ohashi/a65390d1-3a01-41f6-bcb9-97c998a6b643/dataset/KITTI/raw_data/original/kitti_2011_09_28_drive_0021_synced.bag -r 0.1 --pause --clock' 
sleep 1


