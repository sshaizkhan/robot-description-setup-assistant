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
#include "robot_description_setup_framework/utilities.hpp"
#include <algorithm>
#include <filesystem>

namespace robot_description::core_plugins
{
void RobotSelectionWidget::onInit()
{
  RCLCPP_INFO(setup_step_.getLogger(), "Initializing Enhanced Robot Selection Widget");
  
  // Initialize the configuration system with multiple config files
  auto& config_manager = RobotConfigManager::getInstance();
  
  // Load multiple configuration files for different robot types
  std::vector<std::string> config_files = {
    "config/robots.yaml"  // Start with single file, can expand to multiple files
    // "config/industrial_robots.yaml",
    // "config/collaborative_robots.yaml", 
    // "config/research_robots.yaml"
  };
  
  if (!config_manager.loadFromPackageConfigs("robot_description_setup_assistant", config_files)) {
    RCLCPP_ERROR(setup_step_.getLogger(), "Failed to load robot configurations");
    // Fall back to empty configuration - UI will show "No robots available"
  }
  // Enable automatic configuration reloading for development
  config_manager.enableAutoReload(true);

  available_robots_ = config_manager.getAllRobots();
  filtered_robots_ = available_robots_;

  RCLCPP_INFO(setup_step_.getLogger(), "Loaded %zu robot configurations", available_robots_.size());
  
  // Setup UI
  setupLayout();
  setupConnections();
  updateRobotDisplay();
}

void RobotSelectionWidget::setupLayout()
{
  // Main horizontal layout
  main_layout_ = new QHBoxLayout(this);
  main_layout_->setContentsMargins(10, 10, 10, 10);
  main_layout_->setSpacing(15);
  
  // Create main splitter for resizable panels
  main_splitter_ = new QSplitter(Qt::Horizontal, this);
  main_splitter_->setChildrenCollapsible(false);
  
  // LEFT PANEL: Filter widget
  filter_widget_ = new FilterWidget();
  filter_widget_->setMinimumWidth(250);
  filter_widget_->setMaximumWidth(300);
  
  // Set categories in filter widget
  auto& config_manager = RobotConfigManager::getInstance();
  filter_widget_->setCategories(config_manager.getCategories());
  
  main_splitter_->addWidget(filter_widget_);
  
  // CENTER PANEL: Robot grid
  robot_grid_widget_ = new QWidget();
  robot_grid_layout_ = new QVBoxLayout(robot_grid_widget_);
  robot_grid_layout_->setContentsMargins(0, 0, 0, 0);
  robot_grid_layout_->setSpacing(10);
  
  // Robot count label
  robot_count_label_ = new QLabel();
  robot_count_label_->setStyleSheet("QLabel { font-weight: bold; color: #666; }");
  robot_grid_layout_->addWidget(robot_count_label_);
  
  // Scroll area for robot grid
  robot_scroll_area_ = new QScrollArea();
  robot_scroll_area_->setWidgetResizable(true);
  robot_scroll_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  robot_scroll_area_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  
  robot_scroll_content_ = new QWidget();
  robot_grid_ = new QGridLayout(robot_scroll_content_);
  robot_grid_->setSpacing(15);
  robot_grid_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  
  robot_scroll_area_->setWidget(robot_scroll_content_);
  robot_grid_layout_->addWidget(robot_scroll_area_);
  
  robot_grid_widget_->setMinimumWidth(400);
  main_splitter_->addWidget(robot_grid_widget_);
  
  // RIGHT PANEL: Robot specifications
  spec_widget_ = new RobotSpecificationWidget();
  spec_widget_->setMinimumWidth(300);
  spec_widget_->setMaximumWidth(400);
  
  main_splitter_->addWidget(spec_widget_);
  
  // Set splitter proportions (20% - 50% - 30%)
  main_splitter_->setStretchFactor(0, 1);  // Filter panel
  main_splitter_->setStretchFactor(1, 2);  // Robot grid
  main_splitter_->setStretchFactor(2, 1);  // Spec panel
  
  main_layout_->addWidget(main_splitter_);
}

void RobotSelectionWidget::setupConnections()
{
  auto& config_manager = RobotConfigManager::getInstance();
  
  // Filter widget connections
  connect(filter_widget_, &FilterWidget::filterChanged, this, &RobotSelectionWidget::onFilterChanged);
  
  // Configuration manager connections
  connect(&config_manager, &RobotConfigManager::configurationsReloaded, this, &RobotSelectionWidget::onConfigurationsReloaded);
  connect(&config_manager, &RobotConfigManager::configurationError, this, &RobotSelectionWidget::onConfigurationError);
  connect(&config_manager, &RobotConfigManager::robotListUpdated, this, &RobotSelectionWidget::onRobotListUpdated);
}

void RobotSelectionWidget::onFilterChanged(const RobotFilter& filter)
{
  RCLCPP_DEBUG(setup_step_.getLogger(), "Applying robot filter");
  
  auto& config_manager = RobotConfigManager::getInstance();
  
  if (filter.isEmpty()) {
    filtered_robots_ = available_robots_;
  } else {
    filtered_robots_ = config_manager.filterRobots(filter);
  }
  
  updateRobotDisplay(filtered_robots_);
}

void RobotSelectionWidget::updateRobotDisplay()
{
  updateRobotDisplay(filtered_robots_);
}

void RobotSelectionWidget::updateRobotDisplay(const std::vector<RobotConfig>& robots)
{
  // Update robot count
  robot_count_label_->setText(QString("Found %1 robot(s)").arg(robots.size()));
  
  // Clear existing robot buttons
  clearRobotGrid();
  
  if (robots.empty()) {
    // Show "no robots" message
    QLabel* no_robots_label = new QLabel("No robots match the current filter criteria.");
    no_robots_label->setAlignment(Qt::AlignCenter);
    no_robots_label->setStyleSheet("QLabel { color: #999; font-size: 14px; margin: 50px; }");
    robot_grid_->addWidget(no_robots_label, 0, 0, 1, ROBOTS_PER_ROW);
    return;
  }
  
  // Create robot buttons
  for (size_t i = 0; i < robots.size(); ++i) {
    createRobotButton(robots[i], static_cast<int>(i));
  }
}

void RobotSelectionWidget::clearRobotGrid()
{
  // Remove all widgets from grid
  QLayoutItem* item;
  while ((item = robot_grid_->takeAt(0)) != nullptr) {
    if (item->widget()) {
      delete item->widget();
    }
    delete item;
  }
}

void RobotSelectionWidget::createRobotButton(const RobotConfig& robot, int index)
{
  // Create container widget for robot button and info
  QWidget* robot_container = new QWidget();
  robot_container->setFixedSize(ROBOT_BUTTON_WIDTH, ROBOT_BUTTON_HEIGHT);
  robot_container->setStyleSheet("QWidget { border: 1px solid #ddd; border-radius: 8px; background: white; }");
  
  QVBoxLayout* container_layout = new QVBoxLayout(robot_container);
  container_layout->setContentsMargins(10, 10, 10, 10);
  container_layout->setSpacing(5);
  
  // Robot image button
  QPushButton* image_button = new QPushButton();
  image_button->setFixedSize(140, 100);
  image_button->setFlat(true);
  
  // Load robot image
  if (std::filesystem::exists(robot.image_path)) {
    QPixmap pixmap(QString::fromStdString(robot.image_path.string()));
    if (!pixmap.isNull()) {
      QPixmap scaled_pixmap = pixmap.scaled(140, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation);
      image_button->setIcon(QIcon(scaled_pixmap));
      image_button->setIconSize(QSize(140, 100));
    } else {
      image_button->setText("No Image");
      image_button->setStyleSheet("QPushButton { border: 1px dashed #999; color: #999; }");
    }
  } else {
    image_button->setText("Image\nNot Found");
    image_button->setStyleSheet("QPushButton { border: 1px dashed #999; color: #999; }");
  }
  
  container_layout->addWidget(image_button, 0, Qt::AlignCenter);
  
  // Robot name label
  QLabel* name_label = new QLabel(QString::fromStdString(robot.display_name));
  name_label->setAlignment(Qt::AlignCenter);
  name_label->setStyleSheet("QLabel { font-weight: bold; font-size: 12px; }");
  name_label->setWordWrap(true);
  container_layout->addWidget(name_label);
  
  // Key specifications summary
  QLabel* specs_label = new QLabel();
  QString specs_text;
  if (robot.specifications.degrees_of_freedom > 0) {
    specs_text += QString("%1 DOF").arg(robot.specifications.degrees_of_freedom);
  }
  if (robot.specifications.payload_kg > 0.0) {
    if (!specs_text.isEmpty()) specs_text += " • ";
    specs_text += QString("%1kg").arg(robot.specifications.payload_kg, 0, 'f', 1);
  }
  if (robot.specifications.collaborative) {
    if (!specs_text.isEmpty()) specs_text += " • ";
    specs_text += "Collaborative";
  }
  
  specs_label->setText(specs_text);
  specs_label->setAlignment(Qt::AlignCenter);
  specs_label->setStyleSheet("QLabel { font-size: 10px; color: #666; }");
  specs_label->setWordWrap(true);
  container_layout->addWidget(specs_label);
  
  // Package validation indicator
  auto& config_manager = RobotConfigManager::getInstance();
  auto missing_packages = config_manager.getMissingPackages(robot);
  
  QLabel* status_label = new QLabel();
  if (missing_packages.empty()) {
    status_label->setText("✅ Ready");
    status_label->setStyleSheet("QLabel { color: #4caf50; font-size: 10px; font-weight: bold; }");
  } else {
    status_label->setText(QString("⚠️ Missing %1 pkg(s)").arg(missing_packages.size()));
    status_label->setStyleSheet("QLabel { color: #ff9800; font-size: 10px; font-weight: bold; }");
  }
  status_label->setAlignment(Qt::AlignCenter);
  container_layout->addWidget(status_label);
  
  // Connect button signals
  connect(image_button, &QPushButton::clicked, this, [this, robot]() {
    onRobotSelected(robot);
  });
  
  // Add hover effects
  robot_container->setAttribute(Qt::WA_Hover, true);
  robot_container->installEventFilter(this);
  
  // Add to grid
  int row = index / ROBOTS_PER_ROW;
  int col = index % ROBOTS_PER_ROW;
  robot_grid_->addWidget(robot_container, row, col);
}

void RobotSelectionWidget::onRobotSelected(const RobotConfig& robot)
{
  RCLCPP_INFO(setup_step_.getLogger(), "Robot selected: %s", robot.display_name.c_str());
  
  selected_robot_ = robot;
  
  // Update specification panel
  spec_widget_->setRobotConfig(robot);
  
  // Check package dependencies before loading
  auto& config_manager = RobotConfigManager::getInstance();
  auto missing_packages = config_manager.getMissingPackages(robot);
  
  if (!missing_packages.empty()) {
    showRobotValidationDialog(missing_packages);
    return;
  }
  
  // Load the robot URDF
  loadDefinedURDFClick(robot);
}

void RobotSelectionWidget::showRobotValidationDialog(const std::vector<std::string>& missing_packages)
{
  QString missing_list;
  for (size_t i = 0; i < missing_packages.size(); ++i) {
    if (i > 0) missing_list += "\n";
    missing_list += "• " + QString::fromStdString(missing_packages[i]);
  }
  
  QMessageBox msg_box(this);
  msg_box.setWindowTitle("Missing Dependencies");
  msg_box.setIcon(QMessageBox::Warning);
  msg_box.setText(QString("The selected robot (%1) requires packages that are not installed:")
                 .arg(QString::fromStdString(selected_robot_.display_name)));
  msg_box.setDetailedText(missing_list);
  msg_box.setInformativeText("Do you want to continue anyway? The robot may not function properly.");
  msg_box.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
  msg_box.setDefaultButton(QMessageBox::No);
  
  QPushButton* install_button = msg_box.addButton("Install Packages", QMessageBox::ActionRole);
  
  int result = msg_box.exec();
  
  if (msg_box.clickedButton() == install_button) {
    QMessageBox::information(this, "Package Installation", 
                           "Package installation feature will be implemented in a future update.\n"
                           "Please install the required packages manually using your package manager.");
  } else if (result == QMessageBox::Yes) {
    loadDefinedURDFClick(selected_robot_);
  }
  // If No or Cancel, do nothing
}

void RobotSelectionWidget::loadDefinedURDFClick(const RobotConfig& robot_config)
{
  RCLCPP_INFO_STREAM(setup_step_.getLogger(), "Loading URDF for robot: " << robot_config.display_name);
  
  bool result = loadDefinedFile(robot_config);
  
  if (result) {
    RCLCPP_INFO_STREAM(setup_step_.getLogger(), "URDF file loaded successfully");
    
    // Show success message
    QMessageBox::information(this, "Robot Loaded", 
                           QString("Successfully loaded %1!\n\nYou can now proceed to the next step.")
                           .arg(QString::fromStdString(robot_config.display_name)));
  } else {
    RCLCPP_ERROR_STREAM(setup_step_.getLogger(), "Failed to load URDF file");
  }
}

bool RobotSelectionWidget::loadDefinedFile(const RobotConfig& robot_config)
{
  // Build URDF path from robot configuration
  std::filesystem::path urdf_path;
  
  try {
    urdf_path = robot_description::getSharePath(robot_config.urdf_package) / robot_config.urdf_path;
  } catch (const std::exception& e) {
    QMessageBox::warning(this, "Error Loading Files", 
                        QString("Failed to locate package '%1': %2")
                        .arg(QString::fromStdString(robot_config.urdf_package))
                        .arg(e.what()));
    return false;
  }
  
  if (urdf_path.empty()) {
    QMessageBox::warning(this, "Error Loading Files", "No robot model file specified");
    return false;
  }
  
  // Check that the URDF file exists
  if (!std::filesystem::is_regular_file(urdf_path)) {
    QMessageBox::warning(this, "Error Loading Files",
                         QString("Unable to locate the URDF file:\n%1\n\n"
                                "Please ensure the robot packages are properly installed.")
                         .arg(QString::fromStdString(urdf_path.string())));
    return false;
  }
  
  try {
    // Load the URDF file with xacro arguments
    setup_step_.loadURDFFile(urdf_path, robot_config.xacro_args);
    
    // Signal that data has been updated
    Q_EMIT dataUpdated();
    
    return true;
    
  } catch (const std::runtime_error& e) {
    QMessageBox::critical(this, "Error Loading URDF", 
                         QString("Failed to load URDF file:\n%1\n\n"
                                "Error details:\n%2")
                         .arg(QString::fromStdString(urdf_path.string()))
                         .arg(e.what()));
    return false;
  }
}

void RobotSelectionWidget::onConfigurationsReloaded()
{
  RCLCPP_INFO(setup_step_.getLogger(), "Robot configurations reloaded - updating UI");
  
  // Reload robot list
  auto& config_manager = RobotConfigManager::getInstance();
  available_robots_ = config_manager.getAllRobots();
  
  // Update filter categories
  filter_widget_->setCategories(config_manager.getCategories());
  
  // Re-apply current filter
  RobotFilter current_filter = filter_widget_->getCurrentFilter();
  if (current_filter.isEmpty()) {
    filtered_robots_ = available_robots_;
  } else {
    filtered_robots_ = config_manager.filterRobots(current_filter);
  }
  
  // Update display
  updateRobotDisplay();
  
  // Clear specification panel if selected robot is no longer available
  bool selected_robot_still_available = false;
  for (const auto& robot : available_robots_) {
    if (robot.id == selected_robot_.id) {
      selected_robot_still_available = true;
      break;
    }
  }
  
  if (!selected_robot_still_available) {
    spec_widget_->clear();
    selected_robot_ = RobotConfig{};
  }
  
  // Show notification to user
  QMessageBox::information(this, "Configuration Updated", 
                          QString("Robot configurations have been reloaded.\n"
                                 "Found %1 robot(s) matching current filters.")
                          .arg(filtered_robots_.size()));
}

void RobotSelectionWidget::onConfigurationError(const QString& error)
{
  RCLCPP_ERROR(setup_step_.getLogger(), "Configuration error: %s", error.toStdString().c_str());
  
  QMessageBox::warning(this, "Configuration Error", 
                      QString("Error reloading robot configurations:\n\n%1\n\n"
                             "The robot list may not be up to date.")
                      .arg(error));
}

void RobotSelectionWidget::onRobotListUpdated()
{
  RCLCPP_INFO(setup_step_.getLogger(), "Robot list updated");
  
  // This is called when the configuration manager detects changes
  // Reload everything to ensure consistency
  onConfigurationsReloaded();
}

}  // namespace robot_description::core_plugins

#include <pluginlib/class_list_macros.hpp>
PLUGINLIB_EXPORT_CLASS(robot_description::core_plugins::RobotSelectionWidget, robot_description::setup_framework::SetupStepWidget)
