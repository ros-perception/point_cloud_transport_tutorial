#!/usr/bin/env python3

# Copyright (c) 2023 Open Source Robotics Foundation, Inc.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#   * Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#   * Redistributions in binary form must reproduce the above
#     copyright notice, this list of conditions and the following
#     disclaimer in the documentation and/or other materials provided
#     with the distribution.
#   * Neither the name of the TU Darmstadt nor the names of its
#     contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

"""Publisher that automatically publishes to all declared transports."""

import point_cloud_transport_py

import time
import sys

import rclpy
from rclpy.node import Node
from rclpy.serialization import deserialize_message, serialize_message
from sensor_msgs.msg import PointCloud2
import rosbag2_py
from rosidl_runtime_py.utilities import get_message
from point_cloud_transport_py.common import pointCloud2ToString

def pointCloud2ToString(msg: PointCloud2):
    buffer = serialize_message(msg)
    return buffer

if __name__ == '__main__':

    rclpy.init(args=sys.argv)

    bag_path = sys.argv[1]
    serialization_format='cdr'

    storage_options = rosbag2_py.StorageOptions(
        uri=bag_path)

    converter_options = rosbag2_py.ConverterOptions(
        input_serialization_format=serialization_format,
        output_serialization_format=serialization_format)

    reader = rosbag2_py.SequentialReader()
    reader.open(storage_options, converter_options)

    topic_types = reader.get_all_topics_and_types()

    # Create a map for quicker lookup
    type_map = {topic_types[i].name: topic_types[i].type for i in range(len(topic_types))}

    pct = point_cloud_transport_py.PointCloudTransport("point_cloud_transport", "");
    pub = pct.advertise("pct/point_cloud", 100);

    try:
        while reader.has_next():
            (topic, data, t, _) = reader.read_next_ext()
            msg_type = get_message(type_map[topic])
            msg = deserialize_message(data, msg_type)
            pub.publish(pointCloud2ToString(msg))
            time.sleep(0.1)
    except Exception as e:
        print('Error in publisher node!')
        print(e)
    finally:
        # if publisher_node is not None:
        #     publisher_node.destroy_node()
        rclpy.shutdown()
