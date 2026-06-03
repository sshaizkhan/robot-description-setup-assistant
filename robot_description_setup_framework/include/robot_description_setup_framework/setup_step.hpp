/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2012, Willow Garage, Inc.
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
 *   * Neither the name of Willow Garage nor the names of its
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
/* Modified from original code by Dave Coleman */

#pragma once
#include <rclcpp/node.hpp>
#include "robot_description_setup_framework/app_context.hpp"

namespace robot_description
{
/**
 * @brief Contains all of the non-GUI code necessary for doing one "screen" worth of setup.
 */
class SetupStep
{
public:
  SetupStep() = default;
  SetupStep(const SetupStep&) = default;
  SetupStep(SetupStep&&) = default;
  SetupStep& operator=(const SetupStep&) = default;
  SetupStep& operator=(SetupStep&&) = default;
  virtual ~SetupStep() = default;

  /**
   * @brief Called after construction to initialize the step
   * @param parent_node Shared pointer to the parent node
   */
  void initialize(const rclcpp::Node::SharedPtr& parent_node, const AppContextPtr& context)
  {
    parent_node_ = parent_node;
    context_ = context;
    logger_ = std::make_shared<rclcpp::Logger>(parent_node->get_logger().get_child(getName()));
    onInit();
  }

  /**
   * @brief Overridable initialization method
   */
  virtual void onInit()
  {
  }

  /**
   * @brief Returns the name of the setup step
   */
  virtual std::string getName() const = 0;

  /**
   * @brief Return true if the data necessary to proceed with this step has been configured
   */
  virtual bool isReady() const
  {
    return true;
  }

  /**
   * @brief Makes a namespaced logger for this step available to the widget
   */
  const rclcpp::Logger& getLogger() const
  {
    return *logger_;
  }

  rclcpp::Node::SharedPtr getParentNode() const {
    return parent_node_;
  }

  AppContextPtr getContext() const {
    return context_;
  }

protected:
  AppContextPtr context_;
  rclcpp::Node::SharedPtr parent_node_;
  std::shared_ptr<rclcpp::Logger> logger_;
};
}  // namespace robot_description
