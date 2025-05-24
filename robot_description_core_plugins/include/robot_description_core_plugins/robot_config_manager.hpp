#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <yaml-cpp/yaml.h>

namespace robot_description::core_plugins
{

struct RobotConfig
{
  std::string id;
  std::string display_name;
  std::string description;
  std::filesystem::path image_path;
  std::string urdf_package;
  std::filesystem::path urdf_path;
  std::string xacro_args;
  std::string category;
  
  bool isValid() const {
    return !id.empty() && !display_name.empty() && !urdf_package.empty();
  }
};

class RobotConfigManager
{
public:
  static RobotConfigManager& getInstance();
  
  bool loadFromFile(const std::filesystem::path& config_file);
  bool loadFromPackage(const std::string& package_name, const std::string& config_file = "config/robots.yaml");
  
  std::vector<RobotConfig> getAllRobots() const;
  std::vector<RobotConfig> getRobotsByCategory(const std::string& category) const;
  RobotConfig getRobotById(const std::string& id) const;
  
  std::vector<std::string> getCategories() const;
  
private:
  RobotConfigManager() = default;
  std::unordered_map<std::string, RobotConfig> robots_;
  
  RobotConfig parseRobotConfig(const std::string& id, const YAML::Node& node);
};

} // namespace robot_description::core_plugins