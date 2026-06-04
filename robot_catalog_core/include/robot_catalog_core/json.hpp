#pragma once
#include <string>
#include <vector>
#include "robot_catalog_core/types.hpp"

namespace robot_catalog {
std::string json_escape(const std::string& s);
std::string to_json(const CategoryInfo& c);
std::string to_json(const RobotConfig& r);
std::string robots_to_json(const std::vector<RobotConfig>& robots);
std::string categories_to_json(const std::vector<CategoryInfo>& cats);
}  // namespace robot_catalog
