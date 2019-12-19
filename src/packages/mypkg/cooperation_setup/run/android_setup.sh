#! /bin/bash

roscd cooperation_setup/run/

if [ $# -ne 1 ]; then
  echo "Input phone name. "
  exit 1
fi

PC_S_TIME=$(date +%s)
SP_S_TIME=$(adb shell "date +%s")
PC_N_TIME=$(date +%6N)
SP_N_TIME=$(adb shell 'echo ${EPOCHREALTIME:11:17}')

DT_S=$((SP_S_TIME - PC_S_TIME))
DT_N=$((SP_N_TIME - PC_N_TIME))

echo "$1 $DT_S ${DT_N}000" >> "../.$(date +%Y-%m-%d)-android-dt.txt"

DT="$DT_S $DT_N"

echo $DT_S
echo $DT_N
echo $DT