#!/usr/bin/env bash

xterm -e bash -c "source exports.bash; roslaunch cooperation_setup connection_client_setup.launch s:=true" &
sleep 1

xterm -e bash -c "source exports.bash; roslaunch cooperation_setup connection_client_setup.launch p:=true" &
sleep 1