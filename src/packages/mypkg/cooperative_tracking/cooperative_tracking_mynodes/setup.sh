#!/bin/bash


grep -lr 'cooperative_tracking_mynodes' ./* | xargs sed -i.bak -e "s/cooperative_tracking_mynodes/$1/g"
grep -lr 'object_merger' ./* | xargs sed -i.bak -e "s/object_merger/$2/g"
grep -lr 'ObjectMerger' ./* | xargs sed -i.bak -e "s/ObjectMerger/$3/g"
