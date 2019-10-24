#!/bin/bash


#new terminal 
xterm -e bash -c 'roscore' &
sleep 2

#new terminal 
xterm -e bash -c "source ~/test_perception/devel/setup.bash; roslaunch smartphone_localizer phonesensors_raw.launch" &
sleep 1

#new terminal 
#xterm -e bash -c "source ~/test_perception/devel/setup.bash; roslaunch smartphone_localizer phone_localizer.launch" &
#sleep 1

#new terminal 
xterm -e bash -c 'source ~/test_perception/devel/setup.bash; rviz' &
sleep 1

#new terminal 
xterm -e bash -c 'rosbag play /media/ohashi/a65390d1-3a01-41f6-bcb9-97c998a6b643/dataset/UCB/190928.1/non_vehicle/2019-09-28-13-19-26.bag -r 0.5 -s 22 --pause --clock' 
sleep 1



