cmake_minimum_required(VERSION 3.5)
project(rosglove)

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)

add_executable(robot_simulator src/robot_publisher.cpp)
add_executable(listener src/subscriber_member_function.cpp)
ament_target_dependencies(robot_simulator rclcpp std_msgs sensor_msgs nav_msgs tf2_ros tf2_geometry_msgs)
ament_target_dependencies(listener rclcpp std_msgs)

install(TARGETS
  robot_simulator
  listener
  DESTINATION lib/${PROJECT_NAME})

install(DIRECTORY launch urdf
  DESTINATION share/${PROJECT_NAME}/
)

ament_package()