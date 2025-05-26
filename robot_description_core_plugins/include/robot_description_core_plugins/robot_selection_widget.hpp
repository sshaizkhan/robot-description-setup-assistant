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

#pragma once

// ROS2 includes
#include <rclcpp/rclcpp.hpp>

// rdsa includes
#include <robot_description_setup_framework/qt/helper_widgets.hpp>
#include <robot_description_setup_framework/qt/setup_step_widget.hpp>
#include <robot_description_setup_framework/qt/helper_widgets.hpp>
#include <robot_description_setup_framework/utilities.hpp>
#include <robot_description_setup_framework/setup_step.hpp>

#ifndef Q_MOC_RUN
#include <robot_description_core_plugins/robot_selection.hpp>
#include <robot_description_core_plugins/robot_config_manager.hpp>
#include <robot_description_core_plugins/filter_widget.hpp>
#include <robot_description_core_plugins/robot_specification_widget.hpp>
#endif

// Qt includes
#include <QWidget>
#include <QFrame>
#include <QApplication>
#include <QFileDialog>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QString>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QSplitter>
#include <QBoxLayout>
#include <QStackedWidget>
#include <QCheckBox>
#include <QGroupBox> 

class QLabel;
class QPushButton;

namespace robot_description::core_plugins
{

class ClickableRobotWidget : public QWidget
{
  Q_OBJECT
  
public:
  explicit ClickableRobotWidget(const RobotConfig& robot, QWidget* parent = nullptr)
    : QWidget(parent), robot_config_(robot)
  {
    setFixedSize(220, 240);
    setStyleSheet(
      "ClickableRobotWidget { "
      "  border: 1px solid #ddd; "
      "  border-radius: 8px; "
      "  background: white; "
      "  margin: 2px; "
      "} "
      "ClickableRobotWidget:hover { "
      "  border: 2px solid #4CAF50; "
      "  background: #f9f9f9; "
      "}"
    );
    
    // Enable mouse tracking for hover effects
    setAttribute(Qt::WA_Hover, true);
    setCursor(Qt::PointingHandCursor);
  }
  
  const RobotConfig& getRobotConfig() const { return robot_config_; }

Q_SIGNALS:
  void robotClicked(const RobotConfig& robot);

protected:
  void mousePressEvent(QMouseEvent* event) override
  {
    Q_UNUSED(event)
    Q_EMIT robotClicked(robot_config_);
  }
  
  void enterEvent(QEvent* event) override
  {
    setStyleSheet(
      "ClickableRobotWidget { "
      "  border: 2px solid #4CAF50; "
      "  border-radius: 8px; "
      "  background: #f9f9f9; "
      "  margin: 2px; "
      "}"
    );
    QWidget::enterEvent(event);
  }
  
  void leaveEvent(QEvent* event) override
  {
    setStyleSheet(
      "ClickableRobotWidget { "
      "  border: 1px solid #ddd; "
      "  border-radius: 8px; "
      "  background: white; "
      "  margin: 2px; "
      "}"
    );
    QWidget::leaveEvent(event);
  }

private:
  RobotConfig robot_config_;
};


class RobotSelectionWidget : public setup_framework::SetupStepWidget, public setup_framework::RVizIntegratedWidget
{
  Q_OBJECT
  
public:
  void onInit() override;
  void focusGiven() override;
  void setRVizPanel(setup_framework::RVizPanel* rviz_panel) override;
  bool needsRVizPanel() const override { return true; }  // This widget needs RViz
  void debugContentStack() const;
  
  SetupStep& getSetupStep() override
  {
    return setup_step_;
  }

private Q_SLOTS:
  void onFilterChanged(const RobotFilter& filter);
  void onRobotSelected(const RobotConfig& robot);
  void onConfigurationsReloaded();
  void onConfigurationError(const QString& error);
  void onRobotListUpdated();

  void onShowVisualizationToggled(bool show_viz);
  void onShowInformationToggled(bool show_info);

private:
  void setupLayout();
  void setupConnections();
  void updateRobotDisplay();
  void updateRobotDisplay(const std::vector<RobotConfig>& robots);
  void clearRobotGrid();
  void createRobotButton(const RobotConfig& robot, int index);
  void loadDefinedURDFClick(const RobotConfig& robot_config);
  bool loadDefinedFile(const RobotConfig& robot_config);
  void showRobotValidationDialog(const std::vector<std::string>& missing_packages);

  void setupRightPanel();
  void updateRightPanelVisibility();
  void updateToggleButtonStyles();
  void updateVisualizationForSelectedRobot();
  void integrateRVizPanel();  // Handle RViz integration
  void createRVizPlaceholder();  // Create placeholder when RViz not available
  
  // Layout components
  QHBoxLayout* main_layout_;
  QSplitter* main_splitter_;
  
  // Left panel - Filtering
  FilterWidget* filter_widget_;
  
  // Center panel - Robot grid
  QWidget* robot_grid_widget_;
  QVBoxLayout* robot_grid_layout_;
  QScrollArea* robot_scroll_area_;
  QWidget* robot_scroll_content_;
  QGridLayout* robot_grid_;
  QLabel* robot_count_label_;

  // Right panel - NEW: Improved with toggle
  QWidget* right_panel_widget_;
  QVBoxLayout* right_panel_layout_;
  QGroupBox* display_options_group_;
  QCheckBox* show_visualization_check_;
  QCheckBox* show_information_check_;
  QStackedWidget* content_stack_;
  
  // Right panel - Specifications
  RobotSpecificationWidget* spec_widget_;

  // Internal state
  RobotSelection setup_step_;
  std::vector<RobotConfig> available_robots_;
  std::vector<RobotConfig> filtered_robots_;
  RobotConfig selected_robot_;

  bool ui_initialized_ = false;
  bool rviz_integrated_ = false;
  
  // Grid layout parameters
  static const int ROBOTS_PER_ROW = 2;
  static const int ROBOT_BUTTON_WIDTH = 220;
  static const int ROBOT_BUTTON_HEIGHT = 240;
  
};

} // namespace robot_description::core_plugins

