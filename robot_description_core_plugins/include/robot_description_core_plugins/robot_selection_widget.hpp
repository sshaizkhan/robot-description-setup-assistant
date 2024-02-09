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

#include <robot_description_setup_framework/qt/helper_widgets.hpp>
#include <robot_description_setup_framework/qt/setup_step_widget.hpp>

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

#include <rclcpp/rclcpp.hpp>
#include <robot_description_setup_framework/setup_step.hpp>

#ifndef Q_MOC_RUN
#include <robot_description_core_plugins/robot_selection.hpp>
#endif

class QLabel;
class QPushButton;

namespace robot_description::core_plugins
{
class RobotSelectionWidget : public setup_framework::SetupStepWidget
{
  Q_OBJECT
public:
  void onInit() override;

  // void focusGiven() override;

  SetupStep& getSetupStep() override
  {
    return setup_step_;
  }

  // TODO: Qt components
  QImage* robot_image_;
  QScrollArea* scroll_area_;
  QWidget* scroll_area_widget_contents_;
  QGridLayout* scroll_area_grid_layout_;
  QPushButton* robot_image_button_;

  ~RobotSelectionWidget() override;

private Q_SLOTS:
  void onChooseRobotButtonClicked();

private:
  RobotSelection setup_step_;
};

}  // namespace robot_description::core_plugins
