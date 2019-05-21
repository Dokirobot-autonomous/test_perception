#!/bin/bash

#new terminal 
xterm -e bash -c 'roscore' &
sleep 1

#new terminal 
xterm -e bash -c 'source ~/test_perception/devel/setup.bash; rviz' &
sleep 1

#new terminal 
xterm -e bash -c 'source ~/test_perception/devel/setup.bash; roslaunch vision_darknet_detect vision_yolo3_detect.launch' &
sleep 1

#new terminal 
xterm -e bash -c 'source ~/test_perception/devel/setup.bash; roslaunch range_vision_fusion range_vision_fusion.launch' &
sleep 1

#new terminal 
xterm -e bash -c 'rosbag play ~/dataset/KITTI/07.bag --pause --clock' 
sleep 1


