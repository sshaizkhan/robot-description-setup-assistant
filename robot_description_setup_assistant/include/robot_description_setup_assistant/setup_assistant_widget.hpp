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

#pragma once

// ROS
#include <rviz_common/ros_integration/ros_client_abstraction.hpp>

// Qt
#include <QWidget>
#include <QStackedWidget>
#include <QAbstractTableModel>

// Qt
#include <QApplication>
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QStackedWidget>
#include <QString>

//  pluginlib
#include <pluginlib/class_loader.hpp>
#include <rviz_common/ros_integration/ros_node_abstraction_iface.hpp>
class QSplitter;

#include <robot_description_setup_framework/utilities.hpp>
#include <robot_description_setup_framework/qt/rviz_panel.hpp>
#include <robot_description_setup_framework/qt/setup_step_widget.hpp>

#include <robot_description_core_plugins/start_screen_widget.hpp>

#ifndef Q_MOC_RUN
// Other
#include <boost/program_options/variables_map.hpp>  // for parsing input arguments
#endif

#include "robot_description_setup_assistant/navigation_widget.hpp"

namespace robot_description::setup_assistant
{
class SetupRobotDescriptionAssistantWidget : public QWidget
{
  Q_OBJECT
public:
  SetupRobotDescriptionAssistantWidget(const rviz_common::ros_integration::RosNodeAbstractionIface::WeakPtr& ros_node,
                                       QWidget* parent, const boost::program_options::variables_map& args);

  void moveToScreen(const int index);

  void closeEvent(QCloseEvent* event) override;

  virtual bool notify(QObject* rec, QEvent* ev);

private Q_SLOTS:
  void navigationClicked(const QModelIndex& index);

  void updateTimer();

  void onDataUpdate();

  void onAdvanceRequest();

  void onModalModeUpdate(bool isModal);

private:
  rviz_common::ros_integration::RosNodeAbstractionIface::WeakPtr node_abstraction_;
  rclcpp::Node::SharedPtr node_;
  QList<QString> nav_name_list_;
  NavigationWidget* navs_view_;

  robot_description::setup_framework::RVizPanel* rviz_panel_;
  QSplitter* splitter_;
  QStackedWidget* main_content_;
  int current_index_;
  std::mutex change_screen_lock_;

  // Setup Steps
  pluginlib::ClassLoader<robot_description::setup_framework::SetupStepWidget> widget_loader_;
  std::vector<std::shared_ptr<setup_framework::SetupStepWidget>> steps_;

  /// Contains all the configuration data for the setup assistant
  // DataWarehousePtr config_data_;
};
}  // namespace robot_description::setup_assistant
