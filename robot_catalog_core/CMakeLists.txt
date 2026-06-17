cmake_minimum_required(VERSION 3.10)
project(robot_catalog_core)
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
find_package(ament_cmake REQUIRED)
find_package(yaml_cpp_vendor REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(yaml-cpp REQUIRED)

add_library(robot_catalog_core src/catalog.cpp src/json.cpp src/urdf.cpp src/mesh.cpp)
target_include_directories(robot_catalog_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
ament_target_dependencies(robot_catalog_core ament_index_cpp)
target_link_libraries(robot_catalog_core yaml-cpp)

install(DIRECTORY include/ DESTINATION include)
install(TARGETS robot_catalog_core EXPORT export_robot_catalog_core
  ARCHIVE DESTINATION lib LIBRARY DESTINATION lib RUNTIME DESTINATION bin)
ament_export_targets(export_robot_catalog_core HAS_LIBRARY_TARGET)
ament_export_dependencies(ament_index_cpp)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_catalog test/test_catalog.cpp)
  target_link_libraries(test_catalog robot_catalog_core yaml-cpp)
endif()
ament_package()
