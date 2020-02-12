#!/usr/bin/env bash

IFNAME_ANDROID=/home/ohashi/test_perception/src/dataset/UCB/200208/android/2020-02-08-12-41-20
IFNAME_MKZ=/home/ohashi/test_perception/src/dataset/UCB/200208/mkz/2020-02-08-12-41-21
IFNAME_MKS=/home/ohashi/test_perception/src/dataset/UCB/200208/mks/2020-02-08-12-40-47
IFNAME_PRIUS=/home/ohashi/test_perception/src/dataset/UCB/200208/prius/2020-02-08-12-41-32
OFNAME=/home/ohashi/test_perception/src/dataset/UCB/200208/2-2020-02-08-12-41-21

roslaunch mypkg rosbag_androidfix.launch input_file_name:=${IFNAME_ANDROID}.bag output_file_name:=${IFNAME_ANDROID}-androidfixed.bag
sleep 1

roslaunch mypkg rosbag_merger.launch input_file_names:="[${IFNAME_ANDROID}.bag,${IFNAME_MKZ}.bag,${IFNAME_MKS}.bag,${IFNAME_PRIUS}.bag]" output_file_name:="${OFNAME}.bag"
sleep 1

roslaunch mypkg rosbag_filter.launch input_file_name:="${OFNAME}.bag" output_file_name:="${OFNAME}-nonTF.bag"
sleep 1
