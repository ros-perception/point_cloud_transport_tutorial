#!/bin/bash
set -ev

# Configuration.
export COLCON_WS=~/ws
export COLCON_WS_SRC=${COLCON_WS}/src
export DEBIAN_FRONTEND=noninteractive
export ROS_PYTHON_VERSION=3

apt update -qq
apt install -qq -y lsb-release wget curl build-essential

# Dependencies.
curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key -o /usr/share/keyrings/ros-archive-keyring.gpg
echo "deb [signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] http://packages.ros.org/ros2-testing/ubuntu $(lsb_release -cs) main" > /etc/apt/sources.list.d/ros2-testing.list
apt-get update -qq
apt-get install -y python3-colcon-common-extensions \
                   python3-rosdep python3-vcstool python3-vcstools

rosdep init
rosdep update

# Build.
mkdir -p $COLCON_WS_SRC
cp -r $GITHUB_WORKSPACE $COLCON_WS_SRC
cd $COLCON_WS
rosdep install --from-paths ./ -i -y -r --rosdistro $ROS_DISTRO $ROSDEP_ARGS
source /opt/ros/$ROS_DISTRO/setup.bash
colcon build --event-handlers console_direct+

# Tests.
colcon test --event-handlers console_direct+
colcon test-result
