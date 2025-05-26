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
/*Author: Shahwaz Khan*/
/*Modified from the origianl code by Dave Coleman*/

#include "robot_description_setup_assistant/setup_assistant_widget.hpp"
#include <memory>
#include <robot_description_setup_framework/qt/setup_step_widget.hpp>

namespace robot_description::setup_assistant
{

SetupRobotDescriptionAssistantWidget::SetupRobotDescriptionAssistantWidget(
    const rviz_common::ros_integration::RosNodeAbstractionIface::WeakPtr& node, QWidget* parent,
    const boost::program_options::variables_map& args)
  : QWidget(parent)
  , node_abstraction_(node)
  , node_(node_abstraction_.lock()->get_raw_node())
  , widget_loader_("robot_description_setup_framework", "robot_description::setup_framework::SetupStepWidget")
{
  config_data_ = std::make_shared<moveit_setup::DataWarehouse>(node_);

  if (args.count("debug"))
    config_data_->debug = true;

  // Setting the window icon
  std::filesystem::path icon_path = getSharePath("robot_description_setup_assistant") / "resources/icons/rds_logo.png";
  this->setWindowIcon(QIcon(icon_path.c_str()));

  // Basic widget container
  QHBoxLayout* box_layout = new QHBoxLayout(this);
  box_layout->setAlignment(Qt::AlignTop);

  // Create main content stack for various screens
  main_content_ = new QStackedWidget(this);
  current_index_ = -1;

  // Setup Steps --------------------------------------------------------
  std::vector<std::string> setup_steps;
  setup_steps.push_back("robot_description::core_plugins::StartScreenWidget");
  setup_steps.push_back("robot_description::core_plugins::RobotSelectionWidget");
  // setup_steps = node_->get_parameter("setup_steps").as_string_array();

  rviz_panel_ = new robot_description::setup_framework::RVizPanel(this, node_abstraction_, config_data_);
  rviz_panel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  rviz_panel_->hide();

  for (const std::string& setup_step : setup_steps)
  {
    std::shared_ptr<robot_description::setup_framework::SetupStepWidget> widget = widget_loader_.createSharedInstance(setup_step);
    widget->initialize(node_, this, rviz_panel_, config_data_);

    connect(widget.get(), SIGNAL(dataUpdated()), this, SLOT(onDataUpdate()));
    connect(widget.get(), SIGNAL(advanceRequest()), this, SLOT(onAdvanceRequest()));
    connect(widget.get(), SIGNAL(setModalMode(bool)), this, SLOT(onModalModeUpdate(bool)));

    const std::string name = widget->getSetupStep().getName();
    steps_.push_back(widget);

    main_content_->addWidget(widget.get());
    nav_name_list_ << name.c_str();
  }

  // nav_name_list_.push_back("Arm Selection");
  nav_name_list_.push_back("End-Effector Tool");
  nav_name_list_.push_back("Base (Optional)");
  nav_name_list_.push_back("Author Info");

  // Pass command arg values to start screen and show appropriate part of screen
  if (args.count("urdf_path"))
  {
    config_data_->preloadWithURDFPath(args["urdf_path"].as<std::filesystem::path>());
  }
  if (args.count("config_pkg"))
  {
    config_data_->preloadWithFullConfig(args["config_pkg"].as<std::string>());
  }

  // nav_name_list_ = { "Home", "Arm Selection", "End-Effector Tool", "Base (Optional)", "Author Info" };

  // Nactigation Left Pane ------------------------------------------------
  navs_view_ = new NavigationWidget(this);
  navs_view_->setNavs(nav_name_list_);

  if (!steps_.empty())
  {
    navs_view_->setEnabled(0, true);
    moveToScreen(0);
  }

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
  if (static_cast<unsigned int>(current_index_ + 1) < steps_.size())
  {
    moveToScreen(current_index_ + 1);
  }
}

void SetupRobotDescriptionAssistantWidget::onDataUpdate()
{
  for (size_t index = 0; index < steps_.size(); index++)
  {
    bool ready = steps_[index]->isReady();
    navs_view_->setEnabled(index, ready);
  }

  if (rviz_panel_->isReadyForInitialization())
  {
    RCLCPP_INFO(node_->get_logger(), "RViz panel is ready for initialization");
    rviz_panel_->initialize();
    // Replace logo with Rviz screen
    // rviz_panel_->show();
  }
  else if (rviz_panel_->isRobotModelLoaded())
  {
    RCLCPP_INFO(node_->get_logger(), "Robot model is loaded. Updating fixed frame if new model is loaded.");
    rviz_panel_->updateFixedFrame();
  }
}

void SetupRobotDescriptionAssistantWidget::navigationClicked(const QModelIndex& index)
{
  int row = index.row();
  moveToScreen(row);
}

void SetupRobotDescriptionAssistantWidget::moveToScreen(const int index)
{
  std::scoped_lock slock(change_screen_lock_);
  if (!navs_view_->isEnabled(index))
  {
    return;
  }

  if (current_index_ != index)
  {
    // Send the focus lost command to the screen widget
    if (current_index_ >= 0)
    {
      std::shared_ptr<setup_framework::SetupStepWidget> ssw = steps_[current_index_];
      if (!ssw->focusLost())
      {
        navs_view_->setSelected(current_index_);
        return;  // switching not accepted
      }
    }

    current_index_ = index;

    // Unhighlight anything on robot
    rviz_panel_->unhighlightAll();

    // Change screens
    main_content_->setCurrentIndex(index);

    // Send the focus given command to the screen widget
    steps_[current_index_]->focusGiven();

    // Change navigation selected option
    navs_view_->setSelected(index);
  }
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

void SetupRobotDescriptionAssistantWidget::onModalModeUpdate(bool isModal)
{
  navs_view_->setDisabled(isModal);

  for (int i = 0; i < nav_name_list_.count(); ++i)
  {
    navs_view_->setEnabled(i, !isModal && steps_[i]->isReady());
  }
}

}  // namespace robot_description::setup_assistant
