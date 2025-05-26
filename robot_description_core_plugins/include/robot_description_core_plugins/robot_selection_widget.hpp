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
#include <QPainter>

class QLabel;
class QPushButton;

namespace robot_description::core_plugins
{

class ClickableRobotWidget : public QWidget
{
  Q_OBJECT
  
public:
  explicit ClickableRobotWidget(const RobotConfig& robot, QWidget* parent = nullptr)
    : QWidget(parent), robot_config_(robot), is_selected_(false), is_hovered_(false)
  {
    setFixedSize(250, 260);
    setAttribute(Qt::WA_Hover, true);
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);
    
    RCLCPP_INFO(rclcpp::get_logger("RobotButton"), "Created robot button with direct painting: %s", robot.display_name.c_str());
  }
  
  const RobotConfig& getRobotConfig() const { return robot_config_; }
  
  void setSelected(bool selected) 
  {
    if (is_selected_ != selected) {
      is_selected_ = selected;
      update(); // Trigger repaint
      RCLCPP_INFO(rclcpp::get_logger("RobotButton"), "Robot %s selection: %s", 
                  robot_config_.display_name.c_str(), selected ? "SELECTED" : "UNSELECTED");
    }
  }
  
  bool isSelected() const { return is_selected_; }

Q_SIGNALS:
  void robotClicked(const RobotConfig& robot);

protected:
  void mousePressEvent(QMouseEvent* event) override
  {
    Q_UNUSED(event)
    RCLCPP_INFO(rclcpp::get_logger("RobotButton"), "Robot clicked: %s", robot_config_.display_name.c_str());
    Q_EMIT robotClicked(robot_config_);
  }
  
  void enterEvent(QEvent* event) override
  {
    is_hovered_ = true;
    update(); // Trigger repaint
    RCLCPP_INFO(rclcpp::get_logger("RobotButton"), "Mouse entered: %s", robot_config_.display_name.c_str());
    QWidget::enterEvent(event);
  }
  
  void leaveEvent(QEvent* event) override
  {
    is_hovered_ = false;
    update(); // Trigger repaint
    RCLCPP_INFO(rclcpp::get_logger("RobotButton"), "Mouse left: %s", robot_config_.display_name.c_str());
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
      RCLCPP_DEBUG(rclcpp::get_logger("RobotButton"), "Painting SELECTED state for: %s", robot_config_.display_name.c_str());
    } else if (is_hovered_) {
      bgColor = QColor(240, 248, 255);      // Very light blue background  
      borderColor = QColor(76, 175, 80);    // Green border
      borderWidth = 2;
      RCLCPP_DEBUG(rclcpp::get_logger("RobotButton"), "Painting HOVER state for: %s", robot_config_.display_name.c_str());
    } else {
      bgColor = QColor(255, 255, 255);      // White background
      borderColor = QColor(221, 221, 221);  // Gray border
      borderWidth = 2;
      RCLCPP_DEBUG(rclcpp::get_logger("RobotButton"), "Painting DEFAULT state for: %s", robot_config_.display_name.c_str());
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
  RobotConfig robot_config_;
  bool is_selected_;
  bool is_hovered_;
};

class ClickableRobotWidgetWithPalette : public QWidget
{
  Q_OBJECT
  
public:
  explicit ClickableRobotWidgetWithPalette(const RobotConfig& robot, QWidget* parent = nullptr)
    : QWidget(parent), robot_config_(robot), is_selected_(false)
  {
    setFixedSize(250, 260);
    setAutoFillBackground(true);
    
    // Set default palette
    applyDefaultPalette();
    
    setAttribute(Qt::WA_Hover, true);
    setCursor(Qt::PointingHandCursor);
    
    RCLCPP_INFO(rclcpp::get_logger("RobotButton"), "Created robot button with QPalette: %s", robot.display_name.c_str());
  }
  
  const RobotConfig& getRobotConfig() const { return robot_config_; }
  
  void setSelected(bool selected) 
  {
    is_selected_ = selected;
    updatePalette();
  }
  
  bool isSelected() const { return is_selected_; }

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
    if (!is_selected_) {
      applyHoverPalette();
    }
    QWidget::enterEvent(event);
  }
  
  void leaveEvent(QEvent* event) override
  {
    updatePalette();
    QWidget::leaveEvent(event);
  }
  
  void paintEvent(QPaintEvent* event) override
  {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw background
    QColor bgColor;
    QColor borderColor;
    int borderWidth;
    
    if (is_selected_) {
      bgColor = QColor(227, 242, 253);  // Light blue
      borderColor = QColor(33, 150, 243);  // Blue
      borderWidth = 3;
    } else if (underMouse()) {
      bgColor = QColor(240, 248, 255);  // Very light blue
      borderColor = QColor(76, 175, 80);  // Green
      borderWidth = 2;
    } else {
      bgColor = QColor(255, 255, 255);  // White
      borderColor = QColor(221, 221, 221);  // Gray
      borderWidth = 2;
    }
    
    // Draw background with rounded corners
    painter.setBrush(QBrush(bgColor));
    painter.setPen(QPen(borderColor, borderWidth));
    painter.drawRoundedRect(rect().adjusted(borderWidth/2, borderWidth/2, -borderWidth/2, -borderWidth/2), 8, 8);
    
    QWidget::paintEvent(event);
  }

private:
  void applyDefaultPalette()
  {
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(255, 255, 255));  // White background
    setPalette(pal);
    update();
  }
  
  void applyHoverPalette()
  {
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(240, 248, 255));  // Light blue
    setPalette(pal);
    update();
  }
  
  void applySelectedPalette()
  {
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(227, 242, 253));  // Blue
    setPalette(pal);
    update();
  }
  
  void updatePalette()
  {
    if (is_selected_) {
      applySelectedPalette();
    } else {
      applyDefaultPalette();
    }
  }

private:
  RobotConfig robot_config_;
  bool is_selected_;
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
  void loadAndUpdateVisualization(const RobotConfig& robot);

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
  std::vector<ClickableRobotWidget*> robot_buttons_;
  
  // Grid layout parameters
  static const int ROBOTS_PER_ROW = 2;
  static const int ROBOT_BUTTON_WIDTH = 220;
  static const int ROBOT_BUTTON_HEIGHT = 240;
  
};

} // namespace robot_description::core_plugins

