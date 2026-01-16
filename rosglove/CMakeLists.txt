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
find_package(std_srvs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)

add_executable(robot_simulator src/robot_publisher.cpp)
ament_target_dependencies(robot_simulator rclcpp std_msgs sensor_msgs nav_msgs tf2_ros tf2_geometry_msgs)

add_executable(ota_update_node src/ota_update_node.cpp)
ament_target_dependencies(ota_update_node rclcpp std_msgs std_srvs)

install(TARGETS
  robot_simulator
  ota_update_node
  DESTINATION lib/${PROJECT_NAME})

install(DIRECTORY launch urdf
  DESTINATION share/${PROJECT_NAME}/
)

ament_package()