cmake_minimum_required(VERSION 3.10.2)

project(point_cloud_transport_tutorial)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake_ros REQUIRED)

find_package(point_cloud_transport REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rosbag2_cpp REQUIRED)
find_package(sensor_msgs REQUIRED)

include_directories(
  include
)

# encoder
add_executable(encoder_test src/my_encoder.cpp)
ament_target_dependencies(encoder_test point_cloud_transport rosbag2_cpp sensor_msgs)

# publisher
add_executable(publisher_test src/my_publisher.cpp)
ament_target_dependencies(publisher_test point_cloud_transport rclcpp rosbag2_cpp sensor_msgs)

# subscriber
add_executable(subscriber_test src/my_subscriber.cpp)
ament_target_dependencies(subscriber_test point_cloud_transport rclcpp sensor_msgs)

# Install executables
install(
  TARGETS
    encoder_test
    publisher_test
    subscriber_test
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

install(
  DIRECTORY
    resources
  DESTINATION share/${PROJECT_NAME}
)

install(PROGRAMS
  scripts/publisher.py
  scripts/subscriber_old_school.py
  DESTINATION lib/${PROJECT_NAME}
)

# linting tests
if(BUILD_TESTING)
  find_package(ament_cmake_copyright REQUIRED)
  find_package(ament_cmake_cppcheck REQUIRED)
  find_package(ament_cmake_cpplint REQUIRED)
  find_package(ament_cmake_lint_cmake REQUIRED)
  find_package(ament_cmake_uncrustify REQUIRED)
  find_package(ament_cmake_xmllint REQUIRED)

  ament_copyright(EXCLUDE ${_linter_excludes})
  ament_cppcheck(
    EXCLUDE ${_linter_excludes}
    LANGUAGE c++
  )
  ament_cpplint(EXCLUDE ${_linter_excludes})
  ament_lint_cmake()
  ament_uncrustify(
    LANGUAGE c++
  )
  ament_xmllint()
endif()


ament_package()
