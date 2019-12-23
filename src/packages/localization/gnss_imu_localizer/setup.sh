#!/bin/bash


grep -lr 'gnss_imu_localizer' ./* | xargs sed -i.bak -e "s/gnss_imu_localizer/$1/g"
grep -lr 'gnss_imu_localizer_node' ./* | xargs sed -i.bak -e "s/gnss_imu_localizer_node/$2/g"
grep -lr 'GnssImuLocalizer' ./* | xargs sed -i.bak -e "s/GnssImuLocalizer/$3/g"
