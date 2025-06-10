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

#pragma once
// ROS2 includes
#include <rclcpp/rclcpp.hpp>

// Moveit includes
#include <moveit_setup_framework/data/package_settings_config.hpp>
#include <moveit_setup_framework/data/srdf_config.hpp>
#include <moveit_setup_framework/data/urdf_config.hpp>

#include <robot_description_setup_framework/setup_step.hpp>

namespace robot_description::core_plugins
{
class EndEffectorSelection : public SetupStep
{
public:
  std::string getName() const override
  {
    return "End-Effector Selection";
  }

  void onInit() override;

  bool isReady() const override
  {
    return true;  // always ready, no dependencies
  }

  std::filesystem::path getEndEffectorURDFPath();
  std::string getEndEffectorXacroArgs();
  std::filesystem::path getPackagePath();
  bool isXacroFile();

  void loadEndEffectorURDFFile(const std::filesystem::path& urdf_file_path, const std::string& xacro_args);
  void attachEndEffectorToRobot(const std::filesystem::path& end_effector_urdf_path, 
                                const std::string& attachment_link,
                                const std::string& xacro_args = "");

  // Robot compatibility methods
  void setSelectedRobotBrand(const std::string& robot_brand);
  std::string getSelectedRobotBrand() const;

protected:
  std::shared_ptr<moveit_setup::PackageSettingsConfig> package_settings_;
  std::shared_ptr<moveit_setup::SRDFConfig> srdf_config_;
  std::shared_ptr<moveit_setup::URDFConfig> urdf_config_;
  
  // Store end-effector specific information
  std::filesystem::path end_effector_urdf_path_;
  std::string end_effector_xacro_args_;
  std::string selected_robot_brand_;
  std::string attachment_link_;
};
}  // namespace robot_description::core_plugins
