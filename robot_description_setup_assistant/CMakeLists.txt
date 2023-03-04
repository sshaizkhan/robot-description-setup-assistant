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

project(robot_description_setup_assistant VERSION ${PACKAGE_VERSION})

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Adding Windows-OS support
if(WIN32)
  add_compile_definitions(

    # For math constants
    _USE_MATH_DEFINES

    # Minimize Windows namespace collision
    NOMINMAX
    WIN32_LEAN_AND_MEAN
  )
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(Boost REQUIRED program_options)
find_package(rclcpp REQUIRED)
find_package(pluginlib REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)

set(THIS_PACKAGE_INCLUDE_DEPENDS
  ament_index_cpp
  pluginlib
  Qt5Core
  Qt5Widgets
  rclcpp
)

# Header files that need Qt Moc pre-processing for use with Qt signals, etc:
set(HEADERS
  include/robot_description_setup_assistant/navigation_widget.hpp
)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
add_definitions(-DQT_NO_KEYWORDS)

qt5_wrap_cpp(MOC_FILES
  ${HEADERS}
)

add_executable(robot_description_setup_assistant

  src/main.cpp
  # src/setup_assistant_widget.cpp
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

install(TARGETS
  robot_description_setup_assistant
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY include/
  DESTINATION include
)

install(DIRECTORY launch resources
  DESTINATION share/${PROJECT_NAME}
)
ament_export_include_directories(include)

# ament_export_targets(${PROJECT_NAME}Targets HAS_LIBRARY_TARGET)
ament_export_dependencies(${THIS_PACKAGE_INCLUDE_DEPENDS})


if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
