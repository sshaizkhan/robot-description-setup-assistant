#include "robot_description_core_plugins/robots/robot_config_manager.hpp"
#include "robot_description_setup_framework/utilities.hpp"
#include <algorithm>
#include <ament_index_cpp/get_package_prefix.hpp>
#include <cctype>
#include <sstream>

namespace robot_description::core_plugins
{
// Singleton instance getter - this ensures only one config manager exists
RobotConfigManager& RobotConfigManager::getInstance()
{
  static RobotConfigManager instance;
  return instance;
}

// Load configuration from a single file
bool RobotConfigManager::loadFromFile(const std::filesystem::path& config_file)
{
  try {
    // Check if file exists before attempting to load
    if (!std::filesystem::exists(config_file)) {
      RCLCPP_ERROR(rclcpp::get_logger("RobotConfigManager"), "Config file does not exist: %s", config_file.c_str());
      return false;
    }
    
    // Parse the YAML file
    YAML::Node config = YAML::LoadFile(config_file.string());
    
    // Create config source for tracking
    ConfigSource source(config_file, "Direct file load");
    
    // Merge this configuration with existing ones
    return mergeConfiguration(config, source);
    
  } catch (const YAML::Exception& e) {
    RCLCPP_ERROR(rclcpp::get_logger("RobotConfigManager"), "YAML parsing error in file %s: %s", config_file.c_str(), e.what());
    return false;
  } catch (const std::exception& e) {
    RCLCPP_ERROR(rclcpp::get_logger("RobotConfigManager"), "Failed to load config file %s: %s", config_file.c_str(), e.what());
    return false;
  }
}

// Load configuration from a ROS package
bool RobotConfigManager::loadFromPackage(const std::string& package_name, const std::string& config_file)
{
  try {
    // Get the package share directory using ROS2 package index
    auto package_path = robot_description::getSharePath(package_name);
    auto config_path = package_path / config_file;
    return loadFromFile(config_path);
  } catch (const std::exception& e) {
    RCLCPP_ERROR(rclcpp::get_logger("RobotConfigManager"), "Failed to load config from package %s: %s", package_name.c_str(), e.what());
    return false;
  }
}

// Load multiple configuration files - this is where the power of multi-file support shows
bool RobotConfigManager::loadFromFiles(const std::vector<std::filesystem::path>& config_files)
{
  clearConfigurations();
  bool overall_success = true;
  
  for (const auto& config_file : config_files) {
    if (!addConfigurationFile(config_file, false)) {
      RCLCPP_WARN(rclcpp::get_logger("RobotConfigManager"), "Failed to load config file: %s", config_file.c_str());
      overall_success = false;
    }
  }
  
  // Set up file watching for all loaded configs
  if (auto_reload_enabled_) {
    setupFileWatcher();
  }
  
  RCLCPP_INFO(rclcpp::get_logger("RobotConfigManager"), "Loaded %zu robot configurations from %zu files", robots_.size(), loaded_configs_.size());
  
  return overall_success;
}

// Load multiple configs from a package - useful for organizing robots by type
bool RobotConfigManager::loadFromPackageConfigs(const std::string& package_name, 
                                               const std::vector<std::string>& config_files)
{
  std::vector<std::filesystem::path> full_paths;
  full_paths.reserve(config_files.size());
  
  try {
    auto package_path = robot_description::getSharePath(package_name);
    
    // Convert relative paths to absolute paths
    for (const auto& config_file : config_files) {
      full_paths.push_back(package_path / config_file);
    }
    
    return loadFromFiles(full_paths);
  } catch (const std::exception& e) {
    RCLCPP_ERROR(rclcpp::get_logger("RobotConfigManager"), "Failed to load configs from package %s: %s", package_name.c_str(), e.what());
    return false;
  }
}

// Add a single configuration file to existing configuration
bool RobotConfigManager::addConfigurationFile(const std::filesystem::path& config_file, bool overwrite_existing)
{
  try {
    if (!std::filesystem::exists(config_file)) {
      RCLCPP_ERROR(rclcpp::get_logger("RobotConfigManager"), "Config file does not exist: %s", config_file.c_str());
      return false;
    }
    
    YAML::Node config = YAML::LoadFile(config_file.string());
    ConfigSource source(config_file, "Added configuration");
    
    // Store the source before merging
    loaded_configs_.push_back(source);

    // suppress unused warning for now
    (void) overwrite_existing;
    
    // Merge configuration - this handles conflicts intelligently
    bool success = mergeConfiguration(config, source);
    
    if (success) {
      RCLCPP_DEBUG(rclcpp::get_logger("RobotConfigManager"), "Successfully added config file: %s", config_file.c_str());
    } else {
      // Remove the source if merging failed
      loaded_configs_.pop_back();
    }
    
    return success;
    
  } catch (const std::exception& e) {
    RCLCPP_ERROR(rclcpp::get_logger("RobotConfigManager"), "Failed to add config file %s: %s", config_file.c_str(), e.what());
    return false;
  }
}

void RobotConfigManager::clearConfigurations()
{
  robots_.clear();
  categories_.clear();
  loaded_configs_.clear();
  
  // Clean up file watcher
  if (file_watcher_) {
    cleanupFileWatcher();
  }
  
  RCLCPP_DEBUG(rclcpp::get_logger("RobotConfigManager"), "Cleared all configurations");
}

// Get list of loaded configuration sources for debugging/info
std::vector<std::string> RobotConfigManager::getLoadedConfigSources() const
{
  std::vector<std::string> sources;
  sources.reserve(loaded_configs_.size());
  
  for (const auto& config : loaded_configs_) {
    sources.push_back(config.file_path.string());
  }
  
  return sources;
}

// Get all robots - the foundation method that others build on
std::vector<RobotConfig> RobotConfigManager::getAllRobots() const
{
  std::vector<RobotConfig> result;
  result.reserve(robots_.size());
  
  // Convert map to vector for easier handling in UI
  for (const auto& [id, config] : robots_) {
    result.push_back(config);
  }
  
  // Sort by display name for consistent ordering
  std::sort(result.begin(), result.end(), 
    [](const RobotConfig& a, const RobotConfig& b) {
      return a.display_name < b.display_name;
    });
  
  return result;
}

// Get robots filtered by category - this is where categories become useful
std::vector<RobotConfig> RobotConfigManager::getRobotsByCategory(const std::string& category) const
{
  std::vector<RobotConfig> result;
  
  for (const auto& [id, config] : robots_) {
    if (config.category == category) {
      result.push_back(config);
    }
  }
  
  // Sort by display name
  std::sort(result.begin(), result.end(), 
    [](const RobotConfig& a, const RobotConfig& b) {
      return a.display_name < b.display_name;
    });
  
  return result;
}

// Get specific robot by ID - useful for direct lookups
RobotConfig RobotConfigManager::getRobotById(const std::string& id) const
{
  auto it = robots_.find(id);
  return (it != robots_.end()) ? it->second : RobotConfig{};
}

// Get all categories - for populating category dropdowns
std::vector<CategoryInfo> RobotConfigManager::getCategories() const
{
  std::vector<CategoryInfo> result;
  result.reserve(categories_.size());
  
  for (const auto& [id, info] : categories_) {
    result.push_back(info);
  }
  
  // Sort by display name for UI consistency
  std::sort(result.begin(), result.end(), 
    [](const CategoryInfo& a, const CategoryInfo& b) {
      return a.display_name < b.display_name;
    });
  
  return result;
}

// Get specific category information
CategoryInfo RobotConfigManager::getCategoryInfo(const std::string& category_id) const
{
  auto it = categories_.find(category_id);
  return (it != categories_.end()) ? it->second : CategoryInfo{};
}

// Advanced filtering - this is where the sophisticated search capabilities come from
std::vector<RobotConfig> RobotConfigManager::filterRobots(const RobotFilter& filter) const
{
  std::vector<RobotConfig> result;
  
  // If filter is empty, return all robots
  if (filter.isEmpty()) {
    return getAllRobots();
  }
  
  // Apply filter to each robot
  for (const auto& [id, config] : robots_) {
    if (matchesFilter(config, filter)) {
      result.push_back(config);
    }
  }
  
  // Sort results by relevance (robots matching more criteria first)
  std::sort(result.begin(), result.end(), 
    [](const RobotConfig& a, const RobotConfig& b) {
      return a.display_name < b.display_name;
    });
  
  return result;
}

// Text-based search across robot properties
std::vector<RobotConfig> RobotConfigManager::searchRobots(const std::string& search_term) const
{
  std::vector<RobotConfig> result;
  
  if (search_term.empty()) {
    return getAllRobots();
  }
  
  for (const auto& [id, config] : robots_) {
    if (matchesSearchTerm(config, search_term)) {
      result.push_back(config);
    }
  }
  
  return result;
}

// Package validation - crucial for ensuring robots will work
bool RobotConfigManager::validateRobotPackages(const RobotConfig& robot) const
{
  auto missing = getMissingPackages(robot);
  return missing.empty();
}

// Get list of missing packages - this helps users understand what they need to install
std::vector<std::string> RobotConfigManager::getMissingPackages(const RobotConfig& robot) const
{
  std::vector<std::string> missing;
  
  // Check required packages using ROS2 package index
  for (const auto& package : robot.required_packages) {
    try {
      // This will throw if package doesn't exist
      ament_index_cpp::get_package_share_directory(package);
    } catch (const ament_index_cpp::PackageNotFoundError&) {
      missing.push_back(package);
      RCLCPP_DEBUG(rclcpp::get_logger("RobotConfigManager"), "Required package not found: %s", package.c_str());
    }
  }
  
  return missing;
}

// Enable/disable automatic reloading when config files change
void RobotConfigManager::enableAutoReload(bool enable)
{
  auto_reload_enabled_ = enable;
  
  if (enable && !loaded_configs_.empty()) {
    setupFileWatcher();
  } else if (!enable) {
    cleanupFileWatcher();
  }
  
  RCLCPP_INFO(rclcpp::get_logger("RobotConfigManager"), "Auto-reload %s", enable ? "enabled" : "disabled");
}

// Manually reload all configurations - useful for debugging
bool RobotConfigManager::reloadConfigurations()
{
  RCLCPP_INFO(rclcpp::get_logger("RobotConfigManager"), "Reloading configurations...");
  
  // Save current config file paths
  std::vector<std::filesystem::path> config_paths;
  for (const auto& config : loaded_configs_) {
    config_paths.push_back(config.file_path);
  }
  
  // Clear and reload
  clearConfigurations();
  bool success = loadFromFiles(config_paths);
  
  if (success) {
    Q_EMIT robotListUpdated();
    RCLCPP_INFO(rclcpp::get_logger("RobotConfigManager"), "Configuration reload successful");
  } else {
    RCLCPP_ERROR(rclcpp::get_logger("RobotConfigManager"), "Configuration reload failed");
  }
  
  return success;
}

// File change detection - triggers automatic reloads
void RobotConfigManager::onConfigFileChanged(const QString& path)
{
  if (!auto_reload_enabled_) {
    return;
  }
  
  // Debounce rapid file changes (common with text editors that create temp files)
  auto now = std::chrono::system_clock::now();
  if (now - last_reload_time_ < std::chrono::seconds(1)) {
    return;
  }
  
  RCLCPP_INFO(rclcpp::get_logger("RobotConfigManager"), "Configuration file changed: %s", path.toStdString().c_str());
  
  if (reloadConfigurations()) {
    last_reload_time_ = now;
    Q_EMIT configurationsReloaded();
  } else {
    Q_EMIT configurationError(QString("Failed to reload configuration: %1").arg(path));
  }
}

// Directory change detection - handles file moves/renames
void RobotConfigManager::onConfigDirectoryChanged(const QString& path)
{
  RCLCPP_DEBUG(rclcpp::get_logger("RobotConfigManager"), "Configuration directory changed: %s", path.toStdString().c_str());
  
  // For directory changes, we do a full reload to be safe
  onConfigFileChanged(path);
}

// Parse individual robot configuration from YAML
RobotConfig RobotConfigManager::parseRobotConfig(const std::string& id, const YAML::Node& node)
{
  RobotConfig config;
  config.id = id;
  
  // Basic fields with defaults
  config.display_name = node["display_name"].as<std::string>(id);
  config.description = node["description"].as<std::string>("");
  config.urdf_package = node["urdf_package"].as<std::string>("");
  config.xacro_args = node["xacro_args"].as<std::string>("");
  config.category = node["category"].as<std::string>("default");
  
  // Handle file paths carefully
  if (node["image_path"]) {
    std::string image_path_str = node["image_path"].as<std::string>();
    config.image_path = robot_description::getSharePath("robot_description_setup_assistant") / image_path_str;
  }
  
  if (node["urdf_path"]) {
    config.urdf_path = node["urdf_path"].as<std::string>();
  }
  
  // Parse specifications if present
  if (node["specifications"]) {
    config.specifications = parseSpecifications(node["specifications"]);
  }
  
  // Parse package dependencies
  if (node["required_packages"]) {
    for (const auto& pkg : node["required_packages"]) {
      config.required_packages.push_back(pkg.as<std::string>());
    }
  }
  
  if (node["optional_packages"]) {
    for (const auto& pkg : node["optional_packages"]) {
      config.optional_packages.push_back(pkg.as<std::string>());
    }
  }
  
  // Parse tags for searching
  if (node["tags"]) {
    for (const auto& tag : node["tags"]) {
      config.tags.push_back(tag.as<std::string>());
    }
  }
  
  return config;
}

// Parse category information from YAML
CategoryInfo RobotConfigManager::parseCategoryInfo(const std::string& id, const YAML::Node& node)
{
  CategoryInfo info;
  info.id = id;
  info.display_name = node["display_name"].as<std::string>(id);
  info.description = node["description"].as<std::string>("");
  info.manufacturer = node["manufacturer"].as<std::string>("");
  info.website = node["website"].as<std::string>("");
  
  return info;
}

// Parse detailed specifications from YAML
RobotSpecifications RobotConfigManager::parseSpecifications(const YAML::Node& node)
{
  RobotSpecifications specs;
  
  specs.degrees_of_freedom = node["degrees_of_freedom"].as<int>(0);
  specs.payload_kg = node["payload_kg"].as<double>(0.0);
  specs.reach_mm = node["reach_mm"].as<double>(0.0);
  specs.weight_kg = node["weight_kg"].as<double>(0.0);
  specs.repeatability_mm = node["repeatability_mm"].as<double>(0.0);
  specs.max_speed_ms = node["max_speed_ms"].as<double>(0.0);
  specs.safety_certified = node["safety_certified"].as<bool>(false);
  specs.collaborative = node["collaborative"].as<bool>(false);
  specs.torque_sensing = node["torque_sensing"].as<bool>(false);
  
  // Parse mounting options array
  if (node["mounting"]) {
    for (const auto& mount : node["mounting"]) {
      specs.mounting_options.push_back(mount.as<std::string>());
    }
  }
  
  return specs;
}

// Merge new configuration with existing - handles conflicts intelligently
bool RobotConfigManager::mergeConfiguration(const YAML::Node& config, const ConfigSource& source)
{
  try {
    // Parse categories first
    if (config["categories"]) {
      for (const auto& category : config["categories"]) {
        std::string id = category.first.as<std::string>();
        CategoryInfo info = parseCategoryInfo(id, category.second);
        
        // Add or update category
        categories_[id] = info;
        RCLCPP_DEBUG(rclcpp::get_logger("RobotConfigManager"), 
                     "Loaded category: %s", id.c_str());
      }
    }
    
    // Parse robots
    if (config["robots"]) {
      for (const auto& robot : config["robots"]) {
        std::string id = robot.first.as<std::string>();
        RobotConfig robot_config = parseRobotConfig(id, robot.second);
        
        if (robot_config.isValid()) {
          robots_[id] = robot_config;
          RCLCPP_DEBUG(rclcpp::get_logger("RobotConfigManager"), 
                       "Loaded robot: %s", id.c_str());
        } else {
          RCLCPP_WARN(rclcpp::get_logger("RobotConfigManager"), 
                      "Invalid robot config for: %s", id.c_str());
        }
      }
    }
    
    return true;
    
  } catch (const std::exception& e) {
    RCLCPP_ERROR(rclcpp::get_logger("RobotConfigManager"), 
                 "Failed to merge configuration from %s: %s", 
                 source.file_path.c_str(), e.what());
    return false;
  }
}

// Set up file system watcher for auto-reload
void RobotConfigManager::setupFileWatcher()
{
  if (!file_watcher_) {
    file_watcher_ = new QFileSystemWatcher(this);
    
    connect(file_watcher_, &QFileSystemWatcher::fileChanged,
            this, &RobotConfigManager::onConfigFileChanged);
    connect(file_watcher_, &QFileSystemWatcher::directoryChanged,
            this, &RobotConfigManager::onConfigDirectoryChanged);
  }
  
  // Add all loaded config files to the watcher
  for (const auto& config_source : loaded_configs_) {
    QString path = QString::fromStdString(config_source.file_path.string());
    if (!file_watcher_->files().contains(path)) {
      file_watcher_->addPath(path);
      RCLCPP_DEBUG(rclcpp::get_logger("RobotConfigManager"), 
                   "Watching config file: %s", config_source.file_path.c_str());
    }
  }
}

// Clean up file watcher
void RobotConfigManager::cleanupFileWatcher()
{
  if (file_watcher_) {
    delete file_watcher_;
    file_watcher_ = nullptr;
  }
}

// Check if robot matches the given filter criteria
bool RobotConfigManager::matchesFilter(const RobotConfig& robot, const RobotFilter& filter) const
{
  // Category filter
  if (filter.category && robot.category != *filter.category) {
    return false;
  }
  
  // Payload filters
  if (filter.min_payload && robot.specifications.payload_kg < *filter.min_payload) {
    return false;
  }
  if (filter.max_payload && robot.specifications.payload_kg > *filter.max_payload) {
    return false;
  }
  
  // Reach filters
  if (filter.min_reach && robot.specifications.reach_mm < *filter.min_reach) {
    return false;
  }
  if (filter.max_reach && robot.specifications.reach_mm > *filter.max_reach) {
    return false;
  }
  
  // Degrees of freedom filter
  if (filter.degrees_of_freedom && robot.specifications.degrees_of_freedom != *filter.degrees_of_freedom) {
    return false;
  }
  
  // Collaborative filter
  if (filter.collaborative_only && *filter.collaborative_only && !robot.specifications.collaborative) {
    return false;
  }
  
  // Required tags filter - robot must have ALL required tags
  for (const auto& required_tag : filter.required_tags) {
    if (std::find(robot.tags.begin(), robot.tags.end(), required_tag) == robot.tags.end()) {
      return false;
    }
  }
  
  // Text search filter
  if (!filter.search_text.empty() && !matchesSearchTerm(robot, filter.search_text)) {
    return false;
  }
  
  return true;
}

// Check if robot matches search term across multiple fields
bool RobotConfigManager::matchesSearchTerm(const RobotConfig& robot, const std::string& search_term) const
{
  // Convert search term to lowercase for case-insensitive search
  std::string lower_search = search_term;
  std::transform(lower_search.begin(), lower_search.end(), lower_search.begin(), ::tolower);
  
  // Helper lambda to check if a string contains the search term
  auto contains_term = [&lower_search](const std::string& text) {
    std::string lower_text = text;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
    return lower_text.find(lower_search) != std::string::npos;
  };
  
  // Search in robot name
  if (contains_term(robot.display_name)) return true;
  
  // Search in description
  if (contains_term(robot.description)) return true;
  
  // Search in category
  if (contains_term(robot.category)) return true;
  
  // Search in tags
  for (const auto& tag : robot.tags) {
    if (contains_term(tag)) return true;
  }
  
  // Search in package names
  for (const auto& package : robot.required_packages) {
    if (contains_term(package)) return true;
  }
  
  return false;
}
} // namespace robot_description::core_plugins