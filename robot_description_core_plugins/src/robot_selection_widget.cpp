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

namespace robot_description::core_plugins
{
void RobotSelectionWidget::onInit()
{
  QVBoxLayout* main_layout = new QVBoxLayout(this);

  search_label_ = new QLabel("Search for the Arm:", this);

  search_bar_ = new QLineEdit(this);
  search_bar_->setMaximumSize(700, 30);
  search_bar_->setMinimumSize(500, 30);
  search_bar_->setStyleSheet("QLineEdit {"
                             "  border: 2px solid gray;"
                             "  border-radius: 10px;"
                             "  padding: 0 8px;"
                             "  background: white;"
                             "  selection-background-color: darkgray;"
                             "}");

  connect(search_bar_, &QLineEdit::textChanged, this, &RobotSelectionWidget::filterRobotSelection);

  setupHeaderWidget(main_layout);

  main_layout->addWidget(search_label_);
  main_layout->addWidget(search_bar_);

  scroll_area_ = new QScrollArea(this);
  // set size of scroll area
  scroll_area_->setMaximumSize(700, 500);
  scroll_area_->setStyleSheet(
      // Vertical ScrollBar Style
      "QScrollBar:vertical {"
      "  border: 1px solid #999999;"
      "  background:white;"
      "  width:10px;"
      "  margin: 0px 0px 0px 0px;"
      "}"
      "QScrollBar::handle:vertical {"
      "  min-height: 0px;"
      "  border: 1px solid gray;"
      "  border-radius: 4px;"
      "  background-color: lightgray;"
      "}"
      "QScrollBar::add-line:vertical {"
      "  height: 0px;"
      "}"
      "QScrollBar::sub-line:vertical {"
      "  height: 0px;"
      "}"

      // Horizontal ScrollBar Style
      "QScrollBar:horizontal {"
      "  border: 1px solid #999999;"
      "  background:white;"
      "  height:10px;"
      "  margin: 0px 0px 0px 0px;"
      "}"
      "QScrollBar::handle:horizontal {"
      "  min-width: 0px;"
      "  border: 1px solid gray;"
      "  border-radius: 4px;"
      "  background-color: lightgray;"
      "}"
      "QScrollBar::add-line:horizontal {"
      "  width: 0px;"
      "}"
      "QScrollBar::sub-line:horizontal {"
      "  width: 0px;"
      "}");

  scroll_area_widget_contents_ = new QWidget(scroll_area_);
  scroll_area_grid_layout_ = new QGridLayout(scroll_area_widget_contents_);

  const std::array<std::filesystem::path, 9> image_paths = { getRobotImagePath("ur3"),   getRobotImagePath("ur3e"),
                                                             getRobotImagePath("ur5"),   getRobotImagePath("ur5e"),
                                                             getRobotImagePath("ur10"),  getRobotImagePath("ur10e"),
                                                             getRobotImagePath("ur16e"), getRobotImagePath("ur20"),
                                                             getRobotImagePath("ur30") };

  addRobotSelectionButtons(image_paths);

  scroll_area_widget_contents_->setLayout(scroll_area_grid_layout_);
  scroll_area_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  scroll_area_->setWidget(scroll_area_widget_contents_);
  main_layout->addWidget(scroll_area_);
}

void RobotSelectionWidget::setupHeaderWidget(QVBoxLayout* layout)
{
  auto header_widget = new setup_framework::HeaderWidget(
      "Robot Selection",
      "Select the robot you would like to configure. This page allows you to select the robot for the robot "
      "description package that can be coupled with end-effector to create a working robotic arm with tool",
      this);
  layout->addWidget(header_widget);
}

std::filesystem::path RobotSelectionWidget::getRobotImagePath(const std::string& robotName)
{
  return getSharePath("robot_description_setup_assistant") / "resources/graphics/universal_robots" /
         (robotName + ".png");
}

void RobotSelectionWidget::addRobotSelectionButtons(const std::array<std::filesystem::path, 9>& image_paths)
{
  const int desired_width = 150;
  const int desired_height = 150;

  for (const auto& path : image_paths)
  {
    {
      QPushButton* image_button = new QPushButton(scroll_area_widget_contents_);
      QLabel* robot_label = new QLabel(scroll_area_widget_contents_);

      QPixmap pixmap(QString::fromStdString(path.string()));
      QPixmap scaled_pixmap =
          pixmap.scaled(desired_width, desired_height, Qt::KeepAspectRatio, Qt::SmoothTransformation);

      std::string model_name = path.stem().string();

      image_button->setIcon(QIcon(scaled_pixmap));
      image_button->setIconSize(QSize(desired_width, desired_height));
      image_button->setFlat(true);

      robot_elements_[model_name] = RobotButton{ image_button, robot_label };

      // Font settings
      QFont font = robot_label->font();
      font.setBold(true);
      font.setPointSize(font.pointSize() + 2);  // Increase font size
      robot_label->setFont(font);

      // Label text and style
      robot_label->setText(QString::fromStdString(model_name));
      robot_label->setFixedSize(60, 30);                                                // Fixed height (30 pixels)
      robot_label->setAlignment(Qt::AlignCenter);                                       // Center alignment
      robot_label->setStyleSheet("QLabel { border: 2px solid black; padding: 2px; }");  // Black border box

      connect(image_button, &QPushButton::clicked, this, [this, model_name]() {
        QString args = robot_args_[model_name];
        loadDefinedURDFClick(args);
      });

      int index = std::distance(image_paths.begin(), std::find(image_paths.begin(), image_paths.end(), path));
      int row = index / 5;
      int column = index % 5;

      QVBoxLayout* button_layout = new QVBoxLayout();
      button_layout->addWidget(image_button);
      button_layout->addWidget(robot_label, 0, Qt::AlignHCenter);  // Centered with respect to the button

      scroll_area_grid_layout_->addLayout(button_layout, row, column);
    }
  }
}

void RobotSelectionWidget::filterRobotSelection(const QString& text)
{
  for (auto& [model_name, robot_element] : robot_elements_)
  {
    // Check if the model name contains the text from the search bar
    bool is_match = QString::fromStdString(model_name).contains(text, Qt::CaseInsensitive);
    robot_element.button->setVisible(is_match);  // Show/Hide the button
    robot_element.label->setVisible(is_match);   // Show/Hide the label
  }
}

void RobotSelectionWidget::onChooseRobotButtonClicked()
{
  RCLCPP_INFO_STREAM(setup_step_.getLogger(), "RobotSelectionWidget::onChooseRobotButtonClicked()");
  // Set QMessage Box
  QMessageBox::information(this, "Robot Selection", "UR5");
}

void RobotSelectionWidget::loadDefinedURDFClick(const QString& xacro_args)
{
  RCLCPP_INFO_STREAM(setup_step_.getLogger(), "Loading defined URDF file");

  auto result = loadDefinedFile(xacro_args);

  if (result)
  {
    RCLCPP_INFO_STREAM(setup_step_.getLogger(), "URDF file loaded successfully");
  }
  else
  {
    RCLCPP_ERROR_STREAM(setup_step_.getLogger(), "Failed to load URDF file");
  }
}

bool RobotSelectionWidget::loadDefinedFile(const QString& xacro_args)
{
  // define hardcoded path to URDF file
  std::filesystem::path urdf_path = getSharePath("ur_description") / "urdf/ur.urdf.xacro";

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
    setup_step_.loadURDFFile(urdf_path, xacro_args.toStdString());
  }
  catch (const std::runtime_error& e)
  {
    QMessageBox::warning(this, "Error Loading URDF", QString(e.what()));
    return false;
  }

  Q_EMIT dataUpdated();

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
