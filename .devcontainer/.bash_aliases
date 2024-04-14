#!/bin/bash

function set_new_log_dir() {
  export ROS_LOG_DIR=~/src/logs/log_dir_$(date '+%Y_%m_%d_%H_%M_%S')
  mkdir -p $ROS_LOG_DIR
  rm ~/src/logs/latest
  ln -s $ROS_LOG_DIR ~/src/logs/latest
}

function set_log_to_latest() {
  export ROS_LOG_DIR=~/src/logs/latest
}
