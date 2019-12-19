#!/bin/bash


grep -lr 'package_name' ./* | xargs sed -i.bak -e "s/package_name/$1/g"
grep -lr 'node_name' ./* | xargs sed -i.bak -e "s/node_name/$2/g"
grep -lr 'ClassName' ./* | xargs sed -i.bak -e "s/ClassName/$3/g"
