#include "robot_description_core_plugins/end_effector_config_manager.hpp"
#include "robot_description_setup_framework/utilities.hpp"
#include <algorithm>
#include <ament_index_cpp/get_package_prefix.hpp>
#include <cctype>
#include <sstream>

namespace robot_description::core_plugins
{

EndEffectorConfigManager& EndEffectorConfigManager::getInstance()
{
  static EndEffectorConfigManager instance;
  return instance;
}

bool EndEffectorConfigManager::loadFromFile(const std::filesystem::path& config_file)
{
  try {
    if (!std::filesystem::exists(config_file)) {
      RCLCPP_ERROR(rclcpp::get_logger("EndEffectorConfigManager"), "Config file does not exist: %s", config_file.c_str());
      return false;
    }
    
    YAML::Node config = YAML::LoadFile(config_file.string());
    EndEffectorConfigSource source(config_file, "Direct file load");
    
    return mergeConfiguration(config, source);
    
  } catch (const YAML::Exception& e) {
    RCLCPP_ERROR(rclcpp::get_logger("EndEffectorConfigManager"), "YAML parsing error in file %s: %s", config_file.c_str(), e.what());
    return false;
  } catch (const std::exception& e) {
    RCLCPP_ERROR(rclcpp::get_logger("EndEffectorConfigManager"), "Failed to load config file %s: %s", config_file.c_str(), e.what());
    return false;
  }
}

bool EndEffectorConfigManager::loadFromPackage(const std::string& package_name, const std::string& config_file)
{
  try {
    auto package_path = robot_description::getSharePath(package_name);
    auto config_path = package_path / config_file;
    return loadFromFile(config_path);
  } catch (const std::exception& e) {
    RCLCPP_ERROR(rclcpp::get_logger("EndEffectorConfigManager"), "Failed to load config from package %s: %s", package_name.c_str(), e.what());
    return false;
  }
}

std::vector<EndEffectorConfig> EndEffectorConfigManager::getAllEndEffectors() const
{
  std::vector<EndEffectorConfig> result;
  result.reserve(end_effectors_.size());
  
  for (const auto& [id, config] : end_effectors_) {
    result.push_back(config);
  }
  
  std::sort(result.begin(), result.end(), 
    [](const EndEffectorConfig& a, const EndEffectorConfig& b) {
      return a.display_name < b.display_name;
    });
  
  return result;
}

std::vector<EndEffectorConfig> EndEffectorConfigManager::getEndEffectorsByCategory(const std::string& category) const
{
  std::vector<EndEffectorConfig> result;
  
  for (const auto& [id, config] : end_effectors_) {
    if (config.category == category) {
      result.push_back(config);
    }
  }
  
  std::sort(result.begin(), result.end(), 
    [](const EndEffectorConfig& a, const EndEffectorConfig& b) {
      return a.display_name < b.display_name;
    });
  
  return result;
}

std::vector<EndEffectorConfig> EndEffectorConfigManager::getCompatibleEndEffectors(const std::string& robot_brand) const
{
  std::vector<EndEffectorConfig> result;
  
  for (const auto& [id, config] : end_effectors_) {
    // Check if this end-effector is compatible with the robot brand
    auto& compatible_brands = config.compatible_robot_brands;
    if (std::find(compatible_brands.begin(), compatible_brands.end(), robot_brand) != compatible_brands.end()) {
      result.push_back(config);
    }
  }
  
  std::sort(result.begin(), result.end(), 
    [](const EndEffectorConfig& a, const EndEffectorConfig& b) {
      return a.display_name < b.display_name;
    });
  
  return result;
}

std::vector<EndEffectorConfig> EndEffectorConfigManager::filterEndEffectors(const EndEffectorFilter& filter) const
{
  std::vector<EndEffectorConfig> result;
  
  if (filter.isEmpty()) {
    return getAllEndEffectors();
  }
  
  for (const auto& [id, config] : end_effectors_) {
    if (matchesFilter(config, filter)) {
      result.push_back(config);
    }
  }
  
  std::sort(result.begin(), result.end(), 
    [](const EndEffectorConfig& a, const EndEffectorConfig& b) {
      return a.display_name < b.display_name;
    });
  
  return result;
}

std::vector<std::string> EndEffectorConfigManager::getMissingPackages(const EndEffectorConfig& end_effector) const
{
  std::vector<std::string> missing;
  
  for (const auto& package : end_effector.required_packages) {
    try {
      ament_index_cpp::get_package_share_directory(package);
    } catch (const ament_index_cpp::PackageNotFoundError&) {
      missing.push_back(package);
    }
  }
  
  return missing;
}

std::vector<EndEffectorCategoryInfo> EndEffectorConfigManager::getCategories() const
{
  std::vector<EndEffectorCategoryInfo> result;
  result.reserve(categories_.size());
  
  for (const auto& [id, info] : categories_) {
    result.push_back(info);
  }
  
  std::sort(result.begin(), result.end(), 
    [](const EndEffectorCategoryInfo& a, const EndEffectorCategoryInfo& b) {
      return a.display_name < b.display_name;
    });
  
  return result;
}

EndEffectorConfig EndEffectorConfigManager::parseEndEffectorConfig(const std::string& id, const YAML::Node& node)
{
  EndEffectorConfig config;
  config.id = id;
  
  config.display_name = node["display_name"].as<std::string>(id);
  config.description = node["description"].as<std::string>("");
  config.urdf_package = node["urdf_package"].as<std::string>("");
  config.xacro_args = node["xacro_args"].as<std::string>("");
  config.category = node["category"].as<std::string>("default");
  config.manufacturer = node["manufacturer"].as<std::string>("");
  config.mounting_interface = node["mounting_interface"].as<std::string>("");
  
  if (node["image_path"]) {
    std::string image_path_str = node["image_path"].as<std::string>();
    config.image_path = robot_description::getSharePath("robot_description_setup_assistant") / image_path_str;
  }
  
  if (node["urdf_path"]) {
    config.urdf_path = node["urdf_path"].as<std::string>();
  }
  
  if (node["specifications"]) {
    config.specifications = parseSpecifications(node["specifications"]);
  }
  
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
  
  if (node["compatible_robot_brands"]) {
    for (const auto& brand : node["compatible_robot_brands"]) {
      config.compatible_robot_brands.push_back(brand.as<std::string>());
    }
  }
  
  if (node["tags"]) {
    for (const auto& tag : node["tags"]) {
      config.tags.push_back(tag.as<std::string>());
    }
  }
  
  return config;
}

EndEffectorSpecifications EndEffectorConfigManager::parseSpecifications(const YAML::Node& node)
{
  EndEffectorSpecifications specs;
  
  specs.max_opening_mm = node["max_opening_mm"].as<double>(0.0);
  specs.max_force_n = node["max_force_n"].as<double>(0.0);
  specs.max_speed_mms = node["max_speed_mms"].as<double>(0.0);
  specs.weight_kg = node["weight_kg"].as<double>(0.0);
  specs.payload_kg = node["payload_kg"].as<double>(0.0);
  specs.actuation_type = node["actuation_type"].as<std::string>("");
  specs.interface_type = node["interface_type"].as<std::string>("");
  specs.force_feedback = node["force_feedback"].as<bool>(false);
  specs.position_feedback = node["position_feedback"].as<bool>(false);
  specs.adaptive_grip = node["adaptive_grip"].as<bool>(false);
  
  if (node["compatible_objects"]) {
    for (const auto& obj : node["compatible_objects"]) {
      specs.compatible_objects.push_back(obj.as<std::string>());
    }
  }
  
  return specs;
}

bool EndEffectorConfigManager::mergeConfiguration(const YAML::Node& config, const EndEffectorConfigSource& source)
{
  try {
    // Parse categories first
    if (config["categories"]) {
      for (const auto& category : config["categories"]) {
        std::string id = category.first.as<std::string>();
        EndEffectorCategoryInfo info = parseCategoryInfo(id, category.second);
        categories_[id] = info;
      }
    }
    
    // Parse end-effectors
    if (config["end_effectors"]) {
      for (const auto& end_effector : config["end_effectors"]) {
        std::string id = end_effector.first.as<std::string>();
        EndEffectorConfig end_effector_config = parseEndEffectorConfig(id, end_effector.second);
        
        if (end_effector_config.isValid()) {
          end_effectors_[id] = end_effector_config;
          RCLCPP_DEBUG(rclcpp::get_logger("EndEffectorConfigManager"), 
                       "Loaded end-effector: %s", id.c_str());
        } else {
          RCLCPP_WARN(rclcpp::get_logger("EndEffectorConfigManager"), 
                      "Invalid end-effector config for: %s", id.c_str());
        }
      }
    }
    
    return true;
    
  } catch (const std::exception& e) {
    RCLCPP_ERROR(rclcpp::get_logger("EndEffectorConfigManager"), 
                 "Failed to merge configuration from %s: %s", 
                 source.file_path.c_str(), e.what());
    return false;
  }
}

EndEffectorCategoryInfo EndEffectorConfigManager::parseCategoryInfo(const std::string& id, const YAML::Node& node)
{
  EndEffectorCategoryInfo info;
  info.id = id;
  info.display_name = node["display_name"].as<std::string>(id);
  info.description = node["description"].as<std::string>("");
  info.typical_use_case = node["typical_use_case"].as<std::string>("");
  
  return info;
}

bool EndEffectorConfigManager::matchesFilter(const EndEffectorConfig& end_effector, const EndEffectorFilter& filter) const
{
  // Category filter
  if (filter.category && end_effector.category != *filter.category) {
    return false;
  }
  
  // Opening range filters
  if (filter.min_opening && end_effector.specifications.max_opening_mm < *filter.min_opening) {
    return false;
  }
  if (filter.max_opening && end_effector.specifications.max_opening_mm > *filter.max_opening) {
    return false;
  }
  
  // Force range filters
  if (filter.min_force && end_effector.specifications.max_force_n < *filter.min_force) {
    return false;
  }
  if (filter.max_force && end_effector.specifications.max_force_n > *filter.max_force) {
    return false;
  }
  
  // Payload range filters
  if (filter.min_payload && end_effector.specifications.payload_kg < *filter.min_payload) {
    return false;
  }
  if (filter.max_payload && end_effector.specifications.payload_kg > *filter.max_payload) {
    return false;
  }
  
  // Actuation type filter
  if (filter.actuation_type && end_effector.specifications.actuation_type != *filter.actuation_type) {
    return false;
  }
  
  // Interface type filter
  if (filter.interface_type && end_effector.specifications.interface_type != *filter.interface_type) {
    return false;
  }
  
  // Feature filters
  if (filter.force_feedback_required && *filter.force_feedback_required && !end_effector.specifications.force_feedback) {
    return false;
  }
  if (filter.position_feedback_required && *filter.position_feedback_required && !end_effector.specifications.position_feedback) {
    return false;
  }
  if (filter.adaptive_grip_required && *filter.adaptive_grip_required && !end_effector.specifications.adaptive_grip) {
    return false;
  }
  
  // Robot brand compatibility
  if (!filter.compatible_robot_brand.empty()) {
    auto& compatible_brands = end_effector.compatible_robot_brands;
    if (std::find(compatible_brands.begin(), compatible_brands.end(), filter.compatible_robot_brand) == compatible_brands.end()) {
      return false;
    }
  }
  
  // Text search filter
  if (!filter.search_text.empty() && !matchesSearchTerm(end_effector, filter.search_text)) {
    return false;
  }
  
  return true;
}

bool EndEffectorConfigManager::loadFromFiles(const std::vector<std::filesystem::path>& config_files)
{
  clearConfigurations();
  bool overall_success = true;
  
  for (const auto& config_file : config_files) {
    if (!addConfigurationFile(config_file, false)) {
      RCLCPP_WARN(rclcpp::get_logger("EndEffectorConfigManager"), "Failed to load config file: %s", config_file.c_str());
      overall_success = false;
    }
  }
  
  // Set up file watching for all loaded configs
  if (auto_reload_enabled_) {
    setupFileWatcher();
  }
  
  RCLCPP_INFO(rclcpp::get_logger("EndEffectorConfigManager"), "Loaded %zu end-effector configurations from %zu files", end_effectors_.size(), loaded_configs_.size());
  
  return overall_success;
}

bool EndEffectorConfigManager::loadFromPackageConfigs(const std::string& package_name, 
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
    RCLCPP_ERROR(rclcpp::get_logger("EndEffectorConfigManager"), "Failed to load configs from package %s: %s", package_name.c_str(), e.what());
    return false;
  }
}

bool EndEffectorConfigManager::addConfigurationFile(const std::filesystem::path& config_file, bool overwrite_existing)
{
  try {
    if (!std::filesystem::exists(config_file)) {
      RCLCPP_ERROR(rclcpp::get_logger("EndEffectorConfigManager"), "Config file does not exist: %s", config_file.c_str());
      return false;
    }
    
    YAML::Node config = YAML::LoadFile(config_file.string());
    EndEffectorConfigSource source(config_file, "Added configuration");
    
    // Store the source before merging
    loaded_configs_.push_back(source);

    // suppress unused warning for now
    (void) overwrite_existing;
    
    // Merge configuration
    bool success = mergeConfiguration(config, source);
    
    if (success) {
      RCLCPP_DEBUG(rclcpp::get_logger("EndEffectorConfigManager"), "Successfully added config file: %s", config_file.c_str());
    } else {
      // Remove the source if merging failed
      loaded_configs_.pop_back();
    }
    
    return success;
    
  } catch (const std::exception& e) {
    RCLCPP_ERROR(rclcpp::get_logger("EndEffectorConfigManager"), "Failed to add config file %s: %s", config_file.c_str(), e.what());
    return false;
  }
}

void EndEffectorConfigManager::clearConfigurations()
{
  end_effectors_.clear();
  categories_.clear();
  loaded_configs_.clear();
  
  // Clean up file watcher
  if (file_watcher_) {
    cleanupFileWatcher();
  }
  
  RCLCPP_DEBUG(rclcpp::get_logger("EndEffectorConfigManager"), "Cleared all configurations");
}

std::vector<std::string> EndEffectorConfigManager::getLoadedConfigSources() const
{
  std::vector<std::string> sources;
  sources.reserve(loaded_configs_.size());
  
  for (const auto& config : loaded_configs_) {
    sources.push_back(config.file_path.string());
  }
  
  return sources;
}

EndEffectorConfig EndEffectorConfigManager::getEndEffectorById(const std::string& id) const
{
  auto it = end_effectors_.find(id);
  return (it != end_effectors_.end()) ? it->second : EndEffectorConfig{};
}

EndEffectorCategoryInfo EndEffectorConfigManager::getCategoryInfo(const std::string& category_id) const
{
  auto it = categories_.find(category_id);
  return (it != categories_.end()) ? it->second : EndEffectorCategoryInfo{};
}

std::vector<EndEffectorConfig> EndEffectorConfigManager::searchEndEffectors(const std::string& search_term) const
{
  std::vector<EndEffectorConfig> result;
  
  if (search_term.empty()) {
    return getAllEndEffectors();
  }
  
  for (const auto& [id, config] : end_effectors_) {
    if (matchesSearchTerm(config, search_term)) {
      result.push_back(config);
    }
  }
  
  return result;
}

bool EndEffectorConfigManager::validateEndEffectorPackages(const EndEffectorConfig& end_effector) const
{
  auto missing = getMissingPackages(end_effector);
  return missing.empty();
}

void EndEffectorConfigManager::enableAutoReload(bool enable)
{
  auto_reload_enabled_ = enable;
  
  if (enable && !loaded_configs_.empty()) {
    setupFileWatcher();
  } else if (!enable) {
    cleanupFileWatcher();
  }
  
  RCLCPP_INFO(rclcpp::get_logger("EndEffectorConfigManager"), "Auto-reload %s", enable ? "enabled" : "disabled");
}

bool EndEffectorConfigManager::reloadConfigurations()
{
  RCLCPP_INFO(rclcpp::get_logger("EndEffectorConfigManager"), "Reloading configurations...");
  
  // Save current config file paths
  std::vector<std::filesystem::path> config_paths;
  for (const auto& config : loaded_configs_) {
    config_paths.push_back(config.file_path);
  }
  
  // Clear and reload
  clearConfigurations();
  bool success = loadFromFiles(config_paths);
  
  if (success) {
    Q_EMIT endEffectorListUpdated();
    RCLCPP_INFO(rclcpp::get_logger("EndEffectorConfigManager"), "Configuration reload successful");
  } else {
    RCLCPP_ERROR(rclcpp::get_logger("EndEffectorConfigManager"), "Configuration reload failed");
  }
  
  return success;
}

void EndEffectorConfigManager::onConfigFileChanged(const QString& path)
{
  if (!auto_reload_enabled_) {
    return;
  }
  
  // Debounce rapid file changes
  auto now = std::chrono::system_clock::now();
  if (now - last_reload_time_ < std::chrono::seconds(1)) {
    return;
  }
  
  RCLCPP_INFO(rclcpp::get_logger("EndEffectorConfigManager"), "Configuration file changed: %s", path.toStdString().c_str());
  
  if (reloadConfigurations()) {
    last_reload_time_ = now;
    Q_EMIT configurationsReloaded();
  } else {
    Q_EMIT configurationError(QString("Failed to reload configuration: %1").arg(path));
  }
}

void EndEffectorConfigManager::onConfigDirectoryChanged(const QString& path)
{
  RCLCPP_DEBUG(rclcpp::get_logger("EndEffectorConfigManager"), "Configuration directory changed: %s", path.toStdString().c_str());
  
  // For directory changes, we do a full reload to be safe
  onConfigFileChanged(path);
}

void EndEffectorConfigManager::setupFileWatcher()
{
  if (!file_watcher_) {
    file_watcher_ = new QFileSystemWatcher(this);
    
    connect(file_watcher_, &QFileSystemWatcher::fileChanged,
            this, &EndEffectorConfigManager::onConfigFileChanged);
    connect(file_watcher_, &QFileSystemWatcher::directoryChanged,
            this, &EndEffectorConfigManager::onConfigDirectoryChanged);
  }
  
  // Add all loaded config files to the watcher
  for (const auto& config_source : loaded_configs_) {
    QString path = QString::fromStdString(config_source.file_path.string());
    if (!file_watcher_->files().contains(path)) {
      file_watcher_->addPath(path);
      RCLCPP_DEBUG(rclcpp::get_logger("EndEffectorConfigManager"), 
                   "Watching config file: %s", config_source.file_path.c_str());
    }
  }
}

void EndEffectorConfigManager::cleanupFileWatcher()
{
  if (file_watcher_) {
    delete file_watcher_;
    file_watcher_ = nullptr;
  }
}

bool EndEffectorConfigManager::matchesSearchTerm(const EndEffectorConfig& end_effector, const std::string& search_term) const
{
  std::string lower_search = search_term;
  std::transform(lower_search.begin(), lower_search.end(), lower_search.begin(), ::tolower);
  
  auto contains_term = [&lower_search](const std::string& text) {
    std::string lower_text = text;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
    return lower_text.find(lower_search) != std::string::npos;
  };
  
  // Search in end-effector name
  if (contains_term(end_effector.display_name)) return true;
  
  // Search in description
  if (contains_term(end_effector.description)) return true;
  
  // Search in manufacturer
  if (contains_term(end_effector.manufacturer)) return true;
  
  // Search in category
  if (contains_term(end_effector.category)) return true;
  
  // Search in tags
  for (const auto& tag : end_effector.tags) {
    if (contains_term(tag)) return true;
  }
  
  return false;
}

} // namespace robot_description::core_plugins