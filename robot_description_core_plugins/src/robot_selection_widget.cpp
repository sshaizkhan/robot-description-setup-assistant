/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2023, Shahwaz Khan
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
/* Modified from original code by Dave Coleman */

#include "robot_description_core_plugins/robot_selection_widget.hpp"
#include "robot_description_core_plugins/robot_config_manager.hpp"
#include <filesystem>

namespace robot_description::core_plugins
{
void RobotSelectionWidget::onInit()
{
  RobotConfigManager& config_manager = RobotConfigManager::getInstance();
  if (!config_manager.loadFromPackage("robot_description_setup_assistant")) {
    RCLCPP_ERROR(setup_step_.getLogger(), "Failed to load robot configurations");
    return;
  }
  available_robots_ = config_manager.getAllRobots();

  QVBoxLayout* main_layout = new QVBoxLayout(this);

  search_label_ = new QLabel("Search for the Arm:", this);

  search_bar_ = new QLineEdit(this);
  search_bar_->setMaximumSize(720, 30);
  search_bar_->setMinimumSize(720, 30);
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
  scroll_area_->setMaximumSize(720, 500);
  scroll_area_->setStyleSheet("QScrollArea {"
                              "  border: 2px solid gray;"
                              "  border-radius: 10px;"
                              "  padding: 0 8px;"
                              "  selection-background-color: darkgray;"
                              "}");

  scroll_area_widget_contents_ = new QWidget(scroll_area_);
  scroll_area_grid_layout_ = new QGridLayout(scroll_area_widget_contents_);

  addRobotSelectionButtons();

  scroll_area_widget_contents_->setLayout(scroll_area_grid_layout_);
  scroll_area_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  scroll_area_->setWidget(scroll_area_widget_contents_);
  main_layout->addWidget(scroll_area_);
}

void RobotSelectionWidget::setupHeaderWidget(QVBoxLayout* layout)
{
  setup_framework::HeaderWidget * header_widget = new setup_framework::HeaderWidget(
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

void RobotSelectionWidget::addRobotSelectionButtons()
{
  const int desired_width = 150;
  const int desired_height = 150;

  int index = 0;
  for (const RobotConfig& robot_config : available_robots_) {
    QPushButton* image_button = new QPushButton(scroll_area_widget_contents_);
    QLabel* robot_label = new QLabel(scroll_area_widget_contents_);

    // Load image from config
    QPixmap pixmap(QString::fromStdString(robot_config.image_path.string()));
    if (pixmap.isNull()) {
      RCLCPP_WARN(setup_step_.getLogger(), "Failed to load image: %s", 
                  robot_config.image_path.c_str());
      // Use a default placeholder image
      pixmap = QPixmap(desired_width, desired_height);
      pixmap.fill(Qt::lightGray);
    }

    QPixmap scaled_pixmap = pixmap.scaled(desired_width, desired_height, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    image_button->setIcon(QIcon(scaled_pixmap));
    image_button->setIconSize(QSize(desired_width, desired_height));
    image_button->setFlat(true);

    robot_elements_[robot_config.id] = RobotButton{ image_button, robot_label };

    // Setup label
    QFont font = robot_label->font();
    font.setBold(true);
    font.setPointSize(font.pointSize() + 2);
    robot_label->setFont(font);
    robot_label->setText(QString::fromStdString(robot_config.display_name));
    robot_label->setFixedSize(60, 30);
    robot_label->setAlignment(Qt::AlignCenter);
    robot_label->setStyleSheet("QLabel { border: 2px solid black; padding: 2px; }");

    connect(image_button, &QPushButton::clicked, this, [this, robot_config]() {loadDefinedURDFClick(robot_config);});

    // Layout in grid
    int row = index / 4;
    int column = index % 4;

    QVBoxLayout* button_layout = new QVBoxLayout();
    button_layout->addWidget(image_button);
    button_layout->addWidget(robot_label, 0, Qt::AlignHCenter);

    scroll_area_grid_layout_->addLayout(button_layout, row, column);
    index++;
  }
}

void RobotSelectionWidget::filterRobotSelection(const QString& text)
{
  for (std::map<std::string, RobotButton>::iterator it = robot_elements_.begin(); it != robot_elements_.end(); ++it)
  {
    // Check if the model name contains the text from the search bar
    bool is_match = QString::fromStdString(it->first).contains(text, Qt::CaseInsensitive);
    it->second.button->setVisible(is_match);  // Show/Hide the button
    it->second.label->setVisible(is_match);   // Show/Hide the label
  }
}

void RobotSelectionWidget::onChooseRobotButtonClicked()
{
  RCLCPP_INFO_STREAM(setup_step_.getLogger(), "RobotSelectionWidget::onChooseRobotButtonClicked()");
  // Set QMessage Box
  QMessageBox::information(this, "Robot Selection", "UR5");
}

void RobotSelectionWidget::loadDefinedURDFClick(const RobotConfig& robot_config)
{
  RCLCPP_INFO_STREAM(setup_step_.getLogger(), "Loading URDF for robot: " << robot_config.display_name);
  
  auto result = loadDefinedFile(robot_config);
  
  if (result) {
    RCLCPP_INFO_STREAM(setup_step_.getLogger(), "URDF file loaded successfully");
  } else {
    RCLCPP_ERROR_STREAM(setup_step_.getLogger(), "Failed to load URDF file");
  }
}

bool RobotSelectionWidget::loadDefinedFile(const RobotConfig& robot_config)
{
  // Build URDF path from config
  std::filesystem::path urdf_path = robot_description::getSharePath(robot_config.urdf_package) / robot_config.urdf_path;

  if (urdf_path.empty()) {
    QMessageBox::warning(this, "Error Loading Files", "No robot model file specified");
    return false;
  }

  if (!std::filesystem::is_regular_file(urdf_path)) {
    QMessageBox::warning(this, "Error Loading Files",
                         QString("Unable to locate the URDF file: ").append(urdf_path.c_str()));
    return false;
  }
  
  try {
    setup_step_.loadURDFFile(urdf_path, robot_config.xacro_args);
  } catch (const std::runtime_error& e) {
    QMessageBox::warning(this, "Error Loading URDF", QString(e.what()));
    return false;
  }

  Q_EMIT dataUpdated();
  return true;
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
