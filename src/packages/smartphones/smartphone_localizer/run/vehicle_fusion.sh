#!/bin/bash

#BAG_NAME=UCB/191116/1-2019-11-16-14-35-22
BAG_NAME=UCB/191211/prius/2019-12-11-15-38-46
#BAG_FILE=UCB/191116/2-2019-11-16-14-52-45_tmp.launch
#BAG_FILE=UCB/191116/3-2019-11-16-15-05-01.launch
PHONE1_NAME=android/galaxy1
PHONE_NAMES=$PHONE1_NAME
START=0
RATE=0.1


#! roscore
xterm -e bash -c 'roscore' &
sleep 2

#! bagplayer
xterm -e bash -c "roslaunch mypkg rosbag_player.launch bagfile_names:=/media/ohashi/a65390d1-3a01-41f6-bcb9-97c998a6b643/dataset/$BAG_NAME.bag tf_launch:=/media/ohashi/a65390d1-3a01-41f6-bcb9-97c998a6b643/dataset/$BAG_NAME.launch" &
sleep 1

##! detectioin node mks
#xterm -e bash -c "source ~/test_perception/devel/setup.bash; roslaunch lidar_apollo_cnn_seg_detect lidar_apollo_cnn_seg_detect.launch \
#                    name_space:=mks points_src:=velodyne_points points_out:=/detection/lidar_detector/points_cluster objects_out:=/detection/lidar_detector/objects"  &
#sleep 1
#
##! detectioin node prius
#xterm -e bash -c "source ~/test_perception/devel/setup.bash; roslaunch lidar_apollo_cnn_seg_detect lidar_apollo_cnn_seg_detect.launch \
#                    name_space:=prius points_src:=rslidar_points points_out:=/detection/lidar_detector/points_cluster objects_out:=/detection/lidar_detector/objects"  &
#sleep 1
#
##! tracker node
#xterm -e bash -c "source ~/test_perception/devel/setup.bash; roslaunch imm_ukf_pda_track imm_ukf_pda_track.launch" &
#sleep 1


##! perception node mks
#xterm -e bash -c "source ~/test_perception/devel/setup.bash; roslaunch imm_ukf_pda_track lidar_apollo_cnn_seg_tracker.launch \
#                    vehicle_name:=mks points_raw:=velodyne_points"  &
#sleep 1

#! perception node prius
xterm -e bash -c "source ~/test_perception/devel/setup.bash; roslaunch imm_ukf_pda_track lidar_apollo_cnn_seg_tracker.launch \
                    vehicle_name:=prius points_raw:=rslidar_points"  &
sleep 1

##! vehicle_phones_manager.launch
#xterm -e bash -c "source ~/test_perception/devel/setup.bash; roslaunch smartphone_localizer vehicle_phones_manager.launch vehicle_name:=mks phone_names:=$PHONE_NAMES" &
#sleep 1


#new terminal
xterm -e bash -c "source ~/test_perception/devel/setup.bash; roscd smartphone_localizer/rqt/; rqt" &
sleep 1

#! new terminal
xterm -e bash -c "source ~/test_perception/devel/setup.bash; roscd smartphone_localizer/rviz/; rviz -d phone_vehicle_fusion.rviz"
sleep 1

