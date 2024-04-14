#!/bin/bash
package=$1; shift
build_type=$1; shift
package_select=$1; shift

source /home/bot/.bashrc

if [[ $package_select = "true" ]]; then
  if ! colcon build --mixin ${build_type} --packages-select ${package} ; then
    echo "\033[1;33mYou attempted to --package-select $package which failed."
    echo "\033[1;33mIf this was intentional, check all dependencies have built."
  fi
else
  colcon build --mixin $build_type --packages-up-to $package
fi
