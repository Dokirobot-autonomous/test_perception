#!/bin/bash


#new terminal 
xterm -e bash -c 'roscore' &
sleep 1

#new terminal 
xterm -e bash -c 'source ~/test_perception/devel/setup.bash; rviz' &
sleep 1

#new terminal 
xterm -e bash -c "source ~/test_perception/devel/setup.bash; roslaunch lidar_apollo_cnn_seg_detect lidar_apollo_cnn_seg_detect.launch network_definition_file:=/home/ohashi/test_perception/src/packages/perception/lidar_detector/packages/lidar_apollo_cnn_seg_detect/models/velodyne64/deploy.prototxt pretrained_model_file:=/home/ohashi/test_perception/src/packages/perception/lidar_detector/packages/lidar_apollo_cnn_seg_detect/models/velodyne64/deploy.caffemodel points_src:=/rslidar_points marker_display_duration:=1.0" &
sleep 1

#new terminal 
xterm -e bash -c "source ~/test_perception/devel/setup.bash; roslaunch imm_ukf_pda_track imm_ukf_pda_track.launch" &
sleep 1

#new terminal 
#xterm -e bash -c 'rosbag play /media/ohashi/a65390d1-3a01-41f6-bcb9-97c998a6b643/dataset/KITTI/odometry/raw/07.bag -r 0.1 -s 5 --pause --clock' 
#xterm -e bash -c 'rosbag play /media/ohashi/a65390d1-3a01-41f6-bcb9-97c998a6b643/dataset/KITTI/raw_data/original/kitti_2011_09_28_drive_0021_synced.bag -r 0.1 --pause --clock' 
#xterm -e bash -c 'rosbag play /media/ohashi/a65390d1-3a01-41f6-bcb9-97c998a6b643/dataset/UCB/autoware-20190831123106_tf.bag rslidar_points:=/sensor/velodyne/cloud_euclidean -r 0.5 -s 60 --pause --clock' 
#xterm -e bash -c 'rosbag play /media/ohashi/a65390d1-3a01-41f6-bcb9-97c998a6b643/dataset/UCB/190921/autoware-20190921121800.bag rslidar_points:=/sensor/velodyne/cloud_euclidean -r 1 -s 2 --pause --clock'

#new terminal 
xterm -e bash -c 'rosbag play /media/ohashi/a65390d1-3a01-41f6-bcb9-97c998a6b643/dataset/UCB/190928.1/non_vehicle/2019-09-28-13-19-26.bag -r 1.0 -s 22 --pause --clock' &
sleep 1
#new terminal 
xterm -e bash -c 'rosrun tf2_ros static_transform_publisher 0. 0. 0. -3.1415 0. 0. /base_link /rslidar' 
sleep 1 



