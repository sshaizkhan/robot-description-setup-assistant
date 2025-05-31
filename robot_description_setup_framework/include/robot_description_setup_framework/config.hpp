#include "robot_description_common/macros/declare_ptr.hpp"
#include <filesystem>
#include <memory>
#include <rclcpp/logger.hpp>
#include <rclcpp/node.hpp>
#include <robot_description_common/macros/class_forward.hpp>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/yaml.h>

namespace robot_description::setup_framework {

ROBOT_DESCRIPTION_CLASS_FORWARD(
    SetupConfig); // Define SetupConfigPtr, ConstPtr, WeakPtr .. etc

class SetupConfig {
public:
  SetupConfig() = default;
  SetupConfig(const SetupConfig &) = default;
  SetupConfig(SetupConfig &&) = default;
  SetupConfig &operator=(const SetupConfig &) = default;
  SetupConfig &operator=(SetupConfig &&) = default;
  virtual ~SetupConfig() = default;

  /**
   * @brief Called after construction to initialize the step
   * @param parent_node Shared pointer to the parent node
   * @param name
   */
  void initialize(const rclcpp::Node::SharedPtr& parent_node, const std::string& name)
  {
    parent_node_ = parent_node;
    name_ = name;
    logger_ = std::make_shared<rclcpp::Logger>(parent_node->get_logger().get_child(name));
    onInit();
  }

  ///\brief Initialization method
  virtual void onInit() = 0;

  ///\brief Return true if this part of the configuration is completely setup
  virtual bool isConfgigured() const
  {
    return false;
  }

  ///\brief Loads the configuration from an exisitng robot descriptio configuration
  virtual void loadPrevious(const std::filesystem::path& /*config_package_path*/, const YAML::Node& /*nodes*/) = 0;

  ///\brief Optionally save "meta" information for saving in .robot_description_assistant yaml file
  virtual YAML::Node saveToYaml() const 
  {
    return YAML::Node();
  }

protected:
  rclcpp::Node::SharedPtr parent_node_;
  std::string name_;
  std::shared_ptr<rclcpp::Logger> logger_;
};

} // namespace robot_description::setup_framework