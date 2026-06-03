cmake_minimum_required(VERSION 3.10)

# Find the robot_description_common package
find_package(robot_description_common REQUIRED)

set(PACKAGE_XML_PATH "${CMAKE_CURRENT_SOURCE_DIR}/package.xml")
set_package_version("${PACKAGE_XML_PATH}")

message(STATUS "Project version: ${PACKAGE_VERSION}")
project(robot_description_setup_assistant VERSION ${PACKAGE_VERSION})
robot_description_package()

find_package(ament_cmake REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(rviz_common REQUIRED)
find_package(robot_description_setup_framework REQUIRED)
find_package(robot_description_core_plugins REQUIRED)
find_package(Boost REQUIRED program_options)
find_package(pluginlib REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(rclcpp REQUIRED)

add_compile_options(-g)

set(THIS_PACKAGE_INCLUDE_DEPENDS
  rviz_common
  robot_description_setup_framework
  robot_description_core_plugins
  ament_index_cpp
  pluginlib
  Qt5Core
  Qt5Widgets
  rclcpp
)

set(HEADERS
  include/robot_description_setup_assistant/navigation_widget.hpp
  include/robot_description_setup_assistant/setup_assistant_widget.hpp
)


# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
add_definitions(-DQT_NO_KEYWORDS)

qt5_wrap_cpp(MOC_FILES
  ${HEADERS}
)

add_executable(robot_description_setup_assistant
  src/main.cpp
  src/setup_assistant_widget.cpp
  src/navigation_widget.cpp
  ${MOC_FILES}
)

target_include_directories(robot_description_setup_assistant PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
ament_target_dependencies(
  robot_description_setup_assistant
  ${THIS_PACKAGE_INCLUDE_DEPENDS}
)
target_link_libraries(robot_description_setup_assistant
  ${Boost_LIBRARIES}
)

install(
  TARGETS
  robot_description_setup_assistant
  EXPORT ${PROJECT_NAME}Targets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
  INCLUDES DESTINATION include
)
install(DIRECTORY include/
  DESTINATION include
)

ament_export_include_directories(include)
install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})
install(DIRECTORY resources DESTINATION share/${PROJECT_NAME})
install(DIRECTORY config DESTINATION share/${PROJECT_NAME})
ament_export_targets(${PROJECT_NAME}Targets HAS_LIBRARY_TARGET)
ament_export_dependencies(${THIS_PACKAGE_INCLUDE_DEPENDS})

# Unit tests
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
