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

#include <ament_index_cpp/get_package_share_directory.hpp>
#include <rclcpp/serialization.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rosbag2_cpp/reader.hpp>
#include <rosbag2_storage/storage_options.hpp>
#include <rosbag2_cpp/converter_options.hpp>
#include <rosbag2_cpp/converter_interfaces/serialization_format_converter.hpp>

#include <point_cloud_transport/point_cloud_codec.hpp>

int main(int /*argc*/, char ** /*argv*/)
{
  auto logger_ = rclcpp::get_logger("my_encoder");

  point_cloud_transport::PointCloudCodec codec;

  // set the transport to use
  const std::string transport = "draco";
  RCLCPP_INFO(logger_, "Using transport: %s", transport.c_str());

  const std::string bagged_cloud_topic = "/point_cloud";
  std::filesystem::path shared_directory;
  ament_index_cpp::get_package_share_directory("point_cloud_transport_tutorial", shared_directory);
  const std::filesystem::path bag_file = shared_directory / "resources" /
    "rosbag2_2023_08_05-16_08_51";

  // boiler-plate to tell rosbag2 how to read our bag
  rosbag2_storage::StorageOptions storage_options;
  storage_options.uri = bag_file.string();
  storage_options.storage_id = "mcap";
  rosbag2_cpp::ConverterOptions converter_options;
  converter_options.input_serialization_format = "cdr";
  converter_options.output_serialization_format = "cdr";

  // open the rosbag
  rosbag2_cpp::readers::SequentialReader reader;
  reader.open(storage_options, converter_options);

  sensor_msgs::msg::PointCloud2 cloud_msg;
  rclcpp::Serialization<sensor_msgs::msg::PointCloud2> cloud_serialization;
  while (reader.has_next()) {
    // get serialized data
    auto serialized_message = reader.read_next();
    rclcpp::SerializedMessage extracted_serialized_msg(*serialized_message->serialized_data);
    if (serialized_message->topic_name == bagged_cloud_topic) {
      // deserialize and convert to ros2 message
      const size_t original_serialized_size = extracted_serialized_msg.size();
      cloud_serialization.deserialize_message(&extracted_serialized_msg, &cloud_msg);
      const size_t original_deserialized_size = cloud_msg.data.size();

      //
      // Encode using C++ API
      //

      // encode/decode communicate via a serialized message. This was done to support arbitrary
      // encoding formats and to make it easy to bind to other languages (since the serialized
      // message is simply a uchar buffer and its size)
      rclcpp::SerializedMessage compressed_msg;
      const bool encode_success = codec.encode(transport, cloud_msg, compressed_msg);

      // BUT encodeTyped/decodeTyped are also available if you would rather work with the actual
      // encoded message type (which may vary depending on the transport being used).

      if (encode_success) {
        RCLCPP_INFO(
          logger_, "ENCODE Raw size: %zu, compressed size: %zu, ratio: %.2f %%, transport type: %s",
          original_serialized_size, compressed_msg.size(),
          100.0 * compressed_msg.size() / original_serialized_size,
          transport.c_str());
      } else {
        RCLCPP_ERROR(logger_, "Failed to encode message");
      }

      //
      // Decode using C++ API
      //
      sensor_msgs::msg::PointCloud2 decoded_msg;
      const bool decode_success = codec.decode(transport, compressed_msg, decoded_msg);
      if (decode_success) {
        RCLCPP_INFO(
          logger_, "DECODE Raw size: %zu, compressed size: %zu, ratio: %.2f %%, transport type: %s",
          original_deserialized_size, decoded_msg.data.size(),
          100.0 * decoded_msg.data.size() / original_deserialized_size,
          transport.c_str());
      } else {
        RCLCPP_ERROR(logger_, "Failed to decode message");
      }
    }
  }
  reader.close();
}
