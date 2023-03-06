cmake_minimum_required(VERSION 3.8)

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/package.xml PACKAGE_XML_CONTENTS)
string(REGEX MATCH "<version>[0-9]+\\.[0-9]+\\.[0-9]+</version>" PACKAGE_VERSION_REGEX "${PACKAGE_XML_CONTENTS}")
string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" PACKAGE_VERSION "${PACKAGE_VERSION_REGEX}")

message(STATUS "Package version: ${PACKAGE_VERSION}")

# Set the version in the project
set(PROJECT_VERSION ${PACKAGE_VERSION})
set(PROJECT_VERSION_MAJOR ${PACKAGE_VERSION_MAJOR})
set(PROJECT_VERSION_MINOR ${PACKAGE_VERSION_MINOR})
set(PROJECT_VERSION_PATCH ${PACKAGE_VERSION_PATCH})

project(robot_description_resources VERSION ${PACKAGE_VERSION})

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)

install(DIRECTORY config mesh_models
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
