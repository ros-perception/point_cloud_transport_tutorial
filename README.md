# \<POINT CLOUD TRANSPORT TUTORIAL>
 **ROS2 v0.1.**

_**Contents**_

  * [Writing a Simple Publisher](#writing-a-simple-publisher)
    * [Code of the Publisher](#code-of-the-publisher)
    * [Code Explained](#code-of-publisher-explained)
    * [Example of Running the Publisher](#example-of-running-the-publisher)
  * [Writing a Simple Subscriber](#writing-a-simple-subscriber)
    * [Code of the Subscriber](#code-of-the-subscriber)
    * [Code Explained](#code-of-subscriber-explained)
    * [Example of Running the Subscriber](#example-of-running-the-subscriber)
  * [Using Publishers And Subscribers With Plugins](#using-publishers-and-subscribers-with-plugins)
    * [Running the Publisher and Subsriber](#running-the-publisher-and-subsriber)
    * [Changing the Transport Used](#changing-the-transport-used)
    * [Changing Transport Behavior](#changing-transport-behavior)
  * [Implementing Custom Plugins](#implementing-custom-plugins)

# Writing a Simple Publisher
In this section, we'll see how to create a publisher node, which opens a ROS 2 bag and publishes `PointCloud2` messages from it.

This tutorial assumes that you have created your workspace containing [<point_cloud_transport>](https://github.com/ros-perception/point_cloud_transport) and [<point_cloud_transport_plugins>](https://github.com/ros-perception/point_cloud_transport_plugins)

Before we start, change to the directory, clone this repository, and unzip the example rosbag in the resources folder:
```bash
$ cd ~/<point_cloud_transport_ws>/src
$ git clone https://github.com/ros-perception/point_cloud_transport_tutorial
$ cd point_cloud_transport_tutorial
$ tar -C resources/ -xvf resources/rosbag2_2023_08_05-16_08_51.tar.xz
$ cd ~/<point_cloud_transport_ws>
$ colcon build --merge-install --event-handlers console_direct+
```

## Code of the Publisher
Take a look at my_publisher.cpp
```cpp
#include <point_cloud_transport/point_cloud_transport.hpp>

// for reading rosbag
#include <ament_index_cpp/get_package_share_directory.hpp>
#include <rclcpp/serialization.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rosbag2_cpp/reader.hpp>
#include <rosbag2_cpp/storage_options.hpp>
#include <rosbag2_cpp/converter_interfaces/serialization_format_converter.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  auto node = std::make_shared<rclcpp::Node>("point_cloud_publisher");

  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(node);

  point_cloud_transport::PointCloudTransport pct(node);
  point_cloud_transport::Publisher pub = pct.advertise("pct/point_cloud", 100);

  const std::string bagged_cloud_topic = "/point_cloud";
  const std::string shared_directory = ament_index_cpp::get_package_share_directory(
    "point_cloud_transport_tutorial");
  const std::string bag_file = shared_directory + "/resources/rosbag2_2023_08_05-16_08_51";

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
      // deserialize and convert to  message
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
```

## Code of Publisher Explained
Now we'll break down the code piece by piece.

Header for including [<point_cloud_transport>](https://github.com/ros-perception/point_cloud_transport):
```cpp
#include <point_cloud_transport/point_cloud_transport.hpp>
```

Creates *PointCloudTransport* instance and initializes it with our *Node* shared pointer. Methods of *PointCloudTransport* can later be used to create point cloud publishers and subscribers similar to how methods of *Node* are used to create generic publishers and subscribers.

```cpp
point_cloud_transport::PointCloudTransport pct(node);
```

Uses *PointCloudTransport* method to create a publisher on base topic *"pct/point_cloud"*. Depending on whether more plugins are built, additional (per-plugin) topics derived from the base topic may also be advertised. The second argument is the size of our publishing queue.

```cpp
point_cloud_transport::Publisher pub = pct.advertise("pct/point_cloud", 10);
```

Publishes sensor_msgs::PointCloud2 message from the specified rosbag:
```cpp
  sensor_msgs::msg::PointCloud2 cloud_msg;
  //... rosbag boiler plate to populate cloud_msg ...
  // publish the message
  pub.publish(cloud_msg);
  // spin the node...
  executor.spin_some();
  // repeat...
```

## Example of Running the Publisher
To run [my_publisher.cpp](src/my_publisher.cpp) open terminal in the root of workspace and run the following:

```bash
$ source install/setup.bash
$ ros2 run point_cloud_transport_tutorial publisher_test
```

# Writing a Simple Subscriber
In this section, we'll see how to create a subscriber node, which receives `PointCloud2` messages and prints the number of points in them.

## Code of the Subscriber
Take a look at [my_subscriber.cpp](src/my_subscriber.cpp):

```cpp
#include <point_cloud_transport/point_cloud_transport.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<rclcpp::Node>("point_cloud_subscriber");

  point_cloud_transport::PointCloudTransport pct(node);
  point_cloud_transport::Subscriber pct_sub = pct.subscribe(
    "pct/point_cloud", 100,
    [node](const sensor_msgs::msg::PointCloud2::ConstSharedPtr & msg)
    {
      RCLCPP_INFO_STREAM(
        node->get_logger(),
        "Message received, number of points is: " << msg->width * msg->height);
    }, {});

  RCLCPP_INFO_STREAM(node->get_logger(), "Waiting for point_cloud message...");

  rclcpp::spin(node);

  rclcpp::shutdown();

  return 0;
}
```

## Code of Subscriber Explained
Now we'll break down the code piece by piece.

Header for including [<point_cloud_transport>](https://github.com/ros-perception/point_cloud_transport):

```cpp
#include <point_cloud_transport/point_cloud_transport.hpp>
```

Initializes the ROS node:

```cpp
rclcpp::init(argc, argv);
auto node = rclcpp::Node::make_shared("point_cloud_subscriber");
```

Creates *PointCloudTransport* instance and initializes it with our *Node*. Methods of *PointCloudTransport* can later be used to create point cloud publishers and subscribers similar to how methods of *NodeHandle* are used to create generic publishers and subscribers.

```cpp
point_cloud_transport::PointCloudTransport pct(node);
```

Uses *PointCloudTransport* method to create a subscriber on base topic *"pct/point_cloud"*. The second argument is the size of our subscribing queue. The third argument tells the subscriber to execute lambda function whenever a message is received.

```cpp
  point_cloud_transport::Subscriber pct_sub = pct.subscribe(
    "pct/point_cloud", 100,
    [node](const sensor_msgs::msg::PointCloud2::ConstSharedPtr & msg)
    {
      RCLCPP_INFO_STREAM(
        node->get_logger(),
        "draco message received, number of points is: " << msg->width * msg->height);
    }, {});
```

### Select a specific transport

Or you can select a specific transport using the *TransportHint* class. Creates a *TransportHint* shared pointer.
This is how to tell the subscriber that we want to subscribe to a particular transport
(in this case "pct/point_cloud/draco"), rather than the raw "pct/point_cloud" topic.

```cpp
  auto transport_hint = std::make_shared<point_cloud_transport::TransportHints>("draco");
```

Uses *PointCloudTransport* method to create a subscriber on base topic *"pct/point_cloud"* and add the `transport_hint` variable as the last argument.

```cpp
  auto transport_hint = std::make_shared<point_cloud_transport::TransportHints>("draco");
  point_cloud_transport::Subscriber pct_sub = pct.subscribe(
    "pct/point_cloud", 100,
    [node](const sensor_msgs::msg::PointCloud2::ConstSharedPtr & msg)
    {
      RCLCPP_INFO_STREAM(
        node->get_logger(),
        "draco message received, number of points is: " << msg->width * msg->height);
    }, {}, transport_hint.get());
```

## Example of Running the Subscriber
To run my_subscriber.cpp, open terminal in the root of workspace and run the following:

```bash
$ source install/setup.bash
$ ros2 run point_cloud_transport_tutorial subscriber_test --ros-args -p point_cloud_transport:=draco
```

The `point_cloud_transport` parameter is read by the point_cloud_transport library. The complexity of the parameters is hidden in the library.

# Using Publishers And Subscribers With Plugins

## Running the Publisher and Subsriber

Now we can run the Publisher/Subsriber nodes. To run both start two terminal tabs and enter commands:

```bash
$ source install/setup.bash
$ ros2 run point_cloud_transport_tutorial subscriber_test
```

And in the second tab:

```bash
$ source install/setup.bash
$ ros2 run point_cloud_transport_tutorial publisher_test
$ # or choose which plugin you want load (a.k.a. whitelist them).
$ ros2 run point_cloud_transport_tutorial publisher_test --ros-args -p pct.point_cloud.enable_pub_plugins:=["point_cloud_transport/draco"]
```

If both nodes are running properly, you should see the subscriber node start printing out messages similar to:

```bash
Message received, number of points is: XXX
```

To list the topics, which are being published and subscribed to, enter command:
```bash
$ ros2 topic list
```

The output should look similar to this:
```bash
Published topics:
 * /parameter_events [rcl_interfaces/msg/ParameterEvent] 3 publishers
 * /pct/point_cloud [sensor_msgs/msg/PointCloud2] 1 publisher
 * /pct/point_cloud/draco [point_cloud_interfaces/msg/CompressedPointCloud2] 1 publisher
 * /pct/point_cloud/zlib [point_cloud_interfaces/msg/CompressedPointCloud2] 1 publisher
 * /rosout [rcl_interfaces/msg/Log] 3 publishers

Subscribed topics:
 * /parameter_events [rcl_interfaces/msg/ParameterEvent] 2 subscribers
 * /pct/point_cloud/draco [point_cloud_interfaces/msg/CompressedPointCloud2] 1 subscriber
```

To display the ROS computation graph, enter command:

```bash
$ ros2 run rqt_graph rqt_graph
```
You should see a graph similar to this:

![Graph1](https://github.com/ros-perception/point_cloud_transport_tutorial/blob/rolling/readme_images/rosgraph1.png)

## Changing the Transport Used

To check which plugins are built on your machine, enter command:

```bash
$ ros2 run point_cloud_transport list_transports
```

You should see output similar to:

```bash
Declared transports:
point_cloud_transport/draco
point_cloud_transport/raw

Details:
----------
"point_cloud_transport/draco"
 - Provided by package: draco_point_cloud_transport
 - Publisher:
      This plugin publishes a CompressedPointCloud2 using KD tree compression.

 - Subscriber:
      This plugin decompresses a CompressedPointCloud2 topic.

----------
"point_cloud_transport/raw"
 - Provided by package: point_cloud_transport
 - Publisher:
            This is the default publisher. It publishes the PointCloud2 as-is on the base topic.

 - Subscriber:
            This is the default pass-through subscriber for topics of type sensor_msgs/PointCloud2.
```

Shut down your publisher node and restart it. If you list the published topics again and have [<point_cloud_transport_plugins>](https://github.com/ros-perception/point_cloud_transport_plugins) installed, you should see:

```bash
 * /pct/point_cloud/draco [draco_point_cloud_transport/CompressedPointCloud2] 1 publisher
```

```bash
ros2 run point_cloud_transport_tutorial my_subscriber --ros-args -r __node:=draco_listener -p point_cloud_transport:=<point_cloud_transport_type>
ros2 run point_cloud_transport_tutorial my_subscriber --ros-args -r __node:=draco_listener -p point_cloud_transport:=draco
```

If we check the node graph again:

```bash
rqt_graph
```

![Graph2](https://github.com/ros-perception/point_cloud_transport_tutorial/blob/rolling/readme_images/rosgraph2.png)

## Changing Transport Behavior
For a particular transport, we may want to tweak settings such as compression level and speed, quantization of particular attributes of point cloud, etc. Transport plugins can expose such settings through `rqt_reconfigure`. For example, `/point_cloud_transport/draco/` allows you to change multiple parameters of the compression on the fly.

For now let's adjust the position quantization. By default, "draco" transport uses quantization of 14 bits, allowing 16384 distinquishable positions in each axis; let's change it to 8 bits (256 positions):

```bash
$ ros2 run rqt_reconfigure rqt_reconfigure
```

Now pick `/pct/point_cloud/draco` in the drop-down menu and move the quantization_POSITION slider down to 8. If you visualize the messages, such as in RVIZ, you should be able to see the level of detail of the point cloud drop.

Dynamic Reconfigure has updated the dynamically reconfigurable parameter `/pct/point_cloud/draco/quantization_POSITION`. You can verify this by running:

``` bash
ros2 param get /point_cloud_subscriber /pct/point_cloud/draco/quantization_POSITION
```

This should display 8.

Full explanation of the reconfigure parameters and an example of how to use them can be found at [<point_cloud_transport_plugins>](https://github.com/ros-perception/point_cloud_transport_plugins) repository.


### Whitelist point cloud transport

This allows you to specify plugins you do want to load (a.k.a. whitelist them).

```bash
ros2 run point_cloud_transport_tutorial publisher_test --ros-args -p pct.point_cloud.enable_pub_plugins:=["point_cloud_transport/zlib"]
```
