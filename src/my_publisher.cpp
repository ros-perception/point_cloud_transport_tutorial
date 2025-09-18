// Copyright (c) 2023, Czech Technical University in Prague
// Copyright (c) 2023, Open Source Robotics Foundation, Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//    * Neither the name of the Willow Garage nor the names of its
//      contributors may be used to endorse or promote products derived from
//      this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.


#include <iostream>
#include <filesystem>
#include <memory>
#include <string>

// for reading rosbag
#include <ament_index_cpp/get_package_share_directory.hpp>

#include <point_cloud_transport/point_cloud_transport.hpp>
#include <rclcpp/serialization.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rcpputils/filesystem_helper.hpp>
#include <rosbag2_cpp/reader.hpp>
#include <rosbag2_storage/storage_options.hpp>
#include <rosbag2_cpp/converter_interfaces/serialization_format_converter.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  auto node = std::make_shared<rclcpp::Node>("point_cloud_publisher");

  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(node);

  point_cloud_transport::PointCloudTransport pct(*node);
  point_cloud_transport::Publisher pub = pct.advertise("pct/point_cloud", 100);

  const std::string bagged_cloud_topic = "/point_cloud";
  const std::string shared_directory = ament_index_cpp::get_package_share_directory(
    "point_cloud_transport_tutorial");
  std::string bag_file = shared_directory + "/resources/rosbag2_2023_08_05-16_08_51";

  if (argc > 1) {
    bag_file = argv[1];
  }

  if (!std::filesystem::exists(bag_file)) {
    std::cout << "Not able to open file [" << bag_file << "]" << '\n';
    return -1;
  }

  std::cout << "Reading [" << bag_file << "] bagfile" << '\n';

  // boiler-plate to tell rosbag2 how to read our bag
  rosbag2_storage::StorageOptions storage_options;
  storage_options.uri = bag_file;
  storage_options.storage_id = "mcap";
  rosbag2_cpp::ConverterOptions converter_options;
  converter_options.input_serialization_format = "cdr";
  converter_options.output_serialization_format = "cdr";

  // open the rosbag
  rosbag2_cpp::readers::SequentialReader reader;
  reader.open(storage_options, converter_options);

  sensor_msgs::msg::PointCloud2 cloud_msg;
  rclcpp::Serialization<sensor_msgs::msg::PointCloud2> cloud_serialization;
  while (reader.has_next() && rclcpp::ok()) {
    // get serialized data
    auto serialized_message = reader.read_next();
    rclcpp::SerializedMessage extracted_serialized_msg(*serialized_message->serialized_data);
    if (serialized_message->topic_name == bagged_cloud_topic) {
      // deserialize and convert to ros2 message
      cloud_serialization.deserialize_message(&extracted_serialized_msg, &cloud_msg);
      // publish the message
      pub.publish(cloud_msg);
      executor.spin_some();
      rclcpp::sleep_for(std::chrono::milliseconds(100));
    }
  }
  reader.close();

  node.reset();
  rclcpp::shutdown();
}
