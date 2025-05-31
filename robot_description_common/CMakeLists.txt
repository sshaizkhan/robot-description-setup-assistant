cmake_minimum_required(VERSION 3.10)

# ----------------------- #
# Parse version from package.xml
# https://stackoverflow.com/questions/62464367/how-to-get-package-version-from-package-xml-in-cmake

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/package.xml package_xml_str)
if(package_xml_str MATCHES "<version>([0-9]+\\.[0-9]+\\.[0-9]+)</version>")
  set(PACKAGE_VERSION "${CMAKE_MATCH_1}")
else()
  message(FATAL_ERROR "Could not parse project version from package.xml. Aborting.")
endif()

project(robot_description_common VERSION ${PACKAGE_VERSION})

find_package(ament_cmake REQUIRED)

install(
  DIRECTORY cmake
  DESTINATION share/${PROJECT_NAME}
)

install(
  DIRECTORY macros
  DESTINATION include/${PROJECT_NAME}
  FILES_MATCHING PATTERN "*.hpp"
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_export_include_directories(include)

ament_package(
  CONFIG_EXTRAS "robot_description_common-extras.cmake"
)

