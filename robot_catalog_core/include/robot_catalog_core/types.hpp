#pragma once
#include <string>
#include <vector>
#include <optional>

namespace robot_catalog {

struct CategoryInfo {
  std::string id, display_name, description, manufacturer, website;
};

struct RobotSpecifications {
  int degrees_of_freedom = 0;
  double payload_kg = 0.0, reach_mm = 0.0, weight_kg = 0.0;
  double repeatability_mm = 0.0, max_speed_ms = 0.0;
  std::vector<std::string> mounting_options;
  bool safety_certified = false, collaborative = false, torque_sensing = false;
};

// How a component connects to others. For an arm, `tool_frame` is the link that
// an end-effector mounts onto (e.g. "tool0"). For an end-effector, `mount_frame`
// is its own connecting link, attached to the arm's tool_frame at xyz/rpy.
struct AttachInfo {
  std::string tool_frame;   // arms: where tools mount
  std::string mount_frame;  // end_effectors: the link that connects
  double xyz[3] = {0.0, 0.0, 0.0};
  double rpy[3] = {0.0, 0.0, 0.0};
};

struct RobotConfig {
  std::string id, display_name, description, image_path;
  std::string urdf_package, urdf_path, xacro_args, category;
  // Component kind: "arm" (default), "end_effector", or "base".
  std::string type = "arm";
  AttachInfo attach;
  RobotSpecifications specifications;
  std::vector<std::string> required_packages, optional_packages, tags;
};

struct RobotFilter {
  std::optional<std::string> type;  // "arm" | "end_effector" | "base"
  std::optional<std::string> category;
  std::optional<double> min_payload, max_payload, min_reach, max_reach;
  std::optional<int> degrees_of_freedom;
  std::optional<bool> collaborative_only;
  std::vector<std::string> required_tags;
  std::string search_text;
  bool isEmpty() const {
    return !type && !category && !min_payload && !max_payload && !min_reach &&
           !max_reach && !degrees_of_freedom && !collaborative_only &&
           required_tags.empty() && search_text.empty();
  }
};

}  // namespace robot_catalog
