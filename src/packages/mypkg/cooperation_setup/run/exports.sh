#!/bin/bash

export MKS_IP=192.168.0.100 # MKS
export MKZ_IP=192.168.0.101 # MKZ
export PRIUS_IP=192.168.0.102 # Prius

export MASTER=$MKZ_IP
#export MASTER=localhost

export ROS_MASTER_URI=http://MASTER:1234/

export ROS_IP=$MKZ_IP
#export ROS_IP=localhost


