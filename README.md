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
In this section, we'll see how to create a publisher node, which opens a ros2 bag and publishes PointCloud2 messages from it.

This tutorial assumes that you have created your workspace containing [<point_cloud_transport>](https://github.com/john-maidbot/point_cloud_transport) AND [<point_cloud_transport_plugins>](https://github.com/john-maidbot/point_cloud_transport_plugins)

Before we start, change to the directory, clone this repository, and unzip the example rosbag in the resources folder:
~~~~~ bash
$ cd ~/<point_cloud_transport_ws>/src
$ git clone git@github.com:john-maidbot/point_cloud_transport_tutorial.git
$ cd point_cloud_transport_tutorial
$ tar -C resources/ -xvf resources/rosbag2_2023_08_05-16_08_51.tar.xz
$ cd ~/<point_cloud_transport_ws>
~~~~~

## Code of the Publisher
Take a look at my_publisher.cpp:
```cpp

```

## Code of Publisher Explained
Now we'll break down the code piece by piece.

Header for including [<point_cloud_transport>](https://github.com/paplhjak/point_cloud_transport):
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
  //... rosbag boiler plate ...
  // publish the message
  pub.publish(cloud_msg);
  // spin the node...
  rclcpp::spin_some(node);
  // etc...
```

## Example of Running the Publisher
To run my_publisher.cpp, build your ros2 workspace and then run
~~~~~ bash
$ source install/setup.bash
$ ros2 run point_cloud_transport_tutorial publisher_test
~~~~~

# Writing a Simple Subscriber
In this section, we'll see how to create a subscriber node, which receives PointCloud2 messages and prints the number of points in them.

## Code of the Subscriber
Take a look at my_subscriber.cpp.

```cpp
#include <point_cloud_transport/point_cloud_transport.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<rclcpp::Node>("point_cloud_subscriber");

  point_cloud_transport::PointCloudTransport pct(node);
  auto transport_hint = std::make_shared<point_cloud_transport::TransportHints>("draco");
  point_cloud_transport::Subscriber draco_sub = pct.subscribe(
    "pct/point_cloud", 100,
    [node](const sensor_msgs::msg::PointCloud2::ConstSharedPtr & msg)
    {
      RCLCPP_INFO_STREAM(
        node->get_logger(),
        "draco message received, number of points is: " << msg->width * msg->height);
    }, {}, transport_hint.get());

  RCLCPP_INFO_STREAM(node->get_logger(), "Waiting for point_cloud message...");

  rclcpp::spin(node);

  rclcpp::shutdown();

  return 0;
}

```

## Code of Subscriber Explained
Now we'll break down the code piece by piece.

Creates *PointCloudTransport* instance and initializes it with our *Node*. Methods of *PointCloudTransport* can later be used to create point cloud publishers and subscribers similar to how methods of *NodeHandle* are used to create generic publishers and subscribers.
```cpp
point_cloud_transport::PointCloudTransport pct(node);
```
Creates a *TransportHint* shared pointer. This is how to tell the subscriber that we want to subscribe
to a particular transport (in this case "pct/point_cloud/draco"), rather than the raw "pct/point_cloud" topic.
```cpp
  auto transport_hint = std::make_shared<point_cloud_transport::TransportHints>("draco");
```

Uses *PointCloudTransport* method to create a subscriber on base topic *"pct/point_cloud"*. The second argument is the size of our subscribing queue. The third argument tells the subscriber to execute lambda function whenever a message is received. And lastly, we pass in our transport hint.
```cpp
  auto transport_hint = std::make_shared<point_cloud_transport::TransportHints>("draco");
  point_cloud_transport::Subscriber draco_sub = pct.subscribe(
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
~~~~~ bash
$ source install/setup.bash
$ ros2 run point_cloud_transport_tutorial publisher_test
~~~~~

# Using Publishers And Subscribers With Plugins

## Running the Publisher and Subsriber

Now we can run the Publisher/Subsriber nodes. To run both start two terminal tabs and enter commands:
~~~~~ bash
$ source install/setup.bash
$ ros2 run point_cloud_transport_tutorial subscriber_test
~~~~~
And in the second tab:
~~~~~ bash
$ source install/setup.bash
$ ros2 run point_cloud_transport_tutorial publisher_test
~~~~~
If both nodes are running properly, you should see the subscriber node start printing out messages similar to:
~~~~~ bash
draco message received, number of points is: XXX
~~~~~

To list the topics, which are being published and subscribed to, enter command:
~~~~~ bash
$ ros2 topic list
~~~~~

The output should look similar to this:
~~~~~ bash
Published topics:
 * /parameter_events [rcl_interfaces/msg/ParameterEvent] 3 publishers
 * /pct/point_cloud [sensor_msgs/msg/PointCloud2] 1 publisher
 * /pct/point_cloud/draco [point_cloud_interfaces/msg/CompressedPointCloud2] 1 publisher
 * /pct/point_cloud/zlib [point_cloud_interfaces/msg/CompressedPointCloud2] 1 publisher
 * /rosout [rcl_interfaces/msg/Log] 3 publishers

Subscribed topics:
 * /parameter_events [rcl_interfaces/msg/ParameterEvent] 2 subscribers
 * /pct/point_cloud/draco [point_cloud_interfaces/msg/CompressedPointCloud2] 1 subscriber
~~~~~
To display the ROS computation graph, enter command:

~~~~~ bash
$ ros2 run rqt_graph rqt_graph
~~~~~
You should see a graph similar to this:

![Graph1](https://github.com/paplhjak/point_cloud_transport_tutorial/blob/master/readme_images/rosgraph1.png)

## Changing the Transport Used

To check which plugins are built on your machine, enter command:
~~~~~ bash
$ ros2 run point_cloud_transport list_transports
~~~~~
You should see output similar to:
~~~~~ bash
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
~~~~~
Shut down your publisher node and restart it. If you list the published topics again and have [<draco_point_cloud_transport>](https://github.com/paplhjak/draco_point_cloud_transport) installed, you should see:

~~~~~ bash
 * /pct/point_cloud/draco [draco_point_cloud_transport/CompressedPointCloud2] 1 publisher
~~~~~

## Changing Transport Behavior
For a particular transport, we may want to tweak settings such as compression level and speed, quantization of particular attributes of point cloud, etc. Transport plugins can expose such settings through rqt_reconfigure. For example, /point_cloud_transport/draco/ allows you to change multiple parameters of the compression on the fly.

For now let's adjust the position quantization. By default, "draco" transport uses quantization of 14 bits, allowing 16384 distinquishable positions in each axis; let's change it to 8 bits (256 positions):

~~~~~ bash
$ ros2 run rqt_reconfigure rqt_reconfigure
~~~~~

Now pick /pct/point_cloud/draco in the drop-down menu and move the quantization_POSITION slider down to 8. If you visualize the messages, such as in RVIZ, you should be able to see the level of detail of the point cloud drop.

Full explanation of the reconfigure parameters and an example of how to use them can be found at [<draco_point_cloud_transport>](https://github.com/john-maidbot/draco_point_cloud_transport) repository.


# Implementing Custom Plugins (TODO: john-maidbot, ROS2 port of template coming soon)
The process of implementing your own plugins is described in a separate [repository](https://github.com/john-maidbot/templateplugin_point_cloud_transport).
