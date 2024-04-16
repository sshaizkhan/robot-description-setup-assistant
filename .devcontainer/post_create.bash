#!/bin/bash

#update and upgrade any installed packages
sudo apt update
sudo apt -y upgrade

# Install terminator
sudo apt-get install -y terminator

#install nano
sudo apt-get -y install nano


colcon --log-base "./colcon-log" mixin add default "file://`pwd`//rds_ws/src/robot-description-setup-assistant/.devcontainer/colcon_mixins/index.yaml"

sudo chown -R bot:bot /home/bot

ln -s /home/bot/rds_ws/src/robot-description-setup-assistant/.devcontainer/.vscode/ /home/bot/.vscode

# link the container settings
mkdir -p /home/bot/.vscode-server/data/Machine
ln -s /home/bot/rds_ws/src/robot-description-setup-assistant/.devcontainer/settings.json /home/bot/.vscode-server/data/Machine/settings.json

# install Terminator config
mkdir -p /home/bot/.config/terminator
ln -s /home/bot/rds_ws/src/robot-description-setup-assistant/.devcontainer/terminator.config /home/bot/.config/terminator/config

# link .bashrc
rm /home/bot/.bashrc
ln -s /home/bot/rds_ws/src/robot-description-setup-assistant/.devcontainer/.bashrc /home/bot/.bashrc

# link .clangd
rm /home/bot/.clangd
ln -s /home/bot/rds_ws/src/robot-description-setup-assistant/.devcontainer/.clangd /home/bot/.clangd

# link .bash_aliases
# rm /home/bot/.bash_aliases
ln -s /home/bot/rds_ws/src/robot-description-setup-assistant/.devcontainer/.bash_aliases /home/bot/.bash_aliases

# Check if $DISTRO=WSL then change permissions for /dev/dri/card0 and /dev/dri/renderD128
# This is required for running Gazebo in WSL
if [ "$DISTRO" = "WSL" ]; then
    sudo chmod 666 /dev/dri/card0
    sudo chmod 666 /dev/dri/renderD128
fi
