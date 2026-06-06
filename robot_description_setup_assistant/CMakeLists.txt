cmake_minimum_required(VERSION 3.10)
project(robot_description_setup_assistant)

# Data-only package: ships the robot catalog (config/), robot images
# (resources/), and the web-UI launch file. The former Qt/RViz/MoveIt setup
# assistant has been removed in favour of the web UI.
find_package(ament_cmake REQUIRED)

install(DIRECTORY config DESTINATION share/${PROJECT_NAME})
install(DIRECTORY resources DESTINATION share/${PROJECT_NAME})
install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})

ament_package()
