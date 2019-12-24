#!/bin/bash

export MKS_IP=192.168.0.100 # MKS
export MKZ_IP=192.168.0.101 # MKZ
export PRIUS_IP=192.168.0.102 # Prius
export SURFACE_IP=192.168.0.104 # Surface

export MASTER_IP=$MKZ_IP
#export MASTER_IP=$SURFACE_IP
#export MASTER_IP="localhost"

export ROS_MASTER_URI=http://$MASTER_IP:1234/


