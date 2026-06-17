#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "robot_catalog_core/types.hpp"

namespace robot_catalog {

class RobotCatalogCore {
public:
  // Load a single YAML file or, if `path` is a directory, every *.yml/*.yaml
  // under it (recursively), merging their `categories` and `robots` maps.
  // Throws std::runtime_error on YAML/IO error.
  void load(const std::string& path);
  void loadFromFile(const std::string& path);
  void loadFromDirectory(const std::string& dir);

  std::vector<RobotConfig> getAllRobots() const;  // sorted by id (stable)
  bool getRobotById(const std::string& id, RobotConfig& out) const;
  std::vector<CategoryInfo> getCategories() const;
  std::vector<RobotConfig> getRobotsByCategory(const std::string& category) const;
  std::vector<RobotConfig> filterRobots(const RobotFilter& f) const;
  std::vector<RobotConfig> searchRobots(const std::string& term) const;
  std::vector<std::string> getMissingPackages(const RobotConfig& r) const;
  bool validateRobotPackages(const RobotConfig& r) const;

  // Stable page of all robots: [offset, offset+limit). limit <= 0 means "all".
  // `total` is set to the full robot count.
  std::vector<RobotConfig> getRobotsPage(int offset, int limit, int& total) const;

  // Pre-serialized JSON, built once per load (avoids rebuilding for every call).
  const std::string& allRobotsJson() const { return all_robots_json_; }
  const std::string& categoriesJson() const { return categories_json_; }

private:
  static bool matchesFilter(const RobotConfig& r, const RobotFilter& f);
  static bool matchesSearch(const RobotConfig& r, const std::string& term);
  void rebuildCaches();  // sort + serialize after a load
  void mergeFile(const std::string& path);  // merge one yaml into the maps
  std::unordered_map<std::string, RobotConfig> robots_;
  std::unordered_map<std::string, CategoryInfo> categories_;
  std::vector<RobotConfig> sorted_robots_;  // by id, for stable pagination
  std::string all_robots_json_;
  std::string categories_json_;
};

}  // namespace robot_catalog
