/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2012, Shahwaz Khan, Inc.
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

#include "robot_description_core_plugins/robot_selection_widget.hpp"
#include <qboxlayout.h>
#include <qwidget.h>
#include <robot_description_setup_framework/qt/helper_widgets.hpp>
#include <robot_description_setup_framework/utilities.hpp>

namespace robot_description::core_plugins
{
void RobotSelectionWidget::onInit()
{
  QVBoxLayout* main_layout = new QVBoxLayout(this);

  // Top Level Label
  setup_framework::HeaderWidget* header_widget = new setup_framework::HeaderWidget(
      "Robot Selection",
      "Select the robot you would like to configure. This page allows you to select the robot for the robot "
      "description package that cane be coupled with end-effector to create a working robotic arm with tool ",
      this);

  main_layout->addWidget(header_widget);

  scroll_area_ = new QScrollArea(this);
  // set size of scroll area
  scroll_area_->setMinimumSize(480, 350);
  scroll_area_widget_contents_ = new QWidget(scroll_area_);
  scroll_area_grid_layout_ = new QGridLayout(scroll_area_widget_contents_);

  auto image_path = getSharePath("robot_description_setup_assistant") / "resources/icons/rds_logo.png";

  for (int i = 0; i < 10; ++i)
  {  // assuming you want to add 10 images
    QPushButton* image_button = new QPushButton(scroll_area_widget_contents_);
    image_button->setIcon(QIcon(image_path.string().c_str()));  // replace with actual image path
    image_button->setIconSize(QSize(100, 100));                 // set a proper size for the images
    image_button->setFlat(true);
    connect(image_button, SIGNAL(clicked()), this, SLOT(loadDefinedURDFClick()));

    scroll_area_grid_layout_->addWidget(image_button, i / 5, i % 5);  // adjust the grid dimensions as necessary
  }

  scroll_area_widget_contents_->setLayout(scroll_area_grid_layout_);
  // set sizepolicy to set the size of the widget
  scroll_area_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);

  scroll_area_->setWidget(scroll_area_widget_contents_);
  main_layout->addWidget(scroll_area_);

  // Add images to grid layout
  // int row = 0;
  // int col = 0;

  // // Load all robot images
  // for (const std::string& robot_name : robot_names_)
  // {
  //   // Load image
  //   QLabel* image_label = new QLabel(this);
  //   image_label->setPixmap(QPixmap(robot_description_setup_framework::getRobotImage(robot_name).c_str()));
  //   image_label->setScaledContents(true);
  //   image_label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

  //   // Add to grid
  //   scroll_area_grid_layout_->addWidget(image_label, row, col);

  //   // Add button
  //   QPushButton* button = new QPushButton(QString::fromStdString(robot_name), this);
  //   button->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
  //   connect(button, SIGNAL(clicked()), this, SLOT(onButtonClicked()));
  //   scroll_area_grid_layout_->addWidget(button, row + 1, col);

  //   // Increment row and col
  //   col++;
  //   if (col > 2)
  //   {
  //     col = 0;
  //     row += 2;
  //   }
  // }

  // // Set layout
  // scroll_area_widget_contents_->setLayout(scroll_area_grid_layout_);
  // scroll_area_->setWidget(scroll_area_widget_contents_);
  // scroll_area_->setWidgetResizable(true);
  // main_layout->addWidget(scroll_area_);
}

void RobotSelectionWidget::onChooseRobotButtonClicked()
{
  RCLCPP_INFO_STREAM(setup_step_.getLogger(), "RobotSelectionWidget::onChooseRobotButtonClicked()");
  // Set QMessage Box
  QMessageBox::information(this, "Robot Selection", "UR5");
}

void RobotSelectionWidget::loadDefinedURDFClick()
{
  RCLCPP_INFO_STREAM(setup_step_.getLogger(), "Loading defined URDF file");

  auto result = loadDefinedFile();

  if (result)
  {
    RCLCPP_INFO_STREAM(setup_step_.getLogger(), "URDF file loaded successfully");
  }
  else
  {
    RCLCPP_ERROR_STREAM(setup_step_.getLogger(), "Failed to load URDF file");
  }
}

bool RobotSelectionWidget::loadDefinedFile()
{
  // define hardcoded path to URDF file
  std::filesystem::path urdf_path = getSharePath("moveit_resources_ur_description") / "urdf/model.urdf";

  if (urdf_path.empty())
  {
    QMessageBox::warning(this, "Error Loading Files", "No robot model file specified");
    return false;
  }

  // Check that this file exits
  if (!std::filesystem::is_regular_file(urdf_path))
  {
    QMessageBox::warning(this, "Error Loading Files",
                         QString("Unable to locate the URDF file: ").append(urdf_path.c_str()));
    return false;
  }
  try
  {
    setup_step_.loadURDFFile(urdf_path, "");
  }
  catch (const std::runtime_error& e)
  {
    QMessageBox::warning(this, "Error Loading URDF", QString(e.what()));
    return false;
  }

  Q_EMIT dataUpdated();

  RCLCPP_INFO(setup_step_.getLogger(), "Loading Setup Assistant Complete");
  return true;  // success!
}

RobotSelectionWidget::~RobotSelectionWidget()
{
  delete scroll_area_grid_layout_;
  delete scroll_area_widget_contents_;
  delete scroll_area_;
}

}  // namespace robot_description::core_plugins

#include <pluginlib/class_list_macros.hpp>
PLUGINLIB_EXPORT_CLASS(robot_description::core_plugins::RobotSelectionWidget,
                       robot_description::setup_framework::SetupStepWidget)
