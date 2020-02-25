#!/usr/bin/env bash

IFNAME_ANDROID=/home/ohashi/test_perception/src/dataset/UCB/200208/android/2020-02-08-12-53-57
IFNAME_MKZ=/home/ohashi/test_perception/src/dataset/UCB/200208/mkz/2020-02-08-12-54-06
IFNAME_MKS=/home/ohashi/test_perception/src/dataset/UCB/200208/mks/2020-02-08-12-53-22
IFNAME_PRIUS=/home/ohashi/test_perception/src/dataset/UCB/200208/prius/2020-02-08-12-54-06
OFNAME=/home/ohashi/test_perception/src/dataset/UCB/200208/3-2020-02-08-12-54-06
ADD_TIME=[0.0,0.0,35.523647376,-9.324607611]

roslaunch mypkg rosbag_androidfix.launch input_file_name:=${IFNAME_ANDROID}.bag output_file_name:=${IFNAME_ANDROID}-androidfixed.bag
sleep 1

roslaunch mypkg rosbag_merger.launch input_file_names:="[${IFNAME_ANDROID}.bag,${IFNAME_MKZ}.bag,${IFNAME_MKS}.bag,${IFNAME_PRIUS}.bag]" add_times:="${ADD_TIME}" output_file_name:="${OFNAME}.bag"
sleep 1

roslaunch mypkg rosbag_filter.launch input_file_name:="${OFNAME}.bag" output_file_name:="${OFNAME}-nonTF.bag"
sleep 1
