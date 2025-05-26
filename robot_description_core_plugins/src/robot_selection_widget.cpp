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

namespace robot_description::core_plugins
{
void RobotSelectionWidget::onInit()
{
  RCLCPP_INFO(setup_step_.getLogger(), "Initializing Robot Selection Widget");
  
  // Initialize the configuration system with multiple config files
  auto& config_manager = RobotConfigManager::getInstance();
  
  // Load multiple configuration files for different robot types
  std::vector<std::string> config_files = {"config/robots.yaml"};
  
  if (!config_manager.loadFromPackageConfigs("robot_description_setup_assistant", config_files)) {
    RCLCPP_ERROR(setup_step_.getLogger(), "Failed to load robot configurations");
    // Fall back to empty configuration - UI will show "No robots available"
  }
  // Enable automatic configuration reloading for development
  config_manager.enableAutoReload(true);

  available_robots_ = config_manager.getAllRobots();
  filtered_robots_ = available_robots_;

  RCLCPP_INFO(setup_step_.getLogger(), "Loaded %zu robot configurations", available_robots_.size());

  // Setup UI only once
  if (!ui_initialized_) {
    setupLayout();
    setupConnections();
    updateRobotDisplay();
  }

  // If RViz panel is available and not yet integrated, integrate it
  if (rviz_panel_ && !rviz_integrated_ && content_stack_) {
    RCLCPP_INFO(setup_step_.getLogger(), "RViz panel available at onInit, integrating now");
    integrateRVizPanel();
  }
}

void RobotSelectionWidget::setRVizPanel(setup_framework::RVizPanel* rviz_panel)
{
  RCLCPP_INFO(setup_step_.getLogger(), "setRVizPanel called with panel: %p", (void*)rviz_panel);
  
  if (!rviz_panel) {
    RCLCPP_ERROR(setup_step_.getLogger(), "RViz panel is null!");
    return;
  }
  
  if (rviz_integrated_) {
    RCLCPP_INFO(setup_step_.getLogger(), "RViz already integrated, skipping duplicate integration");
    return;
  }
  
  rviz_panel_ = rviz_panel;
  RCLCPP_INFO(setup_step_.getLogger(), "RViz panel stored successfully");
  
  // Only integrate if content stack exists
  if (content_stack_) {
    RCLCPP_INFO(setup_step_.getLogger(), "Content stack available, integrating RViz panel");
    integrateRVizPanel();
  } else {
    RCLCPP_WARN(setup_step_.getLogger(), "Content stack not available yet - will integrate later");
  }
}

void RobotSelectionWidget::integrateRVizPanel()
{
  if (!rviz_panel_) {
    RCLCPP_ERROR(setup_step_.getLogger(), "RViz panel is null in integrateRVizPanel");
    return;
  }
  
  if (!content_stack_) {
    RCLCPP_ERROR(setup_step_.getLogger(), "Content stack is null in integrateRVizPanel");
    return;
  }
  
  if (rviz_integrated_) {
    RCLCPP_INFO(setup_step_.getLogger(), "RViz already integrated, verifying state");
    debugContentStack();
    return;
  }
  
  RCLCPP_INFO(setup_step_.getLogger(), "Starting RViz panel integration");
  RCLCPP_INFO(setup_step_.getLogger(), "Content stack widget count before: %d", content_stack_->count());
  
  // Ensure we have exactly 2 widgets
  if (content_stack_->count() < 2) {
    RCLCPP_ERROR(setup_step_.getLogger(), "Content stack missing placeholder! Creating now.");
    createRVizPlaceholder();
  }
  
  // Remove placeholder at index 1
  if (content_stack_->count() > 1) {
    QWidget* old_widget = content_stack_->widget(1);
    if (old_widget != rviz_panel_) {
      content_stack_->removeWidget(old_widget);
      old_widget->deleteLater();
      RCLCPP_INFO(setup_step_.getLogger(), "Removed placeholder widget");
    }
  }
  
  // Add RViz panel
  rviz_panel_->setParent(content_stack_);
  rviz_panel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  
  int index = content_stack_->addWidget(rviz_panel_);
  RCLCPP_INFO(setup_step_.getLogger(), "Added RViz panel to content stack at index: %d", index);
  RCLCPP_INFO(setup_step_.getLogger(), "Content stack widget count after: %d", content_stack_->count());
  
  // Enable visualization toggle
  if (show_visualization_check_) {
    show_visualization_check_->setEnabled(true);
    show_visualization_check_->setText("Show 3D Visualization");
    RCLCPP_INFO(setup_step_.getLogger(), "Enabled visualization checkbox");
  }
  
  rviz_panel_->show();
  rviz_panel_->setMinimumSize(300, 300);
  
  rviz_integrated_ = true;  // Mark as integrated
  RCLCPP_INFO(setup_step_.getLogger(), "RViz panel integration completed and marked as persistent");
  
  debugContentStack();
}

void RobotSelectionWidget::focusGiven()
{
  RCLCPP_INFO(setup_step_.getLogger(), "focusGiven() called");
  
  // Debug current state
  if (content_stack_) {
    RCLCPP_INFO(setup_step_.getLogger(), "Content stack exists with %d widgets", content_stack_->count());
    debugContentStack();
    
    // If RViz was integrated but content stack lost it, re-integrate
    if (rviz_integrated_ && content_stack_->count() < 2 && rviz_panel_) {
      RCLCPP_WARN(setup_step_.getLogger(), "RViz panel was lost! Re-integrating...");
      rviz_integrated_ = false;  // Reset flag
      integrateRVizPanel();
    }
  } else {
    RCLCPP_ERROR(setup_step_.getLogger(), "Content stack is null in focusGiven!");
  }
}

void RobotSelectionWidget::createRVizPlaceholder()
{
  // Create placeholder widget if RViz integration fails
  QWidget* placeholder = new QWidget();
  placeholder->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
  
  QVBoxLayout* layout = new QVBoxLayout(placeholder);
  layout->setAlignment(Qt::AlignCenter);
  
  QLabel* title = new QLabel("3D Visualization");
  title->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; text-align: center; color: #666; margin-bottom: 10px; }");
  title->setAlignment(Qt::AlignCenter);
  layout->addWidget(title);
  
  QLabel* message = new QLabel("RViz panel will appear here when available.\nSelect a robot to enable 3D visualization.");
  message->setStyleSheet("QLabel { color: #999; text-align: center; line-height: 1.4; }");
  message->setAlignment(Qt::AlignCenter);
  message->setWordWrap(true);
  layout->addWidget(message);
  
  layout->addStretch();
  
  // Remove existing placeholder if any (in case this is called multiple times)
  if (content_stack_->count() > 1) {
    QWidget* old_placeholder = content_stack_->widget(1);
    content_stack_->removeWidget(old_placeholder);
    old_placeholder->deleteLater();
  }
  
  // Add new placeholder as second widget (index 1)
  content_stack_->addWidget(placeholder);
  
  RCLCPP_DEBUG(setup_step_.getLogger(), "Created RViz placeholder widget");
}

void RobotSelectionWidget::setupLayout()
{
  if (ui_initialized_) {
    RCLCPP_WARN(setup_step_.getLogger(), "setupLayout() called multiple times - ignoring");
    return;
  }
  
  RCLCPP_INFO(setup_step_.getLogger(), "Setting up UI layout (first time)");
  
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
  
  robot_grid_widget_->setMinimumWidth(480);
  robot_grid_widget_->setMaximumWidth(600);
  main_splitter_->addWidget(robot_grid_widget_);
  
  // RIGHT PANEL: Setup once and protect from recreation
  setupRightPanel();
  main_splitter_->addWidget(right_panel_widget_);
  
  // Set splitter proportions
  QList<int> sizes;
  sizes << 280;  // Left panel (filter)
  sizes << 520;  // Center panel (robot grid) 
  sizes << 400;  // Right panel (specs/rviz)
  main_splitter_->setSizes(sizes);
  
  main_splitter_->setStretchFactor(0, 0);
  main_splitter_->setStretchFactor(1, 1);
  main_splitter_->setStretchFactor(2, 0);
  
  main_layout_->addWidget(main_splitter_);
  
  ui_initialized_ = true;
  RCLCPP_INFO(setup_step_.getLogger(), "UI layout setup completed");
}

void RobotSelectionWidget::setupRightPanel()
{
  if (right_panel_widget_) {
    RCLCPP_WARN(setup_step_.getLogger(), "setupRightPanel() called multiple times - ignoring");
    return;
  }
  
  RCLCPP_INFO(setup_step_.getLogger(), "Setting up right panel (first time)");
  
  // Create right panel container
  right_panel_widget_ = new QWidget();
  right_panel_widget_->setMinimumWidth(380);
  right_panel_widget_->setMaximumWidth(480);
  right_panel_widget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
  
  right_panel_layout_ = new QVBoxLayout(right_panel_widget_);
  right_panel_layout_->setContentsMargins(10, 10, 10, 10);
  right_panel_layout_->setSpacing(10);
  
  // Display options group
  display_options_group_ = new QGroupBox("Display Options");
  display_options_group_->setMaximumHeight(100);
  display_options_group_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  
  QVBoxLayout* options_layout = new QVBoxLayout(display_options_group_);
  options_layout->setSpacing(5);
  
  show_information_check_ = new QCheckBox("Show Robot Information");
  show_information_check_->setChecked(true);
  
  show_visualization_check_ = new QCheckBox("Show 3D Visualization");
  show_visualization_check_->setChecked(false);
  
  options_layout->addWidget(show_information_check_);
  options_layout->addWidget(show_visualization_check_);
  
  right_panel_layout_->addWidget(display_options_group_);
  
  // Stacked widget for toggling content - ONLY CREATE ONCE
  content_stack_ = new QStackedWidget();
  content_stack_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
  
  // ALWAYS add specification widget first (index 0)
  spec_widget_ = new RobotSpecificationWidget();
  content_stack_->addWidget(spec_widget_);
  
  // ALWAYS add placeholder second (index 1)
  createRVizPlaceholder();
  
  right_panel_layout_->addWidget(content_stack_);
  
  // Set initial state
  content_stack_->setCurrentIndex(0);
  
  RCLCPP_INFO(setup_step_.getLogger(), "Right panel setup complete. Content stack widgets: %d", content_stack_->count());
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

  connect(show_information_check_, &QCheckBox::toggled, this, &RobotSelectionWidget::onShowInformationToggled);
  connect(show_visualization_check_, &QCheckBox::toggled, this, &RobotSelectionWidget::onShowVisualizationToggled);
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

  // Ensure the grid doesn't force horizontal scrolling
  robot_scroll_content_->setMinimumWidth(ROBOTS_PER_ROW * (ROBOT_BUTTON_WIDTH + 20));
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
  ClickableRobotWidget* robot_container = new ClickableRobotWidget(robot);
  robot_container->setFixedSize(ROBOT_BUTTON_WIDTH, ROBOT_BUTTON_HEIGHT);
  robot_container->setStyleSheet(
    "QWidget { "
    "  border: 1px solid #ddd; "
    "  border-radius: 8px; "
    "  background: white; "
    "  margin: 2px; "
    "} "
    "QWidget:hover { "
    "  border: 2px solid #4CAF50; "
    "  background: #f9f9f9; "
    "}"
  );
  
  QVBoxLayout* container_layout = new QVBoxLayout(robot_container);
  container_layout->setContentsMargins(10, 10, 10, 10);
  container_layout->setSpacing(8);
  
  // Robot image button
  QPushButton* image_button = new QPushButton();
  image_button->setFixedSize(180, 120);
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
  name_label->setStyleSheet(
    "QLabel { "
    "  font-weight: bold; "
    "  font-size: 14px; "
    "  color: #333; "
    "  background: #f0f0f0; "
    "  border: 1px solid #ccc; "
    "  border-radius: 4px; "
    "  padding: 4px 8px; "
    "  margin: 2px 0px; "
    "}"
  );
  name_label->setWordWrap(true);
  name_label->setMinimumHeight(30);
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
  connect(image_button, &QPushButton::clicked, this, [this, robot]() {onRobotSelected(robot);});
  connect(robot_container, &ClickableRobotWidget::robotClicked, this, &RobotSelectionWidget::onRobotSelected);

  // Add hover effects
  robot_container->setAttribute(Qt::WA_Hover, true);
  robot_container->installEventFilter(this);
  
  // Add to grid
  int row = index / ROBOTS_PER_ROW;
  int col = index % ROBOTS_PER_ROW;
  robot_grid_->addWidget(robot_container, row, col);
}

void RobotSelectionWidget::onShowInformationToggled(bool show_info)
{
  if (!show_info) {
    return; // Don't process if being unchecked
  }
  
  RCLCPP_INFO(setup_step_.getLogger(), "Switching to information mode");
  
  // Ensure mutual exclusion
  show_visualization_check_->setChecked(false);
  
  // Switch to specification widget (index 0)
  content_stack_->setCurrentIndex(0);
  
  // Update visual feedback
  updateToggleButtonStyles();
}

void RobotSelectionWidget::updateVisualizationForSelectedRobot()
{
  if (selected_robot_.id.empty()) {
    RCLCPP_WARN(setup_step_.getLogger(), "No robot selected for visualization update");
    return;
  }
  
  if (!rviz_panel_) {
    RCLCPP_ERROR(setup_step_.getLogger(), "RViz panel is null - cannot update visualization");
    return;
  }
  
  RCLCPP_INFO(setup_step_.getLogger(), "Updating 3D visualization for robot: %s", selected_robot_.display_name.c_str());
  
  // Load robot URDF which will update RViz
  if (loadDefinedFile(selected_robot_)) {
    RCLCPP_INFO(setup_step_.getLogger(), "Robot URDF loaded successfully, updating RViz display");
    
    // Force RViz update
    try {
      rviz_panel_->updateFixedFrame();
      RCLCPP_INFO(setup_step_.getLogger(), "RViz fixed frame updated");
      
      // Make sure RViz panel is visible and updated
      rviz_panel_->show();
      rviz_panel_->update();
      rviz_panel_->repaint();
      
      RCLCPP_INFO(setup_step_.getLogger(), "RViz panel visibility and update forced");
    } catch (const std::exception& e) {
      RCLCPP_ERROR(setup_step_.getLogger(), "Error updating RViz: %s", e.what());
    }
  } else {
    RCLCPP_ERROR(setup_step_.getLogger(), "Failed to load robot URDF for visualization");
  }
}

void RobotSelectionWidget::debugContentStack() const
{
  if (!content_stack_) {
    RCLCPP_ERROR(setup_step_.getLogger(), "Content stack is null!");
    return;
  }
  
  RCLCPP_INFO(setup_step_.getLogger(), "=== Content Stack Debug Info ===");
  RCLCPP_INFO(setup_step_.getLogger(), "Widget count: %d", content_stack_->count());
  RCLCPP_INFO(setup_step_.getLogger(), "Current index: %d", content_stack_->currentIndex());
  
  for (int i = 0; i < content_stack_->count(); ++i) {
    QWidget* widget = content_stack_->widget(i);
    RCLCPP_INFO(setup_step_.getLogger(), "Widget %d: %p (visible: %s, size: %dx%d)", 
                i, (void*)widget, widget->isVisible() ? "YES" : "NO",
                widget->width(), widget->height());
    
    if (widget == rviz_panel_) {
      RCLCPP_INFO(setup_step_.getLogger(), "  ^ This is the RViz panel");
    } else if (widget == spec_widget_) {
      RCLCPP_INFO(setup_step_.getLogger(), "  ^ This is the specification widget");
    }
  }
  RCLCPP_INFO(setup_step_.getLogger(), "================================");
}

void RobotSelectionWidget::updateRightPanelVisibility()
{
  bool show_info = show_information_check_->isChecked();
  bool show_viz = show_visualization_check_->isChecked();
  
  // Ensure only one option is selected (mutual exclusion)
  if (show_info && show_viz) {
    // This shouldn't happen with proper signal handling, but safety first
    if (sender() == show_information_check_) {
      show_visualization_check_->setChecked(false);
      show_viz = false;
    } else {
      show_information_check_->setChecked(false);
      show_info = false;
    }
  }
  
  // If neither is selected, default to information view
  if (!show_info && !show_viz) {
    show_information_check_->setChecked(true);
    show_info = true;
  }
  
  // Update the stacked widget to show the correct panel
  if (show_info) {
    content_stack_->setCurrentIndex(0);  // Show specification widget
    if (rviz_panel_) {
      rviz_panel_->hide();
    }
    RCLCPP_DEBUG(setup_step_.getLogger(), "Switched to information view");
  } else if (show_viz) {
    content_stack_->setCurrentIndex(1);  // Show RViz widget
    if (rviz_panel_) {
      rviz_panel_->show();
    }
    RCLCPP_DEBUG(setup_step_.getLogger(), "Switched to visualization view");
    
    // If there's a selected robot, make sure it's shown in 3D
    if (!selected_robot_.id.empty()) {
      updateVisualizationForSelectedRobot();
    }
  }
  
  // Update visual feedback
  updateToggleButtonStyles();
}

void RobotSelectionWidget::updateToggleButtonStyles()
{
  // Update the visual appearance of the toggle checkboxes
  if (show_information_check_->isChecked()) {
    show_information_check_->setStyleSheet(
      "QCheckBox { font-weight: bold; color: #4CAF50; }"
    );
    show_visualization_check_->setStyleSheet(
      "QCheckBox { font-weight: normal; color: #666; }"
    );
  } else if (show_visualization_check_->isChecked()) {
    show_information_check_->setStyleSheet(
      "QCheckBox { font-weight: normal; color: #666; }"
    );
    show_visualization_check_->setStyleSheet(
      "QCheckBox { font-weight: bold; color: #4CAF50; }"
    );
  }
}

void RobotSelectionWidget::onShowVisualizationToggled(bool show_viz)
{
  if (!show_viz) {
    RCLCPP_DEBUG(setup_step_.getLogger(), "Visualization toggled OFF");
    return; // Don't process if being unchecked
  }
  
  RCLCPP_INFO(setup_step_.getLogger(), "Switching to 3D visualization mode");
  RCLCPP_INFO(setup_step_.getLogger(), "Content stack count: %d", content_stack_->count());
  RCLCPP_INFO(setup_step_.getLogger(), "RViz panel available: %s", rviz_panel_ ? "YES" : "NO");
  
  // Check if we have at least 2 widgets in the stack
  if (content_stack_->count() < 2) {
    RCLCPP_ERROR(setup_step_.getLogger(), "Content stack doesn't have enough widgets (count: %d)", content_stack_->count());
    show_visualization_check_->setChecked(false);
    show_information_check_->setChecked(true);
    return;
  }
  
  // Ensure mutual exclusion
  show_information_check_->setChecked(false);
  
  // Switch to visualization widget (index 1)
  content_stack_->setCurrentIndex(1);
  RCLCPP_INFO(setup_step_.getLogger(), "Set content stack to index 1");
  
  // Get the current widget and check what it is
  QWidget* current_widget = content_stack_->currentWidget();
  RCLCPP_INFO(setup_step_.getLogger(), "Current widget after switch: %p", (void*)current_widget);
  RCLCPP_INFO(setup_step_.getLogger(), "Is current widget the RViz panel? %s", 
              (current_widget == rviz_panel_) ? "YES" : "NO");
  
  if (current_widget) {
    current_widget->show();
    current_widget->setVisible(true);
    current_widget->update();
    RCLCPP_INFO(setup_step_.getLogger(), "Made current widget visible and updated");
  }
  
  // If there's a selected robot and RViz is available, update visualization
  if (!selected_robot_.id.empty() && rviz_panel_) {
    RCLCPP_INFO(setup_step_.getLogger(), "Updating visualization for selected robot: %s", selected_robot_.display_name.c_str());
    updateVisualizationForSelectedRobot();
  } else {
    RCLCPP_WARN(setup_step_.getLogger(), "Cannot update visualization - robot: %s, rviz_panel: %p", 
                selected_robot_.id.c_str(), (void*)rviz_panel_);
  }
  
  // Update visual feedback
  updateToggleButtonStyles();
}

void RobotSelectionWidget::onRobotSelected(const RobotConfig& robot)
{
  RCLCPP_INFO(setup_step_.getLogger(), "Robot selected: %s", robot.display_name.c_str());
  
  selected_robot_ = robot;
  
  // Update specification panel (always update, even if not currently visible)
  spec_widget_->setRobotConfig(robot);
  
  // Debug content stack state
  debugContentStack();
  
  // Check package dependencies before loading
  auto& config_manager = RobotConfigManager::getInstance();
  auto missing_packages = config_manager.getMissingPackages(robot);
  
  if (!missing_packages.empty()) {
    showRobotValidationDialog(missing_packages);
    return;
  }
  
  // Load the robot URDF - this will make it available for both info and visualization
  loadDefinedURDFClick(robot);
  
  // If we're in visualization mode, update the 3D view
  if (show_visualization_check_->isChecked() && rviz_panel_) {
    RCLCPP_INFO(setup_step_.getLogger(), "Currently in visualization mode, updating 3D view");
    updateVisualizationForSelectedRobot();
  }
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
