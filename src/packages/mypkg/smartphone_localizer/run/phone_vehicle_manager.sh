#!/bin/bash

#! bagfile
xterm -e bash -c "roslaunch /home/ohashi/test_perception/src/dataset/UCB/191213/1-2019-12-13-10-25-02.launch" & sleep 3
#! phone_localizer.launch
xterm -e bash -c "source ~/test_perception/devel/setup.bash; \
                  roslaunch smartphone_localizer phone_localizer.launch \
                  device_name:=$PHONE1_NAME pf_initial_xy_radius:=5 pf_gps_xy_bias:=10.0" & sleep 1
#! perception node mks
xterm -e bash -c "source ~/test_perception/devel/setup.bash; \
                  roslaunch imm_ukf_pda_track lidar_apollo_cnn_seg_tracker.launch \
                  vehicle_name:=mks points_raw:=velodyne_points" & sleep 1
#! vehicle_phones_manager.launch
xterm -e bash -c "source ~/test_perception/devel/setup.bash; \
                  roslaunch smartphone_localizer vehicle_phones_manager.launch
                  vehicle_name:=mks phone_names:=$PHONE_NAMES" & sleep 1

