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

// End-effector specifications structure
struct EndEffectorSpecifications {
  double max_opening_mm = 0.0;      // Maximum gripper opening
  double max_force_n = 0.0;         // Maximum gripping force
  double max_speed_mms = 0.0;       // Maximum closing speed
  double weight_kg = 0.0;           // End-effector weight
  double payload_kg = 0.0;          // Maximum payload it can handle
  std::string actuation_type;       // "pneumatic", "electric", "hydraulic"
  std::string interface_type;       // "modbus", "io", "canbus", "ethernet"
  bool force_feedback = false;      // Has force sensing
  bool position_feedback = false;   // Has position feedback
  bool adaptive_grip = false;       // Can adapt to object shape
  std::vector<std::string> compatible_objects; // Types of objects it can grip

  bool isValid() const {
    return max_opening_mm > 0.0 && max_force_n > 0.0;
  }
};

// End-effector configuration structure
struct EndEffectorConfig {
  std::string id;
  std::string display_name;
  std::string description;
  std::filesystem::path image_path;
  std::string urdf_package;
  std::filesystem::path urdf_path;
  std::string xacro_args;
  std::string category;
  std::string manufacturer;

  // Technical specifications
  EndEffectorSpecifications specifications;
  std::vector<std::string> required_packages;
  std::vector<std::string> optional_packages;
  std::vector<std::string> tags;
  
  // Compatibility information
  std::vector<std::string> compatible_robot_brands; // "universal_robots", "kuka", etc.
  std::string mounting_interface; // "iso_9409", "custom", etc.

  bool isValid() const {
    return !id.empty() && !display_name.empty() && !urdf_package.empty();
  }
};

// End-effector category information
struct EndEffectorCategoryInfo {
  std::string id;
  std::string display_name;
  std::string description;
  std::string typical_use_case;

  EndEffectorCategoryInfo() = default;
  EndEffectorCategoryInfo(const std::string &id, const std::string &display_name,
                         const std::string &description = "",
                         const std::string &use_case = "")
      : id(id), display_name(display_name), description(description), typical_use_case(use_case) {}
};

// Filtering structure for end-effector searches
struct EndEffectorFilter {
  std::optional<std::string> category;
  std::optional<double> min_opening;
  std::optional<double> max_opening;
  std::optional<double> min_force;
  std::optional<double> max_force;
  std::optional<double> min_payload;
  std::optional<double> max_payload;
  std::optional<std::string> actuation_type;
  std::optional<std::string> interface_type;
  std::optional<bool> force_feedback_required;
  std::optional<bool> position_feedback_required;
  std::optional<bool> adaptive_grip_required;
  std::string compatible_robot_brand;
  std::vector<std::string> required_tags;
  std::string search_text;

  bool isEmpty() const {
    return !category && !min_opening && !max_opening && !min_force &&
           !max_force && !min_payload && !max_payload && !actuation_type &&
           !interface_type && !force_feedback_required && !position_feedback_required &&
           !adaptive_grip_required && compatible_robot_brand.empty() &&
           required_tags.empty() && search_text.empty();
  }
};

// Configuration source tracking
struct EndEffectorConfigSource {
  std::filesystem::path file_path;
  std::string description;
  std::chrono::system_clock::time_point load_time;

  EndEffectorConfigSource(const std::filesystem::path &path, const std::string &desc = "")
      : file_path(path), description(desc),
        load_time(std::chrono::system_clock::now()) {}
};

// End-effector configuration manager
class EndEffectorConfigManager : public QObject {
  Q_OBJECT

public:
  // Singleton pattern
  static EndEffectorConfigManager &getInstance();

  // Configuration loading
  bool loadFromFile(const std::filesystem::path &config_file);
  bool loadFromPackage(const std::string &package_name, const std::string &config_file = "config/end_effectors.yaml");
  bool loadFromFiles(const std::vector<std::filesystem::path> &config_files);
  bool loadFromPackageConfigs(const std::string &package_name, const std::vector<std::string> &config_files);
  bool addConfigurationFile(const std::filesystem::path &config_file, bool overwrite_existing = false);

  // Configuration management
  void clearConfigurations();
  std::vector<std::string> getLoadedConfigSources() const;

  // End-effector retrieval methods
  std::vector<EndEffectorConfig> getAllEndEffectors() const;
  std::vector<EndEffectorConfig> getEndEffectorsByCategory(const std::string &category) const;
  EndEffectorConfig getEndEffectorById(const std::string &id) const;

  // Category management
  std::vector<EndEffectorCategoryInfo> getCategories() const;
  EndEffectorCategoryInfo getCategoryInfo(const std::string &category_id) const;

  // Advanced filtering and searching
  std::vector<EndEffectorConfig> filterEndEffectors(const EndEffectorFilter &filter) const;
  std::vector<EndEffectorConfig> searchEndEffectors(const std::string &search_term) const;
  std::vector<EndEffectorConfig> getCompatibleEndEffectors(const std::string &robot_brand) const;

  // Package validation
  bool validateEndEffectorPackages(const EndEffectorConfig &end_effector) const;
  std::vector<std::string> getMissingPackages(const EndEffectorConfig &end_effector) const;

  // Runtime reload capabilities
  void enableAutoReload(bool enable = true);
  bool reloadConfigurations();

Q_SIGNALS:
  void configurationsReloaded();
  void configurationError(const QString &error);
  void endEffectorListUpdated();

private Q_SLOTS:
  void onConfigFileChanged(const QString &path);
  void onConfigDirectoryChanged(const QString &path);

private:
  // Private constructor for singleton
  EndEffectorConfigManager() = default;
  ~EndEffectorConfigManager() = default;
  EndEffectorConfigManager(const EndEffectorConfigManager &) = delete;
  EndEffectorConfigManager &operator=(const EndEffectorConfigManager &) = delete;

  // Internal data storage
  std::unordered_map<std::string, EndEffectorConfig> end_effectors_;
  std::unordered_map<std::string, EndEffectorCategoryInfo> categories_;
  std::vector<EndEffectorConfigSource> loaded_configs_;

  // File watching
  QFileSystemWatcher *file_watcher_ = nullptr;
  bool auto_reload_enabled_ = false;
  std::chrono::system_clock::time_point last_reload_time_;

  // Internal helper methods
  EndEffectorConfig parseEndEffectorConfig(const std::string &id, const YAML::Node &node);
  EndEffectorCategoryInfo parseCategoryInfo(const std::string &id, const YAML::Node &node);
  EndEffectorSpecifications parseSpecifications(const YAML::Node &node);
  bool mergeConfiguration(const YAML::Node &config, const EndEffectorConfigSource &source);
  void setupFileWatcher();
  void cleanupFileWatcher();
  bool matchesFilter(const EndEffectorConfig &end_effector, const EndEffectorFilter &filter) const;
  bool matchesSearchTerm(const EndEffectorConfig &end_effector, const std::string &search_term) const;
};

} // namespace robot_description::core_plugins