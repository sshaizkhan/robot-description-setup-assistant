cmake_minimum_required(VERSION 3.10)
project(robot_catalog_msgs)
find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)
rosidl_generate_interfaces(${PROJECT_NAME}
  "srv/GetRobots.srv"
  "srv/GetCategories.srv"
  "srv/FilterRobots.srv"
  "srv/ValidateRobot.srv"
  "srv/GetUrdf.srv"
  "srv/ResolveMesh.srv"
)
ament_export_dependencies(rosidl_default_runtime)
ament_package()
