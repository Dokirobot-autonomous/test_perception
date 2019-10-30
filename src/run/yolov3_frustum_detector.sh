#!/bin/bash

#new terminal 
xterm -e bash -c 'roscore' &
sleep 1

#new terminal 
xterm -e bash -c 'source ~/test_perception/devel/setup.bash; rviz' &
sleep 1

#new terminal 
xterm -e bash -c 'source ~/test_perception/devel/setup.bash; roslaunch vision_darknet_detect vision_yolo3_detect.launch' &
sleep 1

#new terminal 
xterm -e bash -c 'source ~/test_perception/devel/setup.bash; roslaunch frustum_detector frustum_points_extractor.launch 
' &
sleep 1

#new terminal 
xterm -e bash -c 'source ~/test_perception/devel/setup.bash; roslaunch lidar_euclidean_cluster_detect frustum_ec_detector.launch' &
sleep 1

#new terminal 
xterm -e bash -c 'rosbag play /media/ohashi/a65390d1-3a01-41f6-bcb9-97c998a6b643/dataset/KITTI/odometry/raw/07.bag -r 0.1 -s 0 --pause --clock' 
#xterm -e bash -c 'rosbag play /media/ohashi/a65390d1-3a01-41f6-bcb9-97c998a6b643/dataset/KITTI/raw_data/original/kitti_2011_09_28_drive_0021_synced.bag -r 0.1 --pause --clock' 
sleep 1
