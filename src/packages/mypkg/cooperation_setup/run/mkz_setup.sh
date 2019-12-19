#! /bin/bash

if [ $# -ne 3 ]; then
  echo "Input lat lon ele. "
  exit 1
fi

xterm -e bash -c "roslaunch cooperation_setup mkz_setup.launch origin_fix:=$1,$2,$3"
sleep 1