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
xterm -e bash -c "cd ~/cooperative_perception/; source devel/setup.bash; roslaunch lidar_localizer ndt_mapping_tku.launch" &
sleep 1

#new terminal 
#xterm -e bash -c 'rosbag play /media/ohashi/a65390d1-3a01-41f6-bcb9-97c998a6b643/dataset/KITTI/odometry/raw/07.bag --pause -r 0.1 --clock' 
xterm -e bash -c 'rosbag play /media/ohashi/a65390d1-3a01-41f6-bcb9-97c998a6b643/dataset/UCB/autoware-20190831123106.bag rslidar_points:=/sensor/velodyne/cloud_euclidean -r 0.2 --pause --clock' 
sleep 1


