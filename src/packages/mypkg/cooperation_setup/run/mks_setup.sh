#! /bin/bash

#htpdate 192.168.0.102
#ntpdate -q 192.168.0.102

read -p "Is time sync ok? : "

xterm -e bash -c "source ~/ohashi/test_perception/devel/setup.bash; roslaunch cooperation_setup mks_driver.launch" &
sleep 1

read -p "Hit enter after microstrain_3dm_gx5_45_node is correctly launched: "

xterm -e bash -c "source ~/ohashi/test_perception/devel/setup.bash; roslaunch cooperation_setup mks_setup.launch" &
sleep 1

