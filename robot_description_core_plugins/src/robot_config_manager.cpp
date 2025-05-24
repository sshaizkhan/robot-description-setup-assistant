#include "robot_description_core_plugins/robot_config_manager.hpp"
#include "robot_description_setup_framework/utilities.hpp"
#include <rclcpp/logging.hpp>

namespace robot_description::core_plugins
{

RobotConfigManager& RobotConfigManager::getInstance()
{
  static RobotConfigManager instance;
  return instance;
}

bool RobotConfigManager::loadFromPackage(const std::string& package_name, const std::string& config_file)
{
  try {
    auto package_path = robot_description::getSharePath(package_name);
    auto config_path = package_path / config_file;
    return loadFromFile(config_path);
  } catch (const std::exception& e) {
    RCLCPP_ERROR(rclcpp::get_logger("RobotConfigManager"), "Failed to load config from package %s: %s", package_name.c_str(), e.what());
    return false;
  }
}

bool RobotConfigManager::loadFromFile(const std::filesystem::path& config_file)
{
  try {
    if (!std::filesystem::exists(config_file)) {
      RCLCPP_ERROR(rclcpp::get_logger("RobotConfigManager"), "Config file does not exist: %s", config_file.c_str());
      return false;
    }
    
    YAML::Node config = YAML::LoadFile(config_file.string());
    
    if (!config["robots"]) {
      RCLCPP_ERROR(rclcpp::get_logger("RobotConfigManager"), "No 'robots' section found in config file");
      return false;
    }
    
    robots_.clear();
    
    for (const auto& robot_entry : config["robots"]) {
      std::string id = robot_entry.first.as<std::string>();
      RobotConfig robot = parseRobotConfig(id, robot_entry.second);
      
      if (robot.isValid()) {
        robots_[id] = robot;
        RCLCPP_DEBUG(rclcpp::get_logger("RobotConfigManager"), "Loaded robot: %s", id.c_str());
      } else {
        RCLCPP_WARN(rclcpp::get_logger("RobotConfigManager"), "Invalid robot config for: %s", id.c_str());
      }
    }
    
    RCLCPP_INFO(rclcpp::get_logger("RobotConfigManager"), "Loaded %zu robot configurations", robots_.size());
    return true;
    
  } catch (const std::exception& e) {
    RCLCPP_ERROR(rclcpp::get_logger("RobotConfigManager"), "Failed to load config file %s: %s", config_file.c_str(), e.what());
    return false;
  }
}

RobotConfig RobotConfigManager::parseRobotConfig(const std::string& id, const YAML::Node& node)
{
  RobotConfig config;
  config.id = id;
  
  config.display_name = node["display_name"].as<std::string>(id);
  config.description = node["description"].as<std::string>("");
  config.urdf_package = node["urdf_package"].as<std::string>("");
  config.xacro_args = node["xacro_args"].as<std::string>("");
  config.category = node["category"].as<std::string>("default");
  
  if (node["image_path"]) {
    std::string image_path_str = node["image_path"].as<std::string>();
    config.image_path = robot_description::getSharePath("robot_description_setup_assistant") / image_path_str;
  }
  
  if (node["urdf_path"]) {
    config.urdf_path = node["urdf_path"].as<std::string>();
  }
  
  return config;
}

std::vector<RobotConfig> RobotConfigManager::getAllRobots() const
{
  std::vector<RobotConfig> result;
  result.reserve(robots_.size());
  
  for (const auto& [id, config] : robots_) {
    result.push_back(config);
  }
  
  return result;
}

std::vector<RobotConfig> RobotConfigManager::getRobotsByCategory(const std::string& category) const
{
  std::vector<RobotConfig> result;
  
  for (const auto& [id, config] : robots_) {
    if (config.category == category) {
      result.push_back(config);
    }
  }
  
  return result;
}

RobotConfig RobotConfigManager::getRobotById(const std::string& id) const
{
  auto it = robots_.find(id);
  return (it != robots_.end()) ? it->second : RobotConfig{};
}

std::vector<std::string> RobotConfigManager::getCategories() const
{
  std::set<std::string> categories;
  for (const auto& [id, config] : robots_) {
    categories.insert(config.category);
  }
  return std::vector<std::string>(categories.begin(), categories.end());
}

} // namespace robot_description::core_plugins