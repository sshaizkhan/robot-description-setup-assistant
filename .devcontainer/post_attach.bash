#!/bin/bash

colcon --log-base "./colcon-log" mixin update default
git config --global pull.rebase true

# rosdep update
cd ~/rds_ws/src
rosdep install -r --from-paths . --ignore-src --rosdistro $ROS_DISTRO -y

