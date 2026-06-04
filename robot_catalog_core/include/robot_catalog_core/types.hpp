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

struct RobotConfig {
  std::string id, display_name, description, image_path;
  std::string urdf_package, urdf_path, xacro_args, category;
  RobotSpecifications specifications;
  std::vector<std::string> required_packages, optional_packages, tags;
};

struct RobotFilter {
  std::optional<std::string> category;
  std::optional<double> min_payload, max_payload, min_reach, max_reach;
  std::optional<int> degrees_of_freedom;
  std::optional<bool> collaborative_only;
  std::vector<std::string> required_tags;
  std::string search_text;
  bool isEmpty() const {
    return !category && !min_payload && !max_payload && !min_reach &&
           !max_reach && !degrees_of_freedom && !collaborative_only &&
           required_tags.empty() && search_text.empty();
  }
};

}  // namespace robot_catalog
