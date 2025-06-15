/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2025, Shahwaz Khan
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

#include "robot_description_core_plugins/end_effectors/end_effector_selection_widget.hpp"

namespace robot_description::core_plugins
{
EndEffectorSelectionWidget::EndEffectorSelectionWidget() 
  : main_layout_(nullptr)
  , main_splitter_(nullptr)
  , filter_widget_(nullptr)
  , end_effector_grid_widget_(nullptr)
  , end_effector_grid_layout_(nullptr)
  , end_effector_scroll_area_(nullptr)
  , end_effector_scroll_content_(nullptr)
  , end_effector_grid_(nullptr)
  , end_effector_count_label_(nullptr)
  , right_panel_widget_(nullptr)
  , right_panel_layout_(nullptr)
  , display_options_group_(nullptr)
  , show_visualization_check_(nullptr)
  , show_information_check_(nullptr)
  , content_stack_(nullptr)
  , spec_widget_(nullptr)
  , ui_initialized_(false)
  , rviz_integrated_(false)
{
}

void EndEffectorSelectionWidget::onInit()
{
  RCLCPP_INFO(setup_step_.getLogger(), "Initializing End-Effector Selection Widget");
  
  // Initialize the configuration system
  auto& config_manager = EndEffectorConfigManager::getInstance();
  
  // Load end-effector configuration files
  std::vector<std::string> config_files = {"config/end_effectors.yaml"};
  
  if (!config_manager.loadFromPackageConfigs("robot_description_setup_assistant", config_files)) {
    RCLCPP_ERROR(setup_step_.getLogger(), "Failed to load end-effector configurations");
  }
  
  // Enable automatic configuration reloading for development
  config_manager.enableAutoReload(true);

  available_end_effectors_ = config_manager.getAllEndEffectors();
  filtered_end_effectors_ = available_end_effectors_;

  RCLCPP_INFO(setup_step_.getLogger(), "Loaded %zu end-effector configurations", available_end_effectors_.size());

  // Setup UI only once
  if (!ui_initialized_) {
    setupLayout();
    setupConnections();
    updateEndEffectorDisplay();
  }

  // If RViz panel is available and not yet integrated, integrate it
  if (rviz_panel_ && !rviz_integrated_ && content_stack_) {
    RCLCPP_INFO(setup_step_.getLogger(), "RViz panel available at onInit, integrating now");
    integrateRVizPanel();
  }
}

void EndEffectorSelectionWidget::setRVizPanel(setup_framework::RVizPanel* rviz_panel)
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

void EndEffectorSelectionWidget::integrateRVizPanel()
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
  
  rviz_integrated_ = true;
  RCLCPP_INFO(setup_step_.getLogger(), "RViz panel integration completed");
}

void EndEffectorSelectionWidget::focusGiven()
{
  RCLCPP_INFO(setup_step_.getLogger(), "focusGiven() called");
  
  // Check if we need to filter by robot compatibility
  // This would be set by the robot selection step when user navigates here
  if (!selected_robot_brand_.empty()) {
    RCLCPP_INFO(setup_step_.getLogger(), "Filtering end-effectors for robot brand: %s", selected_robot_brand_.c_str());
    filter_widget_->setCompatibleRobotBrand(selected_robot_brand_);
  }
  
  if (content_stack_) {
    RCLCPP_INFO(setup_step_.getLogger(), "Content stack exists with %d widgets", content_stack_->count());
    
    // If RViz was integrated but content stack lost it, re-integrate
    if (rviz_integrated_ && content_stack_->count() < 2 && rviz_panel_) {
      RCLCPP_WARN(setup_step_.getLogger(), "RViz panel was lost! Re-integrating...");
      rviz_integrated_ = false;
      integrateRVizPanel();
    }
  } else {
    RCLCPP_ERROR(setup_step_.getLogger(), "Content stack is null in focusGiven!");
  }
}

void EndEffectorSelectionWidget::createRVizPlaceholder()
{
  QWidget* placeholder = new QWidget();
  placeholder->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
  
  QVBoxLayout* layout = new QVBoxLayout(placeholder);
  layout->setAlignment(Qt::AlignCenter);
  
  QLabel* title = new QLabel("3D End-Effector Visualization");
  title->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; text-align: center; color: #666; margin-bottom: 10px; }");
  title->setAlignment(Qt::AlignCenter);
  layout->addWidget(title);
  
  QLabel* message = new QLabel("RViz panel will appear here when available.\nSelect an end-effector to enable 3D visualization.");
  message->setStyleSheet("QLabel { color: #999; text-align: center; line-height: 1.4; }");
  message->setAlignment(Qt::AlignCenter);
  message->setWordWrap(true);
  layout->addWidget(message);
  
  layout->addStretch();
  
  // Remove existing placeholder if any
  if (content_stack_->count() > 1) {
    QWidget* old_placeholder = content_stack_->widget(1);
    content_stack_->removeWidget(old_placeholder);
    old_placeholder->deleteLater();
  }
  
  content_stack_->addWidget(placeholder);
  RCLCPP_DEBUG(setup_step_.getLogger(), "Created RViz placeholder widget");
}

void EndEffectorSelectionWidget::setupLayout()
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
  filter_widget_ = new EndEffectorFilterWidget();
  filter_widget_->setMinimumWidth(250);
  filter_widget_->setMaximumWidth(300);
  
  // Set categories in filter widget
  auto& config_manager = EndEffectorConfigManager::getInstance();
  filter_widget_->setCategories(config_manager.getCategories());
  
  main_splitter_->addWidget(filter_widget_);
  
  // CENTER PANEL: End-effector grid
  end_effector_grid_widget_ = new QWidget();
  end_effector_grid_layout_ = new QVBoxLayout(end_effector_grid_widget_);
  end_effector_grid_layout_->setContentsMargins(0, 0, 0, 0);
  end_effector_grid_layout_->setSpacing(10);
  
  // End-effector count label
  end_effector_count_label_ = new QLabel();
  end_effector_count_label_->setStyleSheet("QLabel { font-weight: bold; color: #666; }");
  end_effector_grid_layout_->addWidget(end_effector_count_label_);
  
  // Scroll area for end-effector grid
  end_effector_scroll_area_ = new QScrollArea();
  end_effector_scroll_area_->setWidgetResizable(true);
  end_effector_scroll_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  end_effector_scroll_area_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  end_effector_scroll_content_ = new QWidget();
  end_effector_grid_ = new QGridLayout(end_effector_scroll_content_);
  end_effector_grid_->setSpacing(15);
  end_effector_grid_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  
  end_effector_scroll_area_->setWidget(end_effector_scroll_content_);
  end_effector_grid_layout_->addWidget(end_effector_scroll_area_);
  
  end_effector_grid_widget_->setMinimumWidth(480);
  end_effector_grid_widget_->setMaximumWidth(600);
  main_splitter_->addWidget(end_effector_grid_widget_);
  
  // RIGHT PANEL: Setup once and protect from recreation
  setupRightPanel();
  main_splitter_->addWidget(right_panel_widget_);
  
  // Set splitter proportions
  QList<int> sizes;
  sizes << 280;  // Left panel (filter)
  sizes << 520;  // Center panel (end-effector grid) 
  sizes << 400;  // Right panel (specs/rviz)
  main_splitter_->setSizes(sizes);
  
  main_splitter_->setStretchFactor(0, 0);
  main_splitter_->setStretchFactor(1, 1);
  main_splitter_->setStretchFactor(2, 0);
  
  main_layout_->addWidget(main_splitter_);
  
  ui_initialized_ = true;
  RCLCPP_INFO(setup_step_.getLogger(), "UI layout setup completed");
}

void EndEffectorSelectionWidget::setupRightPanel()
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
  
  show_information_check_ = new QCheckBox("Show End-Effector Information");
  show_information_check_->setChecked(true);
  
  show_visualization_check_ = new QCheckBox("Show 3D Visualization");
  show_visualization_check_->setChecked(false);
  
  options_layout->addWidget(show_information_check_);
  options_layout->addWidget(show_visualization_check_);
  
  right_panel_layout_->addWidget(display_options_group_);
  
  // Stacked widget for toggling content
  content_stack_ = new QStackedWidget();
  content_stack_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
  
  // Add specification widget first (index 0)
  spec_widget_ = new EndEffectorSpecificationWidget();
  content_stack_->addWidget(spec_widget_);
  
  // Add placeholder second (index 1)
  createRVizPlaceholder();
  
  right_panel_layout_->addWidget(content_stack_);
  
  // Set initial state
  content_stack_->setCurrentIndex(0);
  
  RCLCPP_INFO(setup_step_.getLogger(), "Right panel setup complete. Content stack widgets: %d", content_stack_->count());
}

void EndEffectorSelectionWidget::setupConnections()
{
  auto& config_manager = EndEffectorConfigManager::getInstance();
  
  // Filter widget connections
  connect(filter_widget_, &EndEffectorFilterWidget::filterChanged, this, &EndEffectorSelectionWidget::onFilterChanged);
  
  // Configuration manager connections
  connect(&config_manager, &EndEffectorConfigManager::configurationsReloaded, this, &EndEffectorSelectionWidget::onConfigurationsReloaded);
  connect(&config_manager, &EndEffectorConfigManager::configurationError, this, &EndEffectorSelectionWidget::onConfigurationError);
  connect(&config_manager, &EndEffectorConfigManager::endEffectorListUpdated, this, &EndEffectorSelectionWidget::onEndEffectorListUpdated);

  connect(show_information_check_, &QCheckBox::toggled, this, &EndEffectorSelectionWidget::onShowInformationToggled);
  connect(show_visualization_check_, &QCheckBox::toggled, this, &EndEffectorSelectionWidget::onShowVisualizationToggled);
}

void EndEffectorSelectionWidget::onFilterChanged(const EndEffectorFilter& filter)
{
  RCLCPP_DEBUG(setup_step_.getLogger(), "Applying end-effector filter");
  
  auto& config_manager = EndEffectorConfigManager::getInstance();
  
  if (filter.isEmpty()) {
    filtered_end_effectors_ = available_end_effectors_;
  } else {
    filtered_end_effectors_ = config_manager.filterEndEffectors(filter);
  }
  
  updateEndEffectorDisplay(filtered_end_effectors_);
}

void EndEffectorSelectionWidget::updateEndEffectorDisplay()
{
  updateEndEffectorDisplay(filtered_end_effectors_);
}

void EndEffectorSelectionWidget::updateEndEffectorDisplay(const std::vector<EndEffectorConfig>& end_effectors)
{
  // Update end-effector count
  end_effector_count_label_->setText(QString("Found %1 end-effector(s)").arg(end_effectors.size()));
  
  // Clear existing end-effector buttons
  clearEndEffectorGrid();
  
  if (end_effectors.empty()) {
    // Show "no end-effectors" message
    QLabel* no_end_effectors_label = new QLabel("No end-effectors match the current filter criteria.");
    no_end_effectors_label->setAlignment(Qt::AlignCenter);
    no_end_effectors_label->setStyleSheet("QLabel { color: #999; font-size: 14px; margin: 50px; }");
    end_effector_grid_->addWidget(no_end_effectors_label, 0, 0, 1, END_EFFECTORS_PER_ROW);
    return;
  }
  
  // Create end-effector buttons
  for (size_t i = 0; i < end_effectors.size(); ++i) {
    createEndEffectorButton(end_effectors[i], static_cast<int>(i));
  }

  // Ensure the grid doesn't force horizontal scrolling
  end_effector_scroll_content_->setMinimumWidth(END_EFFECTORS_PER_ROW * (END_EFFECTOR_BUTTON_WIDTH + 20));
}

void EndEffectorSelectionWidget::clearEndEffectorGrid()
{
  // Clear button references
  end_effector_buttons_.clear();
  
  // Remove all widgets from grid
  QLayoutItem* item;
  while ((item = end_effector_grid_->takeAt(0)) != nullptr) {
    if (item->widget()) {
      delete item->widget();
    }
    delete item;
  }
}

void EndEffectorSelectionWidget::createEndEffectorButton(const EndEffectorConfig& end_effector, int index)
{
  // Create container widget for end-effector button and info
  ClickableEndEffectorWidget* end_effector_container = new ClickableEndEffectorWidget(end_effector);
  end_effector_container->setFixedSize(END_EFFECTOR_BUTTON_WIDTH, END_EFFECTOR_BUTTON_HEIGHT);
  
  QVBoxLayout* container_layout = new QVBoxLayout(end_effector_container);
  container_layout->setContentsMargins(10, 10, 10, 10);
  container_layout->setSpacing(8);
  
  // End-effector image button
  QPushButton* image_button = new QPushButton();
  image_button->setFixedSize(180, 120);
  image_button->setFlat(true);
  image_button->setStyleSheet("");
  image_button->setAutoFillBackground(false);

  // Load end-effector image
  if (std::filesystem::exists(end_effector.image_path)) {
    QPixmap pixmap(QString::fromStdString(end_effector.image_path.string()));
    if (!pixmap.isNull()) {
      QPixmap scaled_pixmap = pixmap.scaled(140, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation);
      image_button->setIcon(QIcon(scaled_pixmap));
      image_button->setIconSize(QSize(140, 100));
    } else {
      image_button->setText("No Image");
    }
  } else {
    image_button->setText("Image\nNot Found");
  }
  
  container_layout->addWidget(image_button, 0, Qt::AlignCenter);
  
  // End-effector name label
  QLabel* name_label = new QLabel(QString::fromStdString(end_effector.display_name));
  name_label->setAlignment(Qt::AlignCenter);
  name_label->setWordWrap(true);
  name_label->setMinimumHeight(30);
  name_label->setAutoFillBackground(false);
  name_label->setStyleSheet("QLabel { font-weight: bold; font-size: 15px; color: #333; padding: 6px; }");
  container_layout->addWidget(name_label);
  
  // Manufacturer label
  if (!end_effector.manufacturer.empty()) {
    QLabel* manufacturer_label = new QLabel(QString::fromStdString(end_effector.manufacturer));
    manufacturer_label->setAlignment(Qt::AlignCenter);
    manufacturer_label->setStyleSheet("QLabel { font-size: 12px; color: #888; font-weight: bold; }");
    container_layout->addWidget(manufacturer_label);
  }
  
  // Key specifications summary
  QLabel* specs_label = new QLabel();
  QString specs_text;
  if (end_effector.specifications.max_opening_mm > 0.0) {
    specs_text += QString("%1mm opening").arg(end_effector.specifications.max_opening_mm, 0, 'f', 0);
  }
  if (end_effector.specifications.max_force_n > 0.0) {
    if (!specs_text.isEmpty()) specs_text += " • ";
    specs_text += QString("%1N force").arg(end_effector.specifications.max_force_n, 0, 'f', 0);
  }
  if (end_effector.specifications.adaptive_grip) {
    if (!specs_text.isEmpty()) specs_text += " • ";
    specs_text += "Adaptive";
  }
  
  specs_label->setText(specs_text);
  specs_label->setAlignment(Qt::AlignCenter);
  specs_label->setStyleSheet("QLabel { font-size: 10px; color: #666; }");
  specs_label->setWordWrap(true);
  container_layout->addWidget(specs_label);
  
  // Package validation indicator
  auto& config_manager = EndEffectorConfigManager::getInstance();
  auto missing_packages = config_manager.getMissingPackages(end_effector);
  
  QLabel* status_label = new QLabel();
  if (missing_packages.empty()) {
    status_label->setText("✅ Ready");
    status_label->setStyleSheet("QLabel { color: #4caf50; font-size: 10px; font-weight: bold; }");
  } else {
    status_label->setText(QString("⚠️ Missing %1 pkg(s)").arg(missing_packages.size()));
    status_label->setStyleSheet("QLabel { color: #ff9800; font-size: 10px; font-weight: bold; }");
  }
  status_label->setAlignment(Qt::AlignCenter);
  status_label->setAutoFillBackground(false);

  container_layout->addWidget(status_label);
  
  // Connect button signals
  connect(image_button, &QPushButton::clicked, this, [this, end_effector]() {onEndEffectorSelected(end_effector);});
  connect(end_effector_container, &ClickableEndEffectorWidget::endEffectorClicked, this, &EndEffectorSelectionWidget::onEndEffectorSelected);

  // Store button reference
  end_effector_buttons_.push_back(end_effector_container);
  
  // Add to grid
  int row = index / END_EFFECTORS_PER_ROW;
  int col = index % END_EFFECTORS_PER_ROW;
  end_effector_grid_->addWidget(end_effector_container, row, col);
}

void EndEffectorSelectionWidget::onShowInformationToggled(bool show_info)
{
  if (!show_info) {
    return; // Don't process if being unchecked
  }
  
  RCLCPP_INFO(setup_step_.getLogger(), "Switching to information mode");
  
  // Ensure mutual exclusion
  show_visualization_check_->setChecked(false);
  
  // Switch to specification widget (index 0)
  content_stack_->setCurrentIndex(0);
}

void EndEffectorSelectionWidget::updateVisualizationForSelectedEndEffector()
{
  if (selected_end_effector_.id.empty()) {
    RCLCPP_WARN(setup_step_.getLogger(), "No end-effector selected for visualization update");
    return;
  }
  
  if (!rviz_panel_) {
    RCLCPP_ERROR(setup_step_.getLogger(), "RViz panel is null - cannot update visualization");
    return;
  }
  
  loadAndUpdateVisualization(selected_end_effector_);
}

void EndEffectorSelectionWidget::onShowVisualizationToggled(bool show_viz)
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
  
  // Load URDF now that we're switching to visualization mode
  if (!selected_end_effector_.id.empty() && rviz_panel_) {
    RCLCPP_INFO(setup_step_.getLogger(), "Loading URDF and updating visualization for end-effector: %s", 
                selected_end_effector_.display_name.c_str());
    loadAndUpdateVisualization(selected_end_effector_);
  } else if (selected_end_effector_.id.empty()) {
    RCLCPP_INFO(setup_step_.getLogger(), "No end-effector selected yet - visualization will load when end-effector is selected");
  } else {
    RCLCPP_ERROR(setup_step_.getLogger(), "Cannot load visualization - RViz panel not available");
  }
}

void EndEffectorSelectionWidget::onEndEffectorSelected(const EndEffectorConfig& end_effector)
{
  RCLCPP_INFO(setup_step_.getLogger(), "End-effector selected: %s", end_effector.display_name.c_str());
  
  selected_end_effector_ = end_effector;

  // Update visual selection state - clear previous selection
  for (auto* button : end_effector_buttons_) {
    button->setSelected(false);
  }
  
  // Set current selection
  for (auto* button : end_effector_buttons_) {
    if (button->getEndEffectorConfig().id == end_effector.id) {
      button->setSelected(true);
      RCLCPP_INFO(setup_step_.getLogger(), "Set end-effector button as selected: %s", end_effector.display_name.c_str());
      break;
    }
  }
  
  // Update specification panel (always update, even if not currently visible)
  spec_widget_->setEndEffectorConfig(end_effector);
  
  // Check package dependencies before loading
  auto& config_manager = EndEffectorConfigManager::getInstance();
  auto missing_packages = config_manager.getMissingPackages(end_effector);
  
  if (!missing_packages.empty()) {
    showEndEffectorValidationDialog(missing_packages);
    return;
  }
  
  // If we're in visualization mode, update the 3D view
  if (show_visualization_check_->isChecked() && rviz_panel_) {
    RCLCPP_INFO(setup_step_.getLogger(), "Currently in visualization mode, loading URDF for end-effector: %s", 
                end_effector.display_name.c_str());
    loadAndUpdateVisualization(end_effector);
  } else {
    RCLCPP_INFO(setup_step_.getLogger(), "End-effector information updated, URDF not loaded (not in visualization mode)");
  }
}

void EndEffectorSelectionWidget::loadAndUpdateVisualization(const EndEffectorConfig& end_effector)
{
  RCLCPP_INFO(setup_step_.getLogger(), "Loading URDF and updating visualization for end-effector: %s", 
              end_effector.display_name.c_str());
  
  // Check package dependencies first
  auto& config_manager = EndEffectorConfigManager::getInstance();
  auto missing_packages = config_manager.getMissingPackages(end_effector);
  
  if (!missing_packages.empty()) {
    RCLCPP_WARN(setup_step_.getLogger(), "End-effector has missing packages, showing validation dialog");
    showEndEffectorValidationDialog(missing_packages);
    return;
  }
  
  // Load the URDF file
  if (loadDefinedFile(end_effector)) {
    RCLCPP_INFO(setup_step_.getLogger(), "URDF loaded successfully, updating RViz visualization");
    
    // Update RViz visualization
    if (rviz_panel_) {
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
      RCLCPP_ERROR(setup_step_.getLogger(), "RViz panel not available for visualization update");
    }
  } else {
    RCLCPP_ERROR(setup_step_.getLogger(), "Failed to load URDF for visualization");
  }
}

void EndEffectorSelectionWidget::showEndEffectorValidationDialog(const std::vector<std::string>& missing_packages)
{
  QString missing_list;
  for (size_t i = 0; i < missing_packages.size(); ++i) {
    if (i > 0) missing_list += "\n";
    missing_list += "• " + QString::fromStdString(missing_packages[i]);
  }
  
  QMessageBox msg_box(this);
  msg_box.setWindowTitle("Missing Dependencies for 3D Visualization");
  msg_box.setIcon(QMessageBox::Warning);
  msg_box.setText(QString("The selected end-effector (%1) requires packages that are not installed for 3D visualization:")
                 .arg(QString::fromStdString(selected_end_effector_.display_name)));
  msg_box.setDetailedText(missing_list);
  msg_box.setInformativeText("You can still view the end-effector information, but 3D visualization may not work properly. Do you want to continue anyway?");
  msg_box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  msg_box.setDefaultButton(QMessageBox::No);
  
  QPushButton* install_button = msg_box.addButton("Install Packages", QMessageBox::ActionRole);
  
  int result = msg_box.exec();
  
  if (msg_box.clickedButton() == install_button) {
    QMessageBox::information(this, "Package Installation", 
                           "Package installation feature will be implemented in a future update.\n"
                           "Please install the required packages manually using your package manager.");
  } else if (result == QMessageBox::Yes) {
    RCLCPP_INFO(setup_step_.getLogger(), "User chose to continue with missing packages");
    // Try to load anyway
    if (loadDefinedFile(selected_end_effector_)) {
      if (rviz_panel_) {
        rviz_panel_->updateFixedFrame();
      }
    }
  } else {
    RCLCPP_INFO(setup_step_.getLogger(), "User cancelled due to missing packages");
    // Switch back to information mode
    show_visualization_check_->setChecked(false);
    show_information_check_->setChecked(true);
    content_stack_->setCurrentIndex(0);
  }
}

bool EndEffectorSelectionWidget::loadDefinedFile(const EndEffectorConfig& end_effector_config)
{
  // Build URDF path from end-effector configuration
  std::filesystem::path urdf_path;
  
  try {
    urdf_path = robot_description::getSharePath(end_effector_config.urdf_package) / end_effector_config.urdf_path;
  } catch (const std::exception& e) {
    QMessageBox::warning(this, "Error Loading Files", 
                        QString("Failed to locate package '%1': %2")
                        .arg(QString::fromStdString(end_effector_config.urdf_package))
                        .arg(e.what()));
    return false;
  }
  
  if (urdf_path.empty()) {
    QMessageBox::warning(this, "Error Loading Files", "No end-effector model file specified");
    return false;
  }
  
  // Check that the URDF file exists
  if (!std::filesystem::is_regular_file(urdf_path)) {
    QMessageBox::warning(this, "Error Loading Files",
                         QString("Unable to locate the URDF file:\n%1\n\n"
                                "Please ensure the end-effector packages are properly installed.")
                         .arg(QString::fromStdString(urdf_path.string())));
    return false;
  }
  
  try {
    // Load the end-effector URDF file with xacro arguments
    setup_step_.loadEndEffectorURDFFile(urdf_path, end_effector_config.xacro_args);
    
    // Signal that data has been updated
    Q_EMIT dataUpdated();
    
    return true;
    
  } catch (const std::runtime_error& e) {
    QMessageBox::critical(this, "Error Loading URDF", 
                         QString("Failed to load end-effector URDF file:\n%1\n\n"
                                "Error details:\n%2")
                         .arg(QString::fromStdString(urdf_path.string()))
                         .arg(e.what()));
    return false;
  }
}

void EndEffectorSelectionWidget::onConfigurationsReloaded()
{
  RCLCPP_INFO(setup_step_.getLogger(), "End-effector configurations reloaded - updating UI");
  
  // Reload end-effector list
  auto& config_manager = EndEffectorConfigManager::getInstance();
  available_end_effectors_ = config_manager.getAllEndEffectors();
  
  // Update filter categories
  filter_widget_->setCategories(config_manager.getCategories());
  
  // Re-apply current filter
  EndEffectorFilter current_filter = filter_widget_->getCurrentFilter();
  if (current_filter.isEmpty()) {
    filtered_end_effectors_ = available_end_effectors_;
  } else {
    filtered_end_effectors_ = config_manager.filterEndEffectors(current_filter);
  }
  
  // Update display
  updateEndEffectorDisplay();
  
  // Clear specification panel if selected end-effector is no longer available
  bool selected_end_effector_still_available = false;
  for (const auto& end_effector : available_end_effectors_) {
    if (end_effector.id == selected_end_effector_.id) {
      selected_end_effector_still_available = true;
      break;
    }
  }
  
  if (!selected_end_effector_still_available) {
    spec_widget_->clear();
    selected_end_effector_ = EndEffectorConfig{};
  }
  
  // Show notification to user
  QMessageBox::information(this, "Configuration Updated", 
                          QString("End-effector configurations have been reloaded.\n"
                                 "Found %1 end-effector(s) matching current filters.")
                          .arg(filtered_end_effectors_.size()));
}

void EndEffectorSelectionWidget::onConfigurationError(const QString& error)
{
  RCLCPP_ERROR(setup_step_.getLogger(), "Configuration error: %s", error.toStdString().c_str());
  
  QMessageBox::warning(this, "Configuration Error", 
                      QString("Error reloading end-effector configurations:\n\n%1\n\n"
                             "The end-effector list may not be up to date.")
                      .arg(error));
}

void EndEffectorSelectionWidget::onEndEffectorListUpdated()
{
  RCLCPP_INFO(setup_step_.getLogger(), "End-effector list updated");
  
  // This is called when the configuration manager detects changes
  onConfigurationsReloaded();
}

}  // namespace robot_description::core_plugins

#include <pluginlib/class_list_macros.hpp>
PLUGINLIB_EXPORT_CLASS(robot_description::core_plugins::EndEffectorSelectionWidget, robot_description::setup_framework::SetupStepWidget)