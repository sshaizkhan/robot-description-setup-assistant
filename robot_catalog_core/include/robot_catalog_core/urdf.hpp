#pragma once
#include <string>
#include <vector>
#include "robot_catalog_core/types.hpp"

namespace robot_catalog {

struct UrdfResult {
  bool ok = false;        // true if xacro produced XML
  std::string xml;        // expanded URDF on success
  std::string error;      // human-readable reason on failure
};

// Resolve a robot's xacro into URDF XML by running the `xacro` CLI.
// All package/path resolution and process handling happens here in C++.
UrdfResult resolveUrdf(const RobotConfig& robot);

// Split a "key:=value" arg string, honoring single/double quotes.
std::vector<std::string> tokenizeArgs(const std::string& args);

}  // namespace robot_catalog
