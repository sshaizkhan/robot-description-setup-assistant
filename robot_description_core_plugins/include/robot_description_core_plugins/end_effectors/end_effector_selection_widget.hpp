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

#pragma once

// ROS2 includes
#include <rclcpp/rclcpp.hpp>

// rdsa includes
#include <robot_description_setup_framework/qt/helper_widgets.hpp>
#include <robot_description_setup_framework/qt/setup_step_widget.hpp>
#include <robot_description_setup_framework/utilities.hpp>
#include <robot_description_setup_framework/setup_step.hpp>

#ifndef Q_MOC_RUN
#include "end_effector_selection.hpp"
#include "end_effector_config_manager.hpp"
#include "end_effector_filter_widget.hpp"
#include "end_effector_specification_widget.hpp"
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
#include <QPainter>
#include <QMouseEvent>

namespace robot_description::core_plugins
{

class ClickableEndEffectorWidget : public QWidget
{
  Q_OBJECT
  
public:
  explicit ClickableEndEffectorWidget(const EndEffectorConfig& end_effector, QWidget* parent = nullptr)
    : QWidget(parent), end_effector_config_(end_effector), is_selected_(false), is_hovered_(false)
  {
    setFixedSize(250, 260);
    setAttribute(Qt::WA_Hover, true);
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);
    
    RCLCPP_INFO(rclcpp::get_logger("EndEffectorButton"), "Created end-effector button: %s", end_effector.display_name.c_str());
  }
  
  const EndEffectorConfig& getEndEffectorConfig() const { return end_effector_config_; }
  
  void setSelected(bool selected) 
  {
    if (is_selected_ != selected) {
      is_selected_ = selected;
      update(); // Trigger repaint
      RCLCPP_INFO(rclcpp::get_logger("EndEffectorButton"), "End-effector %s selection: %s", 
                  end_effector_config_.display_name.c_str(), selected ? "SELECTED" : "UNSELECTED");
    }
  }
  
  bool isSelected() const { return is_selected_; }

Q_SIGNALS:
  void endEffectorClicked(const EndEffectorConfig& end_effector);

protected:
  void mousePressEvent(QMouseEvent* event) override
  {
    Q_UNUSED(event)
    Q_EMIT endEffectorClicked(end_effector_config_);
  }
  
  void enterEvent(QEvent* event) override
  {
    is_hovered_ = true;
    update();
    QWidget::enterEvent(event);
  }
  
  void leaveEvent(QEvent* event) override
  {
    is_hovered_ = false;
    update();
    QWidget::leaveEvent(event);
  }
  
  void paintEvent(QPaintEvent* event) override
  {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Determine colors based on state
    QColor bgColor;
    QColor borderColor;
    int borderWidth;
    
    if (is_selected_) {
      bgColor = QColor(227, 242, 253);      // Light blue background
      borderColor = QColor(33, 150, 243);   // Blue border
      borderWidth = 3;
    } else if (is_hovered_) {
      bgColor = QColor(240, 248, 255);      // Very light blue background  
      borderColor = QColor(76, 175, 80);    // Green border
      borderWidth = 2;
    } else {
      bgColor = QColor(255, 255, 255);      // White background
      borderColor = QColor(221, 221, 221);  // Gray border
      borderWidth = 2;
    }
    
    // Draw background with rounded corners
    QRect drawRect = rect().adjusted(borderWidth/2, borderWidth/2, -borderWidth/2, -borderWidth/2);
    
    painter.setBrush(QBrush(bgColor));
    painter.setPen(QPen(borderColor, borderWidth));
    painter.drawRoundedRect(drawRect, 8, 8);
    
    // Call parent to paint children
    QWidget::paintEvent(event);
  }

private:
  EndEffectorConfig end_effector_config_;
  bool is_selected_;
  bool is_hovered_;
};

class EndEffectorSelectionWidget : public setup_framework::SetupStepWidget, public setup_framework::RVizIntegratedWidget
{
  Q_OBJECT
  
public:
  EndEffectorSelectionWidget();
  void onInit() override;
  void focusGiven() override;
  void setRVizPanel(setup_framework::RVizPanel* rviz_panel) override;
  bool needsRVizPanel() const override { return true; }
  
  SetupStep& getSetupStep() override
  {
    return setup_step_;
  }

private Q_SLOTS:
  void onFilterChanged(const EndEffectorFilter& filter);
  void onEndEffectorSelected(const EndEffectorConfig& end_effector);
  void onConfigurationsReloaded();
  void onConfigurationError(const QString& error);
  void onEndEffectorListUpdated();
  void onShowVisualizationToggled(bool show_viz);
  void onShowInformationToggled(bool show_info);

private:
  void setupLayout();
  void setupConnections();
  void updateEndEffectorDisplay();
  void updateEndEffectorDisplay(const std::vector<EndEffectorConfig>& end_effectors);
  void clearEndEffectorGrid();
  void createEndEffectorButton(const EndEffectorConfig& end_effector, int index);
  bool loadDefinedFile(const EndEffectorConfig& end_effector_config);
  void showEndEffectorValidationDialog(const std::vector<std::string>& missing_packages);
  void loadAndUpdateVisualization(const EndEffectorConfig& end_effector);

  void setupRightPanel();
  void updateRightPanelVisibility();
  void updateToggleButtonStyles();
  void updateVisualizationForSelectedEndEffector();
  void integrateRVizPanel();
  void createRVizPlaceholder();
  
  // Layout components
  QHBoxLayout* main_layout_;
  QSplitter* main_splitter_;
  
  // Left panel - Filtering
  EndEffectorFilterWidget* filter_widget_;
  
  // Center panel - End-effector grid
  QWidget* end_effector_grid_widget_;
  QVBoxLayout* end_effector_grid_layout_;
  QScrollArea* end_effector_scroll_area_;
  QWidget* end_effector_scroll_content_;
  QGridLayout* end_effector_grid_;
  QLabel* end_effector_count_label_;

  // Right panel - Information and visualization
  QWidget* right_panel_widget_;
  QVBoxLayout* right_panel_layout_;
  QGroupBox* display_options_group_;
  QCheckBox* show_visualization_check_;
  QCheckBox* show_information_check_;
  QStackedWidget* content_stack_;
  
  // Right panel - Specifications
  EndEffectorSpecificationWidget* spec_widget_;

  // Internal state
  EndEffectorSelection setup_step_;
  std::vector<EndEffectorConfig> available_end_effectors_;
  std::vector<EndEffectorConfig> filtered_end_effectors_;
  EndEffectorConfig selected_end_effector_;
  std::string selected_robot_brand_;  // For filtering compatible end-effectors

  bool ui_initialized_ = false;
  bool rviz_integrated_ = false;
  std::vector<ClickableEndEffectorWidget*> end_effector_buttons_;
  
  // Grid layout parameters
  static const int END_EFFECTORS_PER_ROW = 2;
  static const int END_EFFECTOR_BUTTON_WIDTH = 220;
  static const int END_EFFECTOR_BUTTON_HEIGHT = 240;
};

} // namespace robot_description::core_plugins