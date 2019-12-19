#!/bin/bash

#BAG_FILE=UCB/191116/good2.launch
BAG_FILE=UCB/191116/3-2019-11-16-15-05-01.launch
PHONE1_NAME=android/galaxy1
PHONE_NAMES=$PHONE1_NAME
START=105
#START=30
START=50
RATE=0.2


#! roscore
xterm -e bash -c 'roscore' &
sleep 2

#! bagplayer
xterm -e bash -c "cd /media/ohashi/a65390d1-3a01-41f6-bcb9-97c998a6b643/dataset/; roslaunch $BAG_FILE rate:=$RATE start:=$START" &
sleep 1

##! phone_localizer.launch
#xterm -e bash -c "source ~/test_perception/devel/setup.bash; roslaunch smartphone_localizer phone_localizer.launch device_name:=$PHONE1_NAME pf_initial_xy_radius:=5 pf_gps_xy_bias:=10.0" &
#sleep 1

#! perception node mks
xterm -e bash -c "source ~/test_perception/devel/setup.bash; roslaunch imm_ukf_pda_track lidar_apollo_cnn_seg_tracker.launch vehicle_name:=mks points_raw:=velodyne_points" &
sleep 1

#! perception node prius
xterm -e bash -c "source ~/test_perception/devel/setup.bash; roslaunch imm_ukf_pda_track lidar_apollo_cnn_seg_tracker.launch vehicle_name:=prius points_raw:=rslidar_points" &
sleep 1

##! vehicle_phones_manager.launch
#xterm -e bash -c "source ~/test_perception/devel/setup.bash; roslaunch smartphone_localizer vehicle_phones_manager.launch vehicle_name:=mks phone_names:=$PHONE_NAMES" &
#sleep 1

#new terminal
xterm -e bash -c "rqt_console" &
sleep 1

#new terminal
xterm -e bash -c "rqt_logger_level" &
sleep 1

#new terminal
xterm -e bash -c "rqt_graph" &
sleep 1


#! new terminal
xterm -e bash -c "source ~/test_perception/devel/setup.bash; roscd smartphone_localizer/rviz/; rviz -d phone_vehicle_fusion.rviz"
sleep 1

