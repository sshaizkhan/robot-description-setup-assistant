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

#include "robot_description_setup_assistant/setup_assistant_widget.hpp"
#include <mutex>

namespace robot_description::setup_assistant
{

SetupRobotDescriptionAssistantWidget::SetupRobotDescriptionAssistantWidget(
    const rviz_common::ros_integration::RosNodeAbstractionIface::WeakPtr& node, QWidget* parent,
    const boost::program_options::variables_map& /*args*/)
  : QWidget(parent), node_abstraction_(node), node_(node_abstraction_.lock()->get_raw_node())
{
  // Setting the window icon
  auto icon_path = getSharePath("robot_description_setup_assistant") / "resources/icons/rds_logo.png";
  this->setWindowIcon(QIcon(icon_path.c_str()));

  // Basic widget container
  QHBoxLayout* box_layout = new QHBoxLayout(this);
  box_layout->setAlignment(Qt::AlignTop);

  // Create main content stack for various screens
  main_content_ = new QStackedWidget(this);
  current_index_ = -1;

  rviz_panel_ = new robot_description::setup_framework::RVizPanel(this, node_abstraction_);
  main_content_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  rviz_panel_->hide();

  start_screen_ = new core_plugins::StartScreenWidget(this);
  const std::string start_screen_name = start_screen_->getName();
  main_content_->addWidget(start_screen_);

  nav_name_list_ << start_screen_name.c_str();
  nav_name_list_.push_back("Arm Selection");
  nav_name_list_.push_back("End-Effector Tool");
  nav_name_list_.push_back("Base (Optional)");
  nav_name_list_.push_back("Author Info");

  // nav_name_list_ = { "Home", "Arm Selection", "End-Effector Tool", "Base (Optional)", "Author Info" };

  // Nactigation Left Pane ------------------------------------------------
  navs_view_ = new NavigationWidget(this);
  navs_view_->setNavs(nav_name_list_);

  navs_view_->setEnabled(0, true);
  moveToScreen(0);

  // Split screen
  splitter_ = new QSplitter(Qt::Horizontal, this);
  splitter_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  splitter_->addWidget(navs_view_);
  splitter_->addWidget(main_content_);
  splitter_->addWidget(rviz_panel_);
  splitter_->setHandleWidth(6);

  box_layout->addWidget(splitter_);

  // Add event for switching between screens -------------------------
  connect(navs_view_, SIGNAL(clicked(const QModelIndex&)), this, SLOT(navigationClicked(const QModelIndex&)));

  // Final layout setup
  this->setLayout(box_layout);

  // Title
  this->setWindowTitle("Robot Description Setup Assistant");

  // Show screen before message
  QApplication::processEvents();
}

void SetupRobotDescriptionAssistantWidget::onAdvanceRequest()
{
  // TODO: Figure out if this is necessary
}

void SetupRobotDescriptionAssistantWidget::onDataUpdate()
{
  // TODO: Figure out if this is necessary
}

void SetupRobotDescriptionAssistantWidget::navigationClicked(const QModelIndex& index)
{
  int row = index.row();
  moveToScreen(row);
}

void SetupRobotDescriptionAssistantWidget::moveToScreen(int index)
{
  std::scoped_lock lock(change_screen_lock_);
  if (!navs_view_->isEnabled(index))
  {
    return;
  }

  // if(current_index_ != index)
  // {
  //   if (current_index_ >= 0)
  //   {

  //   }
  // }
  current_index_ = index;
  main_content_->setCurrentIndex(index);
  navs_view_->setSelected(index);
}

// ******************************************************************************************
// Ping ROS on interval
// ******************************************************************************************
void SetupRobotDescriptionAssistantWidget::updateTimer()
{
  // TODO: Figure out if there's a ROS 2 equivalent of this that needs to be run
  // ros::spinOnce();  // keep ROS node alive
}

void SetupRobotDescriptionAssistantWidget::closeEvent(QCloseEvent* event)
{
  // Only prompt to close if not in debug mode
  {
    if (QMessageBox::question(this, "Exit Setup Assistant",
                              QString("Are you sure you want to exit the Robot Description Setup Assistant?"),
                              QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel)
    {
      event->ignore();
      return;
    }
  }

  // Shutdown app
  event->accept();
}

// ******************************************************************************************
// Qt Error Handling - TODO
// ******************************************************************************************
bool SetupRobotDescriptionAssistantWidget::notify(QObject* /*receiver*/, QEvent* /*event*/)
{
  QMessageBox::critical(this, "Error", "An error occurred and was caught by Qt notify event handler.", QMessageBox::Ok);

  return false;
}

void SetupRobotDescriptionAssistantWidget::onModalModeUpdate(bool /*isModal*/)
{
  // TODO: Figure out if this is necessary
}

}  // namespace robot_description::setup_assistant
