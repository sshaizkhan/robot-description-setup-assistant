#include "robot_description_setup_framework/data/srdf_config.hpp"

namespace robot_description::setup_framework
{
void SRDFConfig::onInit()
{
  parent_node_->declare_parameter("robot_description_semantic", rclcpp::ParameterType::PARAMETER_STRING);
  changes_ = 0L;
}
void SRDFConfig::loadURDFModel()
{
  if (urdf_model_ != nullptr)
  {
    return;
  }

  auto urdf_config = config_data_->get<URDFConfig>("urdf");
  urdf_model_ = urdf_config->getModelPtr();
  srdf_.robot_name_ = urdf_model_->getName();
  parent_node_->set_parameter(rclcpp::Parameter("robot_description_semantic", srdf_.getSRDFString()));
}

} // namespace robot_description::setup_framework