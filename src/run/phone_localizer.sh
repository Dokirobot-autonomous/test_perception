#!/bin/bash

BAG_NUM=1
PLAY_RATE=1

if [ ${BAG_NUM} = 0 ]; then
  ## Open Parking Place 09/28
ORI_FIX=37.9181551934,-122.331354965,0.0
PF_INIT_FIX=37.918303,-122.331325,0.0
PF_INIT_QUAT=0.057025000453,0.253443002701,0.902796983719,0.342742472887
BAG_FILE=/media/ohashi/a65390d1-3a01-41f6-bcb9-97c998a6b643/dataset/UCB/190928.1/non_vehicle/2019-09-28-13-19-26.bag
elif [ $BAG_NUM = 1 ]; then
## Open Parking Place 09/28
ORI_FIX=37.9181551873,-122.331354939,0.0
PF_INIT_FIX=37.9180604745,-122.331348873,0.0
PER_INIT_DIR=-0.580381653
BAG_FILE=/media/ohashi/a65390d1-3a01-41f6-bcb9-97c998a6b643/dataset/UCB/190928.1/2019-09-28-14-07-05.bag
elif [ $BAG_NUM = 2 ]; then
## Open Parking Place 09/28
ORI_FIX=37.9181016667,-122.331386667,0.0
PF_INIT_FIX=37.918286,-122.331421,0.0
PER_INIT_DIR=2.48057899
BAG_FILE=/media/ohashi/a65390d1-3a01-41f6-bcb9-97c998a6b643/dataset/UCB/191027/mkz/2019-10-27-19-05-40.bag
fi

#new terminal 
xterm -e bash -c 'roscore' &
sleep 2

#new terminal 
xterm -e bash -c 'source ~/test_perception/devel/setup.bash; rviz' &
sleep 2

#new terminal 
xterm -e bash -c "rosbag play ${BAG_FILE} -r ${PLAY_RATE} -s 25 --pause --clock" &
sleep 1

#new terminal
if [ ${BAG_NUM} = 0 ]; then
xterm -e bash -c "rosrun tf2_ros static_transform_publisher 0. 0. 0. -3.1415 0. 0. /base_link /rslidar" &
elif [ $BAG_NUM = 1 ]; then
xterm -e bash -c "rosrun tf2_ros static_transform_publisher 0. 0. 0. -3.1415 0. 0. /base_link_true /rslidar" &
xterm -e bash -c "rosrun tf2_ros static_transform_publisher 0. 0. 0. -2.282229840820556 -0.016004571027008946 0.038261584385544466 /odom /base_link_true" &
elif [ $BAG_NUM = 2 ]; then
#xterm -e bash -c "rosrun tf2_ros static_transform_publisher 0. 0. 0. -3.1415 0. 0. /base_link_true /velodyne" &
xterm -e bash -c "rosrun tf2_ros static_transform_publisher 0. 0. 0. 1.7207 0. 0. /base_link_true /velodyne" &
xterm -e bash -c "rosrun tf2_ros static_transform_publisher 0. 0. 0. 2.434814552290681 0. 0. /odom /base_link_true" &
fi
sleep 1

#new terminal
xterm -e bash -c "source ~/test_perception/devel/setup.bash; roslaunch lidar_apollo_cnn_seg_detect lidar_apollo_cnn_seg_detect.launch network_definition_file:=/home/ohashi/test_perception/src/packages/perception/lidar_detector/packages/lidar_apollo_cnn_seg_detect/models/velodyne64/deploy.prototxt pretrained_model_file:=/home/ohashi/test_perception/src/packages/perception/lidar_detector/packages/lidar_apollo_cnn_seg_detect/models/velodyne64/deploy.caffemodel points_src:=/rslidar_points marker_display_duration:=1.0" &
sleep 1

#new terminal
xterm -e bash -c "source ~/test_perception/devel/setup.bash; roslaunch imm_ukf_pda_track imm_ukf_pda_track.launch" &
sleep 1

#new terminal
xterm -e bash -c "source ~/test_perception/devel/setup.bash; roslaunch smartphone_localizer phone_localizer_old.launch ori_fix:=${ORI_FIX} person_init_fix:=${PF_INIT_FIX} person_init_dir:=${PER_INIT_DIR}"
sleep 1

