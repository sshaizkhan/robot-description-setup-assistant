FROM osrf/ros:humble-desktop-full

# Setting up packages for devops
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --allow-unauthenticated \
lsb-release \
net-tools \
iputils-ping \
apt-utils \
build-essential \
psmisc \
vim-gtk \
scons \
bison \
flex \
git \
wget \
sudo \
gnupg \
keyboard-configuration \
tigervnc-standalone-server \
tigervnc-common \
openbox \
x11-utils \
 && rm -rf /var/lib/apt/lists/*

ARG USERNAME=bot
ARG PASSWORD=""
ARG DISTRO=Linux

ENV USER_NAME=$USERNAME
ENV PASSWORD=$PASSWORD
ENV DISTRO=$DISTRO

RUN adduser --ingroup sudo --disabled-password --gecos "" --shell /bin/bash --home /home/$USER_NAME $USER_NAME
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers
RUN if [ ! -z "$PASSWORD" ]; then echo $USER_NAME:$PASSWORD | chpasswd; fi
ENV HOME /home/$USER_NAME
USER $USER_NAME

RUN rosdep update
RUN sudo apt update

RUN sudo apt install python3-colcon-common-extensions
RUN sudo apt install python3-colcon-mixin
RUN sudo apt-get install -y mesa-utils
RUN sudo apt install python3-vcstool
RUN sudo apt-get install -y python3-pip

# Cyclone dds for Humble
RUN sudo apt install -y ros-$ROS_DISTRO-rmw-cyclonedds-cpp
RUN export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp

# Install Moveit2
RUN sudo apt install -y ros-$ROS_DISTRO-moveit

# Install Bash completion
RUN sudo apt install -y bash-completion


# Install Gazebo Sim - Harmonic
RUN sudo wget https://packages.osrfoundation.org/gazebo.gpg -O /usr/share/keyrings/pkgs-osrf-archive-keyring.gpg
RUN echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/pkgs-osrf-archive-keyring.gpg] http://packages.osrfoundation.org/gazebo/ubuntu-stable $(lsb_release -cs) main" | sudo tee /etc/apt/sources.list.d/gazebo-stable.list > /dev/null
RUN sudo apt-get update
RUN sudo apt-get install gz-harmonic -y

RUN if [ "$DISTRO" = "WSL" ]; then \
        sudo apt-get update && sudo apt-get install -y vainfo && \
        echo "export LIBVA_DRIVER_NAME=d3d12" >> $HOME/.bashrc && \
        echo "export LD_LIBRARY_PATH=/usr/lib/wsl/lib" >> $HOME/.bashrc; \
    fi


# Pre-create the ROS workspace owned by the non-root user so bind-mounting only
# a sub-path (the macOS case) doesn't leave root-owned parents that block colcon
# from writing build/, install/, log/, colcon-log. Harmless on Linux (the full
# rds_ws mount overlays this).
RUN sudo mkdir -p /home/$USER_NAME/rds_ws/src \
    && sudo chown -R $USER_NAME:sudo /home/$USER_NAME/rds_ws


CMD [ "bash" ]
