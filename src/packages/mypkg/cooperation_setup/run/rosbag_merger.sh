#!/usr/bin/env bash

IFNAME_ANDROID=/home/ohashi/test_perception/src/dataset/UCB/191224/android/2019-12-24-14-31-4pedestrians-2vehicles-1phone-androidfixed_filtered
IFNAME_MKZ=/home/ohashi/test_perception/src/dataset/UCB/191224/mkz/2019-12-24-14-31-4pedestrians-2vehicles-1phone
IFNAME_MKS=/home/ohashi/test_perception/src/dataset/UCB/191224/mks/2019-12-24-14-31-4pedestrians-2vehicles-1phone
OFNAME=/home/ohashi/test_perception/src/dataset/UCB/191224/2019-12-24-14-31-4pedestrians-2vehicles-1phone

#roslaunch mypkg rosbag_androidfix.launch input_file_name:=${IFNAME_ANDROID}.bag output_file_name:=${IFNAME_ANDROID}-androidfixed.bag
#sleep 1

roslaunch mypkg rosbag_merger.launch input_file_names:="[${IFNAME_ANDROID}.bag,${IFNAME_MKZ}.bag,${IFNAME_MKS}.bag]" output_file_name:="${OFNAME}.bag" &
sleep 1

roslaunch mypkg rosbag_filter.launch input_file_name:="${OFNAME}.bag" output_file_name:="${OFNAME}-nonTF.bag" &
sleep 1
