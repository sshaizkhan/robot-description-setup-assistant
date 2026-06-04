#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "robot_catalog_core/types.hpp"

namespace robot_catalog {

class RobotCatalogCore {
public:
  // Throws std::runtime_error on YAML/IO error.
  void loadFromFile(const std::string& path);

  std::vector<RobotConfig> getAllRobots() const;
  bool getRobotById(const std::string& id, RobotConfig& out) const;
  std::vector<CategoryInfo> getCategories() const;
  std::vector<RobotConfig> getRobotsByCategory(const std::string& category) const;
  std::vector<RobotConfig> filterRobots(const RobotFilter& f) const;
  std::vector<RobotConfig> searchRobots(const std::string& term) const;
  std::vector<std::string> getMissingPackages(const RobotConfig& r) const;
  bool validateRobotPackages(const RobotConfig& r) const;

private:
  static bool matchesFilter(const RobotConfig& r, const RobotFilter& f);
  static bool matchesSearch(const RobotConfig& r, const std::string& term);
  std::unordered_map<std::string, RobotConfig> robots_;
  std::unordered_map<std::string, CategoryInfo> categories_;
};

}  // namespace robot_catalog
