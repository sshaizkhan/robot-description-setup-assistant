/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2025, Shahwaz Khan
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Shahwaz Khan nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/
/* Author: Shahwaz Khan */

#include "robot_description_core_plugins/end_effectors/end_effector_selection.hpp"

namespace robot_description::core_plugins
{
void EndEffectorSelection::onInit()
{
  package_settings_ = config_data_->get<moveit_setup::PackageSettingsConfig>("package_settings");
  srdf_config_ = config_data_->get<moveit_setup::SRDFConfig>("srdf");
  urdf_config_ = config_data_->get<moveit_setup::URDFConfig>("urdf");
  
  // Initialize end-effector specific data
  end_effector_urdf_path_.clear();
  end_effector_xacro_args_.clear();
  selected_robot_brand_.clear();
  attachment_link_ = "tool0"; // Default attachment link for most robots
}

std::filesystem::path EndEffectorSelection::getEndEffectorURDFPath()
{
  return end_effector_urdf_path_;
}

std::string EndEffectorSelection::getEndEffectorXacroArgs()
{
  return end_effector_xacro_args_;
}

std::filesystem::path EndEffectorSelection::getPackagePath()
{
  return package_settings_->getPackagePath();
}

bool EndEffectorSelection::isXacroFile()
{
  return urdf_config_->isXacroFile();
}

void EndEffectorSelection::loadEndEffectorURDFFile(const std::filesystem::path& urdf_file_path, const std::string& xacro_args)
{
  // Store end-effector information
  end_effector_urdf_path_ = urdf_file_path;
  end_effector_xacro_args_ = xacro_args;
  
  RCLCPP_INFO(getLogger(), "Loading end-effector URDF: %s", urdf_file_path.c_str());
  RCLCPP_INFO(getLogger(), "End-effector xacro args: %s", xacro_args.c_str());
  
  // For now, we load the end-effector URDF directly for visualization
  // In a more advanced implementation, we would merge it with the robot URDF
  urdf_config_->loadFromPath(urdf_file_path, xacro_args);
  srdf_config_->updateRobotModel();
  
  RCLCPP_INFO(getLogger(), "End-effector URDF loaded successfully");
}

void EndEffectorSelection::attachEndEffectorToRobot(const std::filesystem::path& end_effector_urdf_path, 
                                                   const std::string& attachment_link,
                                                   const std::string& xacro_args)
{
  // Store attachment information
  end_effector_urdf_path_ = end_effector_urdf_path;
  end_effector_xacro_args_ = xacro_args;
  attachment_link_ = attachment_link;
  
  RCLCPP_INFO(getLogger(), "Attaching end-effector %s to robot link %s", 
              end_effector_urdf_path.c_str(), attachment_link.c_str());
  
  // TODO: Implement actual URDF merging/attachment logic
  // This would involve:
  // 1. Loading the robot URDF
  // 2. Loading the end-effector URDF
  // 3. Creating a joint between the robot's attachment link and end-effector base
  // 4. Merging the URDFs into a single combined URDF
  // 5. Updating the robot model
  
  // For now, just load the end-effector for visualization
  loadEndEffectorURDFFile(end_effector_urdf_path, xacro_args);
}

void EndEffectorSelection::setSelectedRobotBrand(const std::string& robot_brand)
{
  selected_robot_brand_ = robot_brand;
  RCLCPP_INFO(getLogger(), "Set selected robot brand for compatibility filtering: %s", robot_brand.c_str());
  
  // Determine appropriate attachment link based on robot brand
  if (robot_brand == "universal_robots") {
    attachment_link_ = "tool0";
  } else if (robot_brand == "kuka") {
    attachment_link_ = "tool0";
  } else if (robot_brand == "abb") {
    attachment_link_ = "tool0";
  } else if (robot_brand == "fanuc") {
    attachment_link_ = "tool0";
  } else {
    attachment_link_ = "tool0"; // Default fallback
  }
  
  RCLCPP_INFO(getLogger(), "Set attachment link to: %s", attachment_link_.c_str());
}

std::string EndEffectorSelection::getSelectedRobotBrand() const
{
  return selected_robot_brand_;
}

}  // namespace robot_description::core_plugins