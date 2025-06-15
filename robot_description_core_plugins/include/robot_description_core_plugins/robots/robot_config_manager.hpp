#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <ament_index_cpp/get_package_share_directory.hpp>
#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <rclcpp/rclcpp.hpp>
#include <set>
#include <unordered_map>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace robot_description::core_plugins {

// Structure to hold detailed category information
struct CategoryInfo {
  std::string id;
  std::string display_name;
  std::string description;
  std::string manufacturer;
  std::string website;

  // Default constructor for empty categories
  CategoryInfo() = default;

  // Constructor with all fields
  CategoryInfo(const std::string &id, const std::string &display_name,
               const std::string &description = "",
               const std::string &manufacturer = "",
               const std::string &website = "")
      : id(id)
      , display_name(display_name)
      , description(description)
      , manufacturer(manufacturer)
      , website(website) {}
};

// Detailed robot specifications structure
struct RobotSpecifications {
  int degrees_of_freedom = 0;
  double payload_kg = 0.0;
  double reach_mm = 0.0;
  double weight_kg = 0.0;
  double repeatability_mm = 0.0;
  double max_speed_ms = 0.0;
  std::vector<std::string> mounting_options;
  bool safety_certified = false;
  bool collaborative = false;
  bool torque_sensing = false;

  // Helper method to check if specifications are valid
  bool isValid() const {
    return degrees_of_freedom > 0 && payload_kg > 0.0 && reach_mm > 0.0;
  }
};

// Enhanced robot configuration structure
struct RobotConfig {
  std::string id;
  std::string display_name;
  std::string description;
  std::filesystem::path image_path;
  std::string urdf_package;
  std::filesystem::path urdf_path;
  std::string xacro_args;
  std::string category;

  // Enhanced fields for advanced features
  RobotSpecifications specifications;
  std::vector<std::string> required_packages;
  std::vector<std::string> optional_packages;
  std::vector<std::string> tags;

  // Validation method
  bool isValid() const {
    return !id.empty() && !display_name.empty() && !urdf_package.empty();
  }
};

// Filtering structure for advanced robot searches
struct RobotFilter {
  std::optional<std::string> category;
  std::optional<double> min_payload;
  std::optional<double> max_payload;
  std::optional<double> min_reach;
  std::optional<double> max_reach;
  std::optional<int> degrees_of_freedom;
  std::optional<bool> collaborative_only;
  std::vector<std::string> required_tags;
  std::string search_text;

  // Helper method to check if filter is empty (no constraints applied)
  bool isEmpty() const {
    return !category && !min_payload && !max_payload && !min_reach &&
           !max_reach && !degrees_of_freedom && !collaborative_only &&
           required_tags.empty() && search_text.empty();
  }
};

// Configuration source tracking for multi-file support
struct ConfigSource {
  std::filesystem::path file_path;
  std::string description;
  std::chrono::system_clock::time_point load_time;

  ConfigSource(const std::filesystem::path &path, const std::string &desc = "")
      : file_path(path), description(desc),
        load_time(std::chrono::system_clock::now()) {}
};

// Main configuration manager class
class RobotConfigManager : public QObject {
  Q_OBJECT

public:
  // Singleton pattern for global access
  static RobotConfigManager &getInstance();

  // Basic configuration loading
  bool loadFromFile(const std::filesystem::path &config_file);
  bool loadFromPackage(const std::string &package_name, const std::string &config_file = "config/robots.yaml");

  // Multi-file configuration support
  bool loadFromFiles(const std::vector<std::filesystem::path> &config_files);
  bool loadFromPackageConfigs(const std::string &package_name, const std::vector<std::string> &config_files);
  bool addConfigurationFile(const std::filesystem::path &config_file, bool overwrite_existing = false);

  // Configuration management
  void clearConfigurations();
  std::vector<std::string> getLoadedConfigSources() const;

  // Robot retrieval methods
  std::vector<RobotConfig> getAllRobots() const;
  std::vector<RobotConfig>
  getRobotsByCategory(const std::string &category) const;
  RobotConfig getRobotById(const std::string &id) const;

  // Category management
  std::vector<CategoryInfo> getCategories() const;
  CategoryInfo getCategoryInfo(const std::string &category_id) const;

  // Advanced filtering and searching
  std::vector<RobotConfig> filterRobots(const RobotFilter &filter) const;
  std::vector<RobotConfig> searchRobots(const std::string &search_term) const;

  // Package validation
  bool validateRobotPackages(const RobotConfig &robot) const;
  std::vector<std::string> getMissingPackages(const RobotConfig &robot) const;

  // Runtime reload capabilities
  void enableAutoReload(bool enable = true);
  bool reloadConfigurations();

Q_SIGNALS:
  void configurationsReloaded();
  void configurationError(const QString &error);
  void robotListUpdated();

private Q_SLOTS:
  void onConfigFileChanged(const QString &path);
  void onConfigDirectoryChanged(const QString &path);

private:
  // Private constructor for singleton pattern
  RobotConfigManager() = default;
  ~RobotConfigManager() = default;

  // Delete copy constructor and assignment operator
  RobotConfigManager(const RobotConfigManager &) = delete;
  RobotConfigManager &operator=(const RobotConfigManager &) = delete;

  // Internal data storage
  std::unordered_map<std::string, RobotConfig> robots_;
  std::unordered_map<std::string, CategoryInfo> categories_;
  std::vector<ConfigSource> loaded_configs_;

  // File watching for auto-reload
  QFileSystemWatcher *file_watcher_ = nullptr;
  bool auto_reload_enabled_ = false;
  std::chrono::system_clock::time_point last_reload_time_;

  // Internal helper methods
  RobotConfig parseRobotConfig(const std::string &id, const YAML::Node &node);
  CategoryInfo parseCategoryInfo(const std::string &id, const YAML::Node &node);
  RobotSpecifications parseSpecifications(const YAML::Node &node);
  bool mergeConfiguration(const YAML::Node &config, const ConfigSource &source);
  void setupFileWatcher();
  void cleanupFileWatcher();
  bool matchesFilter(const RobotConfig &robot, const RobotFilter &filter) const;
  bool matchesSearchTerm(const RobotConfig &robot, const std::string &search_term) const;
};

} // namespace robot_description::core_plugins