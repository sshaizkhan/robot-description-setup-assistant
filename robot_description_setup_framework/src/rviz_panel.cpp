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

#include "robot_description_setup_framework/qt/rviz_panel.hpp"

#include <rviz_rendering/render_window.hpp>
#include <rviz_common/view_manager.hpp>
#include <rviz_common/view_controller.hpp>
#include <rviz_common/tool_manager.hpp>

namespace robot_description::setup_framework
{
static const char* ROBOT_DESCRIPTION_TOPIC = "/robot_description";

RVizPanel::RVizPanel(QWidget* parent,
                     const rviz_common::ros_integration::RosNodeAbstractionIface::WeakPtr& node_abstraction,
                     const AppContextPtr& context)
  : QWidget(parent)
  , parent_(parent)
  , node_abstraction_(node_abstraction)
  , node_(node_abstraction_.lock()->get_raw_node())
  , context_(context)
{
  logger_ = std::make_shared<rclcpp::Logger>(node_->get_logger().get_child("RVizPanel"));
}

RVizPanel::~RVizPanel()
{
  if (exec_)
  {
    exec_->cancel();
  }
  if (spin_thread_.joinable())
  {
    spin_thread_.join();
  }
  rviz_manager_.reset();
  rviz_render_panel_.reset();
}

void RVizPanel::initialize()
{
  // ---- RViz render panel + manager ----
  rviz_render_panel_ = std::make_unique<rviz_common::RenderPanel>();
  rviz_render_panel_->setMinimumWidth(300);
  rviz_render_panel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  QApplication::processEvents();
  rviz_render_panel_->getRenderWindow()->initialize();

  rviz_manager_ = std::make_unique<rviz_common::VisualizationManager>(rviz_render_panel_.get(), node_abstraction_,
                                                                      this, node_->get_clock());
  rviz_render_panel_->initialize(rviz_manager_.get());
  rviz_manager_->initialize();
  rviz_manager_->startUpdate();
  rviz_manager_->getToolManager()->addTool("rviz_default_plugins/MoveCamera");

  // ---- Stock RobotModel display, sourced from /robot_description topic ----
  robot_model_display_ = rviz_manager_->createDisplay("rviz_default_plugins/RobotModel", "Robot", true);
  if (robot_model_display_)
  {
    if (auto* p = robot_model_display_->subProp("Description Source"))
    {
      p->setValue("Topic");
    }
    if (auto* p = robot_model_display_->subProp("Description Topic"))
    {
      p->setValue(ROBOT_DESCRIPTION_TOPIC);
    }
  }

  rviz_common::ViewController* view = rviz_manager_->getViewManager()->getCurrent();
  view->subProp("Distance")->setValue(2.0f);

  // ---- Layout ----
  QVBoxLayout* rviz_layout = new QVBoxLayout();
  rviz_layout->addWidget(rviz_render_panel_.get());
  setLayout(rviz_layout);

  QHBoxLayout* btn_layout = new QHBoxLayout();
  rviz_layout->addLayout(btn_layout);

  QCheckBox* visual = new QCheckBox("visual");
  visual->setChecked(true);
  btn_layout->addWidget(visual);
  connect(visual, &QCheckBox::toggled, [this](bool checked) {
    if (robot_model_display_)
    {
      if (auto* p = robot_model_display_->subProp("Visual Enabled"))
      {
        p->setValue(checked);
      }
    }
  });

  QCheckBox* collision = new QCheckBox("collision");
  collision->setChecked(false);
  btn_layout->addWidget(collision);
  connect(collision, &QCheckBox::toggled, [this](bool checked) {
    if (robot_model_display_)
    {
      if (auto* p = robot_model_display_->subProp("Collision Enabled"))
      {
        p->setValue(checked);
      }
    }
  });

  // ---- Embedded RSP + JSP nodes on a background executor ----
  rclcpp::NodeOptions rsp_opts;
  rsp_opts.append_parameter_override(
      "robot_description", std::string("<robot name=\"empty\"><link name=\"base_link\"/></robot>"));
  rsp_node_ = std::make_shared<robot_state_publisher::RobotStatePublisher>(rsp_opts);
  jsp_node_ = std::make_shared<JointStateZeroPublisher>();

  exec_ = std::make_shared<rclcpp::executors::SingleThreadedExecutor>();
  exec_->add_node(rsp_node_);
  exec_->add_node(jsp_node_);
  spin_thread_ = std::thread([this]() { exec_->spin(); });
}

void RVizPanel::loadRobot(const URDFModel& model)
{
  if (!rsp_node_ || !jsp_node_)
  {
    RCLCPP_ERROR(*logger_, "loadRobot called before initialize()");
    return;
  }

  rsp_node_->set_parameter(rclcpp::Parameter("robot_description", model.xml));
  jsp_node_->setJoints(model.movable_joints);

  if (rviz_manager_ && !model.root_link.empty())
  {
    rviz_manager_->setFixedFrame(QString::fromStdString(model.root_link));
  }
  if (robot_model_display_)
  {
    robot_model_display_->setEnabled(true);
  }
}
}  // namespace robot_description::setup_framework
