#!/usr/bin/env bash


#! bagfile
xterm -e bash -c "roslaunch /home/ohashi/test_perception/src/dataset/UCB/191213/1-2019-12-25-10-29.launch" & sleep 3
#! detectioin node mks
xterm -e bash -c "source ~/test_perception/devel/setup.bash; \
                  roslaunch lidar_apollo_cnn_seg_detect lidar_apollo_cnn_seg_detect.launch \
                  name_space:=mkz points_src:=points_raw \
                  points_out:=/detection/lidar_detector/points_cluster \
                  objects_out:=/detection/lidar_detector/objects"  & sleep 1
#! detectioin node prius
xterm -e bash -c "source ~/test_perception/devel/setup.bash; \
                  roslaunch lidar_apollo_cnn_seg_detect lidar_apollo_cnn_seg_detect.launch \
                  name_space:=prius points_src:=rslidar_points \
                  points_out:=/detection/lidar_detector/points_cluster \
                  objects_out:=/detection/lidar_detector/objects"  & sleep 1
#! tracker node
xterm -e bash -c "source ~/test_perception/devel/setup.bash; \
                  roslaunch imm_ukf_pda_track imm_ukf_pda_track.launch" & sleep 1


