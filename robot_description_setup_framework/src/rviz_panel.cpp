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

namespace robot_description::setup_framework
{
RVizPanel::RVizPanel(QWidget* parent,
                     const rviz_common::ros_integration::RosNodeAbstractionIface::WeakPtr& node_abstraction,
                     const moveit_setup::DataWarehousePtr& config_data)
  : QWidget(parent)
  , parent_(parent)
  , node_abstraction_(node_abstraction)
  , node_(node_abstraction_.lock()->get_raw_node())
  , config_data_(config_data)
{
  logger_ = std::make_shared<rclcpp::Logger>(node_->get_logger().get_child("RVizPanel"));
}

void RVizPanel::initialize()
{
  static int initialize_call_count = 0;
  initialize_call_count++;

  // Initialize rviz_render_panel_ if not already done
  if (!isRvizRenderPanelInitialized_)
  {
    rviz_render_panel_ = std::make_unique<rviz_common::RenderPanel>();
    rviz_render_panel_->setMinimumWidth(200);
    rviz_render_panel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QApplication::processEvents();
    rviz_render_panel_->getRenderWindow()->initialize();
    isRvizRenderPanelInitialized_ = true;
  }

  // Initialize rviz_manager_ if not already done
  if (!isRvizManagerInitialized_)
  {
    rviz_manager_ = std::make_unique<rviz_common::VisualizationManager>(rviz_render_panel_.get(), node_abstraction_,
                                                                        this, node_->get_clock());

    rviz_render_panel_->initialize(rviz_manager_.get());
    rviz_manager_->initialize();
    rviz_manager_->startUpdate();

    auto tm = rviz_manager_->getToolManager();
    tm->addTool("rviz_default_plugins/MoveCamera");
    isRvizManagerInitialized_ = true;
  }

  // Initialize or update robot_state_display_
  if (initialize_call_count == 2 && !isRobotStateDisplayInitialized_)
  {
    robot_state_display_ = std::make_unique<moveit_rviz_plugin::RobotStateDisplay>();
    robot_state_display_->setName("Robot State");
    rviz_manager_->addDisplay(robot_state_display_.get(), true);

    updateFixedFrame();

    robot_state_display_->subProp("Robot State Topic")->setValue(QString::fromStdString(MOVEIT_ROBOT_STATE));
    robot_state_display_->subProp("Robot Description")->setValue(QString::fromStdString(ROBOT_DESCRIPTION));
    robot_state_display_->setVisible(true);

    rviz_common::ViewController* view = rviz_manager_->getViewManager()->getCurrent();
    view->subProp("Distance")->setValue(2.0f);

    QVBoxLayout* rviz_layout = new QVBoxLayout();
    rviz_layout->addWidget(rviz_render_panel_.get());
    setLayout(rviz_layout);

    auto btn_layout = new QHBoxLayout();
    rviz_layout->addLayout(btn_layout);

    QCheckBox* btn;
    btn_layout->addWidget(btn = new QCheckBox("visual"), 0);
    btn->setChecked(true);
    connect(btn, &QCheckBox::toggled,
            [this](bool checked) { robot_state_display_->subProp("Visual Enabled")->setValue(checked); });

    btn_layout->addWidget(btn = new QCheckBox("collision"), 1);
    btn->setChecked(false);
    connect(btn, &QCheckBox::toggled,
            [this](bool checked) { robot_state_display_->subProp("Collision Enabled")->setValue(checked); });

    isRobotStateDisplayInitialized_ = true;
  }
  // On subsequent calls, recreate robot_state_display_ to update the model
  else if (initialize_call_count > 2)
  {
    updateFixedFrame();
  }
}

RVizPanel::~RVizPanel()
{
  rviz_manager_.reset();
  rviz_render_panel_.reset();
}

moveit::core::RobotModelPtr RVizPanel::getRobotModel() const
{
  auto urdf = config_data_->get<moveit_setup::URDFConfig>("urdf");

  if (!urdf->isConfigured())
  {
    return nullptr;
  }

  auto srdf = config_data_->get<moveit_setup::SRDFConfig>("srdf");

  return srdf->getRobotModel();
}

void RVizPanel::updateFixedFrame()
{
  auto rm = getRobotModel();
  if (rm && rviz_manager_ && robot_state_display_)
  {
    std::string frame = rm->getModelFrame();
    rviz_manager_->setFixedFrame(QString::fromStdString(frame));
    robot_state_display_->reset();
    robot_state_display_->setVisible(true);
  }
}

void RVizPanel::highlightLinkEvent(const std::string& link_name, const QColor& color)
{
  auto rm = getRobotModel();
  if (!rm)
    return;
  const moveit::core::LinkModel* lm = rm->getLinkModel(link_name);
  if (!lm->getShapes().empty())  // skip links with no geometry
    robot_state_display_->setLinkColor(link_name, color);
}

void RVizPanel::highlightGroupEvent(const std::string& group_name)
{
  auto rm = getRobotModel();
  if (!rm)
    return;
  // Highlight the selected planning group by looping through the links
  if (!rm->hasJointModelGroup(group_name))
    return;

  const moveit::core::JointModelGroup* joint_model_group = rm->getJointModelGroup(group_name);
  if (joint_model_group)
  {
    const std::vector<const moveit::core::LinkModel*>& link_models = joint_model_group->getLinkModels();
    // Iterate through the links
    for (std::vector<const moveit::core::LinkModel*>::const_iterator link_it = link_models.begin();
         link_it < link_models.end(); ++link_it)
      highlightLink((*link_it)->getName(), QColor(255, 0, 0));
  }
}

void RVizPanel::unhighlightAllEvent()
{
  auto rm = getRobotModel();
  if (!rm)
    return;
  // Get the names of the all links robot
  const std::vector<std::string>& links = rm->getLinkModelNamesWithCollisionGeometry();

  // Quit if no links found
  if (links.empty())
  {
    return;
  }

  // check if rviz is ready
  if (!rviz_manager_ || !robot_state_display_)
  {
    return;
  }

  // Iterate through the links
  for (std::vector<std::string>::const_iterator link_it = links.begin(); link_it < links.end(); ++link_it)
  {
    if ((*link_it).empty())
      continue;

    robot_state_display_->unsetLinkColor(*link_it);
  }
}

}  // namespace robot_description::setup_framework
