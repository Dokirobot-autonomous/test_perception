#!/bin/bash


grep -lr 'cooperation' ./* | xargs sed -i.bak -e "s/cooperation/$1/g"
grep -lr 'node_name' ./* | xargs sed -i.bak -e "s/node_name/$2/g"
grep -lr 'ClassName' ./* | xargs sed -i.bak -e "s/ClassName/$3/g"
