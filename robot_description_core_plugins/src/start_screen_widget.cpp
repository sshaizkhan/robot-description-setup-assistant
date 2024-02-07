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

#include "robot_description_core_plugins/start_screen_widget.hpp"
#include "robot_description_setup_framework/utilities.hpp"

namespace robot_description::core_plugins
{

void StartScreenWidget::onInit()
{
  // Basic widget container
  QVBoxLayout* layout = new QVBoxLayout(this);

  // Horizontal layout splitter
  QHBoxLayout* hlayout = new QHBoxLayout();

  // Left side of screen
  QVBoxLayout* left_layout = new QVBoxLayout();

  // Right side of screen
  QVBoxLayout* right_layout = new QVBoxLayout();

  // Right Image Area

  right_image_ = new QImage();
  right_image_label_ = new QLabel(this);

  auto image_path =
      getSharePath("robot_description_setup_assistant") / "resources/graphics/robot_description_setup_assistant.png";

  if (right_image_->load(image_path.string().c_str()))
  {
    right_image_label_->setPixmap(QPixmap::fromImage(*right_image_));
    right_image_label_->setMinimumHeight(384);
  }
  else
  {
    right_image_label_->setText("Error: Unable to load image");
    RCLCPP_ERROR_STREAM(node_->get_logger(), "FAILED TO LOAD " << image_path);
  }

  right_layout->addWidget(right_image_label_);
  right_layout->setAlignment(right_image_label_, Qt::AlignRight | Qt::AlignTop);

  // Top Label Area
  setup_framework::HeaderWidget* header_widget =
      new setup_framework::HeaderWidget("Robot Description Setup Assistant",
                                        "These tools will assist you in creating a Robot Description Package with wide "
                                        "selection of Robotic Arm and end-effector tools. "
                                        "The generated package can then later be used with MoveIt Setup Assistant to "
                                        "generate a MoveIt Configuration Package.",
                                        this);
  layout->addWidget(header_widget);

  // Select Mode Area
  select_mode_widget_ = new SelectModeWidget(this);
  connect(select_mode_widget_->btn_new_, SIGNAL(clicked()), this, SLOT(showNewOptions()));
  connect(select_mode_widget_->btn_existing_, SIGNAL(clicked()), this, SLOT(showExistingOptions()));
  left_layout->addWidget(select_mode_widget_);

  // Path Box Area
  stack_path_ = new setup_framework::LoadPathArgsWidget(
      "Load Existing Robot Description Setup Package",
      "Specify the package name or path of an existing MoveIt configuration package "
      "to be edited for your robot. Example package name: <i>panda_moveit_config</i>",
      "optional xacro arguments:", this, true);
  stack_path_->hide();
  stack_path_->setArguments("");
  connect(stack_path_, SIGNAL(pathChanged(const QString&)), this, SLOT(onPackagePathChanged(const QString&)));
  left_layout->addWidget(stack_path_);

  // urdf File Dialog
  urdf_file_ = new setup_framework::LoadPathArgsWidget(
      "Load URDF File",
      "Specify the path to the URDF file for your robot. This file is typically named <i>robot_description.urdf</i>.",
      "optional xacro arguments:", this, false, true);
  urdf_file_->hide();
  urdf_file_->setArguments("");
  connect(urdf_file_, SIGNAL(pathChanged(const QString&)), this, SLOT(onUrdfPathChanged(const QString&)));
  left_layout->addWidget(urdf_file_);

  // Load setting box
  QHBoxLayout* load_files_layout = new QHBoxLayout();

  // progress bar
  progess_bar_ = new QProgressBar(this);
  progess_bar_->setMaximum(100);
  progess_bar_->setMinimum(0);
  progess_bar_->hide();
  load_files_layout->addWidget(progess_bar_);

  // Load button
  btn_load_ = new QPushButton("Load Package", this);
  btn_load_->setMinimumWidth(180);
  btn_load_->setMinimumHeight(40);
  btn_load_->hide();
  load_files_layout->addWidget(btn_load_);
  load_files_layout->setAlignment(btn_load_, Qt::AlignRight);
  connect(btn_load_, SIGNAL(clicked()), this, SLOT(loadFilesClicked()));

  // Next step instructions
  next_label_ = new QLabel(this);
  QFont next_label_font(QFont().defaultFamily(), 11, QFont::Bold);
  next_label_->setFont(next_label_font);
  next_label_->setText("Success! Now you can proceed to the next step.");
  next_label_->hide();

  // Final Layout
  layout->setAlignment(Qt::AlignTop);
  hlayout->setAlignment(Qt::AlignTop);
  left_layout->setAlignment(Qt::AlignTop);
  right_layout->setAlignment(Qt::AlignTop);

  // Stretch
  left_layout->setSpacing(10);

  // Attach Layouts
  hlayout->addLayout(left_layout);
  hlayout->addLayout(right_layout);
  layout->addLayout(hlayout);

  // Vertical Spacer
  layout->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));

  // Attach bottom layout
  layout->addWidget(next_label_);
  layout->setAlignment(next_label_, Qt::AlignRight);
  layout->addLayout(load_files_layout);

  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  this->setLayout(layout);
}

StartScreenWidget::~StartScreenWidget()
{
  delete right_image_;  // does not have a parent passed to it
}

void StartScreenWidget::focusGiven()
{
  // Set focus on the package path
  stack_path_->setFocus();
}

void StartScreenWidget::showNewOptions()
{
  QMessageBox::information(this, "New Package", "New Package Selected");
  RCLCPP_INFO(node_->get_logger(), "New Package Selected");
}

void StartScreenWidget::showExistingOptions()
{
  QMessageBox::information(this, "Existing Package", "Existing Package Selected");
  RCLCPP_INFO(node_->get_logger(), "Existing Package Selected");
}

void StartScreenWidget::onPackagePathChanged(const QString& package_path)
{
  QMessageBox::information(this, "Package Path", "Package Path Changed");
  RCLCPP_INFO(node_->get_logger(), "Package Path Changed: %s", package_path.toStdString().c_str());
}

void StartScreenWidget::onUrdfPathChanged(const QString& urdf_path)
{
  QMessageBox::information(this, "URDF Path", "URDF Path Changed");
  RCLCPP_INFO(node_->get_logger(), "URDF Path Changed: %s", urdf_path.toStdString().c_str());
}

void StartScreenWidget::loadFilesClicked()
{
  QMessageBox::information(this, "Load Files", "Load Files Clicked");
  RCLCPP_INFO(node_->get_logger(), "Load Files Clicked");
}

bool StartScreenWidget::loadPackageSettings(bool show_warning)
{
  if (show_warning)
  {
    QMessageBox::warning(this, "Load Package", "Load Package Settings");
    return false;
  }
  return true;
}

bool StartScreenWidget::loadNewFiles()
{
  QMessageBox::information(this, "Load New Files", "Load New Files");
  RCLCPP_INFO(node_->get_logger(), "Load New Files");
  return true;
}

bool StartScreenWidget::loadExistingFiles()
{
  QMessageBox::information(this, "Load Existing Files", "Load Existing Files");
  RCLCPP_INFO(node_->get_logger(), "Load Existing Files");
  return true;
}

SelectModeWidget::SelectModeWidget(QWidget* parent) : QFrame(parent)
{
  // Set frame graphics
  setFrameShape(QFrame::StyledPanel);
  setFrameShadow(QFrame::Raised);
  setLineWidth(1);
  setMidLineWidth(0);

  // Basic widget container
  QVBoxLayout* layout = new QVBoxLayout(this);

  // Horizontal layout splitter
  QHBoxLayout* hlayout = new QHBoxLayout();

  // Widget Title
  QLabel* widget_title = new QLabel(this);
  widget_title->setText("Create new or edit existing?");
  QFont widget_title_font(QFont().defaultFamily(), 12, QFont::Bold);
  widget_title->setFont(widget_title_font);
  layout->addWidget(widget_title);
  layout->setAlignment(widget_title, Qt::AlignTop);

  // Widget Instructions
  widget_instructions_ = new QLabel(this);
  widget_instructions_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  widget_instructions_->setWordWrap(true);
  widget_instructions_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  widget_instructions_->setText(
      "All settings for MoveIt are stored in the MoveIt configuration package. Here you have the option to create a "
      "new configuration package or load an existing one. Note: changes to a MoveIt configuration package outside this "
      "Setup Assistant are likely to be overwritten by this tool.");

  layout->addWidget(widget_instructions_);
  layout->setAlignment(widget_instructions_, Qt::AlignTop);

  // New Button
  btn_new_ = new QPushButton(this);
  btn_new_->setText("Create &New \nRobot Description Package");
  hlayout->addWidget(btn_new_);

  // Exist Button
  btn_existing_ = new QPushButton(this);
  btn_existing_->setText("&Edit Existing \nRobot Description Package");
  btn_existing_->setCheckable(true);
  hlayout->addWidget(btn_existing_);

  // Add horizontal layer to vertical layer
  layout->addLayout(hlayout);
  setLayout(layout);
  btn_new_->setCheckable(true);
}

}  // namespace robot_description::core_plugins

#include <pluginlib/class_list_macros.hpp>  // NOLINT
PLUGINLIB_EXPORT_CLASS(robot_description::core_plugins::StartScreenWidget,
                       robot_description::setup_framework::SetupStepWidget)
