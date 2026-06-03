/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2021, PickNik Robotics
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
 *   * Neither the name of PickNik Robotics nor the names of its
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
/* Modified from original code by David V. Lu */

#pragma once

#include <thread>
#include <memory>

#include <rclcpp/rclcpp.hpp>
#include <rclcpp/logger.hpp>

#include <rviz_common/render_panel.hpp>
#include <rviz_common/window_manager_interface.hpp>
#include <rviz_common/visualization_manager.hpp>
#include <rviz_common/display.hpp>
#include <rviz_common/ros_integration/ros_node_abstraction_iface.hpp>

#include <robot_state_publisher/robot_state_publisher.hpp>

#include <QWidget>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QTimer>

#include "robot_description_setup_framework/app_context.hpp"
#include "robot_description_setup_framework/qt/joint_state_zero_publisher.hpp"

namespace robot_description::setup_framework
{
class RVizPanel : public QWidget, public rviz_common::WindowManagerInterface
{
  Q_OBJECT
public:
  RVizPanel(QWidget* parent,
            const rviz_common::ros_integration::RosNodeAbstractionIface::WeakPtr& node_abstraction,
            const AppContextPtr& context);

  RVizPanel(const RVizPanel&) = delete;
  RVizPanel& operator=(const RVizPanel&) = delete;
  RVizPanel(RVizPanel&&) = delete;
  RVizPanel& operator=(RVizPanel&&) = delete;
  ~RVizPanel() override;

  bool isInitialized() const
  {
    return rviz_render_panel_ != nullptr;
  }

  void initialize();

  /**
   * @brief Render the given robot: publishes its URDF on /robot_description,
   *        starts publishing zero joint states, and points the fixed frame at
   *        the model root link.
   */
  void loadRobot(const URDFModel& model);

  QWidget* getParentWindow() override
  {
    return parent_;
  }

  rviz_common::PanelDockWidget* addPane(const QString& /*name*/, QWidget* /*pane*/,
                                        Qt::DockWidgetArea /*area*/ = Qt::LeftDockWidgetArea,
                                        bool /*floating*/ = true) override
  {
    return nullptr;
  }

  void setStatus(const QString& /*message*/) override
  {
  }

protected:
  QWidget* parent_;
  rviz_common::ros_integration::RosNodeAbstractionIface::WeakPtr node_abstraction_;
  rclcpp::Node::SharedPtr node_;
  AppContextPtr context_;
  std::shared_ptr<rclcpp::Logger> logger_;

  std::unique_ptr<rviz_common::RenderPanel> rviz_render_panel_;
  std::unique_ptr<rviz_common::VisualizationManager> rviz_manager_;
  rviz_common::Display* robot_model_display_ = nullptr;

  std::shared_ptr<robot_state_publisher::RobotStatePublisher> rsp_node_;
  std::shared_ptr<JointStateZeroPublisher> jsp_node_;
  rclcpp::executors::SingleThreadedExecutor::SharedPtr exec_;
  QTimer* spin_timer_ = nullptr;  // spins exec_ on the Qt main thread
};
}  // namespace robot_description::setup_framework
